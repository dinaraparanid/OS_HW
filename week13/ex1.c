#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <search.h>

const int EMPTY_QUEUE_RES = INT32_MIN;

typedef struct __concurrent_queue_node {
    long __res_id;
    struct __concurrent_queue_node* __next;
} __concurrent_queue_node;

typedef struct {
    __concurrent_queue_node* __head;
    __concurrent_queue_node* __tail;
    pthread_rwlock_t __head_lock;
    pthread_rwlock_t __tail_lock;
} concurrent_queue;

void concurrent_queue_init(concurrent_queue* const q) {
    __concurrent_queue_node* const init = malloc(sizeof(__concurrent_queue_node));
    init->__next = NULL;

    q->__head = init;
    q->__tail = init;

    pthread_rwlock_init(&q->__head_lock, NULL);
    pthread_rwlock_init(&q->__tail_lock, NULL);
}

void concurrent_queue_destroy(concurrent_queue* const q) {
    for (__concurrent_queue_node* head = q->__head; head != NULL;) {
        __concurrent_queue_node* next = head->__next;
        free(head);
        head = next;
    }

    pthread_rwlock_destroy(&q->__head_lock);
    pthread_rwlock_destroy(&q->__tail_lock);
}

void concurrent_queue_offer(concurrent_queue* const q, const long value) {
    __concurrent_queue_node* const new_node = malloc(sizeof(__concurrent_queue_node));
    new_node->__res_id = value;
    new_node->__next = NULL;

    pthread_rwlock_wrlock(&q->__tail_lock);
    q->__tail->__next = new_node;
    q->__tail = new_node;
    pthread_rwlock_unlock(&q->__tail_lock);
}

long concurrent_queue_poll(concurrent_queue* const q) {
    pthread_rwlock_wrlock(&q->__head_lock);

    __concurrent_queue_node* const next = q->__head->__next;

    if (next == NULL) {
        pthread_rwlock_unlock(&q->__head_lock);
        return EMPTY_QUEUE_RES;
    }

    __concurrent_queue_node* const old = q->__head;
    q->__head = next;

    const long res = next->__res_id;
    free(old);

    pthread_rwlock_unlock(&q->__head_lock);

    return res;
}

int concurrent_queue_empty(concurrent_queue* const q) {
    pthread_rwlock_rdlock(&q->__head_lock);

    __concurrent_queue_node* const first = q->__head->__next;

    if (first == NULL) {
        pthread_rwlock_unlock(&q->__head_lock);
        return 1;
    }

    pthread_rwlock_unlock(&q->__head_lock);
    return 0;
}

typedef enum { THREAD, RESOURCE } node_type;

typedef struct lock_graph_node {
    long index;
    struct lock_graph_node** connections;
    long connections_len;
    node_type type;
} lock_graph_node;

typedef struct {
    lock_graph_node* nodes;
    long threads_len;
    long mutexes_len;
    pthread_rwlock_t __lock;
} lock_graph;

int lock_graph_node_compare(const void* const arg1, const void* const arg2) {
    const lock_graph_node* const a = (const lock_graph_node*) arg1;
    const lock_graph_node* const b = (const lock_graph_node*) arg2;

    if (a->type != b->type)
        return a->index - b->index;

    return a->type - b->type;
}

void lock_graph_node_init(
        lock_graph_node* const graph_node,
        const long index,
        const node_type type
) {
    graph_node->index = index;
    graph_node->connections = malloc(sizeof(lock_graph_node*));
    graph_node->connections_len = 0;
    graph_node->type = type;
}

void lock_graph_node_destroy(lock_graph_node* const graph_node) {
    free(graph_node->connections);
}

void lock_graph_init(
        lock_graph* const graph,
        const long threads_len,
        const long mutexes_len
) {
    graph->nodes = malloc((threads_len + mutexes_len) * sizeof(lock_graph_node));

    for (lock_graph_node* node = graph->nodes; node != graph->nodes + threads_len; ++node)
        lock_graph_node_init(node, node - graph->nodes, THREAD);

    for (lock_graph_node* node = graph->nodes + threads_len; node != graph->nodes + threads_len + mutexes_len; ++node)
        lock_graph_node_init(node, node - graph->nodes - threads_len, RESOURCE);

    graph->threads_len = threads_len;
    graph->mutexes_len = mutexes_len;
}

void lock_graph_destroy(lock_graph* const graph) {
    for (lock_graph_node* n = graph->nodes; n != graph->nodes + graph->threads_len + graph->mutexes_len; ++n)
        lock_graph_node_destroy(n);

    pthread_rwlock_destroy(&graph->__lock);
}

lock_graph_node* lock_graph_thread_at(
        lock_graph* const graph,
        const long thread_index
) {
    return graph->nodes + thread_index;
}

lock_graph_node* lock_graph_mutex_at(
        lock_graph* const graph,
        const long mutex_index
) {
    return graph->nodes + graph->threads_len + mutex_index;
}

void lock_graph_thread_locked(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    pthread_rwlock_wrlock(&graph->__lock);

    lock_graph_node* const node = graph->nodes + graph->threads_len + resource_index;
    node->connections = realloc(node->connections, (++node->connections_len) * sizeof(lock_graph_node));
    node->connections[node->connections_len - 1] = lock_graph_thread_at(graph, thread_index);

    pthread_rwlock_unlock(&graph->__lock);
}

void lock_graph_node_swap_remove(
        lock_graph_node* const node,
        const long index
) {
    if (node->connections_len == 1) {
        node->connections_len = 0;
        return;
    }

    for (long i = 0; i < node->connections_len; ++i) {
        lock_graph_node* const conn = node->connections[i];

        if (conn->index == index) {
            node->connections[i] = node->connections[--node->connections_len];
            return;
        }
    }
}

void lock_graph_thread_unlocked(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    pthread_rwlock_wrlock(&graph->__lock);

    lock_graph_node* const node = graph->nodes + graph->threads_len + resource_index;
    lock_graph_node_swap_remove(node, thread_index);

    pthread_rwlock_unlock(&graph->__lock);
}

void lock_graph_resource_acquired(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    pthread_rwlock_wrlock(&graph->__lock);

    lock_graph_node* const node = graph->nodes + thread_index;
    node->connections = realloc(node->connections, (++node->connections_len) * sizeof(lock_graph_node));
    node->connections[node->connections_len - 1] = lock_graph_mutex_at(graph, resource_index);

    pthread_rwlock_unlock(&graph->__lock);
}

void lock_graph_resource_freed(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    pthread_rwlock_wrlock(&graph->__lock);

    lock_graph_node* const node = graph->nodes + thread_index;
    lock_graph_node_swap_remove(node, resource_index);

    pthread_rwlock_unlock(&graph->__lock);
}

typedef struct {
    long thread_index;
    concurrent_queue* queue;
    pthread_mutex_t* mutexes;
    lock_graph* graph;
} thread_args;

void acquire_resource(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    lock_graph_resource_acquired(graph, thread_index, resource_index);
    sleep(1 + rand() % 4);
    lock_graph_resource_freed(graph, thread_index, resource_index);
}

void reacquire_resource(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    lock_graph_thread_unlocked(graph, thread_index, resource_index);
    acquire_resource(graph, thread_index, resource_index);
}

int dfs(
        lock_graph_node* const cur_node,
        void** const visited
) {
    lock_graph_node** const found_ptr = tsearch(cur_node, visited, lock_graph_node_compare);
    lock_graph_node* const found = *found_ptr;

    if (found != cur_node)
        return 1;

    for (
            lock_graph_node** node = cur_node->connections;
            node != cur_node->connections + cur_node->connections_len;
            ++node
    ) {
        if (dfs(*node, visited))
            return 1;
    }

    return 0;
}

int delete_node(const void* const __a, const void* const __b) { return 0; }

void tfree(void** const root) {
    while (*root != NULL) {
        lock_graph_node* const elem = *(lock_graph_node**) root;
        tdelete(elem, root, delete_node);
    }
}

int has_cycle(lock_graph* const graph, const long thread_index) {
    void* visited = NULL;

    pthread_rwlock_rdlock(&graph->__lock);

    if (dfs(lock_graph_thread_at(graph, thread_index), &visited)) {
        pthread_rwlock_unlock(&graph->__lock);
        return 1;
    }

    pthread_rwlock_unlock(&graph->__lock);
    tfree(&visited);
    return 0;
}

int is_possible_to_acquire_resource(
        lock_graph* const graph,
        const long thread_index,
        const long resource_index
) {
    lock_graph_thread_locked(graph, thread_index, resource_index);
    return !has_cycle(graph, thread_index);
}

void print_mutexes_locked_by_thread(
        lock_graph* const graph,
        const long thread_index
) {
    printf("Mutexes locked by thread %ld:\n", thread_index);

    pthread_rwlock_rdlock(&graph->__lock);

    lock_graph_node* const node = lock_graph_thread_at(graph, thread_index);

    for (lock_graph_node** nd = node->connections; nd != node->connections + node->connections_len; ++nd) {
        lock_graph_node* const mtx = *nd;
        printf("Mutex %ld\n", mtx->index);
    }

    for (long i = 0; i < graph->mutexes_len; ++i) {
        lock_graph_node* const mtx_node = lock_graph_mutex_at(graph, i);

        for (
                lock_graph_node** trd_nd = mtx_node->connections;
                trd_nd != mtx_node->connections + mtx_node->connections_len;
                ++trd_nd
        ) {
            lock_graph_node* const trd = *trd_nd;

            if (trd->index == thread_index)
                printf("Mutex %ld\n", i);
        }
    }

    pthread_rwlock_unlock(&graph->__lock);
}

void* acquire_resources(void* const arg) {
    thread_args* const args = (thread_args*) arg;
    const long thread_index = args->thread_index;
    concurrent_queue* const queue = args->queue;
    pthread_mutex_t* const mutexes = args->mutexes;
    lock_graph* const graph = args->graph;

    while (!concurrent_queue_empty(queue)) {
        const long resource_index = concurrent_queue_poll(queue);
        pthread_mutex_t* const mtx = mutexes + resource_index;

        printf("Thread %ld is trying to lock mutex %ld\n", thread_index, resource_index);

        switch (pthread_mutex_trylock(mtx)) {
            case 0:
                acquire_resource(graph, thread_index, resource_index);
                pthread_mutex_unlock(mtx);
                printf("Thread %ld has unlocked mutex %ld\n", thread_index, resource_index);
                break;

            case EBUSY:
                if (!is_possible_to_acquire_resource(graph, thread_index, resource_index)) {
                    for (long i = 0; i < graph->threads_len; ++i)
                        print_mutexes_locked_by_thread(graph, i);

                    puts("Deadlock is detected");
                    exit(EXIT_FAILURE);
                }

                pthread_mutex_lock(mtx);
                reacquire_resource(graph, thread_index, resource_index);
                pthread_mutex_unlock(mtx);
                break;
        }
    }

    printf("Thread %ld is terminating\n", thread_index);
    return NULL;
}

void handle_requests(
        FILE* const fin,
        pthread_t* const threads,
        const long threads_len,
        pthread_mutex_t* const mutexes,
        const long mutexes_len
) {
    lock_graph* const graph = malloc(sizeof(lock_graph));
    lock_graph_init(graph, threads_len, mutexes_len);

    concurrent_queue* const qs = malloc(threads_len * sizeof(concurrent_queue));

    for (concurrent_queue* q = qs; q != qs + threads_len; ++q)
        concurrent_queue_init(q);

    char* const threads_init = calloc(threads_len, sizeof(char));

    while (!feof(fin)) {
        char input[100];
        fgets(input, 100, fin);

        long t = 0, r = 0;

        if (sscanf(input, "%ld%ld", &t, &r) != 2)
            continue;

        if (t >= threads_len) {
            fprintf(stderr, "Index %ld is >= than the total number of threads %ld\n", t, threads_len);
            return;
        }

        if (r >= mutexes_len) {
            fprintf(stderr, "Index %ld is >= than the total number of mutexes %ld\n", r, mutexes_len);
            return;
        }

        print_mutexes_locked_by_thread(graph, t);

        concurrent_queue* const q = qs + t;
        concurrent_queue_offer(q, r);

        if (threads_init[t] == 0) {
            threads_init[t] = 1;

            thread_args* const args = malloc(sizeof(thread_args));
            args->thread_index = t;
            args->queue = q;
            args->mutexes = mutexes;
            args->graph = graph;

            printf("Thread %ld is created\n", t);

            if (pthread_create(threads + t, NULL, &acquire_resources, args) != 0) {
                perror("pthread_create");
                return;
            }
        }
    }

    void* res;

    for (pthread_t* t = threads; t != threads + threads_len; ++t)
        if (threads_init[t - threads] != 0 && pthread_join(*t, &res) != 0)
            perror("pthread_join");

    puts("No deadlocks");

    lock_graph_destroy(graph);
    free(graph);

    for (concurrent_queue* q = qs; q != qs + threads_len; ++q)
        concurrent_queue_destroy(q);
    free(qs);

    free(threads_init);
}

int main(const int argc, const char** const argv) {
    if (argc < 2) {
        puts("Usage: ./ex1 $m $n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    char* parse_end_ptr;
    const long mutexes_len = strtol(argv[1], &parse_end_ptr, 10);
    const long threads_len = strtol(argv[2], &parse_end_ptr, 10);

    pthread_mutex_t* const mutexes = malloc(mutexes_len * sizeof(pthread_mutex_t));
    pthread_t* const threads = calloc(threads_len, sizeof(pthread_t));

    for (pthread_mutex_t* m = mutexes; m != mutexes + mutexes_len; ++m)
        pthread_mutex_init(m, NULL);

    FILE* const fin = fopen("input.txt", "r");

    if (fin == NULL) {
        puts("input.txt is missing...");
        return EXIT_FAILURE;
    }

    handle_requests(fin, threads, threads_len, mutexes, mutexes_len);

    for (pthread_mutex_t* m = mutexes; m != mutexes + mutexes_len; ++m)
        pthread_mutex_destroy(m);

    fclose(fin);
    free(mutexes);
    free(threads);
    return 0;
}
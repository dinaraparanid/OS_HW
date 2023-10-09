#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>

pthread_cond_t insert_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t remove_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t insert_cond_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t remove_cond_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t buf_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cnt_lock = PTHREAD_MUTEX_INITIALIZER;

const int BUF_SIZE = 1000;

int is_prime(int n) {
    if (n <= 1)
        return 0;

    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0)
            return 0;

    return 1;
}

int n = 0;
int current_number = 0; // k
int primes_cnt = 0; // c

typedef struct entry {
    int data;
    TAILQ_ENTRY(entry) entries;
} entry;

TAILQ_HEAD(queue_head, entry);
typedef struct queue_head queue_head;

queue_head buffer;
atomic_int_fast64_t buf_size = 0;

int get_number_to_check() {
    return current_number != n ? current_number++ : -1;
}

void increment_primes() { ++primes_cnt; }

void insert_item(const int item) {
    pthread_mutex_lock(&remove_cond_lock);

    while (buf_size >= BUF_SIZE)
        pthread_cond_wait(&remove_cond, &remove_cond_lock);

    pthread_mutex_unlock(&remove_cond_lock);

    entry* const elem = malloc(sizeof(entry));
    elem->data = item;

    pthread_mutex_lock(&buf_lock);
    TAILQ_INSERT_TAIL(&buffer, elem, entries);
    ++buf_size;
    pthread_mutex_unlock(&buf_lock);
    pthread_cond_signal(&insert_cond);
}

void* producer(void* const arg) {
    while (1) {
        const int next_num = get_number_to_check();

        if (next_num == -1) {
            insert_item(-1);
            return 0;
        }

        if (next_num < 2)
            continue;

        if (next_num != 2 && next_num != 3 && (next_num % 2 == 0 || next_num % 3 == 0))
            continue;

        insert_item(next_num);
    }
}

int remove_item() {
    pthread_mutex_lock(&buf_lock);

    entry* elem = TAILQ_FIRST(&buffer);

    if (elem == NULL) {
        pthread_mutex_unlock(&buf_lock);
        return 1;
    }

    const int next_num = elem->data;

    if (next_num == -1) {
        pthread_mutex_unlock(&buf_lock);
        return 0;
    }

    TAILQ_REMOVE(&buffer, elem, entries);
    --buf_size;
    free(elem);
    pthread_mutex_unlock(&buf_lock);

    if (is_prime(next_num)) {
        pthread_mutex_lock(&cnt_lock);
        increment_primes();
        pthread_mutex_unlock(&cnt_lock);
    }

    return 1;
}

void* consumer(void* const arg) {
    while (1) {
        while (buf_size == 0) {
            pthread_mutex_lock(&insert_cond_lock);

            struct timespec tm;
            clock_gettime(CLOCK_REALTIME, &tm);
            tm.tv_sec += 1;

            pthread_cond_timedwait(&insert_cond, &insert_cond_lock, &tm);
            pthread_mutex_unlock(&insert_cond_lock);
        }

        if (!remove_item())
            pthread_exit(0);

        pthread_cond_signal(&remove_cond);
    }
}

void init_concurrency() {
    pthread_cond_init(&insert_cond, NULL);
    pthread_cond_init(&remove_cond, NULL);

    pthread_mutex_init(&insert_cond_lock, NULL);
    pthread_mutex_init(&remove_cond_lock, NULL);

    pthread_mutex_init(&buf_lock, NULL);
    pthread_mutex_init(&cnt_lock, NULL);

    TAILQ_INIT(&buffer);
}

void destroy_concurrency() {
    pthread_cond_destroy(&insert_cond);
    pthread_cond_destroy(&remove_cond);

    pthread_mutex_destroy(&insert_cond_lock);
    pthread_mutex_destroy(&remove_cond_lock);

    pthread_mutex_destroy(&buf_lock);
    pthread_mutex_destroy(&cnt_lock);
}

int main(const int argc, char** const argv) {
    init_concurrency();
    char* optend;

    const int n_arg = getopt(argc, argv, "n:m:");
    n = strtol(optarg, &optend, 10);

    const int m_arg = getopt(argc, argv, "n:m:");
    const int m = strtol(optarg, &optend, 10);

    pthread_t* const thrds = malloc(m * sizeof(pthread_t));

    pthread_t prod;
    pthread_create(&prod, NULL, &producer, NULL);

    for (int i = 0; i < m; ++i)
        pthread_create(&thrds[i], NULL, &consumer, NULL);

    void* res;

    for (pthread_t* p = thrds; p != thrds + m; ++p)
        pthread_join(*p, &res);

    pthread_cancel(prod);

    printf("%d\n", primes_cnt);

    free(thrds);
    destroy_concurrency();
    return 0;
}

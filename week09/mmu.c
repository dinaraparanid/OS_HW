#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

const int MEM_ENTRY_SIZE = 8;

typedef struct {
    /** The page is in the physical memory (RAM) */
    int is_valid;

    /** The frame number of the page in the RAM */
    long frame;

    /** The page should be written to disk */
    int is_dirty;

    /** PID of the process who request this page (or 0) */
    int referenced;

    /**
     * Number of times when page is referenced.
     * Required for the implementation of Not Frequently Used algorithm
     */
    long counter_nfu;

    /** Required for the implementation of Aging algorithm */
    uint8_t counter_aging;
} page_table_entry;

long num_of_pages = 0;

const char** commands;
long num_of_commands = 0;

pid_t pager_pid = 0;

page_table_entry* page_table;
char* page_table_file;

long parse_long_arg(const char** argv, const int index) {
    char* arg_end;
    const long arg = strtol(argv[index], &arg_end, 10);

    if (arg <= 0)
        puts("Illegal argument in input");

    return arg;
}

// -------------------------- Page Table --------------------------

size_t page_table_size(const long pages_num) {
    return pages_num * sizeof(page_table_entry);
}

void update_page_table_from_file() {
    for (int i = 0; i < num_of_pages; ++i)
        memmove(
                page_table + i,
                page_table_file + page_table_size(i),
                sizeof(page_table_entry)
        );
}

void log_page_entity(const page_table_entry* const p) {
    printf(
            "Page %ld ---> valid=%d, frame=%ld, dirty=%d, referenced=%d\n",
            p - page_table,
            p->is_valid,
            p->frame,
            p->is_dirty,
            p->referenced
    );
}

void log_init_page_table() {
    puts("-------------------------");
    puts("Initialized page table");

    for (const page_table_entry* p = page_table; p != page_table + num_of_pages; ++p)
        log_page_entity(p);
}

void log_page_table() {
    puts("Page Table");

    for (const page_table_entry* p = page_table; p != page_table + num_of_pages; ++p)
        log_page_entity(p);

    puts("");
}

page_table_entry* init_page_table() {
    return malloc(page_table_size(num_of_pages));
}

char* init_page_table_file() {
    struct stat st;

    if (stat("/tmp/ex2", &st) == -1)
        mkdir("/tmp/ex2", 0755);

    const int fd = open("/tmp/ex2/pagetable", O_RDWR | O_CREAT);

    if (fd == -1) {
        perror("open");
        return NULL;
    }

    if (ftruncate(fd, page_table_size(num_of_pages)) == -1) {
        perror("ftruncate");
        return NULL;
    }

    char* const file = mmap(
            NULL,
            page_table_size(num_of_pages),
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0
    );

    if (file == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    return file;
}

long parse_page_from_request(const char* const request) {
    char* page_ind_end;
    const long page_ind = strtol(request + 1, &page_ind_end, 10);

    if (page_ind < 0 || page_ind >= num_of_pages) {
        puts("Illegal page number in request");
        return -1;
    }

    return page_ind;
}

void store_page_table_entry(const page_table_entry* const entry, const long offset_in_entries) {
    memmove(
            page_table_file + page_table_size(offset_in_entries),
            entry,
            sizeof(page_table_entry)
    );
}

void store_page_table() {
    for (long i = 0; i < num_of_pages; ++i)
        store_page_table_entry(page_table + i, i);
}

void free_mem() {
    munmap(page_table_file, page_table_size(num_of_pages));
    free(page_table);
}

const char* parse_request_type(const char* const mem_request) {
    if (*mem_request == 'R') return "Read";
    if (*mem_request == 'W') return "Write";
    return NULL;
}

void perform_mem_request(const char request, page_table_entry* const page) {
    if (request == 'W') {
        puts("It is a write request then set the dirty field");
        page->is_dirty = 1;
    }

    page->referenced = 0;
}

void on_invalid_input() {
    puts("Invalid request");
    free_mem();
    exit(EXIT_FAILURE);
}

void on_invalid_page(page_table_entry* const p) {
    puts("It is not a valid page --> page fault");
    p->referenced = getpid();
    store_page_table();
    //log_page_table();

    puts("Ask pager to load it from disk (SIGUSR1 signal) and wait");
    kill(pager_pid, SIGUSR1);
}

void handle_mem_requests() {
    static float hit_cnt = 0;
    static float miss_cnt = 0;

    puts("-------------------------");

    static long mem_request_ind = 0;
    update_page_table_from_file();

    for (
            const char** mem_request = commands + mem_request_ind;
            mem_request != commands + num_of_commands;
            ++mem_request, ++mem_request_ind
    ) {
        const char* const request_type = parse_request_type(*mem_request);

        if (request_type == NULL)
            on_invalid_input();

        const long page_ind = parse_page_from_request(*mem_request);

        if (page_ind == -1)
            on_invalid_input();

        printf("%s Request for page %ld\n", request_type, page_ind);

        page_table_entry* const p = page_table + page_ind;

        if (!p->is_valid) {
            on_invalid_page(p);
            ++miss_cnt;
            return;
        }

        ++hit_cnt;
        perform_mem_request(**mem_request, p);

        store_page_table();
        //log_page_table();
    }

    puts("Done all requests.");
    printf("Total hit ratio: %f\n", hit_cnt / (hit_cnt + miss_cnt));
    free_mem();

    puts("MMU sends SIGUSR1 to the pager.");
    kill(pager_pid, SIGUSR1);

    puts("MMU terminates.");
    exit(EXIT_SUCCESS);
}

void sig_handler(const int signum) {
    puts("MMU resumed by SIGCONT signal from pager");
    //log_page_table();

    if (signum == SIGCONT) {
        handle_mem_requests();
        return;
    }
}

int main(const int argc, const char** const argv) {
    num_of_pages = parse_long_arg(argv, 1);

    commands = argv + 2;
    num_of_commands = argc - 3;

    pager_pid = parse_long_arg(argv, argc - 1);

    page_table = init_page_table();
    page_table_file = init_page_table_file();
    update_page_table_from_file();

    signal(SIGCONT, sig_handler);
    raise(SIGCONT);
    for (;;) pause();
}
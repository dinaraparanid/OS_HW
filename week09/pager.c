#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

const int MEM_ENTRY_SIZE = 8;

typedef void signal_handler(int, siginfo_t*, void*);

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
long num_of_frames = 0;
long page_replace_algo = 0;

enum page_replace_algorithms {
    RANDOM,
    NFU,
    AGING
};

char** disk;
atomic_int_fast32_t disk_access_cnt = 0;

page_table_entry* page_table;
char* page_table_file;

char** RAM;
long* frames_status;

long parse_long_arg(const char** argv, const int index) {
    char* arg_end;
    const long arg = strtol(argv[index], &arg_end, 10);

    if (arg <= 0)
        puts("Illegal argument in input");

    return arg;
}

// -------------------------- Page Table --------------------------

page_table_entry new_page() {
    page_table_entry e = {
            .is_valid = 0,
            .frame = -1,
            .is_dirty = 0,
            .referenced = 0,
            .counter_nfu = 0,
            .counter_aging = 0
    };

    return e;
}

size_t page_table_size(const long pages_num) {
    return pages_num * sizeof(page_table_entry);
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

page_table_entry* init_page_table() {
    page_table_entry* const pgt = malloc(page_table_size(num_of_pages));

    for (page_table_entry* p = pgt; p != pgt + num_of_pages; ++p)
        *p = new_page();

    return pgt;
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

// -------------------------- RAM --------------------------

char* random_mem_entry() {
    char* const mem = malloc(MEM_ENTRY_SIZE + 1);

    for (int i = 0; i < MEM_ENTRY_SIZE;) {
        char c = rand() % 128;
        if (isprint(c)) mem[i++] = c;
    }

    mem[MEM_ENTRY_SIZE] = '\0';
    return mem;
}

char** init_RAM() {
    char** const ram = malloc(num_of_frames * sizeof(char*));

    for (char** p = ram; p != ram + num_of_frames; ++p)
        *p = calloc(MEM_ENTRY_SIZE + 1, sizeof(char));

    return ram;
}

void log_frame(char** const frame) {
    printf("Frame %ld ---> %s\n", frame - RAM, *frame);
}

void log_RAM() {
    puts("RAM array");

    for (char** frame = RAM; frame != RAM + num_of_frames; ++frame)
        log_frame(frame);
}

void log_init_RAM() {
    puts("-------------------------");
    puts("Initialized RAM");
    log_RAM();
}

// -------------------------- Disk --------------------------

void log_disk_page(char** const p) {
    printf("Page %ld ---> %s\n", p - disk, *p);
}

void log_init_disk() {
    puts("-------------------------");
    puts("Initialized disk");
    puts("Disk array");

    for (char** p = disk; p != disk + num_of_pages; ++p)
        log_disk_page(p);
}

char** init_disk() {
    char** const dsk = malloc(num_of_pages * sizeof(char*));

    for (char** p = dsk; p != dsk + num_of_pages; ++p)
        *p = random_mem_entry();

    return dsk;
}

// -------------------------- Frames Status --------------------------

long* init_frames_status() {
    long* const frames = malloc(num_of_frames * sizeof(long));
    memset(frames, -1, num_of_frames * sizeof(long));
    return frames;
}

void log_disk_access() {
    printf("disk accesses is %ld so far\n", disk_access_cnt);
}
void log_total_disk_access() {
    printf("%ld disk accesses in total\n", disk_access_cnt);
}

// -------------------------- Swap --------------------------

void swap_in(const long page_ind, const long frame_ind) {
    printf("Copy data from the disk (page=%ld) to RAM (frame=%ld)\n", page_ind, frame_ind);
    //log_RAM();

    strcpy(RAM[frame_ind], disk[page_ind]);
    ++disk_access_cnt;

    page_table[page_ind].is_valid = 1;
    page_table[page_ind].frame = frame_ind;
    frames_status[frame_ind] = page_ind;
    store_page_table();

    //log_disk_access();
}

void free_frame(const long frame_ind) {
    memset(RAM[frame_ind], 0, MEM_ENTRY_SIZE);
    frames_status[frame_ind] = -1;
}

void swap_out(const long frame_ind) {
    const long page_ind = frames_status[frame_ind];

    strcpy(disk[page_ind], RAM[frame_ind]);
    free_frame(frame_ind);
    ++disk_access_cnt;

    page_table[page_ind].is_valid = 0;
    page_table[page_ind].frame = -1;
    page_table[page_ind].is_dirty = 0;
    page_table[page_ind].referenced = 0;
    store_page_table();
}

void update_page_table_from_file() {
    for (int i = 0; i < num_of_pages; ++i)
        memmove(
                page_table + i,
                page_table_file + page_table_size(i),
                sizeof(page_table_entry)
        );

    memset(frames_status, -1, num_of_frames * sizeof(long));

    for (page_table_entry* p = page_table; p != page_table + num_of_pages; ++p)
        if (p->frame != -1 && p->is_valid)
            frames_status[p->frame] = p - page_table;
}

page_table_entry* find_requested_page() {
    for (page_table_entry* pte = page_table; pte != page_table + num_of_pages; ++pte)
        if (pte->referenced != 0)
            return pte;

    return NULL;
}

long find_free_frame() {
    for (long* f = frames_status; f != frames_status + num_of_frames; ++f)
        if (*f == -1) return (f - frames_status);
    return -1;
}

// -------------------------- Page replacement algorithms --------------------------

long random() { return frames_status[rand() % num_of_frames]; }

long prepare_random_frame() {
    const long replaced_page_ind = random();
    const long frame_ind = page_table[replaced_page_ind].frame;
    const page_table_entry* const pte = page_table + replaced_page_ind;

    printf("Chose a random victim page %ld\n", replaced_page_ind);

    if (pte->is_dirty)
        swap_out(frame_ind);

    return frame_ind;
}

long nfu() {
    long best_cnt = INT32_MAX;
    long best_page = 0;

    for (const long* page_ind = frames_status; page_ind != frames_status + num_of_frames; ++page_ind) {
        const page_table_entry* const pte = page_table + *page_ind;

        if (pte->counter_nfu < best_cnt || pte->counter_nfu == best_cnt && !pte->is_dirty) {
            best_cnt = pte->counter_nfu;
            best_page = *page_ind;
        }
    }

    return best_page;
}

long prepare_nfu_frame() {
    const long replaced_page_ind = nfu();
    const long frame_ind = page_table[replaced_page_ind].frame;
    const page_table_entry* const pte = page_table + replaced_page_ind;

    printf(
            "Chose the most NFU victim page %ld with the lowest counter %ld\n",
            replaced_page_ind,
            pte->counter_nfu
    );

    if (pte->is_dirty)
        swap_out(frame_ind);

    return frame_ind;
}

long aging() {
    long best_cnt = INT32_MAX;
    long best_page = 0;

    for (const long* page_ind = frames_status; page_ind != frames_status + num_of_frames; ++page_ind) {
        const page_table_entry* const pte = page_table + *page_ind;

        if (pte->counter_aging < best_cnt || pte->counter_aging == best_cnt && !pte->is_dirty) {
            best_cnt = pte->counter_aging;
            best_page = *page_ind;
        }
    }

    return best_page;
}

long prepare_aging_frame() {
    const long replaced_page_ind = nfu();
    const long frame_ind = page_table[replaced_page_ind].frame;
    const page_table_entry* const pte = page_table + replaced_page_ind;

    printf(
            "Chose the oldest victim page %ld with the lowest counter %hu\n",
            replaced_page_ind,
            pte->counter_aging
    );

    if (pte->is_dirty)
        swap_out(frame_ind);

    return frame_ind;
}

void free_disk() {
    for (char** d = disk; d != disk + num_of_pages; ++d)
        free(*d);
    free(disk);
}

void free_RAM() {
    for (char** f = RAM; f != RAM + num_of_frames; ++f)
        free(*f);
    free(RAM);
}

void free_mem() {
    munmap(page_table_file, page_table_size(num_of_pages));
    remove("/tmp/ex2/pagetable");

    free(page_table);
    free_disk();
    free_RAM();
}

void kill_pager() {
    puts("Pager is terminated");
    exit(EXIT_SUCCESS);
}

void handle_counter(page_table_entry* const pte) {
    if (page_replace_algo == AGING) {
        pte->counter_aging = (pte->counter_aging >> 1) | (pte->referenced << 7);
        return;
    }

    if (page_replace_algo == NFU) {
        ++pte->counter_nfu;
        return;
    }
}

long prepare_frame() {
    if (page_replace_algo == RANDOM) return prepare_random_frame();
    if (page_replace_algo == NFU) return prepare_nfu_frame();
    return prepare_aging_frame();
}

void on_mmu_request_received(const pid_t mmu_pid) {
    printf("A disk access request from MMU Process (pid=%d)\n", mmu_pid);
    update_page_table_from_file();

    page_table_entry* const pte = find_requested_page();

    if (pte == NULL) {
        puts("Requested page was not found. Stopping pager...");

        //log_total_disk_access();
        free_mem();
        kill_pager();
    }

    const long page_ind = pte - page_table;
    printf("Page %ld is referenced\n", page_ind);

    handle_counter(pte);

    long frame_ind = find_free_frame();

    if (frame_ind != -1) {
        printf("We can allocate it to free frame %ld\n", frame_ind);
        swap_in(page_ind, frame_ind);
        goto MMU_CONT;
    }

    puts("We do not have free frames in RAM");
    frame_ind = prepare_frame();

    printf("Replace/Evict it with page %ld to be allocated to frame %ld\n", page_ind, frame_ind);
    swap_in(page_ind, frame_ind);

    MMU_CONT:
    puts("Resume MMU process");
    kill(mmu_pid, SIGCONT);
}

void sig_handler(const int signum, siginfo_t* info, void* vp) {
    if (signum == SIGUSR1) {
        on_mmu_request_received(info->si_pid);
        return;
    }
}

int register_signal(const int sig, signal_handler handler) {
    struct sigaction s, old;
    s.sa_sigaction = handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = SA_SIGINFO;

    const int r = sigaction(sig, &s, &old);
    if (r < 0) perror("sigaction");
    return r;
}

void store_pid() {
    FILE* const pid_file = fopen("pager_pid.txt", "w");
    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);
}

int parse_page_replacement_algo(const char** argv, const int index) {
    const char* const algo = argv[index];
    printf("Algorithm %s was chosen as the page replacement algorithm\n", algo);

    if (strcmp(algo, "random") == 0) return RANDOM;
    if (strcmp(algo, "nfu") == 0) return NFU;
    if (strcmp(algo, "aging") == 0) return AGING;

    puts("Illegal algorithm is chosen!");
    return -1;
}

int main(const int argc, const char** const argv) {
    store_pid();
    srand(time(NULL));

    num_of_pages = parse_long_arg(argv, 1);
    if (num_of_pages <= 0) return EXIT_FAILURE;

    num_of_frames = parse_long_arg(argv, 2);
    if (num_of_frames <= 0) return EXIT_FAILURE;

    page_replace_algo = parse_page_replacement_algo(argv, 3);
    if (page_replace_algo < 0) return EXIT_FAILURE;

    page_table = init_page_table();
    //log_init_page_table();

    RAM = init_RAM();
    //log_init_RAM();

    page_table_file = init_page_table_file();
    store_page_table();

    disk = init_disk();
    //log_init_disk();

    frames_status = init_frames_status();

    register_signal(SIGUSR1, sig_handler);
    for (;;) pause();
}
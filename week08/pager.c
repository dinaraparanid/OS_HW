#include <stdio.h>
#include <stdlib.h>
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
} page_table_entry;

long num_of_pages = 0;
long num_of_frames = 0;

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

char* random_mem_entry() {
    char* const mem = malloc(MEM_ENTRY_SIZE + 1);

    for (int i = 0; i < MEM_ENTRY_SIZE;) {
        char c = rand() % 128;
        if (isprint(c)) mem[i++] = c;
    }

    mem[MEM_ENTRY_SIZE] = '\0';
    return mem;
}

char** init_disk() {
    char** const dsk = malloc(num_of_pages * sizeof(char*));

    for (char** p = dsk; p != dsk + num_of_pages; ++p)
        *p = random_mem_entry();

    return dsk;
}

char** init_RAM() {
    char** const ram = malloc(num_of_frames * sizeof(char*));

    for (char** p = ram; p != ram + num_of_frames; ++p)
        *p = calloc(MEM_ENTRY_SIZE + 1, sizeof(char));

    return ram;
}

long* init_frames_status() {
    long* const frames = malloc(num_of_frames * sizeof(long));
    memset(frames, -1, num_of_frames * sizeof(long));
    return frames;
}

page_table_entry new_page() {
    page_table_entry e = {
            .is_valid = 0,
            .frame = -1,
            .is_dirty = 0,
            .referenced = 0
    };

    return e;
}

size_t page_table_size(const long pages_num) {
    return pages_num * sizeof(page_table_entry);
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

void log_RAM() {
    puts("PAGER:");
    puts("--------- RAM ---------\n");

    for (long i = 0; i < num_of_frames; ++i)
        printf("\nFrame №%ld: %s\n", i, RAM[i]);

    puts("-----------------------\n");
}

void log_page_table_entry(const long page_ind) {
    printf("\nPage №%ld:\n", page_ind);
    printf("Valid: %d\n", page_table[page_ind].is_valid);
    printf("Frame: %ld\n", page_table[page_ind].frame);
    printf("Dirty: %d\n", page_table[page_ind].is_dirty);
    printf("Referenced: %d\n", page_table[page_ind].referenced);
}

void log_page_table() {
    puts("PAGER:");
    puts("--------- Page Table ---------\n");

    for (long i = 0; i < num_of_pages; ++i)
        log_page_table_entry(i);

    puts("-----------------------------\n");
}

void log_disk_accesses() {
    printf("PAGER: Total disk accesses: %ld\n", disk_access_cnt);
}

void swap_in(const long page_ind, const long frame_ind) {
    strcpy(RAM[frame_ind], disk[page_ind]);
    frames_status[frame_ind] = page_ind;
    log_RAM();
}

void free_frame(const long frame_ind) {
    memset(RAM[frame_ind], 0, MEM_ENTRY_SIZE);
    frames_status[frame_ind] = -1;
}

void swap_out(const long frame_ind, const long page_ind) {
    strcpy(disk[page_ind], RAM[frame_ind]);
    free_frame(frame_ind);
    ++disk_access_cnt;

    log_page_table();
    log_RAM();

    page_table[page_ind].is_valid = 1;
    page_table[page_ind].frame = -1;
    page_table[page_ind].is_dirty = 0;
    page_table[page_ind].referenced = 0;
    store_page_table_entry(page_table + page_ind, page_ind);
}

void update_page_table_from_file() {
    for (int i = 0; i < num_of_pages; ++i)
        memmove(
                page_table + i,
                page_table_file + page_table_size(i),
                sizeof(page_table_entry)
        );
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

long prepare_random_frame() {
    const long frame_ind = rand() % num_of_frames;
    const long replace_page_ind = frames_status[frame_ind];
    const page_table_entry* const pte = page_table + replace_page_ind;

    if (pte->is_dirty)
        swap_out(frame_ind, replace_page_ind);

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

void on_mmu_request_received(const pid_t mmu_pid) {
    update_page_table_from_file();
    log_page_table();

    page_table_entry* const pte = find_requested_page();

    if (pte == NULL) {
        puts("НИХУЯ НЕ НАШЕЛ");
        log_page_table();
        log_disk_accesses();
        free_mem();
        exit(EXIT_SUCCESS);
    }

    const long page_ind = pte - page_table;
    long frame_ind = find_free_frame();

    if (frame_ind != -1) {
        swap_in(page_ind, frame_ind);
        goto MMU_CONT;
    }

    frame_ind = prepare_random_frame();
    swap_in(page_ind, frame_ind);

    MMU_CONT: kill(mmu_pid, SIGCONT);
}

void sig_handler(const int signum, siginfo_t* info, void* vp) {
    sleep(1);

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

int main(const int argc, const char** const argv) {
    store_pid();
    srand(time(NULL));

    num_of_pages = parse_long_arg(argv, 1);
    if (num_of_pages <= 0) return EXIT_FAILURE;

    num_of_frames = parse_long_arg(argv, 2);
    if (num_of_frames <= 0) return EXIT_FAILURE;

    disk = init_disk();
    page_table = init_page_table();
    page_table_file = init_page_table_file();

    store_page_table();
    log_page_table();

    RAM = init_RAM();
    log_RAM();
    frames_status = init_frames_status();

    register_signal(SIGUSR1, sig_handler);
    for (;;) pause();
}
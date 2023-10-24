#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

const int LINES = 500 * 1024;

void fill_txt_file() {
    FILE* const txt = fopen("text.txt", "w");

    for (int l = 0; l < LINES; ++l) {
        for (int cnt = 0; cnt < 1024;) {
            FILE* const rd = fopen("/dev/random", "r");
            const char c = fgetc(rd);

            if (isprint(c)) {
                ++cnt;
                fputc(c, txt);
            }

            fclose(rd);
        }

        fputc('\n', txt);
    }

    fclose(txt);
}

size_t to_lowercase(char* const txt) {
    size_t cnt = 0;
    const int len = strlen(txt);

    for (char* c = txt; c != txt + len; ++c) {
        if (isupper(*c)) {
            ++cnt;
            *c = tolower(*c);
        }
    }

    return cnt;
}

void scan_txt_file() {
    const int txt = open("text.txt", O_RDWR);
    if (txt == -1) { perror("open"); return; }

    struct stat st;
    if (fstat(txt, &st) == -1) { perror("fstat"); return; }

    const int chunk_size = sysconf(_SC_PAGE_SIZE) * 1024;
    const int len = st.st_size;
    const int chunks = len / chunk_size;

    size_t uppers_cnt = 0;

    for (int i = 0; i < chunks; ++i) {
        char* const addr = mmap(
                NULL,
                chunk_size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                txt,
                chunk_size * i
        );

        if (addr == MAP_FAILED) {
            perror("mmap"); return;
        }

        uppers_cnt += to_lowercase(addr);

        if (munmap(addr, chunk_size) == -1) {
            perror("munmap"); return;
        }
    }

    printf("Uppers replaced to lowers: %lu\n", uppers_cnt);
    close(txt);
}

int main() {
    fill_txt_file();
    scan_txt_file();
    return 0;
}
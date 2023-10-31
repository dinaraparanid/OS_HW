#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

const int MEM_ALLOC = 1000000000;

int main() {
    for (int i = 0; i < 10; ++i, sleep(1)) {
        void* const mem = malloc(MEM_ALLOC);
        memset(mem, 0, MEM_ALLOC);

        struct rusage usage;
        const int st = getrusage(RUSAGE_SELF, &usage);

        if (st == -1) {
            perror("getrusage");
            return 0;
        }

        printf("Memory usage: %ld KB\n",usage.ru_maxrss);
        // memory is intentionally not cleaned:
        // free(mem);
    }

    return 0;
}
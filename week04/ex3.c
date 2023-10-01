#include <stdio.h>
#include <unistd.h>

int main(const int argc, const char** const argv) {
    const char* const nstr = argv[1];

    int n = 0;
    sscanf(nstr, "%d", &n);

    for (int i = 0; i < n; ++i) {
        const pid_t child = fork();

        if (!child) {
            printf("I am process %d\n", i);
            return 0;
        }

        sleep(5);
    }

    return 0;
}
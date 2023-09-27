#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main() {
    const pid_t first_child = fork();

    if (!first_child) {
        const time_t t = clock();
        puts("I am first child process");
        printf("Execution time: %f\n", (float) t / CLOCKS_PER_SEC * 1000.0);
        return 0;
    }

    printf("First child PID: %d\n", first_child);

    const pid_t second_child = fork();

    if (!second_child) {
        const time_t t = clock();
        puts("I am second child process");
        printf("Execution time: %f\n", (float) t / CLOCKS_PER_SEC * 1000.0);
        return 0;
    }

    printf("Second child PID: %d\n", second_child);
    return 0;
}
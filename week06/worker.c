#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

/** 6 digits for big triangular numbers like 113050 */
const int TRI_BASE = 1000000;

/** current process pid (which executed this program) */
pid_t pid = 0;

/** current process idx (starts from 0) */
int process_idx = 0;

/** number of triangular numbers found so far */
long tris = 0;

int is_triangular(const long n) {
    for (long i = 1; i <= n; ++i)
        if (i * (i + 1) == 2 * n)
            return 1;
    return 0;
}

void signal_handler(const int signum) {
    // prints info about the number of triangulars found

    printf(
            "Process %d (PID=<%d>): count of triangulars found so far is %ld\n",
            process_idx,
            pid,
            tris
    );

    switch(signum) {
        case SIGTSTP:
            // pause the process indefinitely
            printf("Process %d: stopping....\n", process_idx);
            pause();
            break;
        case SIGCONT:
            // continue the process
            printf("Process %d: resuming....\n", process_idx);
            break;
        case SIGTERM:
            // terminate the process
            printf("Process %d: terminating....\n", process_idx);
            exit(EXIT_SUCCESS);
            break;
        default:
            break;
    }
}

/** Generates a big number n */

long big_n() {
    time_t t;
    long n = 0;
    srand((unsigned) time(&t));

    while(n < TRI_BASE)
        n += rand();

    return n % TRI_BASE;
}

int main(int argc, char *argv[]) {
    const char* n_arg = argv[1];
    sscanf(n_arg, "%d", &process_idx);

    pid = getpid();

    signal(SIGTSTP, signal_handler);
    signal(SIGCONT, signal_handler);
    signal(SIGTERM, signal_handler);

    long next_n = big_n() + 1;

    // The first message after creating the process
    printf("Process %d (PID=<%d>): has been started\n", process_idx, pid);
    printf("Process %d (PID=<%d>): will find the next trianguar number from [%ld, inf)\n", process_idx, pid, next_n);

    // initialize counter
    tris = 0;

    for (;; ++next_n) {
        if (is_triangular(next_n)) {
            printf(
                    "Process %d (PID=<%d>): I found this triangular number %ld\n",
                    process_idx,
                    pid,
                    next_n
            );

            ++tris;
        }
    }
}
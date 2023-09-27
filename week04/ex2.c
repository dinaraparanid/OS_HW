#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

const int MAX_SIZE = 120;

int main() {
    double u[MAX_SIZE], v[MAX_SIZE];
    srand(time(NULL));

    for (double* p = u; p != u + MAX_SIZE; ++p)
        *p = rand() % 100;

    for (double* p = v; p != v + MAX_SIZE; ++p)
        *p = rand() % 100;

    puts("Number of processes:");

    int n = 0;
    scanf("%d", &n);

    pid_t* const procs = malloc(n * sizeof(pid_t));

    const int step = MAX_SIZE / n;

    for (int i = 0; i < n; ++i) {
        procs[i] = fork();

        if (!procs[i]) {
            FILE* const temp = fopen("temp.txt", "a");

            int sum = 0;

            const int next_point = step * (i + 1);
            const int final = next_point > MAX_SIZE ? MAX_SIZE : next_point;

            for (int q = step * i; q != final; ++q)
                sum += u[q] * v[q];

            fprintf(temp, "%d\n", sum);
            fclose(temp);
            return 0;
        }
    }

    for (pid_t* p = procs; p != procs + MAX_SIZE; ++p) {
        int status = 0;
        waitpid(*p, &status, 0);

        if (status)
            printf("Error in process %ld: status code %d", p - procs, status);
    }

    FILE* const temp = fopen("temp.txt", "r");

    int res = 0;

    for (int i = 0; i < MAX_SIZE; ++i) {
        int v = 0;
        fscanf(temp, "%d", &v);
        res += v;
    }

    printf("Dot product result: %d\n", res);

    free(procs);
    fclose(temp);
    return 0;
}

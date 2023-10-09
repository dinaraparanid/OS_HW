#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    pthread_t id;
    int i;
    char message[256];
} thread;

void* print_message(void* const arg) {
    char* const message = arg;
    puts(message);
    pthread_exit(message);
}

int main() {
    size_t n = 0;
    printf("Number of threads: ");
    scanf("%lu", &n);

    thread* const data = malloc(n * sizeof(thread));

    void* res;

    for (int i = 0; i < n; ++i) {
        data[i].i = i;
        sprintf(data[i].message, "Hello from thread %i", i);

        pthread_create(&data[i].id, NULL, &print_message, &data[i].message);

        if (pthread_join(data[i].id, &res) == -1) {
            perror("pthread_join");
            return 1;
        }
    }

    free(data);
    return 0;
}
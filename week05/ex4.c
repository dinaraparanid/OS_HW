#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

int is_prime(int n) {
    if (n <= 1)
        return 0;

    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0)
            return 0;

    return 1;
}

int n = 0;
int current_number = 0; // k
int primes_cnt = 0; // c

int get_number_to_check() {
    return current_number != n ? current_number++ : -1;
}

void increment_primes() { ++primes_cnt; }

void* check_primes(void* const arg) {
    while (1) {
        int next_num;

        pthread_mutex_lock(&global_lock);
        next_num = get_number_to_check();
        pthread_mutex_unlock(&global_lock);

        if (next_num == -1)
            return 0;

        if (is_prime(next_num)) {
            pthread_mutex_lock(&global_lock);
            increment_primes();
            pthread_mutex_unlock(&global_lock);
        }
    }
}

int main(const int argc, char** const argv) {
    pthread_mutex_init(&global_lock, NULL);

    char* optend;

    const int n_arg = getopt(argc, argv, "n:m:");
    n = strtol(optarg, &optend, 10);

    const int m_arg = getopt(argc, argv, "n:m:");
    const int m = strtol(optarg, &optend, 10);

    pthread_t* const thrds = malloc(m * sizeof(pthread_t));
    void* res;

    int i = 0;
    for (pthread_t* p = thrds; p != thrds + m; ++p)
        pthread_create(p, NULL, &check_primes, NULL);

    for (pthread_t* p = thrds; p != thrds + m; ++p)
        pthread_join(*p, &res);

    printf("%d\n", primes_cnt);

    free(thrds);
    pthread_mutex_destroy(&global_lock);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
    int a;
    int b;
} prime_request;

int is_prime(int n) {
    if (n <= 1)
        return 0;

    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0)
            return 0;

    return 1;
}

size_t primes_count(const int a, const int b) {
    size_t ret = 0;

    for (int i = a; i < b ; ++i)
        if (is_prime(i))
            ++ret;

    return ret;
}

void* prime_counter(void* const arg) {
    const prime_request req = *(const prime_request* const) arg;
    size_t* const count = malloc(sizeof(size_t));
    *count = primes_count(req.a, req.b);
    return count;
}

int main(const int argc, char** const argv) {
    char* optend;

    const int n_arg = getopt(argc, argv, "n:m:");
    const int n = strtol(optarg, &optend, 10);

    const int m_arg = getopt(argc, argv, "n:m:");
    const int m = strtol(optarg, &optend, 10);

    pthread_t* const thrds = malloc(m * sizeof(pthread_t));
    prime_request* const reqs = malloc(m * sizeof(prime_request));

    const int part = n / m;

    int i = 0;
    for (int q = 0; q < m - 1; i += part, ++q) {
        reqs[q].a = i; reqs[q].b = i + part;
        pthread_create(&thrds[q], NULL, &prime_counter, reqs + q);
    }

    reqs[m - 1].a = i; reqs[m - 1].b = i + part + n % m;
    pthread_create(&thrds[m - 1], NULL, &prime_counter, reqs + m - 1);

    size_t cnt = 0;
    void* res;

    for (pthread_t* p = thrds; p != thrds + m; ++p) {
        pthread_join(*p, &res);
        cnt += *(size_t*) res;
    }

    printf("%lu\n", cnt);

    free(thrds);
    free(reqs);
    return 0;
}

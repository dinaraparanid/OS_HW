#include <stdio.h>
#include <stdint.h>

uint32_t tribonacci(const uint32_t n) {
    uint32_t f = 0;
    if (n == 0) return f;

    uint32_t s = 1;
    if (n == 1) return s;

    uint32_t t = 1;
    if (n == 2) return t;

    for (uint32_t i = 3; i <= n; ++i) {
        const uint32_t next = f + s + t;
        f = s;
        s = t;
        t = next;
    }

    return t;
}

int main() {
    printf("%u\n", tribonacci(4));
    printf("%u\n", tribonacci(36));
    return 0;
}
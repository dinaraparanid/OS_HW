#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int const_tri(int* const p, int n) {
    *p = 0;
    if (n == 0) return *p;

    p[1] = 1;
    if (n == 1) return p[1];

    p[2] = 1;
    if (n == 2) return p[2];

    n -= 3;

    while (n--) {
        p[2] += *p + p[1];
        p[1] = p[2] - p[1] - *p;
        *p = p[2] - p[1] - *p;
    }

    return p[2];
}

int main() {
    const int x = 1;
    const int* const q = &x;

    int* const p = malloc(3 * sizeof(int));
    *p = *q; printf("%p, ", p);
    p[1] = *q; printf("%p, ", p + 1);
    p[2] = 2 * *q; printf("%p\n", p + 2);

    assert((char*) (p + 1) - (char*)p == sizeof(int));
    assert((char*) (p + 2) - (char*) (p + 1) == sizeof(int));
    assert((char*) (p + 2) - (char*)p == 2 * sizeof(int));
    printf("8-th tribonacci number: %d", const_tri(p, 8));

    free(p);
    return 0;
}
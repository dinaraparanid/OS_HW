#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

const int INT_SUM_INIT_VALUE = 0;
const int INT_MUL_INIT_VALUE = 1;
const int INT_MAX_INIT_VALUE = INT32_MIN;

const double DOUBLE_SUM_INIT_VALUE = 0.0;
const double DOUBLE_MUL_INIT_VALUE = 1.0;
const double DOUBLE_MAX_INIT_VALUE = DBL_MIN;

void* aggregate(
        const void* const base,
        const size_t elem_size,
        const size_t n,
        const void* const initial_value,
        void* (*opr)(const void*, const void*)
) {
    void* res = malloc(elem_size);
    memmove(res, initial_value, elem_size);

    for (const char* p = base; p != base + elem_size * n; p += elem_size) {
        void* res2 = opr(res, p);
        free(res);
        res = res2;
    }

    return res;
}

#define __BIN_OP__(A, B, O, T) \
    const T arg1 = *(const T*) (A); \
    const T arg2 = *(const T*) (B); \
    const T res = arg1 O arg2;      \
    T* const ans = malloc(sizeof(T)); \
    memmove(ans, &res, sizeof(T));    \
    return ans

#define __MAX_OP__(A, B, T) \
    const T arg1 = *(const T*) (A); \
    const T arg2 = *(const T*) (B); \
    const T res = arg1 > arg2 ? arg1 : arg2; \
    T* const ans = malloc(sizeof(T)); \
    memmove(ans, &res, sizeof(T)); \
    return ans

void* sum_int(const void* const lhs, const void* const rhs) {
    __BIN_OP__(lhs, rhs, +, int);
}

void* sum_double(const void* const lhs, const void* const rhs) {
    __BIN_OP__(lhs, rhs, +, double);
}

void* mul_int(const void* const lhs, const void* const rhs) {
    __BIN_OP__(lhs, rhs, *, int);
}

void* mul_double(const void* lhs, const void* rhs) {
    __BIN_OP__(lhs, rhs, *, double);
}

void* max_int(const void* lhs, const void* rhs) {
    __MAX_OP__(lhs, rhs, int);
}

void* max_double(const void* lhs, const void* rhs) {
    __MAX_OP__(lhs, rhs, double);
}

int main() {
    int int_arr[] = {1, 2, 3, 4, 5};
    double double_arr[] = {2.5, 5.0, 7.5, 10, 12.5};

    int* const int_sum_res = aggregate(int_arr, sizeof(int), 5, &INT_SUM_INIT_VALUE, sum_int);
    printf("%d\n", *int_sum_res);
    free(int_sum_res);

    int* const int_mul_res = aggregate(int_arr, sizeof(int), 5, &INT_MUL_INIT_VALUE, mul_int);
    printf("%d\n", *int_mul_res);
    free(int_mul_res);

    int* const int_max_res = aggregate(int_arr, sizeof(int), 5, &INT_MAX_INIT_VALUE, max_int);
    printf("%d\n", *int_max_res);
    free(int_max_res);

    double* const double_sum_res = aggregate(double_arr, sizeof(double), 5, &DOUBLE_SUM_INIT_VALUE, sum_double);
    printf("%lf\n", *double_sum_res);
    free(double_sum_res);

    double* const double_mul_res = aggregate(double_arr, sizeof(double), 5, &DOUBLE_MUL_INIT_VALUE, mul_double);
    printf("%lf\n", *double_mul_res);
    free(double_mul_res);

    double* const double_max_res = aggregate(double_arr, sizeof(double), 5, &DOUBLE_MAX_INIT_VALUE, max_double);
    printf("%lf\n", *double_max_res);
    free(double_max_res);

    return 0;
}
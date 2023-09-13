#include <stdio.h>
#include <stdint.h>
#include <float.h>

int main() {
    int32_t i = INT32_MAX;
    uint16_t uh = UINT16_MAX;
    int64_t ld = INT64_MAX;
    float f = FLT_MAX;
    double d = DBL_MAX;

    printf("i32: size %lu value %d\n", sizeof(i), i);
    printf("u16: size %lu value %d\n", sizeof(uh), uh);
    printf("i64: size %lu value %ld\n", sizeof(ld), ld);
    printf("f32: size %lu value %f\n", sizeof(f), f);
    printf("f64: size %lu value %f\n", sizeof(d), d);
    return 0;
}
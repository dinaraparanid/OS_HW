#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void print_convert_error() { puts("cannot convert!"); }

int correct_radix(const int radix) {
    return radix >= 2 && radix <= 10;
}

int belong_correct_radix(const int64_t number, const int radix) {
    char buf[100];
    sprintf(buf, "%ld", number);

    const int len = strlen(buf);

    // foreach-like loop
    for (char* c = buf; c != buf + len; ++c)
        if (*c - '0' >= radix)
            return 0;

    return 1;
}

int validate_conversion(const int64_t number, const int init_radix, const int final_radix) {
    if (!correct_radix(init_radix) || !correct_radix(final_radix)) {
        print_convert_error();
        return 0;
    }

    if (!belong_correct_radix(number, init_radix)) {
        print_convert_error();
        return 0;
    }

    return 1;
}

int64_t convert_to_ten_radix(const int64_t number, const int init_radix) {
    if (init_radix == 10)
        return number;

    const int is_negative = number < 0;
    const int64_t absolute = llabs(number);

    char buf[100];
    sprintf(buf, "%ld", absolute);

    int acc = 0;
    const int len = strlen(buf);

    // foreach-like loop
    for (char* c = buf; c != buf + len; ++c)
        acc += (*c - '0') * pow(init_radix, len - 1 - (c - buf));

    return is_negative ? -acc : acc;
}

// Can be replaced with an indexed version,
// but for estetical purposes,
// I use adress arithmetics which is also *(p + i) or p[i]

void print_revert(const char* const left, const char* const right_exclusive) {
    for (const char* c = right_exclusive - 1; c != left; --c)
        putchar(*c);
    putchar(*left); putchar('\n');
}

void print_converted_to_radix(const int64_t number, const int radix) {
    char res[200];
    int size = 0;

    const int is_negative = number < 0;
    int64_t absolute = llabs(number);

    while (absolute >= radix) {
        size += sprintf(res + size, "%ld", absolute % radix);
        absolute /= radix;
    }

    size += sprintf(res + size, "%ld", absolute);
    if (is_negative) putchar('-');
    print_revert(res, res + size);
}

void perform_conversion(const int64_t number, const int init_radix, const int final_radix) {
    const int64_t tenth_radix = convert_to_ten_radix(number, init_radix);

    final_radix == 10
        ? printf("%ld\n", tenth_radix)
        : print_converted_to_radix(tenth_radix, final_radix);
}

void convert(const int64_t number, const int init_radix, const int final_radix) {
    if (!validate_conversion(number, init_radix, final_radix))
        return;

    perform_conversion(number, init_radix, final_radix);
}

int main() {
    puts("Print number, initial radix and final radix");

    int64_t number = 0;
    int init = 0, final = 0;
    scanf("%ld %d %d", &number, &init, &final);

    convert(number, init, final);
    return 0;
}

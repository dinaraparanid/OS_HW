#include <stdio.h>
#include <string.h>
#include <ctype.h>

void trim_string(char* const str) {
    str[strcspn(str, "\n")] = '\0';
}

size_t count(const char* const str, const char c) {
    size_t cnt = 0;

    const size_t len = strlen(str);
    const char lower = tolower(c);

    for (const char* p = str; p != str + len; ++p)
        cnt += tolower(*p) == lower ? 1 : 0;

    return cnt;
}

void count_all(char* const str) {
    trim_string(str);

    const size_t len = strlen(str);
    const char* p = str;

    for (; p != str + len - 1; ++p)
        printf("%c:%lu, ", tolower(*p), count(str, *p));

    printf("%c:%lu\n", tolower(*p), count(str, *p));
}

int main() {
    puts("Type a string:");

    char input[300];
    fgets(input, 256, stdin);
    count_all(input);

    return 0;
}
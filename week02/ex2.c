#include <stdio.h>
#include <string.h>

void print_revert(const char* const left, const char* const right_exclusive) {
    for (const char* c = right_exclusive - 1; c != left; --c)
        putchar(*c);
    putchar(*left); putchar('\n');
}

int main() {
    puts("Type a string character-by-character:");

    char input[300];
    fgets(input, 256, stdin);

    const char* find = strchr(input, '.');
    find != NULL ? print_revert(input, find) : print_revert(input, input + strlen(input));

    return 0;
}
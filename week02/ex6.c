#include <stdio.h>

// *******

void row(const int n) {
    if (n == 0) {
        putchar('\n');
        return;
    }

    putchar('*');
    row(n - 1);
}

// *
// **
// ***
// ****
// *****
// ******
// *******

void ladder(const int n) {
    if (n < 1) return;
    ladder(n - 1);
    row(n);
}

// *******
// ******
// *****
// ****
// ***
// **
// *

void inversed_ladder(const int n) {
    if (n < 1) return;
    row(n);
    inversed_ladder(n - 1);
}

// *
// **
// ***
// ****
// ***
// **
// *

void hill(const int n) {
    const int mid = n >> 1;
    ladder(mid);
    if (n % 2 != 0) row(mid + 1);
    inversed_ladder(mid);
}

// *******
// *******
// *******
// *******

void rectangle(const int width, const int height) {
    if (height == 0) return;
    row(width);
    rectangle(width, height - 1);
}

// *******
// *******
// *******
// *******
// *******
// *******
// *******

void square(const int n) { rectangle(n, n); }

int main() {
    puts("Type number:");

    int n = 0;
    scanf("%d", &n);

    puts("Ladder:\n");
    ladder(n);
    putchar('\n');

    puts("Hill:\n");
    hill(n);
    putchar('\n');

    puts("Square:\n");
    square(n);

    return 0;
}
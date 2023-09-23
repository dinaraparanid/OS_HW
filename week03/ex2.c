#include <stdio.h>
#include <math.h>

typedef struct {
    double x;
    double y;
} point;

double distance(const point p1, const point p2) {
    return sqrt(pow((p1.x - p2.x), 2) + pow((p1.y - p2.y), 2));
}

double area(const point p1, const point p2, const point p3) {
    return (p1.x * p2.y - p2.x * p1.y + p2.x - p3.y - p3.x * p2.y + p3.x * p1.y - p1.x - p3.y) / 2;
}

int main() {
    const point a = { .x = 2.5, .y = 6 };
    const point b = { .x = 1, .y = 2.2 };
    const point c = { .x = 10, .y = 6 };

    printf("Distance between A and B: %lf\n", distance(a, b));
    printf("Area of ABC: %lf\n", area(a, b, c));

    return 0;
}
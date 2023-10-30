#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

const int PASSWORD_LEN = 8;
const int MSG_LEN = 5 + 8;

int main() {
    const pid_t pid = getpid();
    printf("PID: %d\n", pid);

    FILE* const fin = fopen("/tmp/ex1.pid", "w");
    fprintf(fin, "%d", pid);
    fclose(fin);

    char mem_file[100];
    sprintf(mem_file, "/proc/%d/mem", pid);

    char* const password = malloc((PASSWORD_LEN + 1) * sizeof(char));

    sprintf(password, "pass:");

    for (int i = 5; i < MSG_LEN;) {
        FILE* const random = fopen("/dev/random", "r");
        const char c = fgetc(random);
        if (isprint(c)) password[i++] = c;
        fclose(random);
    }

    password[MSG_LEN] = '\0';
    puts(password);

    for (;;);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void print_content() {
    FILE* input = fopen("text.txt", "r");
    char buffer[200];

    while (fgets(buffer, 200, input) != NULL)
        puts(buffer);

    fclose(input);
}

void terminate() {
    puts("Process terminating...");
    exit(0);
}

void sig_handler(const int signum) {
    switch (signum) {
        case SIGUSR1:
            print_content();
            break;

        case SIGUSR2: terminate();
        default: break;
    }
}

int main() {
    uid_t uid = getuid();
    gid_t gid = getgid();

    if (setregid(gid, 0) == -1 || setreuid(uid, 0) == -1) {
        puts("`sudo` mode is required for this app");
        exit(0);
    }

    FILE* agent_pid = fopen("/var/run/os_hw_agent.pid", "w");
    fclose(agent_pid);

    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    for (;;) sleep(1);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

pid_t agent_pid = 0;

#define PANIC_AGENT_NOT_FOUND \
    puts("Error: No agent found"); \
    return 0

void print_menu() {
    printf("Choose command {“read”, “exit”, “stop”, “continue”} to send to the agent: ");
}

void sig_handler(const int signum) {
    switch (signum) {
        case SIGINT: {
            if (agent_pid != 0)
                kill(agent_pid, SIGTERM);
            exit(0);
        }

        default: signal(SIGINT, SIG_DFL);
    }
}

void trim_string(char* const str) {
    str[strcspn(str, "\n")] = '\0';
}

int main() {
    FILE* agent_pid_f = fopen("/var/run/os_hw_agent.pid", "r");

    if (agent_pid_f == NULL) {
        PANIC_AGENT_NOT_FOUND;
    }

    if (fscanf(agent_pid_f, "%d", &agent_pid) != 1) {
        PANIC_AGENT_NOT_FOUND;
    }

    puts("Agent found");
    fclose(agent_pid_f);

    for (;;) {
        print_menu();

        char command[50];
        char* const err = fgets(command, 50, stdin);

        if (err == NULL)
            continue;

        trim_string(command);

        if (strcmp(command, "read") == 0) {
            kill(agent_pid, SIGUSR1);
        } else if (strcmp(command, "exit") == 0) {
            kill(agent_pid, SIGUSR2);
            return 0;
        } else if (strcmp(command, "stop") == 0) {
            kill(agent_pid, SIGSTOP);
        } else if (strcmp(command, "continue") == 0) {
            kill(agent_pid, SIGCONT);
        }
    }

    return 0;
}
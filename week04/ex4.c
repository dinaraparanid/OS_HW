#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

const size_t MAX_LINE_SIZE = 500;
const size_t MAX_COMMAND_SIZE = 100;

void trim_string(char* const str) {
    str[strcspn(str, "\n")] = '\0';
}

int main() {
    while (1) {
        putchar('$'); putchar(' ');
        char line[MAX_LINE_SIZE];
        fgets(line, MAX_LINE_SIZE, stdin);

        char* tok = strtok(line, " ");

        char command[MAX_COMMAND_SIZE];
        sprintf(command, "%s", "/bin/");
        strcpy(command + 5, tok);
        trim_string(command);

        size_t new_argc = 0;
        char** new_argv = malloc(sizeof(char*));

        int is_background = 0;

        while (tok != NULL) {
            char* arg = malloc(strlen(tok) + 1);
            strcpy(arg, tok);
            trim_string(arg);

            if (strcmp(arg, "&") == 0) {
                is_background = 1;
                free(arg);
                break;
            }

            new_argv[new_argc++] = arg;
            tok = strtok(NULL, " ");
        }

        const pid_t child = fork();

        if (!child) {
            execv(command, new_argv);
            exit(EXIT_FAILURE);
        } else {
            if (!is_background) {
                int status = 0;
                waitpid(child, &status, 0);
            }
        }
    }

    return 0;
}
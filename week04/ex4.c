#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

const size_t MAX_COMMAND_SIZE = 1000;

void trim_string(char* const str) {
    str[strcspn(str, "\n")] = '\0';
}

int main() {
    while (1) {
        const pid_t child = fork();

        if (!child) {
            putchar('$'); putchar(' ');
            char line[MAX_COMMAND_SIZE];
            fgets(line, MAX_COMMAND_SIZE, stdin);

            char* tok = strtok(line, " ");

            char command[200];
            sprintf(command, "%s", "/bin/");
            strcpy(command + 5, tok);
            trim_string(command);

            size_t new_argc = 0;
            char** new_argv = malloc(sizeof(char*));

            while (tok != NULL) {
                new_argv = realloc(new_argv, ++new_argc * sizeof(char*));
                new_argv[new_argc - 1] = malloc(strlen(tok) + 1);
                strcpy(new_argv[new_argc - 1], tok);
                trim_string(new_argv[new_argc - 1]);
                tok = strtok(NULL, " ");
            }

            execv(command, new_argv);
            exit(EXIT_FAILURE);
        }

        int status = 0;
        waitpid(child, &status, 0);
    }

    return 0;
}
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

const int MAX_MESSAGE_SIZE = 0x400;

void publisher(const int wfd) {
    char msg[MAX_MESSAGE_SIZE];

    while (1) {
        fgets(msg, MAX_MESSAGE_SIZE, stdin);

        const ssize_t bytes = write(wfd, msg, strlen(msg) + 1);

        if (bytes == -1) {
            perror("write");
            return;
        }
    }
}

void subscriber(const int rfd) {
    char msg[MAX_MESSAGE_SIZE];

    while (1) {
        const ssize_t bytes = read(rfd, msg, MAX_MESSAGE_SIZE);

        if (bytes == -1) {
            perror("read");
            return;
        }

        puts(msg);
    }
}

int main() {
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    const pid_t ps = fork();

    if (!ps) {
        publisher(fd[1]);
        return 0;
    }

    const pid_t sc = fork();

    if (!sc) {
        subscriber(fd[0]);
        return 0;
    }

    int ps_stat = 0;
    waitpid(ps, &ps_stat, 0);

    if (ps_stat == -1) {
        perror("waitpid");
        return -1;
    }

    int sc_stat = 0;
    waitpid(sc, &sc_stat, 0);

    if (sc_stat == -1) {
        perror("waitpid");
        return -1;
    }

    close(fd[0]); close(fd[1]);
    return 0;
}
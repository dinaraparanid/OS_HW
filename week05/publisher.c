#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

const int MAX_MESSAGE_SIZE = 0x400;

void child_publisher(const size_t ind, const int fd) {
    char fifo_name[100];
    sprintf(fifo_name, "%s%lu", "/tmp/ex1/s", ind);

    mkdir("/tmp/ex1", 0755);
    int fifo_fd = mkfifo(fifo_name, 0644);

    if (fifo_fd != -1)
        close(fifo_fd);

    char msg[MAX_MESSAGE_SIZE];

    while (1) {
        const ssize_t bytes_read = read(fd, msg, MAX_MESSAGE_SIZE);

        if (bytes_read == -1) {
            perror("read");
            return;
        }

        fifo_fd = open(fifo_name, O_WRONLY);

        if (fifo_fd == -1) {
            perror("open");
            return;
        }

        const ssize_t bytes_write = write(fifo_fd, msg, bytes_read);
        close(fifo_fd);

        if (bytes_write == -1) {
            perror("write");
            return;
        }
    }
}

int main(const int argc, const char** const argv) {
    const char* const nstr = argv[1];

    int n = 0;
    sscanf(nstr, "%d", &n);

    pid_t* const children = malloc(n * sizeof(pid_t));
    int** const pipes = malloc(n * sizeof(int*));

    for (int** p = pipes; p != pipes + n; ++p) {
        *p = malloc(2 * sizeof(int));
        pipe(*p);
    }

    for (int i = 0; i < n; ++i) {
        children[i] = fork();

        if (!children[i]) {
            child_publisher(i, pipes[i][0]);
            return 0;
        }
    }

    while (1) {
        char msg[MAX_MESSAGE_SIZE];
        fgets(msg, MAX_MESSAGE_SIZE, stdin);

        for (int** p = pipes; p != pipes + n; ++p) {
            const ssize_t bytes = write((*p)[1], msg, strlen(msg) + 1);

            if (bytes == -1) {
                perror("write");
                goto CLEAN;
            }
        }
    }

    CLEAN:
    free(children);

    for (int** p = pipes; p != pipes + n; ++p)
        free(*p);
    free(pipes);

    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>

const int MAX_MESSAGE_SIZE = 0x400;

int main(const int argc, const char** const argv) {
    const char* const idstr = argv[1];

    size_t id = 0;
    sscanf(idstr, "%lu", &id);

    char fifo_name[100];
    sprintf(fifo_name, "%s%lu", "/tmp/ex1/s", id);

    char msg[MAX_MESSAGE_SIZE];

    while (1) {
        int fd = open(fifo_name, O_RDONLY);

        if (fd == -1) {
            perror("open");
            return 1;
        }

        const ssize_t bytes = read(fd, msg, MAX_MESSAGE_SIZE);
        close(fd);

        if (bytes == -1) {
            perror("read");
            return 1;
        }

        puts(msg);
    }

    return 0;
}
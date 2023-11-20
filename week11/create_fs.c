#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/**
 * Creates a file to act as a disk and
 * formats the file system residing on the disk
 */

int main(const int argc, const char** const argv) {
    if (argc == 1) {
        fprintf(stderr, "usage: %s <disk-file-name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Creating  a 128KB  file in %s\n", argv[1]);
    puts("This file will act as a dummy disk and will hold your filesystem");

    // Open the disk file for writing
    const int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    puts("Formatting your filesystem...");

    char* const buf = calloc(1024, 1);

    // Write the superblock

    // Mark superblock as allocated in the free block list
    // all other blocks are free, all inodes are zeroed out

    buf[0] = 1;

    // Write out the superblock

    if (write(fd, buf, 1024) < 0) {
        perror("write");
        close(fd); free(buf);
        return EXIT_FAILURE;
    }

    buf[0] = 0;

    // Write out the remaining 127 data blocks, all zeroed out

    for (int i = 0; i < 127; ++i) {
        if (write(fd, buf, 1024) < 0) {
            perror("write");
            close(fd); free(buf);
            return EXIT_FAILURE;
        }
    }

    close(fd); free(buf);
    return EXIT_SUCCESS;
}
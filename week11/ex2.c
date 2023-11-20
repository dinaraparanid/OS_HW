#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

const char CREATE = 'C';
const char LIST = 'L';
const char READ = 'R';
const char WRITE = 'W';
const char DELETE = 'D';

const int FREE_BLOCK_LIST_SIZE = 1 << 7;
const int INODES_SIZE = 1 << 4;

typedef struct {
    /** File name */
    char name[16];

    /** File size (in number of blocks) */
    int size;

    /** Direct block pointers */
    int block_pointers[8];

    /** 0 => inode is free; 1 => in use */
    int used;
} inode;

const int DATA_BLOCKS_OFFSET = FREE_BLOCK_LIST_SIZE + INODES_SIZE * sizeof(inode);

void trim_string(char* const str) {
    str[strcspn(str, "\n")] = '\0';
}

void read_free_block_list(FILE* const fs, char blocks[130]) {
    fseek(fs, 0, SEEK_SET);
    fread(blocks, 1, FREE_BLOCK_LIST_SIZE, fs);
}

void write_free_block_list(FILE* const fs, char blocks[130]) {
    fseek(fs, 0, SEEK_SET);
    fwrite(blocks, 1, FREE_BLOCK_LIST_SIZE, fs);
}

void write_inode(FILE* const fs, const inode* const node, const int inode_index) {
    fseek(fs, FREE_BLOCK_LIST_SIZE + inode_index * sizeof(inode), SEEK_SET);
    fwrite(node, sizeof(inode), 1, fs);
}

/** Create a file with this name and this size */

int create(FILE* const fs, const char name[16], const int size) {
    char blocks[130];
    read_free_block_list(fs, blocks);

    int free_blocks = 0;

    for (int i = 0; i < FREE_BLOCK_LIST_SIZE && free_blocks < size; ++i)
        if (blocks[i] == 0)
            ++free_blocks;

    if (free_blocks < size) {
        fprintf(stderr, "Not enough memory to allocate file %s with %d blocks\n", name, size);
        return -1;
    }

    inode free_inode;
    int free_inode_index = 0;

    for (; free_inode_index < INODES_SIZE; ++free_inode_index) {
        fread(&free_inode, sizeof(inode), 1, fs);

        if (!free_inode.used) {
            free_inode.used = 1;
            strcpy(free_inode.name, name);
            free_inode.size = size;
            break;
        }
    }

    for (int i = 0, bp = 0; i < size; ++i) {
        if (blocks[bp] == 0) {
            blocks[bp] = 1;
            free_inode.block_pointers[i] = bp;
        }
    }

    write_free_block_list(fs, blocks);
    write_inode(fs, &free_inode, free_inode_index);

    return 0;
}

/** Delete the file with this name */

int delete(FILE* const fs, const char name[16]) {
    fseek(fs, FREE_BLOCK_LIST_SIZE, SEEK_SET);

    for (int i = 0; i < INODES_SIZE; ++i) {
        inode node;
        fread(&node, sizeof(inode), 1, fs);

        if (!node.used && strcmp(node.name, name) == 0) {
            char blocks[130];
            read_free_block_list(fs, blocks);

            for (int* free_block = node.block_pointers; free_block != node.block_pointers + node.size; ++free_block)
                blocks[*free_block] = 0;

            node.used = 0;

            write_free_block_list(fs, blocks);
            write_inode(fs, &node, i);
            return 0;
        }
    }

    return -1;
}

/** List names of all files on disk */

int ls(FILE* const fs) {
    inode node;
    fseek(fs, FREE_BLOCK_LIST_SIZE, SEEK_SET);

    for (int i = 0; i < INODES_SIZE; ++i) {
        fread(&node, sizeof(inode), 1, fs);

        if (node.used)
            printf("file: %s blocks: %d\n", node.name, node.size);
    }

    return 0;
}

/** Read this block from this file */

int _read(FILE* const fs, const char name[16], const int block_num, char buf[1024]) {
    fseek(fs, FREE_BLOCK_LIST_SIZE, SEEK_SET);

    for (int i = 0; i < INODES_SIZE; ++i) {
        inode node;
        fread(&node, sizeof(inode), 1, fs);

        if (node.used && strcmp(node.name, name) == 0) {
            if (block_num >= node.size) {
                fprintf(stderr, "Block number %d cannot be >= then total number of blocks %d\n", block_num, node.size);
                return -1;
            }

            const int addr = node.block_pointers[block_num];
            fseek(fs, DATA_BLOCKS_OFFSET + addr * 1024, SEEK_SET);
            fread(buf, 1024, 1, fs);
            return 0;
        }
    }

    return -1;
}

/** Write this block to this file */

int _write(FILE* const fs, const char name[16], const int block_num, char buf[1024]) {
    fseek(fs, FREE_BLOCK_LIST_SIZE, SEEK_SET);

    for (int i = 0; i < INODES_SIZE; ++i) {
        inode node;
        fread(&node, sizeof(inode), 1, fs);

        if (node.used && strcmp(node.name, name) == 0) {
            if (block_num >= node.size) {
                fprintf(stderr, "Block number %d cannot be >= then total number of blocks %d\n", block_num, node.size);
                return -1;
            }

            const int addr = node.block_pointers[block_num];
            fseek(fs, DATA_BLOCKS_OFFSET + addr * 1024, SEEK_SET);
            fwrite(buf, 1024, 1, fs);
            return 0;
        }
    }

    return -1;
}

void on_create(FILE* const fs, char* const input) {
    char c = 0; char filename[16]; int filesize = 0;
    sscanf(input, "%c %s %d", &c, filename, &filesize);
    trim_string(filename);

    puts("Create is called");
    create(fs, filename, filesize);
}

void on_delete(FILE* const fs, char* const input) {
    char c = 0, filename[16];
    sscanf(input, "%c, %s", &c, filename);
    trim_string(filename);

    puts("Delete is called");
    delete(fs, filename);
}

void on_ls(FILE* const fs) {
    puts("Ls is called");
    ls(fs);
}

void on_read(FILE* const fs, char* const input) {
    char c = 0, filename[16]; int block_num = 0;
    sscanf(input, "%c %s %d", &c, filename, &block_num);
    trim_string(filename);

    char buf[1024];

    if (_read(fs, filename, block_num, buf) == 0) {
        puts("Read is called");
        puts(buf);
    }
}

void gen_bebra_msg(char buf[1024]) {
    for (int i = 0; i < 1023; ++i) {
        if (i % 5 == 0) { buf[i] = 'B'; continue; }
        if (i % 5 == 1) { buf[i] = 'E'; continue; }
        if (i % 5 == 2) { buf[i] = 'B'; continue; }
        if (i % 5 == 3) { buf[i] = 'R'; continue; }
                          buf[i] = 'A';
    }

    buf[1023] = '\0';
}

void on_write(FILE* const fs, char* const input) {
    char c = 0, filename[16]; int block_num = 0;
    sscanf(input, "%c %s %d", &c, filename, &block_num);
    trim_string(filename);

    char buf[1024];
    gen_bebra_msg(buf);

    if (_write(fs, filename, block_num, buf) == 0) {
        puts("Write is called");
        puts(buf);
    }
}

int main(const int argc, const char** argv) {
    if (argc == 1) {
        fprintf(stderr, "usage: %s <input-file-name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* const fin = fopen(argv[1], "rb");

    if (fin == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    char fs_filename[16];

    if (fgets(fs_filename, 16, fin) == NULL) {
        perror("fgets");
        fclose(fin);
        return EXIT_FAILURE;
    }

    trim_string(fs_filename);
    FILE* const fs = fopen(fs_filename, "rb+");

    while (!feof(fin)) {
        char buf[50];
        fgets(buf, 50, fin);

        char command = 0;
        sscanf(buf, "%c", &command);

        if (command == CREATE) {
            on_create(fs, buf);
        } else if (command == LIST) {
            on_ls(fs);
        } else if (command == READ) {
            on_read(fs, buf);
        } else if (command == WRITE) {
            on_write(fs, buf);
        } else if (command == DELETE) {
            on_delete(fs, buf);
        }
    }

    fclose(fin);
    fclose(fs);
    return 0;
}
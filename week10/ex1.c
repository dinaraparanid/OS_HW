#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/inotify.h>

typedef struct dirent dirent;
typedef struct stat file_stat;

ino_t src_inode;

char* construct_file_path(const char* const parent_dir, const char* const file) {
    const long path_len = strlen(parent_dir);
    char* const full_path = malloc(PATH_MAX);
    strcpy(full_path, parent_dir);
    full_path[path_len] = '/';
    strcpy(full_path + path_len + 1, file);
    return full_path;
}

file_stat try_file_stat(const char* const path) {
    file_stat s;

    if (stat(path, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    return s;
}

int filter(const dirent* const entry) {
    return entry->d_ino == src_inode ? 1 : 0;
}

void print_hlink(const dirent* const e) {
    puts("-------- Hard-Link --------");
    printf("I-node number: %ju\n", (uintmax_t) e->d_ino);

    char buf[PATH_MAX];
    realpath(e->d_name, buf);
    printf("Path: %s\n", buf);
}

int hlinks(const char* const parent_path, const char* const source, dirent*** const entries) {
    src_inode = try_file_stat(source).st_ino;

    const int ent_sz = scandir(parent_path, entries, filter, NULL);

    if (ent_sz == -1) {
        perror("scandir");
        return -1;
    }

    return ent_sz;
}

void print_stat(const char* const file) {
    file_stat sb;

    if (stat(file, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    printf("File type:                ");

    switch (sb.st_mode & S_IFMT) {
        case S_IFBLK:  puts("block device");            break;
        case S_IFCHR:  puts("character device");        break;
        case S_IFDIR:  puts("directory");               break;
        case S_IFIFO:  puts("FIFO/pipe");               break;
        case S_IFLNK:  puts("symlink");                 break;
        case S_IFREG:  puts("regular file");            break;
        case S_IFSOCK: puts("socket");                  break;
        default:       puts("unknown?");                break;
    }

    printf("Path:                     %s\n", file);
    printf("I-node number:            %ju\n", (uintmax_t) sb.st_ino);
    printf("Mode:                     %jo (octal)\n", (uintmax_t) sb.st_mode);
    printf("Link count:               %ju\n", (uintmax_t) sb.st_nlink);
    printf("Ownership:                UID=%ju   GID=%ju\n", (uintmax_t) sb.st_uid, (uintmax_t) sb.st_gid);
    printf("Preferred I/O block size: %jd bytes\n", (intmax_t) sb.st_blksize);
    printf("File size:                %jd bytes\n", (intmax_t) sb.st_size);
    printf("Blocks allocated:         %jd\n", (intmax_t) sb.st_blocks);
    printf("Last status change:       %s", ctime(&sb.st_ctime));
    printf("Last file access:         %s", ctime(&sb.st_atime));
    printf("Last file modification:   %s", ctime(&sb.st_mtime));
}

void find_all_hlinks(const char* const path, const char* const source) {
    dirent** entries;
    const int ent_sz = hlinks(path, source, &entries);

    if (ent_sz == -1)
        exit(EXIT_FAILURE);

    for (dirent** e = entries; e != entries + ent_sz; ++e)
        print_hlink(*e);

    free(entries);
}

void unlink_all(const char* const path, const char* const source) {
    dirent** entries;

    const int ent_sz = hlinks(path, source, &entries);

    if (ent_sz == -1)
        exit(EXIT_FAILURE);

    char* last_hlink = construct_file_path(path, (**entries).d_name);
    const file_stat tmp_stat = try_file_stat(last_hlink);
    const char* last_modified = ctime(&tmp_stat.st_mtime);

    for (dirent** e = entries + 1; e != entries + ent_sz; ++e) {
        char* const hlink = construct_file_path(path, (**e).d_name);
        const file_stat stat = try_file_stat(hlink);
        const char* const modified = ctime(&stat.st_mtime);
        const int cmp = strcmp(modified, last_modified);

        if (cmp <= 0) {
            unlink(hlink);
            free(hlink);
        } else if (cmp > 0) {
            last_modified = modified;
            unlink(last_hlink);
            free(last_hlink);
            last_hlink = hlink;
        }
    }

    printf("Actual hardlink: %s\n", last_hlink);
    print_stat(last_hlink);

    free(last_hlink);
    free(entries);
}

void create_hard_link(const char* const source, const char* const lnk) {
    if (link(source, lnk) == -1) {
        perror("link");
        exit(EXIT_FAILURE);
    }
}

void create_sym_link(const char* const source, const char* const link) {
    if (symlink(source, link) == -1) {
        perror("symlink");
        exit(EXIT_FAILURE);
    }
}

void create_file(const char* const path) {
    FILE* const file = fopen(path, "w");
    fputs("Hello world.", file);
    fclose(file);
}

void modify_file(const char* const path) {
    FILE* const file = fopen(path, "w");
    fprintf(file, "BEBRA %s", path);
    fclose(file);
}

int main(const int argc, const char** const argv) {
    const char* const path = argv[1];

    if (path == NULL) {
        fputs("Path was not provided", stderr);
        return EXIT_FAILURE;
    }

    char* const myfile1_path = construct_file_path(path, "myfile1.txt");
    char* const myfile11_path = construct_file_path(path, "myfile11.txt");
    char* const myfile12_path = construct_file_path(path, "myfile12.txt");
    char* const myfile13_path = construct_file_path(path, "myfile13.txt");

    puts("********** Create myfile1.txt **********");
    create_file(myfile1_path);
    sleep(3);

    puts("********** Hard-Link myfile11.txt **********");
    create_hard_link(myfile1_path, myfile11_path);
    sleep(3);

    puts("********** Hard-Link myfile12.txt **********");
    create_hard_link(myfile1_path, myfile12_path);
    sleep(3);

    puts("********** Hard-Links for myfile1.txt **********");
    find_all_hlinks(path, myfile1_path);
    sleep(3);

    puts("********** Move myfile1.txt **********");
    rename(myfile1_path, "/tmp/myfile1.txt");
    sleep(3);

    puts("********** Modify myfile11.txt **********");
    modify_file(myfile11_path);
    sleep(3);

    puts("********** Symlink myfile13.txt **********");
    create_sym_link("/tmp/myfile1.txt", myfile13_path);
    sleep(3);

    puts("********** Modify /tmp/myfile1.txt **********");
    modify_file("/tmp/myfile1.txt");
    sleep(3);

    puts("********** Unlink All **********");
    unlink_all(path, myfile11_path);
    sleep(3);

    free(myfile1_path);
    free(myfile11_path);
    free(myfile12_path);
    free(myfile13_path);
    return 0;
}
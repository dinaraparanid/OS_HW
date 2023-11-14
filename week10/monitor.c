#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/inotify.h>

typedef struct dirent dirent;
typedef struct inotify_event inotify_event;
typedef struct stat file_stat;

const char* path;

char** entries;
size_t ent_sz = 0;

int* wds;
size_t wds_sz = 0;

char* construct_file_path(const char* const parent_dir, const char* const file) {
    const long path_len = strlen(parent_dir);
    char* const full_path = malloc(PATH_MAX);
    strcpy(full_path, parent_dir);
    full_path[path_len] = '/';
    strcpy(full_path + path_len + 1, file);
    return full_path;
}

DIR* try_dir() {
    DIR* const dir = opendir(path);

    for (dirent* e = readdir(dir);; e = readdir(dir)) {
        if (e == NULL)
            break;

        entries = realloc(entries, ++ent_sz * sizeof(char*));
        entries[ent_sz - 1] = construct_file_path(path, e->d_name);
    }

    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    return dir;
}

void print_stat(const char* const file) {
    file_stat sb;
    printf("Try file stat: %s\n", file);

    if (stat(file, &sb) == -1)
        return;

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

void full_stat() {
    print_stat(path);

    for (char** e = entries; e != entries + ent_sz; ++e)
        print_stat(*e);
}

void handle_signal(const int signum) {
    if (signum == SIGINT) {
        full_stat();
        free(entries);
        free(wds);
        exit(EXIT_SUCCESS);
    }
}

const char* try_path(const char** const argv) {
    const char* const p = argv[1];

    if (p == NULL) {
        fputs("Path was not provided", stderr);
        exit(EXIT_FAILURE);
    }

    return p;
}

int try_inotify_init() {
    const int fd = inotify_init();

    if (fd == -1) {
        perror("inotify_init");
        return EXIT_FAILURE;
    }

    return fd;
}

int* try_wds() {
    int* const wdss = calloc(1, sizeof(int));

    if (wdss == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return wdss;
}

void subscribe(const int fd, const char* const source) {
    int* const wdss = realloc(wds, ++wds_sz * sizeof(int));

    if (wdss == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }

    wds = wdss;

    const int res = inotify_add_watch(
            fd,
            source,
            IN_ACCESS | IN_CREATE | IN_DELETE | IN_MODIFY | IN_OPEN | IN_ATTRIB
    );

    if (res == -1) {
        fprintf(stderr, "Cannot watch %s: %s", source, strerror(errno));
        exit(EXIT_FAILURE);
    }

    wds[wds_sz - 1] = res;
}

void add_file_to_entries(char* const path) {
    entries = realloc(entries, ++ent_sz * sizeof(char*));
    entries[ent_sz - 1] = path;
}

void launch_monitoring(const int fd) {
    char buf[4096] __attribute__ ((aligned(__alignof__(inotify_event))));

    const inotify_event* event;
    const int path_len = strlen(path);

    for (;;) {
        const ssize_t len = read(fd, buf, sizeof(buf));

        if (len <= 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        for (char* ptr = buf; ptr < buf + len; ptr += sizeof(inotify_event) + event->len) {
            event = (const inotify_event*) ptr;

            char* stat_path = malloc(PATH_MAX);
            long stat_path_len = 0;

            strcpy(stat_path, path);
            stat_path_len += path_len;
            stat_path[stat_path_len] = '/';
            ++stat_path_len;

            if (event->len)
                strcpy(stat_path + stat_path_len, event->name);

            printf("%s ", stat_path);

            if (event->mask & IN_ACCESS) {
                puts("is accessed");
                print_stat(stat_path);
            } else if (event->mask & IN_CREATE) {
                add_file_to_entries(stat_path);
                subscribe(fd, stat_path);
                puts("is created");
                print_stat(stat_path);
            } else if (event->mask & IN_DELETE) {
                puts("is deleted");
            } else if (event->mask & IN_MODIFY) {
                puts("is modified");
                print_stat(stat_path);
            } else if (event->mask & IN_OPEN) {
                puts("is opened");
                print_stat(stat_path);
            } else {
                puts("has new metadata");
                print_stat(stat_path);
            }
        }
    }
}

int main(const int argc, const char** const argv) {
    path = try_path(argv);
    const int fd = try_inotify_init();
    entries = malloc(sizeof(char*));
    wds = try_wds();

    subscribe(fd, path);

    for (char** e = entries; e != entries + ent_sz; ++e)
        subscribe(fd, *e);

    full_stat();

    signal(SIGINT, handle_signal);

    launch_monitoring(fd);
}
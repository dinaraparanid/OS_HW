#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdatomic.h>

#define MIN(A, B) (A) < (B) ? (A) : (B)
#define MAX(A, B) (A) > (B) ? (A) : (B)

typedef struct directory directory;

typedef struct {
    uint32_t id;
    char name[100];
    size_t size;
    char* data;
    directory* directory;
} file;

typedef struct directory {
    char name[100];
    file* files;
    struct directory* directories;
    uint8_t nf;
    uint8_t nd;
    char* path;
} directory;

atomic_uint_fast32_t file_id_cnt = 0;

void overwrite_file(file* const file, const char* const str) {
    const size_t len = strlen(str);
    file->size = len + 1;
    file->data = realloc(file->data, len + 1);
    strcpy(file->data, str);
    file->data[len] = '\0';
}

void append_to_file(file* const file, const char* const str) {
    const size_t data_len = strlen(file->data);
    const size_t str_len = strlen(str);
    const size_t len = data_len + str_len;
    file->size = len + 1;
    file->data = realloc(file->data, len + 1);
    strcpy(file->data + data_len, str);
    file->data[len] = '\0';
}

void printp_file(const file* const file) {
    const size_t dir_path_len = strlen(file->directory->path);
    const size_t name_len = strlen(file->name);

    char* full_path = malloc(dir_path_len + name_len + 2);
    strcpy(full_path, file->directory->path);
    full_path[dir_path_len] = '/';
    strcpy(full_path + dir_path_len + 1, file->name);
    full_path[dir_path_len + name_len + 1] = '\0';

    puts(full_path);
    free(full_path);
}

void add_file(file* const f, directory* const dir) {
    f->directory = dir;
    dir->files = realloc(dir->files, ++dir->nf * sizeof(file));

    file* last = &dir->files[dir->nf - 1];
    strcpy(last->name, f->name);
    last->size = f->size;
    overwrite_file(last, f->data);
    last->directory = f->directory;
}

directory create_home() {
    const directory h = {
            .name = "home",
            .files = malloc(sizeof(file)),
            .directories = malloc(sizeof(directory)),
            .nf = 0,
            .nd = 0,
            .path = "/home"
    };

    return h;
}

directory create_bin() {
    const directory b = {
            .name = "bin",
            .files = malloc(sizeof(file)),
            .directories = malloc(sizeof(directory)),
            .nf = 0,
            .nd = 0,
            .path = "/bin"
    };

    return b;
}

void dircpy(directory* const dest, const directory* const src) {
    const size_t name_sz = strlen(src->name);
    strcpy(dest->name, src->name);
    dest->name[name_sz] = '\0';

    dest->files = malloc(MAX(src->nf, 1) * sizeof(file));
    memmove(dest->files, src->files, src->nf * sizeof(file));

    dest->directories = malloc(MAX(src->nd, 1) * sizeof(directory));
    memmove(dest->directories, src->directories, src->nd * sizeof(directory));

    const size_t path_sz = strlen(src->path);
    dest->path = malloc(path_sz + 1);
    strcpy(dest->path, src->path);
    dest->path[path_sz] = '\0';
}

// Note: In C23 the first parameter is not required for va_start,
// see https://en.cppreference.com/w/c/variadic/va_start

directory* compose_subdirs(const size_t dirs_num, ...) {
    va_list args;
    va_start(args, dirs_num);

    directory* const dirs = malloc(dirs_num * sizeof(directory));

    for (directory* d = dirs; d != dirs + dirs_num; ++d) {
        const directory sd = va_arg(args, directory);
        dircpy(d, &sd);
    }

    va_end(args);
    return dirs;
}

file create_bash() {
    const file bash = {
            .id = file_id_cnt++,
            .name = "bash",
            .size = 0,
            .data = malloc(1),
            .directory = NULL
    };

    return bash;
}

file create_ex3_1_c() {
    char* const ex3_1_data = "int printf(const char* format, ...)";
    const size_t ex3_1_data_size = strlen(ex3_1_data);

    const file ex3_1_c = {
            .id = file_id_cnt++,
            .name = "ex3_1.c",
            .size = ex3_1_data_size,
            .data = malloc(ex3_1_data_size + 1),
            .directory = NULL
    };

    strcpy(ex3_1_c.data, ex3_1_data);
    ex3_1_c.data[ex3_1_data_size] = '\0';
    return ex3_1_c;
}

file create_ex3_2_c() {
    char* const ex3_2_data = "//This is a comment in C language";
    const size_t ex3_2_data_size = strlen(ex3_2_data);

    const file ex3_2_c = {
            .id = file_id_cnt++,
            .name = "ex3_2.c",
            .size = ex3_2_data_size,
            .data = malloc(ex3_2_data_size + 1),
            .directory = NULL
    };

    strcpy(ex3_2_c.data, ex3_2_data);
    ex3_2_c.data[ex3_2_data_size] = '\0';
    return ex3_2_c;
}

void free_file_data(file* const f) { free(f->data); }

void free_dir_data(directory* const dir) {
    for (directory* sd = dir->directories; sd != dir->directories + dir->nd; ++sd)
        free_dir_data(sd);

    free(dir->directories);

    for (file* f = dir->files; f != dir->files + dir->nf; ++f)
        free_file_data(f);

    free(dir->files);
}

int main() {
    directory home = create_home();
    directory bin = create_bin();

    directory root = {
            .name = "root",
            .files = malloc(sizeof(file)),
            .directories = compose_subdirs(2, home, bin),
            .nf = 0,
            .nd = 2,
            .path = "/"
    };

    file bash = create_bash();
    add_file(&bash, &bin);

    file ex3_1_c = create_ex3_1_c();
    add_file(&ex3_1_c, &home);

    file ex3_2_c = create_ex3_2_c();
    add_file(&ex3_2_c, &home);

    overwrite_file(&bash, "Bourne Again Shell!!");
    append_to_file(&ex3_1_c, "int main(){printf(”Hello World!”)}");

    printp_file(&bash);
    printp_file(&ex3_1_c);
    printp_file(&ex3_2_c);

    free_dir_data(&root);
    return 0;
}
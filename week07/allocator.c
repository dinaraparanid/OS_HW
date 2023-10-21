#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <search.h>
#include <sys/queue.h>

const int __MEM_SIZE = 1e7;
const int __ELEM_SIZE = sizeof(uint32_t);
const int __MAX_ADDRS = 1e5;

typedef struct addr_entry {
    size_t addr;
    LIST_ENTRY(addr_entry) entries;
} addr_entry;

LIST_HEAD(addr_list, addr_entry);
typedef struct addr_list addr_list;

typedef struct allocator {
    void* data;
    addr_list addr_space;
} allocator;

typedef struct {
    void* start;
    addr_entry* addr_ent;
    size_t items_num;
} mem_entry;

allocator* __allocator_new() {
    allocator* alloc = malloc(sizeof(allocator));
    alloc->data = calloc(__MEM_SIZE, __ELEM_SIZE);
    LIST_INIT(&alloc->addr_space);
    hcreate(__MAX_ADDRS);
    return alloc;
}

void __alloc_set(char* const start, const size_t adrs, const size_t size, addr_list* const list) {
    memset(start, 1, __ELEM_SIZE * size);

    char* const key = malloc(50);
    sprintf(key, "%lu", adrs);

    addr_entry* const addr_ent = malloc(sizeof(addr_entry));
    addr_ent->addr = adrs;
    LIST_INSERT_HEAD(list, addr_ent, entries);

    mem_entry* const entry = malloc(sizeof(mem_entry));
    entry->start = start;
    entry->items_num = size;
    entry->addr_ent = addr_ent;

    const ENTRY item = { .key = key, .data = entry };
    hsearch(item, ENTER);
}

void __allocate_first_fit(allocator* const self, const size_t adrs, const size_t size) {
    for (char* p = self->data; p != self->data + __MEM_SIZE * __ELEM_SIZE;) {
        if (*p != 0) {
            ++p;
            continue;
        }

        char* const start = p;
        size_t sz = 0;

        for (; p != self->data + __MEM_SIZE * __ELEM_SIZE && sz < __ELEM_SIZE * size; ++p, ++sz)
            if (*p != 0)
                break;

        if (sz >= __ELEM_SIZE * size) {
            __alloc_set(start, adrs, size, &self->addr_space);
            return;
        }
    }

    fprintf(stderr, "Out of memory error for adrs %lu and size %lu\n", adrs, size);
}

void __allocate_best_fit(allocator* const self, const size_t adrs, const size_t size) {
    size_t best_sz = INT32_MAX;
    char* best_start;

    for (char* p = self->data; p != self->data + __MEM_SIZE * __ELEM_SIZE;) {
        if (*p != 0) {
            ++p;
            continue;
        }

        char* const start = p;
        size_t sz = 0;

        for (;p != self->data + __MEM_SIZE * __ELEM_SIZE; ++p, ++sz)
            if (*p != 0)
                break;

        if (sz < __ELEM_SIZE * size)
            continue;

        if (sz < best_sz) {
            best_sz = sz;
            best_start = start;
        }
    }

    if (best_sz == INT32_MAX) {
        fprintf(stderr, "Out of memory error for adrs %lu and size %lu\n", adrs, size);
        return;
    }

    __alloc_set(best_start, adrs, size, &self->addr_space);
}

void __allocate_worst_fit(allocator* const self, const size_t adrs, const size_t size) {
    size_t best_sz = 0;
    char* best_start;

    for (char* p = self->data; p != self->data + __MEM_SIZE * __ELEM_SIZE;) {
        if (*p != 0) {
            ++p;
            continue;
        }

        char* const start = p;
        size_t sz = 0;

        for (;p != self->data + __MEM_SIZE * __ELEM_SIZE; ++p, ++sz)
            if (*p != 0)
                break;

        if (sz < __ELEM_SIZE * size)
            continue;

        if (sz > best_sz) {
            best_sz = sz;
            best_start = start;
        }
    }

    if (best_sz == INT32_MAX) {
        fprintf(stderr, "Out of memory error for adrs %lu and size %lu\n", adrs, size);
        return;
    }

    __alloc_set(best_start, adrs, size, &self->addr_space);
}

void __alloc_clear(const size_t adrs) {
    char* const key = malloc(50);
    sprintf(key, "%lu", adrs);

    const ENTRY item = { .key = key };
    ENTRY* const elem = hsearch(item, FIND);

    if (elem == NULL)
        return;

    mem_entry* const entry = elem->data;
    memset(entry->start, 0, entry->items_num * __ELEM_SIZE);

    LIST_REMOVE(entry->addr_ent, entries);
    free(entry->addr_ent);
    free(entry);
}

void __allocator_free(allocator* const self) {
    for (addr_entry* e = LIST_FIRST(&self->addr_space);; e = LIST_FIRST(&self->addr_space)) {
        if (e == NULL)
            break;

        LIST_REMOVE(e, entries);
    }

    free(self->data);
}

struct {
    allocator* (*new) ();
    void (*allocate)(struct allocator* self, size_t adrs, size_t size);
    void (*clear)(size_t adrs);
    void (*free)(struct allocator* self);
} first_fit_allocator = {
        .new = __allocator_new,
        .allocate = __allocate_first_fit,
        .clear = __alloc_clear,
        .free = __allocator_free
};

struct {
    allocator* (*new) ();
    void (*allocate)(struct allocator* self, size_t adrs, size_t size);
    void (*clear)(size_t adrs);
    void (*free)(struct allocator* self);
} best_fit_allocator = {
        .new = __allocator_new,
        .allocate = __allocate_best_fit,
        .clear = __alloc_clear,
        .free = __allocator_free
};

struct {
    allocator* (*new) ();
    void (*allocate)(struct allocator* self, size_t adrs, size_t size);
    void (*clear)(size_t adrs);
    void (*free)(struct allocator* self);
} worst_fit_allocator = {
        .new = __allocator_new,
        .allocate = __allocate_worst_fit,
        .clear = __alloc_clear,
        .free = __allocator_free
};

#define TEST_ALLOCATOR(ALLOC) \
    allocator* const alloc = ALLOC.new(); \
                              \
    while (!feof(fin)) {      \
        char command[50];     \
        fscanf(fin, "%s", command);       \
                              \
        if (strcmp(command, "allocate") == 0) { \
            size_t addr = 0, size = 0;    \
            fscanf(fin, "%lu%lu", &addr, &size);\
            ALLOC.allocate(alloc, addr, size);  \
            continue;         \
        }                     \
                              \
        if (strcmp(command, "clear") == 0) {    \
            size_t addr = 0;  \
            fscanf(fin, "%lu", &addr);    \
            ALLOC.clear(addr);\
        }                     \
    }                         \
                              \
    ALLOC.free(alloc)

int main() {
    FILE* const fin = fopen("queries.txt", "r");

    if (fin == NULL) {
        fputs("queries.txt is not found", stderr);
        return 1;
    }

    const clock_t start = clock();
    TEST_ALLOCATOR(worst_fit_allocator);
    const clock_t end = clock();

    const double duration = 1000.0 * (end - start) / CLOCKS_PER_SEC;
    printf("CPU time used: %.2f ms\n", duration);

    fclose(fin);
    return 0;
}
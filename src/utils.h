#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

typedef struct Allocator Allocator;

struct Allocator {
    void *memory;
    size_t head;
};

Allocator create_allocator(size_t);
void allocator_free(Allocator*);
void *mmalloc(Allocator*, size_t);

#endif


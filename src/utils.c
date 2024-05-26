#include <stdio.h>

#include "utils.h"

Allocator create_allocator(size_t size) {
    return (Allocator){ .memory=malloc(size), .head=0 };
}

void allocator_free(Allocator *allocator) {
    if (allocator->memory != NULL) {
        free(allocator->memory);
    }
    allocator->memory = NULL;
    allocator->head = 0;
}

void *mmalloc(Allocator *allocator, size_t size) {
    void *memory = allocator->memory + allocator->head;
    allocator->head = allocator->head + size;
    return memory;
}

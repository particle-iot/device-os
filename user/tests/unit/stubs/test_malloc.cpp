#include "test_malloc.h"

void* t_malloc(size_t size) {
    return ::malloc(size);
}

void* t_calloc(size_t count, size_t size) {
    return ::calloc(count, size);
}

void* t_realloc(void* ptr, size_t size) {
    return ::realloc(ptr, size);
}

void t_free(void* ptr) {
    ::free(ptr);
}

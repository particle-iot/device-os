#include "system_task.h"

#include <stdlib.h>

void* system_pool_alloc(size_t size, void* reserved) {
    return ::malloc(size);
}

void system_pool_free(void* ptr, void* reserved) {
    ::free(ptr);
}

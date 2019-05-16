#include <stdlib.h>
#include <string.h>
#include <sys/config.h>
#include <reent.h>
#include <malloc.h>
#include "interrupts_hal.h"
#include "service_debug.h"

extern void *pvPortMalloc( size_t xWantedSize );
extern void vPortFree( void *pv );
extern size_t xPortGetFreeHeapSize( void );
extern size_t xPortGetMinimumEverFreeHeapSize( void );
extern size_t xPortGetHeapSize( void );
extern size_t xPortGetBlockSize( void* ptr );
extern void __malloc_lock(struct _reent *ptr);
extern void __malloc_unlock(struct _reent *ptr);

static void panic_if_in_isr() {
    if (HAL_IsISR()) {
        PANIC(HeapError, "Heap usage from an ISR");
        while (1);
    }
}

void* _malloc_r(struct _reent *r, size_t s) {
    (void)r;
    panic_if_in_isr();
    void* ptr = pvPortMalloc((size_t)s);
    return ptr;
}

void _free_r(struct _reent* r, void* ptr) {
    panic_if_in_isr();
    // Hack of the century. We cannot free reent->_current_locale, because it's in
    // .text section on most of our platforms in flash and is simply a constant "C"
    if (r && ptr == r->_current_locale) {
        ptr = NULL;
    }
    vPortFree(ptr);
}

void _cfree_r(struct _reent* r, void* ptr) {
    _free_r(r, ptr);
}

void* _calloc_r(struct _reent* r, size_t n, size_t elem) {
    void* ptr = _malloc_r(r, (size_t)(n * elem));
    if (ptr != NULL) {
        memset(ptr, 0, (size_t)(elem * n));
    }
    return ptr;
}

void* _realloc_r(struct _reent* r, void *ptr, size_t newsize) {
    (void)r;

    panic_if_in_isr();

    if (newsize == 0) {
        vPortFree(ptr);
        return NULL;
    }

    void *p = pvPortMalloc(newsize);
    if (p) {
        if (ptr != NULL) {
            memcpy(p, ptr, newsize);
            vPortFree(ptr);
        }
    }
    return p;
}

static struct mallinfo current_mallinfo = {0};

struct mallinfo _mallinfo_r(struct _reent* r) {
    panic_if_in_isr();
    __malloc_lock(r);

    current_mallinfo.arena = xPortGetHeapSize();
    current_mallinfo.fordblks = xPortGetFreeHeapSize();
    current_mallinfo.uordblks = current_mallinfo.arena - current_mallinfo.fordblks;
    current_mallinfo.usmblks = current_mallinfo.arena - xPortGetMinimumEverFreeHeapSize();

    __malloc_unlock(r);

    return current_mallinfo;
}

void _malloc_stats_r(struct _reent* r) {
}

size_t _malloc_usable_size_r(struct _reent* r, void* ptr) {
    (void)r;

    panic_if_in_isr();

    return xPortGetBlockSize(ptr);
}

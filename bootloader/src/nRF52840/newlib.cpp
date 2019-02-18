#include <stdlib.h>
#include <cstddef>
#include "service_debug.h"

extern "C" {

void* __dso_handle = nullptr;

// Exceptions are disabled on the nRF52840 platforms, and default implementations of these functions
// pull malloc() and free() into the bootloader
void* __cxa_allocate_exception(size_t) throw() {
    return nullptr;
}

void __cxa_free_exception(void*) throw() {
}

} // extern "C"

/**
 * Shared newlib implementation for stm32 devices. (This is probably suitable for all embedded devices on gcc.)
 */

extern "C" {

/*
 * Implement C++ new/delete operators using the heap
 */

void *operator new(size_t size)
{
    return nullptr;
}

void *operator new[](size_t size)
{
    return nullptr;
}

void operator delete(void *p)
{
}

void operator delete[](void *p)
{
}


int _kill(int pid, int sig) __attribute((weak));

/* Bare metal, no processes, so error */
int _kill(int pid, int sig)
{
    return -1;
}

/* Bare metal, no processes, so always process id 1 */
int _getpid(void) __attribute((weak));
int _getpid(void)
{
    return 1;
}

void _exit(int status) __attribute((weak));
void _exit(int status) {

    PANIC(Exit,"Exit Called");

    while (1) {
        ;
    }
}



/* Default implementation for call made to pure virtual function. */
void __cxa_pure_virtual() {
    PANIC(PureVirtualCall,"Call on pure virtual");
    while (1);
}

/* Provide default implemenation for __cxa_guard_acquire() and
 * __cxa_guard_release(). Note: these must be revisited if a multitasking
 * OS is ported to this platform. */
__extension__ typedef int __guard __attribute__((mode (__DI__)));
int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
void __cxa_guard_abort (__guard *) {};

int __cxa_atexit(void (*f)(void *), void *p, void *d) {
    return 0;
}

int _write(int file, char *ptr, int len) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _close(int file) { return 0; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _fstat(int file, void *sbuf) { return 0; }
int _isatty(int file) { return 0; }

} /* extern "C" */


namespace __gnu_cxx {

void __verbose_terminate_handler()
{
    abort();
}

} /* namespace __gnu_cxx */

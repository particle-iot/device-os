#include "service_debug.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "delay_hal.h"

/**
 * Shared newlib implementation for stm32 devices. (This is probably suitable for all embedded devices on gcc.)
 */

extern "C" {

/*
 * Implement C++ new/delete operators using the heap
 */

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void *p)
{
	free(p);
}

void operator delete[](void *p)
{
	free(p);
}

void operator delete(void *p, size_t size)
{
	free(p);
}

void operator delete[](void *p, size_t size)
{
	free(p);
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

void __assert_func(const char *file, int line, const char* func, const char* expr) {
    LOG(ERROR, "Assertion failed: %s:%d %s (%s)", file, line, func, expr);
    PANIC(AssertionFailure, expr);
    while(1);
}

// Saves a few kB of flash.
char* strerror(int errnum) {
    return (char*)"";
}

// There is a bug in newlib in how it manages fake stdin/stdout/stderr
// and we'll leak memory. Stub it out
int __wrap_puts(const char* s) {
    return -1;
}

int __wrap_printf(const char* fmt, ...) {
    return -1;
}

} /* extern "C" */


namespace __gnu_cxx {

void __verbose_terminate_handler()
{
	abort();
}

} /* namespace __gnu_cxx */

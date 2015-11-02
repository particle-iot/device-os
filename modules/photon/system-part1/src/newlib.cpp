#include "service_debug.h"
#include <stdlib.h>

extern "C" {

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

/*
int _write(int file, char *ptr, int len) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _close(int file) { return 0; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _fstat(int file, void *sbuf) { return 0; }
int _isatty(int file) { return 0; }
*/

} /* extern "C" */

namespace __gnu_cxx {

void __verbose_terminate_handler()
{
    abort();
}

}

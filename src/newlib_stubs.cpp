/* Define caddr_t as char* */
#include <sys/types.h>

/* Define abort() */
#include <stdlib.h>

/*
 * The default pulls in 70K of garbage
 */

namespace __gnu_cxx {
void __verbose_terminate_handler()
{
	for(;;);
}
}

/*
 * The default pulls in about 12K of garbage
 */

extern "C" void __cxa_pure_virtual()
{
	for(;;);
}

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

extern "C" {

/******************************************************
 * System call reference with suggested stubs:
 * http://sourceware.org/newlib/libc.html#Syscalls
 *****************************************************/

void _exit(int status)
{
	while(1);
}

/*
 * _sbrk() -  allocate incr bytes of memory from the heap.
 *
 *            Return a pointer to the memory, or abort if there
 *            is insufficient memory available on the heap.
 *
 *            The heap is all the RAM that exists between _end and
 *            __Stack_Init, both of which are calculated by the linker.
 *
 *            _end marks the end of all the bss segments, and represents
 *            the highest RAM address used by the linker to locate data
 *            (either initialised or not.)
 *
 *            __Stack_Init marks the bottom of the stack, as reserved
 *            in the linker script (../linker/linker_stm32f10x_md*.ld)
 */
caddr_t _sbrk(int incr)
{
	extern char _end, __Stack_Init;
	static char *heap_end = &_end;
	char *prev_heap_end = heap_end;

	heap_end += incr;

	if (heap_end > &__Stack_Init) {
		abort();
	}

	return (caddr_t) prev_heap_end;
}

/* Bare metal, no processes, so error */
int _kill(int pid, int sig)
{
	return -1;
}

/* Bare metal, no processes, so always process id 1 */
int _getpid(void)
{
	return 1;
}

} /* extern "C" */

/**
 ******************************************************************************
 * @file    newlib_stubs.cpp
 * @authors Matthew McGowan, Brett Walach
 * @date    10 February 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Define caddr_t as char* */
#include <sys/types.h>

/* Define abort() */
#include <stdlib.h>
#include "debug.h"
extern "C" { 
  void _exit(int status); 
} /* extern "C" */
#define abort() _exit(-1)

extern unsigned long __preinit_array_start;
extern unsigned long __preinit_array_end;
extern unsigned long __init_array_start;
extern unsigned long __init_array_end;
extern unsigned long __fini_array_start;
extern unsigned long __fini_array_end;

static void call_constructors(unsigned long *start, unsigned long *end) __attribute__((noinline));

static void call_constructors(unsigned long *start, unsigned long *end)
{
	unsigned long *i;
	void (*funcptr)();
	for (i = start; i < end; i++)
	{
		funcptr=(void (*)())(*i);
		funcptr();
	}
}

extern "C" {
void CallConstructors(void)
{
	call_constructors(&__preinit_array_start, &__preinit_array_end);
	call_constructors(&__init_array_start, &__init_array_end);
	call_constructors(&__fini_array_start, &__fini_array_end);
}

void *__dso_handle = NULL;

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

/******************************************************
 * System call reference with suggested stubs:
 * http://sourceware.org/newlib/libc.html#Syscalls
 *****************************************************/
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
		PANIC(OutOfHeap,"Out Of Heap");
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

void _exit(int status) {  
	PANIC(Exit,"Exit Called");
	while (1);
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

} /* extern "C" */

namespace __gnu_cxx {

void __verbose_terminate_handler()
{
	abort();
}

} /* namespace __gnu_cxx */
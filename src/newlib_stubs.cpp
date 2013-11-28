/**
  ******************************************************************************
  * @file    newlib_stubs.cpp
  * @author  Zachary Crockett
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   
  ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

/* Define caddr_t as char* */
#include <sys/types.h>

/* Define abort() */
#include <stdlib.h>

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
} /* extern "C" */

extern "C"
{
void *__dso_handle = NULL;
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

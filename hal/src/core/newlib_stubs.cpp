/**
  ******************************************************************************
  * @file    newlib_stubs.cpp
  * @author  Zachary Crockett
  * @version V1.0.0
  * @date    24-April-2013
  * @brief
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "malloc.h"
/* Define abort() */
#include <stdlib.h>
#include "debug.h"
#ifdef __CS_SOURCERYGXX_REV__
#define abort() _exit(-1);
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include "stm32f10x_usart.h"

#endif

extern unsigned long link_constructors_location;
extern unsigned long link_constructors_end;

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
	call_constructors(&link_constructors_location, &link_constructors_end);
}


void *__dso_handle = NULL;
}

extern "C" {

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
extern char _end, __Stack_Init;
static char *heap_end = &_end;
caddr_t _sbrk(int incr)
{
	char *prev_heap_end = heap_end;

	heap_end += incr;

	if (heap_end > &__Stack_Init) {
	        PANIC(OutOfHeap,"Out Of Heap");
		abort();
	}

	return (caddr_t) prev_heap_end;
}

uint32_t freeheap()
{
	struct mallinfo info = mallinfo();
    return &__Stack_Init-heap_end + info.fordblks;
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

    while (1) {
        ;
    }
}



#ifdef __CS_SOURCERYGXX_REV__
#if USE_UART_FOR_STDIO
#ifndef STDOUT_USART
#define STDOUT_USART 2
#endif

#ifndef STDERR_USART
#define STDERR_USART 2
#endif

#ifndef STDIN_USART
#define STDIN_USART 2
#endif
#endif
/*
 write
 Write a character to a file. `libc' subroutines will use this system routine for output to all files, including stdout
 Returns -1 on error or number of bytes sent
 */
int _write(int file, char *ptr, int len) {
    int n;
    switch (file) {
    case STDOUT_FILENO: /*stdout*/
        for (n = 0; n < len; n++) {
#if STDOUT_USART == 1
            while ((USART1->SR & USART_FLAG_TC) == (uint16_t)RESET) {}
            USART1->DR = (*ptr++ & (uint16_t)0x01FF);
#elif  STDOUT_USART == 2
            while ((USART2->SR & USART_FLAG_TC) == (uint16_t) RESET) {
            }
            USART2->DR = (*ptr++ & (uint16_t) 0x01FF);
#elif  STDOUT_USART == 3
            while ((USART3->SR & USART_FLAG_TC) == (uint16_t)RESET) {}
            USART3->DR = (*ptr++ & (uint16_t)0x01FF);
#endif
        }
        break;
    case STDERR_FILENO: /* stderr */
        for (n = 0; n < len; n++) {
#if STDERR_USART == 1
            while ((USART1->SR & USART_FLAG_TC) == (uint16_t)RESET) {}
            USART1->DR = (*ptr++ & (uint16_t)0x01FF);
#elif  STDERR_USART == 2
            while ((USART2->SR & USART_FLAG_TC) == (uint16_t) RESET) {
            }
            USART2->DR = (*ptr++ & (uint16_t) 0x01FF);
#elif  STDERR_USART == 3
            while ((USART3->SR & USART_FLAG_TC) == (uint16_t)RESET) {}
            USART3->DR = (*ptr++ & (uint16_t)0x01FF);
#endif
        }
        break;
    default:
        errno = EBADF;
        return -1;
    }
    return len;
}

void _exit(int status) {
    _write(1, (char *)"exit", 4);
    PANIC(Exit,"Exit Called");

    while (1) {
        ;
    }
}


int _close(int file) {
    return -1;
}

/*
 lseek
 Set position in a file. Minimal implementation:
 */
int _lseek(int file, int ptr, int dir) {
    return 0;
}
/*
 read
 Read a character to a file. `libc' subroutines will use this system routine for input from all files, including stdin
 Returns -1 on error or blocks until the number of characters have been read.
 */


int _read(int file, char *ptr, int len) {
    int n;
    int num = 0;
    switch (file) {
    case STDIN_FILENO:
        for (n = 0; n < len; n++) {
            char c = 0;
#if   STDIN_USART == 1
            while ((USART1->SR & USART_FLAG_RXNE) == (uint16_t)RESET) {}
            c = (char)(USART1->DR & (uint16_t)0x01FF);
#elif STDIN_USART == 2
            while ((USART2->SR & USART_FLAG_RXNE) == (uint16_t) RESET) {}
            c = (char) (USART2->DR & (uint16_t) 0x01FF);
#elif STDIN_USART == 3
            while ((USART3->SR & USART_FLAG_RXNE) == (uint16_t)RESET) {}
            c = (char)(USART3->DR & (uint16_t)0x01FF);
#endif
            *ptr++ = c;
            num++;
        }
        break;
    default:
        errno = EBADF;
        return -1;
    }
    return num;
}
#endif


/*
int _write(int file, char *ptr, int len) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _close(int file) { return 0; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _fstat(int file, void *sbuf) { return 0; }
int _isatty(int file) { return 0; }
*/

} /* extern "C" */


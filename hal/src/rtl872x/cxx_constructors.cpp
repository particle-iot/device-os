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
#include <errno.h>
#include <malloc.h>
/* Define abort() */
#include <stdlib.h>
#include "debug.h"
extern "C" {
  void _exit(int status);
} /* extern "C" */
#define abort() _exit(-1)

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

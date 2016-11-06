#define DYNALIB_EXPORT

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for siprintf(), siscanf() and other extensions
#endif

#include <stdlib.h>     // for malloc, free, realloc
#include <stdio.h>
#include <stdarg.h> // for va_list
#include "rt_dynalib.h"

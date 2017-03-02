void platform_startup()
{
}

/* Define caddr_t as char* */
#include <sys/types.h>
#include <errno.h>
#include <malloc.h>
/* Define abort() */
#include <stdlib.h>

caddr_t _sbrk(int incr)
{
   return 0;
}

// Naive memcpy
void* memcpy(void *dest, const void *src, size_t n) {
    const char* p = src;
    char* q = dest;
    while (n--) {
        *q++ = *p++;
    }
    return dest;
}

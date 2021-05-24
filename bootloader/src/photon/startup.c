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

// Naive memXXX functions

void* memcpy(void *dest, const void *src, size_t n) {
    const uint8_t* p = src;
    uint8_t* q = dest;
    while (n--) {
        *q++ = *p++;
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
  const volatile uint8_t* p1 = (const volatile uint8_t*)s1;
  const volatile uint8_t* p2 = (const volatile uint8_t*)s2;

  while (n--)
  {
    if (*p1++ != *p2++)
        return *p1 - *p2;
  }
  return 0;
}

__attribute__((used)) void* memset(void* s, int c, size_t n) {
    // XXX: do not change!
    // GCC 10 optimizations with LTO enabled break this function otherwise
    volatile uint8_t* p = (volatile uint8_t*)s;
    const uint8_t v = c & 0xff;
    volatile size_t tmp = n;
    while (tmp--) {
        *p++ = v;
    }
    return s;
}

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

void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    uint8_t v = c & 0xff;
    while (n--) {
        *p++ = v;
    }
    return s;
}

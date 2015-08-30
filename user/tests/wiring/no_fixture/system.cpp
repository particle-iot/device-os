
#include "application.h"
#include "unit-test/unit-test.h"

test(System_FreeMemory)
{
    // this test didn't work on the core attempting to allocate the current value of
    // freeMemory(), presumably because of fragmented heap from
    // relatively large allocations during the handshake, so the request was satisfied
    // without calling _sbrk()
    // 4096 was chosen to be small enough to allocate, but large enough to force _sbrk() to be called.)

    uint32_t free1 = System.freeMemory();
    if (free1>128) {
        void* m1 = malloc(1024*6);
        uint32_t free2 = System.freeMemory();
        free(m1);
        assertLess(free2, free1);
    }
}


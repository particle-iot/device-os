
#include "application.h"
#include "unit-test/unit-test.h"

test(System_FreeMemory)
{
    uint32_t free1 = System.freeMemory();
    char* m1 = new char[free1-1024];
    uint32_t free2 = System.freeMemory();
    delete[] m1;

    assertLess(free2, free1);

}

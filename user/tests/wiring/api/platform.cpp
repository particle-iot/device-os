
#include "testapi.h"

test(system_ticks)
{
    uint32_t value;
    API_COMPILE(value=System.ticks());
    API_COMPILE(value=System.ticksPerMicrosecond());
    API_COMPILE(System.ticksDelay(30));
    (void)value;
}
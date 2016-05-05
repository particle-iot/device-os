

#include "testapi.h"

test(string_constructor_printable)
{
    IPAddress address;
    API_COMPILE(String(address));
    (void)address;
}

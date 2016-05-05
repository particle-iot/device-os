
#include "testapi.h"

#include <string.h>
#include <stdlib.h>

// The C-runtime is in some ways also part of our API since coders are relying on it.
// These tests ensure the program successfully links.

test(sprintf_links)
{
    char buf[50];
    API_COMPILE(sprintf(buf,"123"));
    API_COMPILE(snprintf(buf, 50, "123"));
}



#include "rng_hal.h"
#include <stdlib.h>

void HAL_RNG_Configuration(void)
{
}

uint32_t HAL_RNG_GetRandomNumber(void)
{
    return rand();
}

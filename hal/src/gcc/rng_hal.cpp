
#include "rng_hal.h"

#include "system_error.h"
#include <boost/random/random_device.hpp>

using namespace boost::random;

random_device rng;

void HAL_RNG_Configuration(void)
{
}

uint32_t HAL_RNG_GetRandomNumber(void)
{
	return rng();
}

int hal_rng_reseed(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_rng_entropy_read_raw(uint16_t* samples, size_t count, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}


#include "rng_hal.h"
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

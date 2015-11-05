
#include "eckeygen.h"
#include "protocol_selector.h"
#include "hal_platform.h"

#ifdef PARTICLE_PROTOCOL
#if HAL_PLATFORM_CLOUD_UDP

int gen_ec_key(uint8_t* buffer, size_t max_length, int (*f_rng) (void *), void *p_rng)
{
	return 0;
}

#endif
#endif

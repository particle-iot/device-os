
// these are HAL functions that are called directly by the DTLS
// implementation. Over time these should disappear as we move
// the DTLS functions into callbacks, so we get more control over
// the return value from
// the _gettimeofday() and similar functions that call HAL functions directly.

#include <stdint.h>
#include <stdlib.h>
#include "logging.h"
#include "diagnostics.h"

extern "C" uint32_t HAL_RNG_GetRandomNumber()
{
	return rand();
}


extern "C" uint32_t HAL_Timer_Get_Milli_Seconds()
{
	static uint32_t millis = 0;
	return ++millis;
}


extern "C" uint32_t HAL_Timer_Get_Micro_Seconds()
{
	return HAL_Timer_Get_Milli_Seconds()*1000;
}

extern "C" uint32_t HAL_Core_Compute_CRC32(const uint8_t* buf, size_t length)
{
	return 0;
}

extern "C" void log_message(int level, const char *category, LogAttributes *attr, void *reserved, const char *fmt, ...)
{
}

extern "C" void log_write(int level, const char *category, const char *data, size_t size, void *reserved)
{
}

extern "C" int diag_register_source(const diag_source* src, void* reserved) {
	return 0;
}

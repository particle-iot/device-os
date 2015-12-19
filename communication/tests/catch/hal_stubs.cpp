
// these are HAL functions that are called directly by the DTLS
// implementation. Over time these should disappear as we move
// the DTLS functions into callbacks, so we get more control over
// the return value from
// the _gettimeofday() and similar functions that call HAL functions directly.

#include <stdint.h>
#include <stdlib.h>

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

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
#include <stdlib.h>
#include <stdint.h>
#include "wiced_result.h"
#include "tls_callbacks.h"

extern void* tls_host_malloc( const char* name, uint32_t size );
extern void  tls_host_free  ( void* p );
extern wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length );
extern uint32_t HAL_RNG_GetRandomNumber(void);

TlsCallbacks tlsCallbacks = {0};

void* tls_host_malloc( const char* name, uint32_t size )
{
  return malloc((size_t)size);
}

void  tls_host_free  ( void* p )
{
  free(p);
}

wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length )
{
  uint8_t* data = (uint8_t*)buffer;
  while (buffer_length>=4)
  {
    *((uint32_t*)data) = HAL_RNG_GetRandomNumber();
    data += 4;
    buffer_length -= 4;
  }
  while (buffer_length-->0)
  {
    *data++ = HAL_RNG_GetRandomNumber();
  }
  return WICED_SUCCESS;
}

int tls_set_callbacks(TlsCallbacks cb) {
  tlsCallbacks = cb;
  return 0;
}

uint64_t tls_host_get_time_ms()
{
  if (tlsCallbacks.tls_host_get_time_ms) {
    return tlsCallbacks.tls_host_get_time_ms();
  }
  return 0;
}

#endif

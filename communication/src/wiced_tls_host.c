#if PLATFORM_ID == 6 || PLATFORM_ID == 8
#include <stdlib.h>
#include <stdint.h>
#include "wiced_result.h"
#include "tls_callbacks.h"

extern void* tls_host_malloc( const char* name, uint32_t size );
extern void  tls_host_free  ( void* p );
extern wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length );
extern uint32_t HAL_RNG_GetRandomNumber(void);

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
TlsCallbacks tlsCallbacks = {0};

void* tls_host_malloc( const char* name, uint32_t size )
{
  if (tlsCallbacks.tls_host_malloc) {
    return tlsCallbacks.tls_host_malloc(name, size);
  }
  return NULL;
}

void tls_host_free (void* p)
{
  if (tlsCallbacks.tls_host_free) {
    tlsCallbacks.tls_host_free(p);
  }
}

wiced_result_t wiced_crypto_get_random( void* buffer, uint16_t buffer_length )
{
  if (tlsCallbacks.wiced_crypto_get_random) {
    return tlsCallbacks.wiced_crypto_get_random(buffer, buffer_length);
  }
  return WICED_ERROR;
}

uint64_t tls_host_get_time_ms()
{
  if (tlsCallbacks.tls_host_get_time_ms) {
    return tlsCallbacks.tls_host_get_time_ms();
  }
  return 0;
}

int tls_set_callbacks(TlsCallbacks cb) {
  tlsCallbacks = cb;
  return 0;
}

#else

int tls_set_callbacks(TlsCallbacks cb) {
  return 0;
}

#endif /* defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE */



#endif

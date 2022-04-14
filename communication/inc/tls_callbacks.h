#pragma once

#include <time.h>
#include "wiced_result.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t version;
    uint64_t (*tls_host_get_time_ms)();
    void* (*tls_host_malloc)(const char*, uint32_t);
    void  (*tls_host_free)(void*);
    wiced_result_t (*wiced_crypto_get_random)(void*, uint16_t);
} TlsCallbacks;

int tls_set_callbacks(TlsCallbacks callbacks);

#ifdef	__cplusplus
}
#endif
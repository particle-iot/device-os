#pragma once

#include <time.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t version;
	uint64_t (*tls_host_get_time_ms)();
} TlsCallbacks;

int tls_set_callbacks(TlsCallbacks callbacks);

#ifdef	__cplusplus
}
#endif
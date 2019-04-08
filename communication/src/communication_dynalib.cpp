
#include "spark_protocol_functions.h"
#include "service_debug.h"
#include "protocol_selector.h"

#ifdef USE_MBEDTLS
#else
# if PLATFORM_ID == 6 || PLATFORM_ID == 8
#  include "wiced_security.h"
#  include "crypto_open/x509.h"
#  include "crypto_open/bignum.h"
#  include "micro-ecc/configuration.h"
#  include "micro-ecc/uECC.h"
#  include "tls_callbacks.h"
# endif // PLATFORM_ID == 6 || PLATFORM_ID == 8
#endif // USE_MBEDTLS

#define DYNALIB_EXPORT
#include "communication_dynalib.h"
#define INTERRUPTS_HAL_EXCLUDE_PLATFORM_HEADERS
#include "core_hal.h"

#if PARTICLE_PROTOCOL
#include "lightssl_protocol.h"
#include "dtls_protocol.h"
#else
#include "core_protocol.h"
#endif

/**
 * Allocate an instance of the CoreProtocol. By doing it here rather than in system
 * we ensure the structure is allocated the correct amount of memory, cf. a system
 * module using a newer version of comms lib where the size has grown.
 *
 * @return A pointer to the static instance.
 */
ProtocolFacade* create_protocol(ProtocolFactory factory)
{
    	// if compile time only TCP, then that's the only option, otherwise
    	// choose between UDP and TCP
#if PARTICLE_PROTOCOL
#if HAL_PLATFORM_CLOUD_UDP
    	if (factory==PROTOCOL_DTLS) {
    		DEBUG("creating DTLS protocol");
    		return new particle::protocol::DTLSProtocol();
    	}
#endif
#if HAL_PLATFORM_CLOUD_TCP
    	if (factory==PROTOCOL_LIGHTSSL)
    	{
    		DEBUG("creating LightSSL protocol");
		return new particle::protocol::LightSSLProtocol();
    	}
#endif
    	return nullptr;
#else
		DEBUG("creating CoreProtocol");
		return new CoreProtocol();
#endif
}


ProtocolFacade* spark_protocol_instance(void)
{
	static ProtocolFacade* protocol = nullptr;

	if (!protocol) {
		bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
		DEBUG("UDP enabled %d", udp);
		protocol = create_protocol(udp ? PROTOCOL_DTLS : PROTOCOL_LIGHTSSL);
	}
	return protocol;
}

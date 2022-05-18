
#include "spark_protocol_functions.h"
#include "service_debug.h"
#include "protocol_selector.h"

#define DYNALIB_EXPORT
#include "communication_dynalib.h"
#define INTERRUPTS_HAL_EXCLUDE_PLATFORM_HEADERS
#include "core_hal.h"

#include "lightssl_protocol.h"
#include "dtls_protocol.h"

/**
 * Allocate an instance of the Protocol. By doing it here rather than in system
 * we ensure the structure is allocated the correct amount of memory, cf. a system
 * module using a newer version of comms lib where the size has grown.
 *
 * @return A pointer to the static instance.
 */
ProtocolFacade* create_protocol(ProtocolFactory factory)
{
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

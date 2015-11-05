
#include "spark_protocol_functions.h"

#define DYNALIB_EXPORT
#include "communication_dynalib.h"
#include "protocol_selector.h"

#ifdef PARTICLE_PROTOCOL
#include "core_hal.h"
#include "lightssl_protocol.h"
#include "dtls_protocol.h"
#else
#include "spark_protocol.h"
#endif

/**
 * Allocate an instance of the SparkProtocol. By doing it here rather than in system
 * we ensure the structure is allocated the correct amount of memory, cf. a system
 * module using a newer version of comms lib where the size has grown.
 *
 * @return A pointer to the static instance.
 */
ProtocolFacade* create_protocol(ProtocolFactory factory)
{
    	// if compile time only TCP, then that's the only option, otherwise
    	// choose between UDP and TCP
#ifdef PARTICLE_PROTOCOL
#if HAL_PLATFORM_CLOUD_UDP
    	if (factory==PROTOCOL_DTLS)
    		return new particle::protocol::DTLSProtocol();
    	else
#endif
		return new particle::protocol::LightSSLProtocol();
#else
    		return new SparkProtocol();
#endif
}


ProtocolFacade* spark_protocol_instance(void)
{
	static ProtocolFacade* protocol = nullptr;

	if (!protocol) {
		bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
		protocol = create_protocol(udp ? PROTOCOL_DTLS : PROTOCOL_LIGHTSSL);
	}
	return protocol;
}

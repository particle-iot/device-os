
#include "spark_protocol_functions.h"

#define DYNALIB_EXPORT
#include "communication_dynalib.h"
#include "protocol_selector.h"

#ifdef PARTICLE_PROTOCOL
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

ProtocolFacade* spark_protocol_instance()
{
    static ProtocolFacade* sp = NULL;
    if (sp==NULL) {
#ifdef PARTICLE_PROTOCOL
    		sp = new particle::protocol::LightSSLProtocol();
#else
    		sp = new SparkProtocol();
#endif
    }
    return sp;
}

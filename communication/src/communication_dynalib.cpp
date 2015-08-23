
#include "particle_protocol_functions.h"

#define DYNALIB_EXPORT
#include "communication_dynalib.h"

#include "particle_protocol.h"

/**
 * Allocate an instance of the ParticleProtocol. By doing it here rather than in system
 * we ensure the structure is allocated the correct amount of memory, cf. a system
 * module using a newer version of comms lib where the size has grown.
 *
 * @return A pointer to the static instance.
 */
ParticleProtocol* particle_protocol_instance()
{
    static ParticleProtocol* sp = NULL;
    if (sp==NULL)
        sp = new ParticleProtocol();
    return sp;
}

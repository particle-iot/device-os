
#include "spark_protocol_functions.h"

#define DYNALIB_EXPORT
#include "communication_dynalib.h"

#include "spark_protocol.h"

/**
 * Allocate an instance of the SparkProtocol. By doing it here rather than in system
 * we ensure the structure is allocated the correct amount of memory, cf. a system
 * module using a newer version of comms lib where the size has grown.
 *
 * @return A pointer to the static instance.
 */
SparkProtocol* spark_protocol_instance()
{
    static SparkProtocol* sp = NULL;
    if (sp==NULL)
        sp = new SparkProtocol();
    return sp;
}

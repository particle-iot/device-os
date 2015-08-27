

// this file is used to compile the Catch unit tests on gcc, so contains few dependencies

#include "system_task.h"
#include <algorithm>

using std::min;

/**
 * Series 1s (5 times), 2s (5 times), 4s (5 times)...64s (5 times) then to 128s thereafter.
 * @param connection_attempts
 * @return The number of milliseconds to backoff.
 */
unsigned backoff_period(unsigned connection_attempts)
{
    if (!connection_attempts)
        return 0;
    unsigned exponent = min(7u, (connection_attempts-1)/5);
    return 1000*(1<<exponent);
}


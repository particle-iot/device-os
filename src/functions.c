
#include "functions.h"
#include <stdlib.h>

/**
 * Handle the cryptographically secure random seed from the cloud by using
 * it to seed the stdlib PRNG. 
 * @param seed  A random value from a cryptographically secure random number generator.
 * 
 * This function has weak linkage, so that user code may re-define this function and
 * handle the random number in some other way. For example, to combine with local
 * entropy sources.
 */
__attribute__((weak)) void random_seed_from_cloud(unsigned int seed){
    srand(seed);
}


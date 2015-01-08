
#include "functions.h"
#include "handshake.h"
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

#ifdef __arm__
 __attribute__((weak)) void random_seed_from_cloud(unsigned int seed){
    srand(seed);
}
#else
// weak linking doesn't seem to be working on regular gcc
 void random_seed_from_cloud(unsigned int seed)
{
    srand(seed);
}
#endif


 int decrypt_rsa(const uint8_t* ciphertext, const uint8_t* private_key, uint8_t* plaintext, int plaintext_len) {
    rsa_context rsa;
    init_rsa_context_with_private_key(&rsa, private_key);    
    int err = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, &plaintext_len, ciphertext, plaintext, plaintext_len);
    rsa_free(&rsa);
    return err ? 0 : plaintext_len;
 }
 
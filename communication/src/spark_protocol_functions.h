/* 
 * File:   functions.h
 * Author: mat
 *
 * Created on 04 September 2014, 00:10
 */

#ifndef FUNCTIONS_H
#define	FUNCTIONS_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Handle the cryptographically secure random seed from the cloud.
 * @param seed  A random value. This is typically used to seed a pseudo-random generator. 
 */
extern void random_seed_from_cloud(unsigned int seed);

/**
 * Decrypt a buffer using the given public key.
 * @param ciphertext        The ciphertext to decrypt
 * @param private_key       The private key (in DER format).
 * @param plaintext         buffer to hold the resulting plaintext
 * @param max_plaintext_len The size of the plaintext buffer
 * @return The number of plaintext bytes in the plain text buffer, or 0 on error.
 */
extern int decrypt_rsa(const uint8_t* ciphertext, const uint8_t* private_key, 
        uint8_t* plaintext, int max_plaintext_len);

#ifdef	__cplusplus
}
#endif

#endif	/* FUNCTIONS_H */


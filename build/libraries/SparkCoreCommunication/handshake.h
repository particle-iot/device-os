#include "tropicssl/rsa.h"
#include "tropicssl/sha1.h"
#include <string.h>

extern int ciphertext_from_nonce_and_id(const unsigned char *nonce,
                                 const unsigned char *id,
                                 const unsigned char *pubkey,
                                 unsigned char *ciphertext);

int decipher_aes_credentials(const unsigned char *private_key,
                             const unsigned char *ciphertext,
                             unsigned char *aes_credentials);

void calculate_ciphertext_hmac(const unsigned char *ciphertext,
                               const unsigned char *hmac_key,
                               unsigned char *hmac);

int verify_signature(const unsigned char *signature,
                     const unsigned char *pubkey,
                     const unsigned char *expected_hmac);

void init_rsa_context_with_public_key(rsa_context *rsa,
                                      const unsigned char *pubkey);

void init_rsa_context_with_private_key(rsa_context *rsa,
                                       const unsigned char *private_key);

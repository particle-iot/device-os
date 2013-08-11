#include "tropicssl/rsa.h"
#include <string.h>

int ciphertextFromNonceAndID(const unsigned char *nonce,
                             const unsigned char *id,
                             const unsigned char *pubkey,
                             unsigned char *ciphertext)
{
  unsigned char plaintext[52];

  memcpy(plaintext, nonce, 40);
  memcpy(plaintext + 40, id, 12);

  rsa_context rsa;
  memset(&rsa, 0, sizeof(rsa_context));

	rsa.len = 256;
	mpi_read_binary(&rsa.N, pubkey, 256);
	mpi_read_string(&rsa.E, 16, "10001");

  return rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, 52, plaintext, ciphertext);
}

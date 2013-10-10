#include "handshake.h"

int ciphertext_from_nonce_and_id(const unsigned char *nonce,
                                 const unsigned char *id,
                                 const unsigned char *pubkey,
                                 unsigned char *ciphertext)
{
  unsigned char plaintext[52];

  memcpy(plaintext, nonce, 40);
  memcpy(plaintext + 40, id, 12);

  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, pubkey);

  int ret = rsa_pkcs1_encrypt(&rsa, RSA_PUBLIC, 52, plaintext, ciphertext);
  rsa_free(&rsa);
  return ret;
}

int decipher_aes_credentials(const unsigned char *private_key,
                             const unsigned char *ciphertext,
                             unsigned char *aes_credentials)
{
  rsa_context rsa;
  init_rsa_context_with_private_key(&rsa, private_key);

  int len = 256;
  int ret = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, &len, ciphertext,
                              aes_credentials, 40);
  rsa_free(&rsa);
  return ret;
}

void calculate_ciphertext_hmac(const unsigned char *ciphertext,
                               const unsigned char *hmac_key,
                               unsigned char *hmac)
{
  sha1_hmac(hmac_key, 40, ciphertext, 256, hmac);
}

int verify_signature(const unsigned char *signature,
                     const unsigned char *pubkey,
                     const unsigned char *expected_hmac)
{
  rsa_context rsa;
  init_rsa_context_with_public_key(&rsa, pubkey);

  int ret = rsa_pkcs1_verify(&rsa, RSA_PUBLIC, RSA_RAW, 20,
                             expected_hmac, signature);
  rsa_free(&rsa);
  return ret;
}

void init_rsa_context_with_public_key(rsa_context *rsa,
                                      const unsigned char *pubkey)
{
  rsa_init(rsa, RSA_PKCS_V15, RSA_RAW, NULL, NULL);

  rsa->len = 256;
  mpi_read_binary(&rsa->N, pubkey + 33, 256);
  mpi_read_string(&rsa->E, 16, "10001");
}

/* Very simple ASN.1 parsing.
 * Mainly needed because, even though all the RSA big integers
 * are always a specific number of bytes, the key generation
 * and encoding process sometimes pads each number with a
 * leading zero byte.  Theoretical range of DER file size is
 * 1187-1194. We have actually seen 1190-1194.
 */
void init_rsa_context_with_private_key(rsa_context *rsa,
                                       const unsigned char *private_key)
{
  rsa_init(rsa, RSA_PKCS_V15, RSA_RAW, NULL, NULL);

  rsa->len = 256;

  int i = 10;
  if (private_key[i])
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->N, private_key + i, 256);
  mpi_read_string(&rsa->E, 16, "10001");

  i = i + 264;
  if (private_key[i])
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->D, private_key + i, 256);

  i = i + 258;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->P, private_key + i, 128);

  i = i + 130;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->Q, private_key + i, 128);

  i = i + 130;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->DP, private_key + i, 128);

  i = i + 130;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->DQ, private_key + i, 128);

  i = i + 130;
  if (private_key[i] & 1)
  {
    // key contains an extra zero byte
    ++i;
  }
  ++i;

  mpi_read_binary(&rsa->QP, private_key + i, 128);
}

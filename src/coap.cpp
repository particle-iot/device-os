#include "coap.h"
#include <stdio.h>

void hello_from_aes_credentials(const unsigned char *credentials,
                                unsigned char *hello_buf,
                                int &hello_size)
{
  unsigned char key[16];
  unsigned char iv[16];
  memcpy(key, credentials, 16);
  memcpy(iv, credentials + 16, 16);
  
  aes_context ctx;
  aes_setkey_enc(&ctx, key, 128);
  aes_crypt_cbc(&ctx, AES_ENCRYPT, hello_size, iv, hello_buf, hello_buf);
}

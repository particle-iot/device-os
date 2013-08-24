#include "protobufs.h"
#include <stdio.h>

void hello_from_aes_credentials(const unsigned char *credentials,
                                unsigned char *hello_buf,
                                int &hello_size)
{
  unsigned char key[16];
  unsigned char iv[16];
  memcpy(key, credentials, 16);
  memcpy(iv, credentials + 16, 16);

  Envelope envelope;
  envelope._type = _HELLO;
  envelope._counter = iv[0] << 24 | iv[1] << 16 | iv[2] << 8 | iv[3];
  int bytes_written = Envelope_write_delimited_to(&envelope, hello_buf, 0);
  /*

  printf("Envelope bytes written into hello_buf: %d\n", bytes_written);
  hello_size = (bytes_written + 16) & ~15;
  unsigned char pad_byte = hello_size - bytes_written;
  memset(hello_buf + bytes_written, pad_byte, pad_byte);

  FILE *f = fopen("z-hello", "wb");
  fwrite(hello_buf, 1, hello_size, f);
  fclose(f);
  
  aes_context ctx;
  aes_setkey_enc(&ctx, key, 128);
  aes_crypt_cbc(&ctx, AES_ENCRYPT, hello_size, iv, hello_buf, hello_buf);
  */
}

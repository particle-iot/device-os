#include "protobufs.h"

void hello_from_aes_credentials(const unsigned char *credentials,
                                unsigned char *hello_buf,
                                int &hello_size)
{
  unsigned char key[16];
  unsigned char iv[16];
  memcpy(key, credentials, 16);
  memcpy(iv, credentials + 16, 16);

  ::spark::Envelope envelope;
  envelope.set_type(spark::HELLO);
  envelope.hello();
  ::google::protobuf::uint32 counter = (iv[0] << 3) | (iv[1] << 2)
                                     | (iv[2] << 1) | iv[4];
  envelope.set_counter(counter);
  memset(hello_buf, 0, 16);
  envelope.SerializeToArray(hello_buf, 16);
  
  aes_context ctx;
  aes_setkey_enc(&ctx, key, 128);
  aes_crypt_cbc(&ctx, AES_ENCRYPT, 16, iv, hello_buf, hello_buf);

  hello_size = (envelope.ByteSize() + 16) & ~15;
}

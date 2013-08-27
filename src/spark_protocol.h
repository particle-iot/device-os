#include "coap.h"
#include "tropicssl/aes.h"

class SparkProtocol
{
  public:
    int init(const unsigned char *private_key,
             const unsigned char *pubkey,
             const unsigned char *encrypted_credentials,
             const unsigned char *signature);
    void hello(unsigned char *buf);
    void hello(unsigned char *buf, unsigned char token);
    CoAPMessageType::Enum received_message(unsigned char *buf);

  private:
    aes_context aes;
    unsigned char key[16];
    unsigned char iv[16];
    unsigned char salt[8];
    unsigned short _message_id;

    unsigned short next_message_id();
};

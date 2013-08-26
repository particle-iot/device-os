#include "coap.h"

class SparkProtocol
{
  public:
    void init(const unsigned char *private_key,
              const unsigned char *pubkey,
              const unsigned char *encrypted_credentials,
              const unsigned char *signature);
    CoAPMessageType::Enum received_message(const unsigned char *ciphertext);
};

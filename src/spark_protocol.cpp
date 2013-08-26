#include "spark_protocol.h"

void SparkProtocol::init(const unsigned char *private_key,
                         const unsigned char *pubkey,
                         const unsigned char *encrypted_credentials,
                         const unsigned char *signature)
{

}

CoAPMessageType::Enum
  SparkProtocol::received_message(const unsigned char *ciphertext)
{
  return CoAPMessageType::FUNCTION_CALL;
}

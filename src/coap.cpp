#include "coap.h"

CoAPCode::Enum CoAP::code(const unsigned char *message)
{
  switch (message[1])
  {
    case 0x00: return CoAPCode::EMPTY;
    case 0x01: return CoAPCode::GET;
    case 0x02: return CoAPCode::POST;
    case 0x03: return CoAPCode::PUT;
    default: return CoAPCode::ERROR;
  }
}

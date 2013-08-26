namespace CoAPMessageType {
  enum Enum {
    FUNCTION_CALL,
    VARIABLE_REQUEST,
    CHUNK,
    UPDATE_BEGIN,
    UPDATE_DONE,
    KEY_CHANGE,
    ERROR
  };
}

namespace CoAPCode {
  enum Enum {
    GET,
    POST,
    PUT,
    ERROR
  };
}

class CoAP
{
  public:
    static CoAPCode::Enum code(const unsigned char *message);
};

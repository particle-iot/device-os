namespace CoAPMessageType {
  enum Enum {
    HELLO,
    DESCRIBE,
    FUNCTION_CALL,
    VARIABLE_REQUEST,
    UPDATE_BEGIN,
    UPDATE_DONE,
    CHUNK,
    KEY_CHANGE,
    SIGNAL_START,
    SIGNAL_STOP,
    EMPTY,
    ERROR
  };
}

namespace CoAPCode {
  enum Enum {
    GET,
    POST,
    PUT,
    EMPTY,
    ERROR
  };
}

class CoAP
{
  public:
    static CoAPCode::Enum code(const unsigned char *message);
};

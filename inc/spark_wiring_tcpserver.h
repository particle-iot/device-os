#include "Server.h"

class TCPClient;

class TCPServer : 
public Server {
private:
  uint16_t _port;
  void accept();
public:
  TCPServer(uint16_t);
  TCPClient available();
  virtual void begin();
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  using Print::write;
};
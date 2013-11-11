#include "w5100.h"
#include "socket.h"

extern "C" {
  #include "string.h"
}

#include "Arduino.h"

#include "Ethernet.h"
#include "TCPClient.h"
#include "TCPServer.h"
#include "Dns.h"

uint16_t TCPClient::_srcport = 1024;

TCPClient::TCPClient() : _sock(MAX_SOCK_NUM) 
{
}

TCPClient::TCPClient(uint8_t sock) : _sock(sock) 
{
}

int TCPClient::connect(const char* host, uint16_t port) 
{

}

int TCPClient::connect(IPAddress ip, uint16_t port) 
{

}

size_t TCPClient::write(uint8_t b) 
{

}

size_t TCPClient::write(const uint8_t *buf, size_t size) 
{

}

int TCPClient::available() 
{

}

int TCPClient::read() 
{

}

int TCPClient::read(uint8_t *buf, size_t size) 
{

}

int TCPClient::peek() 
{

}

void TCPClient::flush() 
{

}

void TCPClient::stop() 
{
  
}

uint8_t TCPClient::connected() 
{
 
}

uint8_t TCPClient::status() 
{

}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.

TCPClient::operator bool()
{
  
}

#include "w5100.h"
#include "socket.h"
extern "C" {
#include "string.h"
}

#include "Ethernet.h"
#include "TCPClient.h"
#include "TCPServer.h"

TCPServer::TCPServer(uint16_t port)
{
  _port = port;
}

void TCPServer::begin()
{

}

void TCPServer::accept()
{

}

TCPClient TCPServer::available()
{

}

size_t TCPServer::write(uint8_t b) 
{

}

size_t TCPServer::write(const uint8_t *buffer, size_t size) 
{

}

#include "w5100.h"
#include "socket.h"
#include "Ethernet.h"
#include "Udp.h"
#include "Dns.h"

/* Constructor */
UDP::UDP() : _sock(MAX_SOCK_NUM) {}

/* Start UDP socket, listening at local port PORT */
uint8_t UDP::begin(uint16_t port) 
{

}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int UDP::available() 
{

}

/* Release any resources being used by this UDP instance */
void UDP::stop()
{

}

int UDP::beginPacket(const char *host, uint16_t port)
{

}

int UDP::beginPacket(IPAddress ip, uint16_t port)
{

}

int UDP::endPacket()
{

}

size_t UDP::write(uint8_t byte)
{

}

size_t UDP::write(const uint8_t *buffer, size_t size)
{

}

int UDP::parsePacket()
{

}

int UDP::read()
{

}

int UDP::read(unsigned char* buffer, size_t len)
{


}

int UDP::peek()
{
  // Unlike recv, peek doesn't check to see if there's any data available, so we must.
  // If the user hasn't called parsePacket yet then return nothing otherwise they
  // may get the UDP header

}

void UDP::flush()
{

}
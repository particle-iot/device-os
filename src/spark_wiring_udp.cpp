/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
*/

#include "spark_wiring_udp.h"

/* Constructor */
UDP::UDP() : _sock(MAX_SOCK_NUM) {}

/* Start UDP socket, listening at local port PORT */
uint8_t UDP::begin(uint16_t port) 
{
	//To Do
	return 0;
}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int UDP::available() 
{
	//To Do
	return 0;
}

/* Release any resources being used by this UDP instance */
void UDP::stop()
{

}

int UDP::beginPacket(const char *host, uint16_t port)
{
	//To Do
	return 0;
}

int UDP::beginPacket(IPAddress ip, uint16_t port)
{
	//To Do
	return 0;
}

int UDP::endPacket()
{
	//To Do
	return 0;
}

size_t UDP::write(uint8_t byte)
{
	//To Do
	return 0;
}

size_t UDP::write(const uint8_t *buffer, size_t size)
{
	//To Do
	return 0;
}

int UDP::parsePacket()
{
	//To Do
	return 0;
}

int UDP::read()
{
	//To Do
	return 0;
}

int UDP::read(unsigned char* buffer, size_t len)
{
	//To Do
	return 0;
}

int UDP::peek()
{
  // Unlike recv, peek doesn't check to see if there's any data available, so we must.
  // If the user hasn't called parsePacket yet then return nothing otherwise they
  // may get the UDP header
	//To Do
	return 0;
}

void UDP::flush()
{

}

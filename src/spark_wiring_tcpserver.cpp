/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
*/

#include "spark_wiring_tcpclient.h"
#include "spark_wiring_tcpserver.h"

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
	//To Do
	return TCPClient(MAX_SOCK_NUM);
}

size_t TCPServer::write(uint8_t b) 
{
	//To Do
	return 0;
}

size_t TCPServer::write(const uint8_t *buffer, size_t size) 
{
	//To Do
	return 0;
}

/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_TCPSERVER_H
#define __SPARK_WIRING_TCPSERVER_H

#include "spark_wiring.h"

class TCPClient;

class TCPServer {
private:
	uint16_t _port;
	long _sock;
	TCPClient _clients[MAX_SOCK_NUM];
	void acceptClientConnections();

public:
	TCPServer(uint16_t);

	TCPClient available();
	void begin();
	size_t write(uint8_t);
	size_t write(const uint8_t *buf, size_t size);
	//using Print::write;
};

#endif

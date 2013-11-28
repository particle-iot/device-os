/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_TCPSERVER_H
#define __SPARK_WIRING_TCPSERVER_H

#include "spark_wiring.h"

class TCPClient;

class TCPServer : public Print {
private:
	uint16_t _port;
	long _sock;
	TCPClient _clients[MAX_SOCK_NUM];
	void acceptClientConnections();

public:
	TCPServer(uint16_t);

	TCPClient available();
	virtual void begin();
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buf, size_t size);

	using Print::write;
};

#endif

/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_TCPCLIENT_H
#define __SPARK_WIRING_TCPCLIENT_H

#include "spark_wiring.h"


class TCPClient {

public:
	TCPClient();
	TCPClient(uint8_t sock);

	uint8_t status();
	int connect(IPAddress ip, uint16_t port);
	int connect(const char *host, uint16_t port);
	size_t write(uint8_t);
	size_t write(const uint8_t *buf, size_t size);
	int available();
	int read();
	int read(uint8_t *buf, size_t size);
	int peek();
	void flush();
	void stop();
	uint8_t connected();
	operator bool();

	friend class TCPServer;

	//using Print::write;

private:
	static uint16_t _srcport;
	long _sock;
};

#endif

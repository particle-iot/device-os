/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_TCPCLIENT_H
#define __SPARK_WIRING_TCPCLIENT_H

#include "spark_wiring.h"


class TCPClient : public Stream {

public:
	TCPClient();
	TCPClient(uint8_t sock);

	uint8_t status();
	virtual int connect(IPAddress ip, uint16_t port);
	virtual int connect(const char *host, uint16_t port);
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buf, size_t size);
	virtual int available();
	virtual int read();
	virtual int read(uint8_t *buf, size_t size);
	virtual int peek();
	virtual void flush();
	virtual void stop();
	virtual uint8_t connected();
	virtual operator bool();

	friend class TCPServer;

	using Print::write;

private:
	static uint16_t _srcport;
	long _sock;
};

#endif

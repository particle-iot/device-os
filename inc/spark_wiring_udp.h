/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_UDP_H
#define __SPARK_WIRING_UDP_H

#include "spark_wiring.h"

#define RX_BUF_MAX_SIZE	512

class UDP : public Stream {
private:
	uint8_t _sock;
	uint16_t _port;
	IPAddress _remoteIP;
	uint16_t _remotePort;
	sockaddr _remoteSockAddr;
	socklen_t _remoteSockAddrLen;
	uint8_t _buffer[RX_BUF_MAX_SIZE];
	uint16_t _offset;
	uint16_t _remaining;

public:
	UDP();

	virtual uint8_t begin(uint16_t);
	virtual void stop();
	virtual int beginPacket(IPAddress ip, uint16_t port);
	virtual int beginPacket(const char *host, uint16_t port);
	virtual int endPacket();
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buffer, size_t size);
	virtual int parsePacket();
	virtual int available();
	virtual int read();
	virtual int read(unsigned char* buffer, size_t len);
	virtual int read(char* buffer, size_t len) { return read((unsigned char*)buffer, len); };
	virtual int peek();
	virtual void flush();
	virtual IPAddress remoteIP() { return _remoteIP; };
	virtual uint16_t remotePort() { return _remotePort; };

	using Print::write;
};

#endif

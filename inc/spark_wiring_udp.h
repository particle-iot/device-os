/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_UDP_H
#define __SPARK_WIRING_UDP_H

#include "spark_wiring.h"

#define RX_BUF_MAX_SIZE	512

class UDP {
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

	uint8_t begin(uint16_t);
	void stop();
	int beginPacket(IPAddress ip, uint16_t port);
	int beginPacket(const char *host, uint16_t port);
	int endPacket();
	size_t write(uint8_t);
	size_t write(const uint8_t *buffer, size_t size);
	int parsePacket();
	int available();
	int read();
	int read(unsigned char* buffer, size_t len);
	int read(char* buffer, size_t len) { return read((unsigned char*)buffer, len); };
	int peek();
	void flush();
	IPAddress remoteIP() { return _remoteIP; };
	uint16_t remotePort() { return _remotePort; };
};

#endif

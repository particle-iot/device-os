/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
*/

#include "spark_wiring_udp.h"

UDP::UDP() : _sock(MAX_SOCK_NUM)
{

}

uint8_t UDP::begin(uint16_t port) 
{
	sockaddr tUDPAddr;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock < 0)
	{
		return 0;
	}

	_sock = sock;
	_port = port;

	tUDPAddr.sa_family = AF_INET;

	tUDPAddr.sa_data[0] = (_port & 0xFF00) >> 8;
	tUDPAddr.sa_data[1] = (_port & 0x00FF);
	tUDPAddr.sa_data[2] = 0;
	tUDPAddr.sa_data[3] = 0;
	tUDPAddr.sa_data[4] = 0;
	tUDPAddr.sa_data[5] = 0;

	if (bind(_sock, (sockaddr*)&tUDPAddr, sizeof(tUDPAddr)) < 0)
	{
		return 0;
	}

	return 1;
}

int UDP::available() 
{
	return _available;
}

void UDP::stop()
{
	if (closesocket(_sock) == 0)
	{
		_sock = MAX_SOCK_NUM;
	}
}

int UDP::beginPacket(const char *host, uint16_t port)
{
	uint32_t ip_addr = 0;

	if(gethostbyname((char*)host, strlen(host), &ip_addr) > 0)
	{
		IPAddress remote_addr(BYTE_N(ip_addr, 3), BYTE_N(ip_addr, 2), BYTE_N(ip_addr, 1), BYTE_N(ip_addr, 0));

		return beginPacket(remote_addr, port);
	}

	return 0;
}

int UDP::beginPacket(IPAddress ip, uint16_t port)
{
	_remoteIP = ip;
	_remotePort = port;

	_remoteSockAddr.sa_family = AF_INET;

	_remoteSockAddr.sa_data[0] = (_remotePort & 0xFF00) >> 8;
	_remoteSockAddr.sa_data[1] = (_remotePort & 0x00FF);

	_remoteSockAddr.sa_data[2] = _remoteIP._address[0];
	_remoteSockAddr.sa_data[3] = _remoteIP._address[1];
	_remoteSockAddr.sa_data[4] = _remoteIP._address[2];
	_remoteSockAddr.sa_data[5] = _remoteIP._address[3];

	_remoteSockAddrLen = sizeof(_remoteSockAddr);

	return 1;
}

int UDP::endPacket()
{
	return 1;
}

size_t UDP::write(uint8_t byte)
{
	return write(&byte, 1);
}

size_t UDP::write(const uint8_t *buffer, size_t size)
{
	return sendto(_sock, buffer, size, 0, &_remoteSockAddr, _remoteSockAddrLen);
}

int UDP::parsePacket()
{
	_types_fd_set_cc3000 readSet;
	timeval timeout;

	FD_ZERO(&readSet);
	FD_SET(_sock, &readSet);

	timeout.tv_sec = 0;
	timeout.tv_usec = 5000;

	_available = 0;

	if (select(_sock + 1, &readSet, NULL, NULL, &timeout) > 0)
	{
		if (FD_ISSET(_sock, &readSet))
		{
		    //read 0 bytes to get IP and port
		    if (recvfrom(_sock, NULL, 0, 0, &_remoteSockAddr, &_remoteSockAddrLen) == 0)
		    {
		    	_remotePort = _remoteSockAddr.sa_data[0] << 8;
		    	_remotePort = _remoteSockAddr.sa_data[1] | _remotePort;

		    	_remoteIP._address[0] = _remoteSockAddr.sa_data[2];
		    	_remoteIP._address[1] = _remoteSockAddr.sa_data[3];
		    	_remoteIP._address[2] = _remoteSockAddr.sa_data[4];
		    	_remoteIP._address[3] = _remoteSockAddr.sa_data[5];

		    	_available = 1;
		    }
		}
	}

	return _available;
}

int UDP::read()
{
	uint8_t b;
	if (recvfrom(_sock, &b, 1, 0, &_remoteSockAddr, &_remoteSockAddrLen) > 0)
	{
		return b;
	}
	else
	{
		return -1;
	}
}

int UDP::read(unsigned char* buffer, size_t len)
{
	return recvfrom(_sock, buffer, len, 0, &_remoteSockAddr, &_remoteSockAddrLen);
}

int UDP::peek()
{
	if (!available())
	{
		return -1;
	}

	return read();
}

void UDP::flush()
{
	while (available())
	{
		read();
	}
}

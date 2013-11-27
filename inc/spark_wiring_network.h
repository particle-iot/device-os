/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_NETWORK_H
#define __SPARK_WIRING_NETWORK_H

#include "spark_wiring.h"

class NetworkClass
{
public:
	NetworkClass();

	uint8_t* macAddress(uint8_t* mac);
	IPAddress localIP();
	IPAddress subnetMask();
	IPAddress gatewayIP();
	char* SSID();

	friend class TCPClient;
	friend class TCPServer;
};

extern NetworkClass Network;

#endif

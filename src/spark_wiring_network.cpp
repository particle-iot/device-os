/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#include "spark_wiring_network.h"

NetworkClass::NetworkClass()
{
}

uint8_t* NetworkClass::macAddress(uint8_t* mac)
{
	memcpy(mac, ip_config.uaMacAddr, 6);
	return mac;
}

IPAddress NetworkClass::localIP()
{
	return IPAddress(ip_config.aucIP[3], ip_config.aucIP[2], ip_config.aucIP[1], ip_config.aucIP[0]);
}

IPAddress NetworkClass::subnetMask()
{
	return IPAddress(ip_config.aucSubnetMask[3], ip_config.aucSubnetMask[2], ip_config.aucSubnetMask[1], ip_config.aucSubnetMask[0]);
}

IPAddress NetworkClass::gatewayIP()
{
	return IPAddress(ip_config.aucDefaultGateway[3], ip_config.aucDefaultGateway[2], ip_config.aucDefaultGateway[1], ip_config.aucDefaultGateway[0]);
}

char* NetworkClass::SSID()
{
	return (char *)ip_config.uaSSID;
}

NetworkClass Network;

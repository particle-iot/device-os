/*
  TCP/UDP library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_IPADDRESS_H
#define __SPARK_WIRING_IPADDRESS_H

#include "spark_wiring.h"

class IPAddress {
private:
	uint8_t _address[4];  // IPv4 address
	// Access the raw byte array containing the address.  Because this returns a pointer
	// to the internal structure rather than a copy of the address this function should only
	// be used when you know that the usage of the returned uint8_t* will be transient and not
	// stored.
	uint8_t* raw_address() { return _address; };

public:
	// Constructors
	IPAddress();
	IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
	IPAddress(uint32_t address);
	IPAddress(const uint8_t *address);

	// Overloaded cast operator to allow IPAddress objects to be used where a pointer
	// to a four-byte uint8_t array is expected
	//operator uint32_t() { return *((uint32_t*)_address); };
	//bool operator==(const IPAddress& addr) { return (*((uint32_t*)_address)) == (*((uint32_t*)addr._address)); };
	bool operator==(const uint8_t* addr);

	// Overloaded index operator to allow getting and setting individual octets of the address
	uint8_t operator[](int index) const { return _address[index]; };
	uint8_t& operator[](int index) { return _address[index]; };

	// Overloaded copy operators to allow initialisation of IPAddress objects from other types
	IPAddress& operator=(const uint8_t *address);
	IPAddress& operator=(uint32_t address);

	friend class TCPClient;
	friend class TCPServer;
	friend class UDP;
};

const IPAddress INADDR_NONE(0,0,0,0);

#endif

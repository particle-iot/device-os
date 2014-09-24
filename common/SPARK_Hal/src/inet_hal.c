#include "inet_hal.h"
#include "socket.h"

int inet_gethostbyname(char* hostname, uint16_t hostnameLen, uint32_t* out_ip_addr)
{
    return gethostbyname(hostname, hostnameLen, out_ip_addr);
}

// inet_ping in wlan_hal.c
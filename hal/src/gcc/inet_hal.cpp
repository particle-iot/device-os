
#include "inet_hal.h"

#include "device_globals.h"

namespace ip = boost::asio::ip;

int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr,
        network_interface_t nif, void* reserved)
{
    out_ip_addr->ipv4 = 0;
    ip::tcp::resolver resolver(device_io_service);
    ip::tcp::resolver::query query(hostname, "");
    for(ip::tcp::resolver::iterator i = resolver.resolve(query);
                            i != ip::tcp::resolver::iterator();
                            ++i)
    {
        ip::tcp::endpoint end = *i;
        ip::address addr = end.address();
        if (addr.is_v4()) {
            ip::address_v4 addr_v4 = addr.to_v4();
            out_ip_addr->ipv4 = addr_v4.to_ulong();
        }
    }
    return 0;
}

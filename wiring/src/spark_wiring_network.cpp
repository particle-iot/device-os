
#include "spark_wiring_network.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_wifi.h"

using namespace spark;

NetworkClass& NetworkClass::from(network_interface_t nif)
{
#if Wiring_WiFi_AP
    if (nif==1)
        return AP;
#endif
    return Network;
}
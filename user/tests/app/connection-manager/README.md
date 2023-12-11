# Overview
This application is intended to facilitate testing of the various Automatic Connection Managment features.
The application takes commands on Serial1 to exercise different functions

To enable the test app:
Add the following includes to `system_task.cpp` 
```
#if PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL
#include "ifapi.h"
#include "system_network_diagnostics.h"
#include "system_connection_manager.h"
#include "system_cache.h"
#include "system_network_manager.h"
#endif
```
Add the following code to the `system_internal()` test function 

```
#if PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL
    case 4: {
        particle::system::ConnectionTester::instance()->testConnections();
        return nullptr;
    }
    case 5: {
        return reinterpret_cast<void*>(&lwip_stats);
    }
    case 6: {
        return (void*)particle::system::ConnectionTester::instance()->getConnectionMetrics();
    }
    case 7: {
        // log lwip counters
        LOG(TRACE,"[%s] IP rx %u %u %u %u | UDP rx %u %u %u %u", 
                (const char *)reserved, lwip_stats.ip.recv, lwip_stats.ip.drop, lwip_stats.ip.err, lwip_stats.ip.proterr,
                lwip_stats.udp.recv, lwip_stats.udp.drop, lwip_stats.udp.err, lwip_stats.udp.proterr);
        return nullptr;
    }
    case 8: {
        // Get cached cellular ID
        static uint8_t cacheRead[70] = {};
        memset(cacheRead, 0x00, sizeof(cacheRead));
        services::SystemCache::instance().get(services::SystemCacheKey::CELLULAR_DEVICE_INFO, &cacheRead, sizeof(cacheRead));
        return &cacheRead;
    }
    case 9: {
        // Delete cached cellular ID
        services::SystemCache::instance().del(services::SystemCacheKey::CELLULAR_DEVICE_INFO);
        return nullptr;
    }
    case 10: {
        // Log internal network manager interface states
        int interfaces[3] = { NETWORK_INTERFACE_ETHERNET, NETWORK_INTERFACE_CELLULAR, NETWORK_INTERFACE_WIFI_STA };
        for (int i = 0; i < 3; i++) {
            if_t iface;
            if_get_by_index(interfaces[i], &iface);
            bool ifEnabled = particle::system::NetworkManager::instance()->isInterfaceEnabled(iface);
            bool ifConfigured = particle::system::NetworkManager::instance()->isConfigured(iface);
            bool ifOn = particle::system::NetworkManager::instance()->isInterfaceOn(iface);
            bool ifPhy = particle::system::NetworkManager::instance()->isInterfacePhyReady(iface);
            bool ipUp = ((int)particle::system::NetworkManager::instance()->getInterfaceIp4State(iface) == 2);
            LOG(TRACE, "IF %d enabled %d configured %d power %d phyready %d ipConfigured %d", interfaces[i], ifEnabled, ifConfigured, ifOn, ifPhy, ipUp);
        }
        return nullptr;
    }
#endif //#if PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL
```


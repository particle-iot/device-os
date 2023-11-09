#include "Particle.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
STARTUP(System.enable(SYSTEM_FLAG_PM_DETECTION));

#define CMD_SERIAL Serial1

Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, 
{
    { "app", LOG_LEVEL_ALL }, 
    //{ "sys.power", LOG_LEVEL_TRACE },
    { "comm.protocol", LOG_LEVEL_TRACE },
    { "system", LOG_LEVEL_TRACE },
    { "network", LOG_LEVEL_TRACE },
    { "ncp.at", LOG_LEVEL_TRACE },
    { "system.nm", LOG_LEVEL_TRACE },
    { "system.cm", LOG_LEVEL_TRACE },
    { "system.nd", LOG_LEVEL_TRACE },
    { "ncp.client", LOG_LEVEL_TRACE },
    { "net.rltkncp", LOG_LEVEL_TRACE },
    { "net.ifapi", LOG_LEVEL_TRACE },
    { "net.ppp.client", LOG_LEVEL_WARN },
    { "net.en", LOG_LEVEL_TRACE },
    { "service.ntp", LOG_LEVEL_TRACE },
    { "net.en", LOG_LEVEL_TRACE },
    { "mux", LOG_LEVEL_ERROR },
    { "net.ppp.ipcp", LOG_LEVEL_ERROR },
    { "comm.cloud.posix", LOG_LEVEL_TRACE },
}   
);

// Copied from system_connection_manager.h
struct ConnectionMetrics {
    network_interface_t interface;
    int socketDescriptor;
    uint8_t *txBuffer;
    uint8_t *rxBuffer;
    int testPacketSize;
    uint32_t testPacketSequenceNumber;
    int txPacketCount;
    int rxPacketCount;
    int txPacketStartMillis;
    int totalPacketWaitMillis;

    // uint32_t dnsResolutionAttempts;
    // uint32_t dnsResolutionFailures;
    // uint32_t socketConnAttempts;
    // uint32_t socketConnFailures;
    uint32_t txBytes;
    uint32_t rxBytes;
    uint32_t avgPacketRoundTripTime;
};

// Copied from HAL
struct __attribute__((packed)) CellularDeviceCached
{
    uint16_t size;
    uint16_t version;
    char iccid[21];
    char imei[16];
    int dev;
    char radiofw[25];
};

static void printCellularInfo() {
    CellularDevice device = {};
    device.size = sizeof(device);
    cellular_device_info(&device, NULL);
    // Log.info("sizeof(CellularDevice) %d device.size %d", sizeof(CellularDevice), device.size);
    // Log.dump(&device, sizeof(device));
    Log.info("HAL    ICCID %s IMEI %s dev %d FW %s", device.iccid, device.imei, device.dev, device.radiofw);
};

static void printCellularCacheInfo(CellularDeviceCached * cache) {
    Log.info("Cached ICCID %s IMEI %s dev %d FW %s", cache->iccid, cache->imei, cache->dev, cache->radiofw);

};

WiFiAccessPoint ap[5];

void logNetworkStates() {
    int wifiReady, cellReady = -1;
#if HAL_PLATFORM_CELLULAR
    cellReady = Cellular.ready();
#endif
#if HAL_PLATFORM_WIFI
    wifiReady = WiFi.ready();
#endif

    auto activeNetwork = Particle.connectionInterface();
    Log.info("Ethernet Ready: %d WiFi ready: %d Cellular ready: %d Cloud conn: %lu", 
        Ethernet.ready(), 
        wifiReady, 
        cellReady,
        static_cast<network_interface_t>(activeNetwork));
}

void setup() {
    Serial1.begin(115200);
}
 
void loop() {
    if (!CMD_SERIAL.available()) {
        return;
    }

    while (CMD_SERIAL.available()) {
        char c = CMD_SERIAL.read();
        logNetworkStates();

        if (c == 'p') {
            Log.info("Particle.publishVitals");
            Particle.publishVitals(particle::NOW);
        }
        else if(c == '1') {
            // Run the internal connection test
            system_internal(4, nullptr);
        }
        else if(c == '2') {
            stats_* lwipStats = (stats_*)system_internal(5, nullptr); 
            Log.info("LWIP link  xmit: %u recv %u drop %u err %u", 
                lwipStats->link.xmit, lwipStats->link.recv, lwipStats->link.drop, lwipStats->link.err);
            Log.info("LWIP udp   xmit: %u recv %u drop %u err %u proterr %u", 
                lwipStats->udp.xmit, lwipStats->udp.recv, lwipStats->udp.drop, lwipStats->udp.err, lwipStats->udp.proterr);

            // Connection tester metrics
            const Vector<ConnectionMetrics>* ConnectionTesterDiagnostics = (const Vector<ConnectionMetrics>*)(system_internal(6, nullptr));
            for (auto& i: *ConnectionTesterDiagnostics) {
                Log.info("interface %lu tx bytes %lu rx bytes %lu", 
                    i.interface, i.txBytes, i.rxBytes, i.avgPacketRoundTripTime);
            }
        }
        else if(c == '3') {
            Particle.disconnect(CloudDisconnectOptions().clearSession(true));
            waitUntil(Particle.disconnected);
            Ethernet.connect();
            Log.info("Binding to ethernet interface");
            Particle.connect(Ethernet);
        }
        else if(c == '4') {
            Particle.disconnect();
            waitUntil(Particle.disconnected);
            Log.info("Binding to cell interface");
            Particle.connect(Cellular);
        }
        else if(c == '5') {
            Particle.disconnect(CloudDisconnectOptions().clearSession(true));
            waitUntil(Particle.disconnected);
            Log.info("Binding to wifi interface");
            Particle.connect(WiFi);
        }
        else if(c == '6') {
            Particle.disconnect();
            waitUntil(Particle.disconnected);
            Log.info("connecting with default interface priority");
            Particle.connect();
        }
#if HAL_PLATFORM_WIFI
        else if (c == 'a') {
            bool result = WiFi.clearCredentials();
            Log.info("Clear wifi creds result %d", result);
        }
        else if (c == 'w') {
            static bool wifiState = true;
            Log.info("Wifi state: %d", wifiState);

            if(wifiState){
                WiFi.connect();
            } else {
                WiFi.disconnect();
            }
            wifiState = !wifiState;
        }
        else if (c == 'l') {
            int found = WiFi.getCredentials(ap, 5);
            Log.info("Found %d wifi creds", found);
            for (int i = 0; i < found; i++) {
                Log.info("ssid: %s", ap[i].ssid);
                Log.info("security: %d", (int) ap[i].security);
                Log.info("cipher: %d", (int) ap[i].cipher);
            }
        }
#endif
#if HAL_PLATFORM_CELLULAR
        else if (c == 'c') {
            static bool cellState = true;
            Log.info("Cell state: %d", cellState);
            
            if(cellState){
                Cellular.on();
                Cellular.connect();
            } else {
                Cellular.disconnect();
                Cellular.off();
            }
            cellState = !cellState;
        }
        else if(c == '9') {
            // Query the modem for cellular info. If it is off, query the cached values
            printCellularInfo();
        }
        else if(c == 'z') {
            // Turn off modem
            Cellular.off();
            waitFor(Cellular.isOff, 30000);

            // Get cellular info when modem is off:
            // -> expect NO AT commands to be sent
            // -> expect cached value to be returned
            // These should be the same
            printCellularInfo();
            CellularDeviceCached * cache = (CellularDeviceCached*)system_internal(8, nullptr);
            printCellularCacheInfo(cache);

            // Delete cache
            system_internal(9, nullptr);
            // Expect both to be null (cannot query modem, and cache is deleted)
            printCellularInfo();
            cache = (CellularDeviceCached*)system_internal(8, nullptr);
            printCellularCacheInfo(cache);

            // Turn back on, query HAL, expect it to get cached
            // -> expect AT commands to be sent
            // -> expect info to be cached
            Cellular.on();
            waitFor(Cellular.isOn, 30000);
            // These should be the same
            printCellularInfo();
            cache = (CellularDeviceCached*)system_internal(8, nullptr);
            printCellularCacheInfo(cache);

        }  
#endif // HAL_PLATFORM_CELLULAR
        else if (c == 'g') {
            static bool ethernetConnect = true;
            Log.info("ethernetConnect: %d", ethernetConnect);
            
            if(ethernetConnect){
                Ethernet.connect();
            } else {
                Ethernet.disconnect();
            }
            ethernetConnect = !ethernetConnect;
        }
    }
}
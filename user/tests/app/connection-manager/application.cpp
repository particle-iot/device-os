#include "Particle.h"
#include "scope_guard.h"
#include "resolvapi.h"
#include "ifapi.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
STARTUP(System.enable(SYSTEM_FLAG_PM_DETECTION));

retained uint8_t resetRetry = 0;
#define ETHERNET_RETRY_MAX (10)
#define MAX_CELLULAR_OFF_TIME (3*60*1000)

#define CMD_SERIAL Serial

SerialLogHandler logHandler(LOG_LEVEL_ALL,
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

#if HAL_PLATFORM_CELLULAR
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
#endif

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
    Log.info("Ethernet Ready: %d Cellular ready: %d WiFi ready: %d Cloud conn: %lu",
        Ethernet.ready(),
        cellReady,
        wifiReady,
        static_cast<network_interface_t>(activeNetwork));
    // Detailed interface state
    system_internal(10, nullptr);
}

void setup() {
    CMD_SERIAL.begin(115200);

    if (System.featureEnabled(FEATURE_DISABLE_LISTENING_MODE)) {
        Log.info("Make sure Listening Mode is enabled so we can enter WiFi credentials");
        System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    }

    if (System.featureEnabled(FEATURE_ETHERNET_DETECTION)) {
        Log.info("FEATURE_ETHERNET_DETECTION enabled");
    } else {
        Log.info("Enabling Ethernet...");
        System.enableFeature(FEATURE_ETHERNET_DETECTION);
    }

    Log.info("Checking if Ethernet is on...");
    if (Ethernet.isOn()) {
        Log.info("Ethernet is on");
        uint8_t macAddrBuf[8] = {};
        uint8_t* macAddr = Ethernet.macAddress(macAddrBuf);
        if (macAddr != nullptr) {
            Log.info("Ethernet MAC: %02x %02x %02x %02x %02x %02x",
                    macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        }
        // Ethernet.connect();
        // waitFor(Ethernet.ready, 30000);
        // Log.info("Ethernet.ready: %d", Ethernet.ready());
        resetRetry = 0;
    } 
    else if (++resetRetry <= ETHERNET_RETRY_MAX) {
        Log.info("Ethernet is off or not detected, attmpting to remap pins: %d/%d", resetRetry, ETHERNET_RETRY_MAX);

#if PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_MSOM
        if (resetRetry == 4) {
            Log.info("Reset Ethernet chip");
            pinMode(A7, OUTPUT);
            digitalWrite(A7, LOW);
            delay(1000);
            digitalWrite(A7, HIGH);
            delay(1000);
            pinMode(A7, INPUT); // HI-Z
            delay(1000);
        }
#endif

        if_wiznet_pin_remap remap = {};
        remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;

        remap.cs_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_CS_PIN_DEFAULT; // default
        remap.reset_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_RESET_PIN_DEFAULT; // default
        remap.int_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_INT_PIN_DEFAULT; // default

        // remap.cs_pin = D8; // MSoM Eval Board
        // remap.reset_pin = A7;
        // remap.int_pin = D22;

        // remap.cs_pin = D5; // Feather Wing
        // remap.reset_pin = D3;
        // remap.int_pin = D4;

        // remap.cs_pin = D2;
        // remap.reset_pin = D0;
        // remap.int_pin = D1;

        // remap.cs_pin = D10;
        // remap.reset_pin = D6;
        // remap.int_pin = D7;

        // remap.cs_pin = A0;
        // remap.reset_pin = A2;
        // remap.int_pin = A1;

        // remap.cs_pin = 50; // bad pin config
        // remap.reset_pin = 100;
        // remap.int_pin = 150;

        auto ret = if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);
        if (ret != SYSTEM_ERROR_NONE) {
            Log.error("Ethernet GPIO config error: %d", ret);
        } else {
            if (System.featureEnabled(FEATURE_ETHERNET_DETECTION)) {
                Log.info("FEATURE_ETHERNET_DETECTION enabled");
            } else {
                Log.info("Enabling Ethernet...");
                System.enableFeature(FEATURE_ETHERNET_DETECTION);
            }
            delay(500);
            System.reset();
        }
    }

    // Do not connect by default so we can try to cold boot and connect with various methods
    // Particle.connect();
}

void loop() {
    if (!CMD_SERIAL.available()) {
        return;
    }

    while (CMD_SERIAL.available()) {
        char c = CMD_SERIAL.read();
        logNetworkStates();

        LOG_PRINTF_C(INFO, "app", "\r\nSerial Command: %c\r\n", c);

        if (c == 'p') {
            Log.info("Particle.publishVitals");
            Particle.publishVitals(particle::NOW);
        }
        else if(c == '1') {
            // Run the internal connection test
            system_internal(4, nullptr);
        }
        else if(c == '2') {
#if HAL_PLATFORM_WIFI
            // Prefer wifi
            WiFi.prefer();
            // Confirm that we prefer wifi 
            if (Network.prefer() == WiFi) {
                Log.info("Wifi is preferred");
            }
            // No longer prefer wifi, revert to default
            WiFi.prefer(false);
            if (Network.prefer() == Network) {
                Log.info("Default is preferred");
            }
#endif
#if HAL_PLATFORM_CELLULAR
            // Prefer cellular
            Cellular.prefer();
            // Confirm cellular is preferred 
            if (Network.prefer() == Cellular) {
                Log.info("Cellular is preferred");
            }
            // Confirm calling Network does not change preference
            Network.prefer();
            if (Network.prefer() == Cellular) {
                Log.info("Cellular is still preferred");
            }
#endif
            // Clear any set network preference
            Network.prefer(false);
            if (Network.prefer() == Network) {
                Log.info("Default is preferred");
            }
        }
        else if(c == '3') {
            Log.info("Prefer Ethernet");
            Ethernet.prefer();
        }
        else if(c == '4') {
            Log.info("Prefer None");
            Network.prefer();
        }
#if HAL_PLATFORM_WIFI
        else if(c == '5') {
            Log.info("Prefer WiFi");
            WiFi.prefer();
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
       else if (c == 'a') {
            bool result = WiFi.clearCredentials();
            Log.info("Clear wifi creds result %d", result);
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
        else if(c == '6') {
            Log.info("Prefer Cellular");
            Cellular.prefer();
        }
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
            waitFor(Cellular.isOff, MAX_CELLULAR_OFF_TIME);

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

            if (ethernetConnect) {
                Ethernet.connect();
            } else {
                Ethernet.disconnect();
            }
            ethernetConnect = !ethernetConnect;
        }
        else if (c == 'i') {
            // Print the known DNS servers
            resolv_dns_servers* dns = {};
            resolv_get_dns_servers(&dns);
            SCOPE_GUARD({
                resolv_free_dns_servers(dns);
            });
            for (auto s = dns; s != nullptr; s = s->next) {
                // Only IPv4 for now
                SockAddr a(s->server);
                if (a.family() == AF_INET) {
                    Log.info("DNS IP %s", a.toString().c_str());
                }
            }
        }
    }
}
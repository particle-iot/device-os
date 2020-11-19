#include "Particle.h"
#include "esp32_soch.h"
#include "sdspi_host.h"
#include "port.h"

#define GPS_WAKEUP          0
#define WIFI_WAKEUP         0
#define FUEL_WAKEUP         1

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

volatile bool idle = true;

#if PLATFORM_ID == PLATFORM_TRACKER
#if GPS_WAKEUP
os_semaphore_t gpsReadySem = nullptr;
// Dirty hack!
uint8_t ubxCfgMsg[] = {
    0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x04, 0x00, 0x3B, 0x01, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x07,
    0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x06, 0x00, 0x0E, 0x37,
    0xb5, 0x62, 0x06, 0x00, 0x01, 0x00, 0x04, 0x0b, 0x25,
    0xb5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x04, 0x00, 0x3b, 0x01, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x07,
    0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x06, 0x00, 0x0E, 0x37
};

void gpsIntHandler(void) {
    if (gpsReadySem) {
        os_semaphore_give(gpsReadySem, false);
    }
}

os_thread_return_t gpsThread(void *param) {
    while (1) {
        if (os_semaphore_take(gpsReadySem, 10000/*ms*/, false)) {
            Log.error("Waiting GPS data timeout!");
            continue;
        }
        idle = false;
        Log.info("Reading GPS data...");
        // Read data
        SPI1.beginTransaction(SPISettings(5*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(GPS_CS, LOW);
        // delayMicroseconds(50);
        uint8_t temp;
        uint16_t len = 0;
        while(SPI1.transfer(0xFF) == 0xFF); // Just in case
        do {
            temp = SPI1.transfer(0xFF);
            // Serial1.printf("%c", temp);
            len++;
        } while (temp != 0xFF);
        digitalWrite(GPS_CS, HIGH);
        SPI1.endTransaction();
        Log.info("Reading GPS data DONE: %d bytes\r\n", len);
        idle = true;
    }
}

void initGps() {
    pinMode(GPS_INT, INPUT_PULLUP);
    pinMode(GPS_BOOT, OUTPUT);
    digitalWrite(GPS_BOOT, HIGH);
    pinMode(GPS_PWR, OUTPUT);
    digitalWrite(GPS_PWR, HIGH);
    pinMode(GPS_CS, OUTPUT);
    digitalWrite(GPS_CS, HIGH);
    pinMode(GPS_RST, OUTPUT);
    digitalWrite(GPS_RST, HIGH);
    digitalWrite(GPS_RST, LOW);
    delay(50);
    digitalWrite(GPS_RST, HIGH);

    if (!os_semaphore_create(&gpsReadySem, 1, 0)) {
        Thread* thread = new Thread("gpsThread", gpsThread);

        attachInterrupt(GPS_INT, gpsIntHandler, FALLING);
        // Enable TX-ready
        SPI1.beginTransaction(SPISettings(5*MHZ, MSBFIRST, SPI_MODE0));
        digitalWrite(GPS_CS, LOW);
        delay(50);
        uint8_t in_byte;
        for (uint8_t i = 0; i < sizeof(ubxCfgMsg); i++) {
            in_byte = SPI1.transfer(ubxCfgMsg[i]);
            if (in_byte != 0xFF) {
                Serial1.write(in_byte);
            }
            delay(10);
        }
        digitalWrite(GPS_CS, HIGH);
        delay(5);
        SPI1.endTransaction();
        Log.info("Initialize GPS done.");
    } else {
        LOG(ERROR, "Creating GNSS ready semophore failed.");
    }
}
#endif // WGPS_WAKEUP

#if WIFI_WAKEUP
#define READ_BUFFER_LEN 4096
spi_context_t context;
uint8_t espTxBuffer[READ_BUFFER_LEN] = "";
uint8_t espRxBuffer[READ_BUFFER_LEN + 1] = "";
os_semaphore_t wifiReadySem = nullptr;

void wifiIntHandler(void) {
    if (wifiReadySem) {
        os_semaphore_give(wifiReadySem, false);
    }
}

os_thread_return_t esp32Thread(void *param) {
    while (1) {
        if (os_semaphore_take(wifiReadySem, 20000/*ms*/, false)) {
            Log.error("Waiting Wi-Fi data timeout!");
            continue;
        }
        idle = false;
        if (digitalRead(WIFI_INT) == LOW) {
            Log.info("[WIFI] Reading WIFI data...");
            while (digitalRead(WIFI_INT) == LOW) {
                size_t size_read = READ_BUFFER_LEN;
                esp_err_t err = at_sdspi_get_packet(&context, espRxBuffer, READ_BUFFER_LEN, &size_read);
                if (err == ESP_ERR_NOT_FOUND) {
                    Log.info("interrupt but no data can be read");
                } else if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED) {
                    Log.info("rx packet error: %08X", err);
                }
                Log.info("RX: %s", espRxBuffer);
                memset(espRxBuffer, '\0', sizeof(espRxBuffer));
            }
            Log.info("[WIFI] Reading WIFI data DONE!!!");
        }
        idle = true;
    }
}

void initWifi() {
    pinMode(WIFI_INT, INPUT_PULLUP);
    pinMode(WIFI_CS, OUTPUT);
    digitalWrite(WIFI_CS, HIGH);
    pinMode(WIFI_BOOT, OUTPUT);
    digitalWrite(WIFI_BOOT, HIGH);
    pinMode(WIFI_EN, OUTPUT);
    digitalWrite(WIFI_EN, LOW);
    delay(500);
    digitalWrite(WIFI_EN, HIGH);
    delay(100);
    
    esp_err_t err = at_sdspi_init();
    assert(err == ESP_OK);
    memset(&context, 0x0, sizeof(spi_context_t));

    if (!os_semaphore_create(&wifiReadySem, 1, 0)) {
        Log.info("Initialize Wi-Fi done.");
    }
    Thread* thread = new Thread("esp32Thread", esp32Thread);

    attachInterrupt(WIFI_INT, wifiIntHandler, FALLING);
}

void wifiScan() {
    Log.info("[WIFI] Send request to scan.");
    char cmd[] = "AT+CWLAP\r\n";
    int atDataLen = strlen(cmd);
    memcpy(espTxBuffer, cmd, atDataLen);
    Log.info("[WIFI] atDataLen: %d, data: %s", atDataLen, espTxBuffer);
    esp_err_t err = at_sdspi_send_packet(&context, espTxBuffer, atDataLen, UINT32_MAX);
    Log.info("[WIFI] end at_sdspi_send_packet");
    if (err != ESP_OK) {
        Log.info("[WIFI] Send error, %d\n", err);
    }
    memset(espTxBuffer, '\0', sizeof(espTxBuffer));
}
#endif // WIFI_WAKEUP
#endif // PLATFORM_ID == PLATFORM_TRACKER

#if HAL_PLATFORM_FUELGAUGE_MAX17043 && FUEL_WAKEUP
void lowBatHandler() {
    FuelGauge fuelGauge(true);
    fuelGauge.clearAlert();
}

void initFuelGauge() {
    FuelGauge fuelGauge(true);
    fuelGauge.begin(); 
    fuelGauge.quickStart();
    fuelGauge.wakeup();
    fuelGauge.setAlertThreshold(30);
    fuelGauge.clearAlert();
    Log.info("Current percentage: %.2f%%", fuelGauge.getSoC());
}
#endif

void setup() {
    Log.info("Application started.");

    SPI1.begin();

#if PLATFORM_ID == PLATFORM_TRACKER
#if WIFI_WAKEUP
    initWifi();
#endif

#if GPS_WAKEUP
    initGps();
#endif
#endif

#if HAL_PLATFORM_FUELGAUGE_MAX17043 && FUEL_WAKEUP
    initFuelGauge();
#endif
}

void loop() {
    if (idle) {
        Log.info("Enter sleep mode.");
        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::STOP).duration(10s);
#if PLATFORM_ID == PLATFORM_TRACKER
#if GPS_WAKEUP
        config.gpio(GPS_INT, FALLING);
#endif
#if WIFI_WAKEUP
        config.gpio(WIFI_INT, FALLING);
#endif
#endif
#if HAL_PLATFORM_FUELGAUGE_MAX17043 && FUEL_WAKEUP
        config.gpio(LOW_BAT_UC, FALLING);
#endif
        SystemSleepResult result = System.sleep(config);
        if (result.error() != SYSTEM_ERROR_NONE) {
            Log.error("Failed to enter sleep mode");
            delay(3s);
        } else {
            Log.info("Exit sleep mode.");
            if (result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO) {
                Log.info("Device is woken up by pin: %d", result.wakeupPin());
            } else if (result.wakeupReason() == SystemSleepWakeupReason::BY_RTC) {
                Log.info("Device is woken up by RTC");
                FuelGauge fuelGauge(true);
                Log.info("Current percentage: %.2f%%", fuelGauge.getSoC());
#if PLATFORM_ID == PLATFORM_TRACKER && WIFI_WAKEUP
                wifiScan();
#endif
            } else {
                Log.error("Device is woken up unexpectedly, reason: %d", result.wakeupReason());
            }
        }
    }
#if PLATFORM_ID == PLATFORM_TRACKER && WIFI_WAKEUP
    static system_tick_t elapsed = millis();
    if (millis() - elapsed > 10000) {
        elapsed = millis();
        wifiScan();
    }
#endif
}

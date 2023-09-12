#include "Particle.h"

#if PLATFORM_ID == PLATFORM_ARGON
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler(115200, LOG_LEVEL_ALL);

// set to serial port configuration for test
#define SERIAL (Serial1)
#define SERIAL_BAUD (460800)
#define SERIAL_TX_INTERVAL_MS (10)
#define SERIAL_RX_TIMEOUT_MS (50)

// comment in/out to test with or without a thread scanning for BLE adverts
#define FEATURE_BLE

// for Jacuzzi this is a valid a valid modbus request for 19 registers starting
// at address 3000, can be modified for alternate test cases
uint8_t TX_BUF[] = "12345678";
constexpr size_t TX_LEN = sizeof(TX_BUF) - 1; // -1 to skip null terminator

constexpr size_t RX_EXPECT_LEN = 43; // expected length of modbus response
constexpr uint8_t testBuff[RX_EXPECT_LEN] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40,
    0x41, 0x42, 0x43
};

#ifdef FEATURE_BLE

static Thread *thread = nullptr;

void _ble_scan_result_cb(const BleScanResult *result, void *context)
{
    return;
}

void _ble_scan_thread_f()
{
    // short repeated scans
    BLE.setScanTimeout(10);

    // disable active scan
    // only get advert, don't request scan response for less overhead and
    // hopefully reduced power consumption on sensors
    BleScanParams params = {};
    params.version = BLE_API_VERSION;
    params.size = sizeof(BleScanParams);
    BLE.getScanParameters(params);
    params.active = false;
    BLE.setScanParameters(params);

    while(true)
    {
        static system_tick_t last = millis();
        if ((millis() - last) > 5000) {
            Log.info("BLE Scan thread.");
            last = millis();
        }
        BLE.scan(_ble_scan_result_cb, nullptr);
    }
}

#endif

void hex_dump(LogLevel level, uint8_t *data, int len, const char *prefix = nullptr) {
    if(prefix) {
        Log.print(level, prefix);
    }
	Log.dump(level, data, len);
	Log.print(level, "\n\r");
}

void setup() {
    waitUntil(Serial.isConnected);
    // while (!Serial.isConnected());
    // delay(1s);
    delay(3s);

    SERIAL.begin(SERIAL_BAUD);

#ifdef FEATURE_BLE
    BLE.on();
    BLE.setPPCP(12, 12, 0, 200); // increase BLE speed, may break third party

    thread = new Thread("sensor_ble_scan",
        _ble_scan_thread_f,
        OS_THREAD_PRIORITY_DEFAULT);
#endif

    Particle.connect();
}

void loop() {
    static int rx_timeout = 0, buffer_overrun = 0, crc_error = 0, rx_len_bad = 0, total_tx = 0, total_rx = 0;
    static bool timeout = false;
    static size_t rx_len = 0;

    if (timeout) {
        rx_timeout++;
        timeout = false;
    }

    // check for system blocking by monitoring for long loop delays
    // if we don't get a spam of these then should be servicing the serial port
    // on a reasonable timeframe
    static system_tick_t last_loop_ms = millis();
    auto loop_delta_ms = millis() - last_loop_ms;
    if (loop_delta_ms > 10) {
        Log.info("loop delay: %lu", loop_delta_ms);
    }
    last_loop_ms = millis();

    // handle sending TX buffer when requested
    // not running test long enough to care about rollover on millis()
    static system_tick_t next_request_tx = millis();
    static auto last_tx_ms = millis();
    if (next_request_tx && millis() > next_request_tx) {
        uint8_t count = SERIAL.available();
        if (count) {
            Log.info("!!!!available before TX: %d, rx_len: %d, millis: %ld", count, rx_len, millis());
        }
        // uint8_t before = SERIAL.availableForWrite();
        // enable_tx();
        SINGLE_THREADED_BLOCK() {
            SERIAL.write(TX_BUF, TX_LEN);
            SERIAL.flush();
            // Log.error("SERIAL.flush();");
        }
        // uint8_t after = SERIAL.availableForWrite();
        last_tx_ms = millis();
        // hex_dump(LOG_LEVEL_INFO, TX_BUF, TX_LEN, "TX: ");
        next_request_tx = 0;
        total_tx++;
        // Log.info("TX STARTED: %d, %d-%d", total_tx, before, after);
    }

    // it not currently sending and it has been too long then the rx must have
    // failed
    // call a timeout and request the next send
    if (!next_request_tx && (millis() - last_tx_ms > 1000)) {
        next_request_tx = millis();
        timeout = true;
        uint8_t count = SERIAL.available();
        Log.error("RX-TIMEOUT millis: %ld, count: %d, tx_total: %d", millis(), count, total_tx);
    }

    // receive bytes into accumulating frame
    static uint8_t rx_buf[256];
    static auto last_rx_ms = millis();
    while (SERIAL.available()) {
        if (timeout) {
            Log.error("Timeout, but received first data now. millis: %ld", millis());
            timeout = false;
        }
        int byte = SERIAL.read();
        if (byte >= 0) {
            if (rx_len == 0) {
                // Log.info("RX FIRST BYTE");
                total_rx++;
            }
            if (rx_len < sizeof(rx_buf)) {
                rx_buf[rx_len++] = (uint8_t)byte;
            } else {
                Log.error("BUFFER-OVERRUN");
                buffer_overrun++;
                hex_dump(LOG_LEVEL_ERROR, rx_buf, rx_len, "RX: ");
                rx_len = 0;
            }
        }
        last_rx_ms = millis();
    }
    // terminate frame after timeout from last received byte
    // validate expected length and crc
    auto time_since_last_rx = millis() - last_rx_ms;
    if (rx_len && time_since_last_rx > SERIAL_RX_TIMEOUT_MS) {
        bool error = false;
        if (rx_len != RX_EXPECT_LEN) {
            Log.info("RX-LEN-ERROR=%u (%lu ms)", rx_len, time_since_last_rx);
            error = true;
            rx_len_bad++;
            SERIAL.peek();
        } else {
            if (memcmp(testBuff, rx_buf, rx_len)) {
                crc_error++;
                error = true;
            }
        }
        if (error) {
            Log.info("total=%d, rx_total=%d, timeout=%d, overrun=%d, rx_len=%d, crc=%d available=%d", total_tx, total_rx, rx_timeout, buffer_overrun, rx_len_bad, crc_error, SERIAL.available());
            hex_dump(LOG_LEVEL_INFO, rx_buf, rx_len, "RX: ");
            if (rx_len_bad || crc_error) {
                hal_usart_test(SERIAL.interface(), nullptr);
                while (true) {
                    delay(1000);
                    hal_usart_test(SERIAL.interface(), nullptr);
                }
                SPARK_ASSERT(false);
            }
        }

        rx_len = 0;
        // schedule the next request after delay
        next_request_tx = millis() + SERIAL_TX_INTERVAL_MS;
    }

    // actions once a second
    // print out stats
    // feed watchdog
    auto now_sec = System.uptime();
    static auto last_sec = now_sec;
    if ((now_sec - last_sec) >= 3) {
        Log.info("total=%d, timeout=%d, overrun=%d, rx_len=%d, crc=%d, total_rx=%d", total_tx, rx_timeout, buffer_overrun, rx_len_bad, crc_error, total_rx);
        last_sec = now_sec;
    }
}
#else









#include "Particle.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler(115200, LOG_LEVEL_ALL);

#define SERIAL (Serial1)
#define SERIAL_BAUD (460800)

const char TX_BUF[] = "12345678";

constexpr uint8_t testBuff[43] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40,
    0x41, 0x42, 0x43
};

/* executes once at startup */
void setup() {
    SERIAL.begin(SERIAL_BAUD);
}

int rxCount = 0;
int lastCount = 0;

uint16_t availableForWrite = 0;

/* executes continuously after setup() runs */
void loop() {
    static uint8_t rxBuf[16];
    static uint8_t rxLen = 0;
    
    if (SERIAL.available()) {
        Log.info("avai: %d", SERIAL.available());
        while (SERIAL.available()) {
            uint8_t temp = SERIAL.read();
            if (rxLen < strlen(TX_BUF)) {
                rxBuf[rxLen++] = temp;
            }
        }
        Log.info("len: %d", rxLen);
    }
    if (rxLen == strlen(TX_BUF)) {
        if (!memcmp(rxBuf, TX_BUF, rxLen)) {
            availableForWrite = SERIAL.availableForWrite();
            int written = 0;
            SINGLE_THREADED_BLOCK() {
                written = SERIAL.write(testBuff, sizeof(testBuff));
                SERIAL.flush();
            }
            rxCount++;
            Log.info("request: %d, written=%d testbuf=%d", rxCount, written, sizeof(testBuff));
        } else {
           Log.info("mismatch "); 
           Log.dump(LOG_LEVEL_ERROR, rxBuf, rxLen);
	       Log.print(LOG_LEVEL_ERROR, "\n\r");
        }
        memset(rxBuf, 0x00, sizeof(rxBuf));
        rxLen = 0;
        Log.info("rxLen: %d", rxLen);
    }

    auto now_sec = System.uptime();
    static auto last_sec = now_sec;
    if ((now_sec - last_sec) >= 10) {
        last_sec = now_sec;
        if (rxCount != lastCount) {
            lastCount = rxCount;
        } else {
            Log.info("rxCount=%d, availableForWrite: %d", rxCount, availableForWrite);
            // while (1);
            rxLen = 0;
            memset(rxBuf, 0, sizeof(rxBuf));
            if (rxCount != 0) {
                Log.info("Sending dummy bytes");
                for (int i = 0; i < 16; i++) {
                    SERIAL.write(0x00);
                    delay(1000);
                }
                SERIAL.flush();
                while(1);
            }
        }
    }
}
#endif
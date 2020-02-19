#include "Particle.h"
#include "pcal6416a.h"
#include "am18x5.h"
#include "mcp25625.h"
#include "bmi160.h"

#define TEST_CELLULAR                   0
#define TEST_GPS                        1
#define TEST_IO_EXP_INT                 0
#define TEST_RTC                        0
#define TEST_CAN_TRANSCEIVER            0
#define TEST_FUEL_GAUGE                 0
#define TEST_PMIC                       0
#define TEST_SENSOR                     0
#define TEST_ESP32                      1

#if PLATFORM_ID == PLATFORM_B5SOM
#define SPIIF                           SPI
#elif PLATFORM_ID == PLATFORM_TRACKER
#define SPIIF                           SPI1
#endif

#if PLATFORM_ID == PLATFORM_B5SOM
// GPS
IoExpanderPinObj gpsResetPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN5);
IoExpanderPinObj gpsPwrEnPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN4);
IoExpanderPinObj gpsCsPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN3);

// ESP32
IoExpanderPinObj esp32BootPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN0);
IoExpanderPinObj esp32CsPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN1);
IoExpanderPinObj esp32EnPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN2);
IoExpanderPinObj esp32WakeupPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN7);

// SENSOR
IoExpanderPinObj accelEnPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN5);

// CAN_TRANSCEIVER
IoExpanderPinObj canCsPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN1);
IoExpanderPinObj canPwrEnPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN0);

#elif PLATFORM_ID == PLATFORM_TRACKER
// GPS
IoExpanderPinObj gpsResetPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN7);
IoExpanderPinObj gpsPwrEnPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN6);
IoExpanderPinObj gpsCsPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN2);

// ESP32
IoExpanderPinObj esp32BootPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN4);
IoExpanderPinObj esp32CsPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN5);
IoExpanderPinObj esp32EnPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN7);
IoExpanderPinObj esp32WakeupPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN6);

// SENSOR
IoExpanderPinObj accelEnPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN5);

// CAN_TRANSCEIVER
IoExpanderPinObj canCsPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN1);
IoExpanderPinObj canPwrEnPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN0);
#endif


#if TEST_FUEL_GAUGE
FuelGauge gauge(Wire, true);
#endif

#if TEST_PMIC
PMIC pmic(true);
#endif // TEST_PMIC

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

SYSTEM_MODE(MANUAL);

SerialLogHandler log(LOG_LEVEL_ALL);


static int deselectAllCsPins() {
    CHECK(canCsPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(canCsPin.write(IoExpanderPinValue::HIGH));
    CHECK(esp32CsPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(esp32CsPin.write(IoExpanderPinValue::HIGH));
    CHECK(gpsCsPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(gpsCsPin.write(IoExpanderPinValue::HIGH));
    return 0;
}

#if TEST_CELLULAR
static int lines;
static int handler(int type, const char* buf, int len, int* lines) {
    Log.info("type: %d, data: %s", type, buf);
    return 0;
}
#endif // TEST_CELLULAR

#if TEST_GPS
static int gpsConfigurePins() {
    CHECK(gpsPwrEnPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(gpsPwrEnPin.write(IoExpanderPinValue::HIGH));
    CHECK(gpsResetPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(gpsResetPin.write(IoExpanderPinValue::LOW));
    delay(500);
    return gpsResetPin.write(IoExpanderPinValue::HIGH);
}

static int gpsCsSelect(bool select) {
    if (select) {
        return gpsCsPin.write(IoExpanderPinValue::LOW);
    } else {
        return gpsCsPin.write(IoExpanderPinValue::HIGH);
    }
}
#endif // TEST_GPS

#if TEST_IO_EXP_INT
static void onP07IntHandler(void* context) {
    Log.println("Entered onP07IntHandler().");
}

static void onP17IntHandler(void* context) {
    Log.println("Entered onP17IntHandler().");
}

static int configureIntPin() {
    CHECK(intPin.mode(IoExpanderPinMode::INPUT_PULLUP));
    CHECK(intPin.attachInterrupt(IoExpanderIntTrigger::FALLING, onP07IntHandler, nullptr));
    CHECK(intPin1.mode(IoExpanderPinMode::INPUT_PULLDOWN));
    return intPin1.attachInterrupt(IoExpanderIntTrigger::RISING, onP17IntHandler, nullptr);
}

static int configureTrigPin() {
    CHECK(trigPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(trigPin.write(IoExpanderPinValue::HIGH));
    CHECK(trigPin1.mode(IoExpanderPinMode::OUTPUT));
    return trigPin1.write(IoExpanderPinValue::LOW);
}

static int setTriggerPin(bool val) {
    if (val) {
        return trigPin.write(IoExpanderPinValue::HIGH);
    } else {
        return trigPin.write(IoExpanderPinValue::LOW);
    }
}

static int setTriggerPin1(bool val) {
    if (val) {
        return trigPin1.write(IoExpanderPinValue::HIGH);
    } else {
        return trigPin1.write(IoExpanderPinValue::LOW);
    }
}
#endif // TEST_IO_EXP_INT

#if TEST_ESP32
static void esp32WakeupPinHandler(void* context) {
    Log.info("Wakeup by ESP32!");
}
#endif // TEST_ESP32

void setup() {
    // Serial.begin(115200);
    while(!Serial.isConnected());
    Log.info("Application started.");

    // I/O Expander initialization
#if PLATFORM_ID == PLATFORM_B5SOM
    if (PCAL6416A.begin(PCAL6416A_I2C_ADDRESS, PCAL6416A_RESET_PIN, PCAL6416A_INT_PIN) != SYSTEM_ERROR_NONE) {
#elif PLATFORM_ID == PLATFORM_TRACKER
    if (PCAL6416A.begin(PCAL6416A_I2C_ADDRESS, PCAL6416A_RESET_PIN, PCAL6416A_INT_PIN, &Wire1) != SYSTEM_ERROR_NONE) {
#endif
        LOG(ERROR, "PCAL6416A.begin() failed.");
    }

    deselectAllCsPins();
    SPIIF.setDataMode(SPI_MODE0);
    SPIIF.setClockSpeed(2 * 1000 * 1000);
    SPIIF.begin();

#if TEST_RTC
    AM18X5.begin(AM18X5_I2C_ADDRESS);
    uint16_t partNumber;
    if (AM18X5.getPartNumber(&partNumber) != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "AM18X5.getPartNumber() failed.");
    } else {
        LOG(INFO, "AM18X5 part number: 0x%04X", partNumber);
    }
#endif // TEST_RTC

#if TEST_CAN_TRANSCEIVER
    if (MCP25625.begin() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "MCP25625.begin()  failed.");
    }
#endif // TEST_CAN_TRANSCEIVER

#if TEST_IO_EXP_INT
    if (configureTrigPin() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "configureTrigPin() failed.");
    }
    if (configureIntPin() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "configureIntPin() failed.");
    }
#endif // TEST_IO_EXP_INT

#if TEST_GPS
    // Test ublox GPS
    if (gpsConfigurePins() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "configureGnssPins() failed.");
    }
#endif // TEST_GPS

#if TEST_CELLULAR
    // Test EG91-NA GPS
    Cellular.on();
    // Must add delay here
    delay(5000);
    Cellular.command(handler, &lines, 5000, "AT+QGPS=1");
    // Must add delay here
    delay(5000);
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"outport\",\"uartdebug\"");
#endif // TEST_CELLULAR

#if TEST_CAN_TRANSCEIVER
    canPwrEnPin.mode(IoExpanderPinMode::OUTPUT);
    canPwrEnPin.write(IoExpanderPinValue::HIGH);
#endif // TEST_CAN_TRANSCEIVER

#if TEST_FUEL_GAUGE
    FuelGauge gauge(Wire, true);
    gauge.begin();
    LOG(INFO, "Battery voltage: %f", gauge.getVCell());
#endif

#if TEST_PMIC
    pmic.begin();
    LOG(INFO, "PMIC version: 0x%02x", pmic.getVersion());
#endif

#if TEST_SENSOR
    accelEnPin.mode(IoExpanderPinMode::OUTPUT);
    accelEnPin.write(IoExpanderPinValue::HIGH);
    BMI160.begin(BMI160_I2C_ADDRESS);
    uint8_t chipId;
    if (BMI160.getChipId(&chipId) != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "BMI160.getChipId() failed.");
    } else {
        LOG(INFO, "BMI160 chip ID: 0x%02x", chipId);
    }
#endif

#if TEST_ESP32
    // Configure ESP32 pin
    esp32CsPin.mode(IoExpanderPinMode::OUTPUT);
    esp32CsPin.write(IoExpanderPinValue::HIGH);
    esp32BootPin.mode(IoExpanderPinMode::OUTPUT);
    esp32BootPin.write(IoExpanderPinValue::HIGH);
    esp32EnPin.mode(IoExpanderPinMode::OUTPUT);
    // Reset ESP32
    esp32EnPin.write(IoExpanderPinValue::LOW);
    delay(100);
    esp32EnPin.write(IoExpanderPinValue::HIGH);

    esp32WakeupPin.mode(IoExpanderPinMode::INPUT_PULLUP);
    esp32WakeupPin.attachInterrupt(IoExpanderIntTrigger::FALLING, esp32WakeupPinHandler, nullptr);
    esp32WakeupPin.inputLatch(true);
#endif // TEST_ESP32
}

void loop() {
#if TEST_GPS
    if (gpsCsSelect(true) == SYSTEM_ERROR_NONE) {
        uint8_t in_byte;
        do {
            in_byte = SPIIF.transfer(0xFF);
            if (in_byte != 0xFF) {
                Log.printf("%c", in_byte);
            }
        } while (in_byte != 0xFF);

        if (gpsCsSelect(false) != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "gpsCsSelect( false ) failed.");
        }
        LOG(INFO, "Read GPS done.");
    } else {
        LOG(ERROR, "gpsCsSelect( true ) failed.");
    }
    delay(2000);
#endif // TEST_GPS

#if TEST_IO_EXP_INT
    delay(3000);
    setTriggerPin(0);
    setTriggerPin(1);
    delay(3000);
    setTriggerPin1(0);
    setTriggerPin1(1);
#endif

#if TEST_CAN_TRANSCEIVER
    uint8_t val;
    if (MCP25625.getCanCtrl(&val) != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "MCP25625.getCanCtrl() failed.");
    } else {
        LOG(INFO, "MCP25625 CANCTRL: 0x%02X", val);
    }
    delay(3000);
#endif

#if TEST_FUEL_GAUGE
    LOG(INFO, "Baterry level: %f", gauge.getNormalizedSoC());
    delay(3000);
#endif

#if TEST_PMIC
    LOG(INFO, "PMIC version: 0x%02x", pmic.getVersion());
    delay(1000);
#endif

#if TEST_ESP32
    const int len = strlen("This is the receiver, sending data for transmission number 0000");
    static uint8_t rxBuffer[64];
    static uint8_t txBuffer[64];
    esp32CsPin.write(IoExpanderPinValue::LOW);
    memset(rxBuffer, 0, sizeof(rxBuffer));
    memset(txBuffer, 0, sizeof(txBuffer));
    sprintf((char*)txBuffer, "Hello, I'm B5SoM!");
    SPIIF.transfer(txBuffer, rxBuffer, len, nullptr);
    Log.printf("rx data: %s\r\n", rxBuffer);
    esp32CsPin.write(IoExpanderPinValue::HIGH);
    delay(2000);
#endif // TEST_ESP32
}

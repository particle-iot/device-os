#include "Particle.h"
#include "io_expander.h"

#define TEST_GPS                        0
#define TEST_IO_EXP_INT                 1

#if TEST_GPS
#define GPS_RESET_PIN_PORT              (IoExpanderPort::PORT0)
#define GPS_RESET_PIN                   (IoExpanderPin::PIN5)
#define GPS_PW_EN_PIN_PORT              (IoExpanderPort::PORT0)
#define GPS_PW_EN_PIN                   (IoExpanderPin::PIN4)
#define GPS_CS_PIN_PORT                 (IoExpanderPort::PORT0)
#define GPS_CS_PIN                      (IoExpanderPin::PIN3)
#endif // TEST_GPS

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

SYSTEM_MODE(MANUAL);

SerialLogHandler log(LOG_LEVEL_ALL);


#if TEST_GPS
static int deselectAllCsPins() {
    IoExpanderPinConfig config = {};
    config.dir = IoExpanderPinDir::OUTPUT;

    config.port = IoExpanderPort::PORT0;
    config.pin = IoExpanderPin::PIN1; // CAN bus /CS pin
    CHECK(IOExpander.configure(config));
    CHECK(IOExpander.write(IoExpanderPort::PORT0, IoExpanderPin::PIN1, IoExpanderPinValue::HIGH));

    config.port = IoExpanderPort::PORT1;
    config.pin = IoExpanderPin::PIN1; // Wi-Fi /CS pin
    CHECK(IOExpander.configure(config));
    CHECK(IOExpander.write(IoExpanderPort::PORT1, IoExpanderPin::PIN1, IoExpanderPinValue::HIGH));

    config.port = IoExpanderPort::PORT1;
    config.pin = IoExpanderPin::PIN4; // MEMS /CS pin
    CHECK(IOExpander.configure(config));
    CHECK(IOExpander.write(IoExpanderPort::PORT1, IoExpanderPin::PIN4, IoExpanderPinValue::HIGH));

    return SYSTEM_ERROR_NONE;
}

static int gpsConfigurePins() {
    IoExpanderPinConfig config = {};
    config.dir = IoExpanderPinDir::OUTPUT;

    config.port = GPS_CS_PIN_PORT;
    config.pin = GPS_CS_PIN; // /CS pin
    CHECK(IOExpander.configure(config));
    CHECK(IOExpander.write(GPS_CS_PIN_PORT, GPS_CS_PIN, IoExpanderPinValue::HIGH));

    config.port = GPS_PW_EN_PIN_PORT;
    config.pin = GPS_PW_EN_PIN; // PW_EN pin
    CHECK(IOExpander.configure(config));
    CHECK(IOExpander.write(GPS_PW_EN_PIN_PORT, GPS_PW_EN_PIN, IoExpanderPinValue::HIGH));

    config.port = GPS_RESET_PIN_PORT;
    config.pin = GPS_RESET_PIN; // /RESET pin
    CHECK(IOExpander.configure(config));
    CHECK(IOExpander.write(GPS_RESET_PIN_PORT, GPS_RESET_PIN, IoExpanderPinValue::LOW));
    delay(500);
    CHECK(IOExpander.write(GPS_RESET_PIN_PORT, GPS_RESET_PIN, IoExpanderPinValue::HIGH));

    return SYSTEM_ERROR_NONE;
}

static int gpsCsSelect(bool select) {
    if (select) {
        CHECK(IOExpander.write(GPS_CS_PIN_PORT, GPS_CS_PIN, IoExpanderPinValue::LOW));
    } else {
        CHECK(IOExpander.write(GPS_CS_PIN_PORT, GPS_CS_PIN, IoExpanderPinValue::HIGH));
    }
    return SYSTEM_ERROR_NONE;
}
#endif // TEST_GPS

#if TEST_IO_EXP_INT
static void onP07IntHandler() {
    Serial.println("Entered onP07IntHandler().");
}

static int configureIntPin() {
    IoExpanderPinConfig config = {};
    config.port = IoExpanderPort::PORT0;
    config.pin = IoExpanderPin::PIN7;
    config.dir = IoExpanderPinDir::INPUT;
    config.pull = IoExpanderPinPull::PULL_UP;
    config.inputLatch = true;
    config.intEn = true;
    config.trig = IoExpanderIntTrigger::FALLING;
    config.callback = onP07IntHandler;
    return IOExpander.configure(config);
}

static int configureTrigPin() {
    IoExpanderPinConfig config = {};
    config.dir = IoExpanderPinDir::OUTPUT;
    config.port = IoExpanderPort::PORT0;
    config.pin = IoExpanderPin::PIN6;
    CHECK(IOExpander.configure(config));
    return IOExpander.write(IoExpanderPort::PORT0, IoExpanderPin::PIN6, IoExpanderPinValue::HIGH);
}

static int setTriggerPin(bool val) {
    if (val) {
        CHECK(IOExpander.write(IoExpanderPort::PORT0, IoExpanderPin::PIN6, IoExpanderPinValue::HIGH));
    } else {
        CHECK(IOExpander.write(IoExpanderPort::PORT0, IoExpanderPin::PIN6, IoExpanderPinValue::LOW));
    }
    return SYSTEM_ERROR_NONE;
}
#endif // TEST_IO_EXP_INT

void setup() {
    // Serial.begin(115200);
    while(!Serial.isConnected());
    Serial.println("Application started.");

    // I/O Expander initialization
    if (IOExpander.init(IO_EXPANDER_I2C_ADDRESS, IO_EXPANDER_RESET_PIN, IO_EXPANDER_INT_PIN) != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "IOExpander.init() failed.");
    }
    if (IOExpander.reset() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "IOExpander.reset() failed.");
    }

#if TEST_IO_EXP_INT
    if (configureTrigPin() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "configureTrigPin() failed.");
    }
    if (configureIntPin() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "configureIntPin() failed.");
    }
#endif // TEST_IO_EXP_INT

#if TEST_GPS
    deselectAllCsPins();

    if (gpsConfigurePins() != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "configureGnssPins() failed.");
    }
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockSpeed(5 * 1000 * 1000);
    SPI.begin();
#endif // TEST_GPS
}

void loop() {
#if TEST_GPS
    if (gpsCsSelect(true) == SYSTEM_ERROR_NONE) {
        uint8_t in_byte;
        do {
            in_byte = SPI.transfer(0xFF);
            if (in_byte != 0xFF) {
                Serial.write(in_byte);
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
#endif
}

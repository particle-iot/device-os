#include "Particle.h"
#include "pcal6416a.h"

#define TEST_GPS                        1
#define TEST_IO_EXP_INT                 0

#if TEST_GPS
IoExpanderPinObj gpsResetPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN5);
IoExpanderPinObj gpsPwrEnPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN4);
IoExpanderPinObj gpsCsPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN3);

IoExpanderPinObj canCsPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN1);
IoExpanderPinObj wifiCsPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN1);
IoExpanderPinObj accelCsPin(PCAL6416A, IoExpanderPort::PORT1, IoExpanderPin::PIN4);
#endif // TEST_GPS

#if TEST_IO_EXP_INT
IoExpanderPinObj intPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN7);
IoExpanderPinObj trigPin(PCAL6416A, IoExpanderPort::PORT0, IoExpanderPin::PIN6);
#endif

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

SYSTEM_MODE(MANUAL);

SerialLogHandler log(LOG_LEVEL_ALL);


#if TEST_GPS
static int deselectAllCsPins() {
    CHECK(canCsPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(canCsPin.write(IoExpanderPinValue::HIGH));
    CHECK(wifiCsPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(wifiCsPin.write(IoExpanderPinValue::HIGH));
    CHECK(accelCsPin.mode(IoExpanderPinMode::OUTPUT));
    return accelCsPin.write(IoExpanderPinValue::HIGH);
}

static int gpsConfigurePins() {
    CHECK(gpsCsPin.mode(IoExpanderPinMode::OUTPUT));
    CHECK(gpsCsPin.write(IoExpanderPinValue::HIGH));
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
static void onP07IntHandler() {
    Serial.println("Entered onP07IntHandler().");
}

static int configureIntPin() {
    CHECK(intPin.mode(IoExpanderPinMode::INPUT_PULLUP));
    return intPin.attachInterrupt(IoExpanderIntTrigger::FALLING, onP07IntHandler);
}

static int configureTrigPin() {
    CHECK(trigPin.mode(IoExpanderPinMode::OUTPUT));
    return trigPin.write(IoExpanderPinValue::HIGH);
}

static int setTriggerPin(bool val) {
    if (val) {
        return trigPin.write(IoExpanderPinValue::HIGH);
    } else {
        return trigPin.write(IoExpanderPinValue::LOW);
    }
}
#endif // TEST_IO_EXP_INT

void setup() {
    // Serial.begin(115200);
    while(!Serial.isConnected());
    Serial.println("Application started.");

    // I/O Expander initialization
    if (PCAL6416A.begin(IO_EXPANDER_I2C_ADDRESS, IO_EXPANDER_RESET_PIN, IO_EXPANDER_INT_PIN) != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "PCAL6416A.begin() failed.");
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

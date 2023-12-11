
#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"
#include "check.h"
#include <utility>

namespace {

#if HAL_PLATFORM_FUELGAUGE_MAX17043
std::pair<hal_pin_t, hal_pin_t> i2cToSdaSclPins(hal_i2c_interface_t i2c) {
#if HAL_PLATFORM_GEN == 3
    switch (i2c) {
        case HAL_I2C_INTERFACE1: {
            return {SDA, SCL};
        }
        case HAL_I2C_INTERFACE2: {
#if PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_ESOMX
            return {PMIC_SDA, PMIC_SCL};
#else
            return {D2, D3};
#endif //PLATFORM_ID == PLATFORM_BORON && PLATFORM_TRACKER
        }
        case HAL_I2C_INTERFACE3: {
            return {D9, D8};
        }
    }
#else
    #error "Unsupported platform"
#endif // HAL_PLATFORM_GEN == 3
    return {PIN_INVALID, PIN_INVALID};
}
#endif // HAL_PLATFORM_FUELGAUGE_MAX17043

class SoftWire {
public:
    SoftWire(hal_i2c_interface_t i2c, hal_pin_t sda, hal_pin_t scl)
            : i2c_(i2c),
              sda_(sda),
              scl_(scl) {
        hal_i2c_lock(i2c_, nullptr);
    }

    ~SoftWire() {
        hal_i2c_unlock(i2c_, nullptr);
    }

    void init() {
        hal_i2c_end(i2c_, nullptr);
        hal_gpio_config_t conf = {
            .size = sizeof(conf),
            .version = HAL_GPIO_VERSION,
            .mode = OUTPUT_OPEN_DRAIN_PULLUP,
            .set_value = true,
            .value = 1,
            .drive_strength = HAL_GPIO_DRIVE_DEFAULT
        };
        hal_gpio_configure(sda_, &conf, nullptr);
        hal_gpio_configure(scl_, &conf, nullptr);
    }

    int transmit(const uint8_t* data, const WireTransmission& transmission, bool abort = false, size_t abortBitAfterAddress = 0, bool abortAck = false) {
        auto conf = transmission.halConfig();
        Log.info("transmit to %02x %lu bytes (abort %d %d)", conf.address, conf.quantity, abort, abortBitAfterAddress);

        startCondition();
        clockByteOut(conf.address << 1);
        CHECK(readAck());

        size_t i;
        const auto bitMargin = 8 + abortAck;
        for (i = 0; i < conf.quantity && (!abort || i < (abortBitAfterAddress + (bitMargin - 1)) / bitMargin); i++) {
            if (!abort || ((i + 1) * (bitMargin)) <= abortBitAfterAddress) {
                clockByteOut(data[i]);
                CHECK(readAck());
            } else {
                clockByteOut(data[i], abortBitAfterAddress % (bitMargin));
            }
        }

        if (!abort && !(conf.flags & HAL_I2C_TRANSMISSION_FLAG_STOP)) {
            stopCondition();
        }

        return i;
    }

    int receive(uint8_t* data, const WireTransmission& transmission, bool abort = false, size_t abortBitAfterAddress = 0, bool abortAck = false) {
        auto conf = transmission.halConfig();
        Log.info("receive from %02x %lu bytes (abort %d %d)", conf.address, conf.quantity, abort, abortBitAfterAddress);

        startCondition();
        clockByteOut(conf.address << 1 | 0x01);
        CHECK(readAck());

        size_t i;
        for (i = 0; i < conf.quantity && (!abort || i < (abortBitAfterAddress + 7) / 8); i++) {
            if (!abort || ((i + 1) * 8) <= abortBitAfterAddress) {
                data[i] = clockByteIn();
                if (i != conf.quantity - 1) {
                    ackOut(true);
                } else {
                    ackOut(false);
                }
            } else {
                data[i] = clockByteIn(abortBitAfterAddress % 8);
            }
        }
        if (!abort) {
            stopCondition();
        }

        return i;
    }

    int readRegister(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len) {
        CHECK(transmit(&reg, WireTransmission(addr).quantity(1).stop(false)));
        return receive(buf, WireTransmission(addr).quantity(len));
    }

    int writeRegister(uint8_t addr, uint8_t reg, const uint8_t* buf, size_t len) {
        auto tmpBuf = std::make_unique<uint8_t[]>(len + 1);
        CHECK_TRUE(tmpBuf, SYSTEM_ERROR_NO_MEMORY);
        tmpBuf[0] = reg;
        memcpy(&tmpBuf[1], buf, len);
        return transmit(tmpBuf.get(), WireTransmission(addr).quantity(len));
    }

private:
    void startCondition() {
        Log.info("start");
        hal_gpio_write(sda_, 0);
        HAL_Delay_Microseconds(50);
        hal_gpio_write(scl_, 0);
        HAL_Delay_Microseconds(50);
    }

    void stopCondition() {
        Log.info("stop");
        hal_gpio_write(sda_, 0);
        HAL_Delay_Microseconds(50);
        hal_gpio_write(scl_, 1);
        HAL_Delay_Microseconds(50);
        hal_gpio_write(sda_, 1);
        HAL_Delay_Microseconds(50);
    }

    void clockByteOut(uint8_t data, size_t bits = 8) {
        Log.info("clock out %d bits (%02x)", bits, data);
        uint8_t res = 0x00;
        for (size_t i = 0; i < bits; i++) {
            hal_gpio_write(sda_, data & 0x80);
            res |= hal_gpio_read(sda_) << (8 - i - 1);
            hal_gpio_write(scl_, 1);
            HAL_Delay_Microseconds(50);
            hal_gpio_write(scl_, 0);
            HAL_Delay_Microseconds(50);
            data <<= 1;
        }
        Log.info("clocked out (%02x)", res);
    }

    uint8_t clockByteIn(size_t bits = 8) {
        Log.info("clocking in %d bits", bits);
        uint8_t data = 0x00;
        while (bits-- > 0) {
            hal_gpio_write(sda_, 1);
            hal_gpio_write(scl_, 1);
            HAL_Delay_Microseconds(50);
            data |= (hal_gpio_read(sda_) & 0x01) << bits;
            hal_gpio_write(scl_, 0);
            HAL_Delay_Microseconds(50);
        }
        Log.info("clocked in (%02x)", data);
        return data;
    }

    int readAck() {
        hal_gpio_write(sda_, 1);
        hal_gpio_write(scl_, 1);
        HAL_Delay_Microseconds(50);
        bool ack = !hal_gpio_read(sda_);
        hal_gpio_write(scl_, 0);
        HAL_Delay_Microseconds(50);
        Log.info("read ack %d", ack);
        hal_gpio_write(sda_, 0);
        if (!ack) {
            return SYSTEM_ERROR_PROTOCOL;
        }
        return 0;
    }

    int ackOut(bool ack) {
        Log.info("out ack %d", ack);
        hal_gpio_write(sda_, !ack);
        hal_gpio_write(scl_, 1);
        HAL_Delay_Microseconds(50);
        hal_gpio_write(scl_, 0);
        HAL_Delay_Microseconds(50);
        return 0;
    }

private:
    hal_i2c_interface_t i2c_;
    hal_pin_t sda_;
    hal_pin_t scl_;
};

}

#if PLATFORM_ID == PLATFORM_TRACKER

test(I2C_01_Wire3_Cannot_Be_Enabled_While_Wire_Is_Enabled) {
    Wire.begin();
    assertTrue(Wire.isEnabled());
    // Wire3 cannot be enabled while Wire is enabled.
    Wire3.begin();
    assertFalse(Wire3.isEnabled());

    Wire.end();
    assertFalse(Wire.isEnabled());
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    Wire3.end();
    assertFalse(Wire3.isEnabled());
}

test(I2C_02_Wire_Cannot_Be_Enabled_While_Wire3_Is_Enabled) {
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    // Wire cannot be enabled while Wire3 is enabled.
    Wire.begin();
    assertFalse(Wire.isEnabled());

    Wire3.end();
    assertFalse(Wire3.isEnabled());
    Wire.begin();
    assertTrue(Wire.isEnabled());
    Wire.end();
    assertFalse(Wire.isEnabled());
}

test(I2C_03_Wire3_Cannot_Be_Enabled_While_Serial1_Is_Enabled) {
    Serial1.begin(115200);
    assertTrue(Serial1.isEnabled());
    // Wire3 cannot be enabled while Serial1 is enabled.
    Wire3.begin();
    assertFalse(Wire3.isEnabled());

    Serial1.end();
    assertFalse(Serial1.isEnabled());
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    Wire3.end();
    assertFalse(Wire3.isEnabled());
}

test(I2C_04_Serial1_Cannot_Be_Enabled_While_Wire3_Is_Enabled) {
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    // Serial1 cannot be enabled while Wire3 is enabled.
    Serial1.begin(115200);
    assertFalse(Serial1.isEnabled());

    Wire3.end();
    assertFalse(Wire3.isEnabled());
    Serial1.begin(115200);
    assertTrue(Serial1.isEnabled());
    Serial1.end();
    assertFalse(Serial1.isEnabled());
}

#endif // PLATFORM_ID == PLATFORM_TRACKER

test(I2C_05_Hal_Sleep_API_Test) {
    Wire.lock();
    bool enabled = Wire.isEnabled();
    SCOPE_GUARD({
        hal_i2c_sleep(HAL_I2C_INTERFACE1, false, nullptr);
        if (enabled) {
            Wire.begin();
        } else {
            Wire.end();
        }
        Wire.unlock();
    });

    // Suspend and resotre I2C
    Wire.begin();
    assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, nullptr), (int)SYSTEM_ERROR_NONE);  // Suspend
    assertFalse(Wire.isEnabled());
    assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, nullptr), (int)SYSTEM_ERROR_NONE); // Restore
    assertTrue(Wire.isEnabled());
}

#if HAL_PLATFORM_FUELGAUGE_MAX17043
test(I2C_06_I2c_Sleep_FuelGauge) {
    FuelGauge fuel(true);
    fuel.begin();
    fuel.wakeup();
    auto ver = fuel.getVersion();
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (ver < 0) {
        skip();
        return;
    }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    assertMoreOrEqual(ver, 0);
    assertNotEqual(ver, 0x0000);
    assertNotEqual(ver, 0xffff);
    assertEqual(hal_i2c_sleep(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, true, nullptr), 0);
    SCOPE_GUARD({
        if (!hal_i2c_is_enabled(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, nullptr)) {
            hal_i2c_sleep(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, false, nullptr);
        }
    });
    assertFalse(hal_i2c_is_enabled(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, nullptr));
    assertEqual(hal_i2c_sleep(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, false, nullptr), 0);
    assertTrue(hal_i2c_is_enabled(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, nullptr));
    auto ver2 = fuel.getVersion();
    assertEqual(ver, ver2);
}

test(I2C_07_bus_reset_is_not_destructive) {
    constexpr uint16_t MAX17043_DEFAULT_CONFIG = 0x971c;

    FuelGauge fuel;
    fuel.begin();
    fuel.wakeup();
    auto ver = fuel.getVersion();
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (ver < 0) {
        skip();
        return;
    }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    assertMoreOrEqual(ver, 0);
    assertNotEqual(ver, 0x0000);
    assertNotEqual(ver, 0xffff);

    SCOPE_GUARD({
        fuel.begin();
        hal_i2c_reset(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, 0, nullptr);
        fuel.reset();
    });

    auto pins = i2cToSdaSclPins(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C);
    SoftWire wire(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, pins.first, pins.second);

    hal_i2c_reset(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, 0, nullptr);
    fuel.reset();

    // Validate that fuel gauge has default configuration
    uint8_t buf[2] = {0xff, 0xff};
    assertEqual(0, fuel.readConfigRegister(buf[0], buf[1]));
    uint16_t config = buf[0] << 8 | buf[1];
    assertEqual(config, MAX17043_DEFAULT_CONFIG);

    // Validate that we can read exactly the same configuration with soft i2c implementation
    wire.init();
    memset(buf, 0xff, sizeof(buf));
    assertMoreOrEqual(wire.readRegister(MAX17043_ADDRESS, CONFIG_REGISTER, buf, sizeof(buf)), 0);
    config = buf[0] << 8 | buf[1];
    assertEqual(config, MAX17043_DEFAULT_CONFIG);

    const uint8_t txBuf[4] = {CONFIG_REGISTER, 0x55, 0xaa, 0x00};
    for (int i = 0; i <= 3 * 9 - 2 /* ignore full write without ack bit and full write with ack bit */; i++) {
        wire.init();
        assertMoreOrEqual(wire.transmit(txBuf, WireTransmission(MAX17043_ADDRESS).quantity(3), true /* abort */, i, true), 0);
        fuel.begin();
        hal_i2c_reset(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, 0, nullptr);
        memset(buf, 0xff, sizeof(buf));
        assertEqual(0, fuel.readConfigRegister(buf[0], buf[1]));
        config = buf[0] << 8 | buf[1];
        assertEqual(config, MAX17043_DEFAULT_CONFIG);
    }
}

#endif // HAL_PLATFORM_FUELGAUGE_MAX17043


#include "application.h"
#include "unit-test/unit-test.h"

SerialLogHandler g_logSerial(LOG_LEVEL_ALL);

static void init_i2c(){
    Wire1.begin();
}

// TODO: Test aquireWireBuffer behavior, on systems with and without i2c peripherals used by the system

#ifdef HAL_PLATFORM_EXTERNAL_RTC

// Test > 32 byte reads by reading from AM18x5 RTC peripheral
test(I2C_00_long_read)
{
    init_i2c();

    // read full register range
    const uint8_t REGISTER_RANGE = 64;
    uint8_t long_read_buffer[REGISTER_RANGE] = {};
    uint8_t read_length = REGISTER_RANGE; 

    // Check read values against ID registers
    const uint8_t ID0_REGISTER = 0x28;
    const uint8_t ID0_REGISTER_VALUE = 0x18;
    const uint8_t ID1_REGISTER = 0x29;
    const uint8_t ID1_REGISTER_VALUE = 0x05;

    hal_i2c_interface_t wire_interface = HAL_PLATFORM_EXTERNAL_RTC_I2C;
    uint8_t wire_rtc_address = HAL_PLATFORM_EXTERNAL_RTC_I2C_ADDR;
    uint8_t start_rtc_register = 0x00;

    hal_i2c_begin_transmission(wire_interface, wire_rtc_address, nullptr);
    hal_i2c_write(wire_interface, static_cast<uint8_t>(start_rtc_register), nullptr);
    hal_i2c_end_transmission(wire_interface, false, nullptr);
    assertMore(hal_i2c_request(wire_interface, wire_rtc_address, read_length, true, nullptr), 0);

    int32_t size = hal_i2c_available(wire_interface, nullptr);
    assertEqual(size, read_length);
    
    for (int32_t i = 0; i < size; i++) {
        long_read_buffer[i] = hal_i2c_read(wire_interface, nullptr);
    }

    Log.info("I2C_00_long_read result %d\n", size);
    Log.dump(long_read_buffer, read_length);
    Log.info(" \n");

    assertEqual(long_read_buffer[ID0_REGISTER], ID0_REGISTER_VALUE);
    assertEqual(long_read_buffer[ID1_REGISTER], ID1_REGISTER_VALUE);

    assertTrue(true);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC

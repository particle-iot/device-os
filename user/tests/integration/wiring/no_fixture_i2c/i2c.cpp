
#include "application.h"
#include "unit-test/unit-test.h"

//SerialLogHandler g_logSerial(LOG_LEVEL_ALL);

static const int WIRE_ACQUIRE_BUFFER_SIZE = HAL_PLATFORM_I2C_BUFFER_SIZE(HAL_I2C_INTERFACE1) + 1;
static const int WIRE1_ACQUIRE_BUFFER_SIZE = HAL_PLATFORM_I2C_BUFFER_SIZE(HAL_I2C_INTERFACE2) + 1;
static const int WIRE1_TRACKER_MINIMUM_BUFFER_SIZE = 512;

static hal_i2c_config_t allocateWireConfig(uint32_t bufferSize) {
    hal_i2c_config_t config = {
        .size = sizeof(hal_i2c_config_t),
        .version = HAL_I2C_CONFIG_VERSION_2,
        .rx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .rx_buffer_size = bufferSize,
        .tx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .tx_buffer_size = bufferSize,
        .flags = HAL_I2C_CONFIG_FLAG_FREEABLE
    };
    return config;
}

static void freeWireBuffers(hal_i2c_config_t * config){
    free(config->tx_buffer);
    free(config->rx_buffer);
}

hal_i2c_config_t acquireWireBuffer()
{
    return allocateWireConfig(WIRE_ACQUIRE_BUFFER_SIZE);
}

#if Wiring_Wire1
hal_i2c_config_t acquireWire1Buffer()
{
    return allocateWireConfig(WIRE1_ACQUIRE_BUFFER_SIZE);
}
#endif

test(I2C_00_hal_init_default_buffer)
{
    // Initializing the HAL with a null config is allowed and should allocate system defaults
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, nullptr), (int)SYSTEM_ERROR_NONE);

#if (PLATFORM_ID == PLATFORM_TRACKER) || (PLATFORM_ID == PLATFORM_TRACKERM)
    // Tracker platforms should have buffers at least 512 bytes large, allocated by the system on startup
    // IE trying to re-initialize the HAL with same size should fail
    hal_i2c_config_t wire1_small_buffers = allocateWireConfig(WIRE1_TRACKER_MINIMUM_BUFFER_SIZE);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, &wire1_small_buffers), (int)SYSTEM_ERROR_NOT_ENOUGH_DATA);
#endif 
}

test(I2C_01_test_acquire_wire_buffer) {
    // Construct TwoWire object with user acquireWireBuffer() configuration structures, which should re-initialize the I2C HAL
    Wire.begin();

    // Attempt to re-initialize the HAL with different sized buffers to implicitly determine if the user buffers were used by the HAL
    // System Defaults < User values from acquireWireBuffer < Values used here to re-initialize the HAL

    // Equal size buffers should fail
    hal_i2c_config_t wire_smaller_buffers = allocateWireConfig(WIRE_ACQUIRE_BUFFER_SIZE);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, &wire_smaller_buffers), (int)SYSTEM_ERROR_NOT_ENOUGH_DATA);
    freeWireBuffers(&wire_smaller_buffers);

    // Larger sized buffers should succeed
    hal_i2c_config_t wire_larger_buffers = allocateWireConfig(WIRE_ACQUIRE_BUFFER_SIZE + 1);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, &wire_larger_buffers), (int)SYSTEM_ERROR_NONE);
    freeWireBuffers(&wire_larger_buffers);

    // Reinitializing with a NULL config is not allowed
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, nullptr), (int)SYSTEM_ERROR_INVALID_ARGUMENT);

    #if Wiring_Wire1
    Wire1.begin();
    hal_i2c_config_t wire1_smaller_buffers = allocateWireConfig(WIRE1_ACQUIRE_BUFFER_SIZE);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, &wire1_smaller_buffers), (int)SYSTEM_ERROR_NOT_ENOUGH_DATA);
    freeWireBuffers(&wire1_smaller_buffers);

    hal_i2c_config_t wire1_larger_buffers = allocateWireConfig(WIRE1_ACQUIRE_BUFFER_SIZE + 1);
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, &wire1_larger_buffers), (int)SYSTEM_ERROR_NONE);
    freeWireBuffers(&wire1_larger_buffers);

    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE2, nullptr), (int)SYSTEM_ERROR_INVALID_ARGUMENT);
    #endif
}

#if HAL_PLATFORM_EXTERNAL_RTC

test(I2C_02_long_read)
{
    // Test > 32 byte reads by reading from AM18x5 RTC peripheral
    Wire1.begin();

    // Read full register range
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

    Wire1.beginTransmission(wire_rtc_address);
    Wire1.write(&start_rtc_register, sizeof(start_rtc_register));
    Wire1.endTransmission();
    Wire1.requestFrom(wire_rtc_address, read_length);
    int32_t size = Wire1.available();
    assertMore(Wire1.available(), 0);

    for (int32_t i = 0; i < size; i++) {
        long_read_buffer[i] = (uint8_t)Wire1.read();
    }

    // Log.info("I2C_02_long_read result %d\n", size);
    // Log.dump(long_read_buffer, read_length);
    // Log.info(" \n");

    assertEqual(long_read_buffer[ID0_REGISTER], ID0_REGISTER_VALUE);
    assertEqual(long_read_buffer[ID1_REGISTER], ID1_REGISTER_VALUE);

    assertTrue(true);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC

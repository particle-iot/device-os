
// This contains all the global instances used by Wiring
// This allows the wiring library to be used as a utility library for String, Stream, Print etc.. without
// instantiating the globals.

#include "spark_wiring_i2c.h"
#include "i2c_hal.h"
#include <new>

#ifndef SPARK_WIRING_NO_I2C

namespace {

hal_i2c_config_t defaultWireConfig() {
	hal_i2c_config_t config = {
		.size = sizeof(hal_i2c_config_t),
		.version = HAL_I2C_CONFIG_VERSION_1,
		.rx_buffer = new (std::nothrow) uint8_t[I2C_BUFFER_LENGTH],
		.rx_buffer_size = I2C_BUFFER_LENGTH,
		.tx_buffer = new (std::nothrow) uint8_t[I2C_BUFFER_LENGTH],
		.tx_buffer_size = I2C_BUFFER_LENGTH
	};

	return config;
}

} // anonymous

hal_i2c_config_t __attribute__((weak)) acquireWireBuffer()
{
	return defaultWireConfig();
}

#if Wiring_Wire1
hal_i2c_config_t __attribute__((weak)) acquireWire1Buffer()
{
	return defaultWireConfig();
}
#endif

#if Wiring_Wire3
hal_i2c_config_t __attribute__((weak)) acquireWire3Buffer()
{
	return defaultWireConfig();
}
#endif

TwoWire& __fetch_global_Wire()
{
	static TwoWire wire(HAL_I2C_INTERFACE1, acquireWireBuffer());
	return wire;
}

#if Wiring_Wire1
TwoWire& __fetch_global_Wire1()
{
	static TwoWire wire(HAL_I2C_INTERFACE2, acquireWire1Buffer());
	return wire;
}

#endif

/* System PMIC and Fuel Gauge I2C3 */
#if Wiring_Wire3
TwoWire& __fetch_global_Wire3()
{
	static TwoWire wire(HAL_I2C_INTERFACE3, acquireWire3Buffer());
	return wire;
}

#endif

#endif //SPARK_WIRING_NO_I2C

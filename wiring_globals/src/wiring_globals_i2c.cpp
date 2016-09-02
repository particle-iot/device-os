
// This contains all the global instances used by Wiring
// This allows the wiring library to be used as a utility library for String, Stream, Print etc.. without
// instantiating the globals.

#include "spark_wiring_i2c.h"
#include "i2c_hal.h"

#ifndef SPARK_WIRING_NO_I2C

TwoWire& __fetch_global_Wire()
{
	static TwoWire wire(HAL_I2C_INTERFACE1);
	return wire;
}

#if Wiring_Wire1
TwoWire& __fetch_global_Wire1()
{
	static TwoWire wire(HAL_I2C_INTERFACE2);
	return wire;
}

#endif

/* System PMIC and Fuel Gauge I2C3 */
#if Wiring_Wire3
TwoWire& __fetch_global_Wire3()
{
	static TwoWire wire(HAL_I2C_INTERFACE3);
	return wire;
}

#endif

#endif //SPARK_WIRING_NO_I2C

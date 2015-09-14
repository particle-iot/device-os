
// This contains all the global instances used by Wiring
// This allows the wiring library to be used as a utility library for String, Stream, Print etc.. without
// instantiating the globals.

#include "spark_wiring_i2c.h"
#include "i2c_hal.h"

#ifndef SPARK_WIRING_NO_I2C

TwoWire Wire(HAL_I2C_INTERFACE1);

#if Wiring_Wire1
TwoWire Wire1(HAL_I2C_INTERFACE2);
#endif

/* System PMIC and Fuel Guage I2C3 */
#if Wiring_Wire3
TwoWire Wire3(HAL_I2C_INTERFACE3);
#endif

#endif //SPARK_WIRING_NO_I2C



#include "spark_wiring_spi.h"
#include "core_hal.h"
#include "spark_macros.h"

#ifndef SPARK_WIRING_NO_SPI

SPIClass SPI(HAL_SPI_INTERFACE1);

#if Wiring_SPI1
SPIClass SPI1(HAL_SPI_INTERFACE2);
#endif

#if Wiring_SPI2
SPIClass SPI2(HAL_SPI_INTERFACE3);
#endif

#endif //SPARK_WIRING_NO_SPI


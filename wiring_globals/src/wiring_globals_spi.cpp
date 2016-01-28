

#include "spark_wiring_spi.h"
#include "core_hal.h"
#include "spark_macros.h"

#ifndef SPARK_WIRING_NO_SPI


SPIClass& __fetch_global_SPI()
{
	static SPIClass spi(HAL_SPI_INTERFACE1);
	return spi;
}

#if Wiring_SPI1
SPIClass& __fetch_global_SPI1()
{
	static SPIClass spi1(HAL_SPI_INTERFACE2);
	return spi1;
}
#endif

#if Wiring_SPI2
SPIClass& __fetch_global_SPI2()
{
	static SPIClass spi2(HAL_SPI_INTERFACE3);
	return spi2;
}
#endif

#endif //SPARK_WIRING_NO_SPI


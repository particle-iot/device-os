

#include "spark_wiring_spi.h"
#include "core_hal.h"
#include "spark_macros.h"

#ifndef SPARK_WIRING_NO_SPI

namespace particle {
namespace globals {

SPIClass& instanceSpi() {
    static SPIClass instance(HAL_SPI_INTERFACE1);
    return instance;
}

#if Wiring_SPI1
SPIClass& instanceSpi1() {
    static SPIClass instance(HAL_SPI_INTERFACE2);
    return instance;
}
#endif // Wiring_SPI1

#if Wiring_SPI2
SPIClass& instanceSpi2() {
    static SPIClass instance(HAL_SPI_INTERFACE3);
    return instance;
}

#endif // Wiring_SPI2

} } // particle::globals

#endif //SPARK_WIRING_NO_SPI


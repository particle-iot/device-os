
#include "testapi.h"

namespace api_test_namespace {
// We should be able to define variables and types named SPI, SPI1 and SPI2

struct DummyStruct {
};

DummyStruct SPI;
DummyStruct SPI1;
DummyStruct SPI2;


namespace test {

struct SPI {
};

struct SPI1 {
};

struct SPI2 {
};

} // test

namespace proxy {

void spiFuncRef(SPIClass& spi) {
    spi.begin();
}

void spiFuncCref(const SPIClass& spi) {
    (void)spi;
}

void spiFuncPtr(SPIClass* spi) {
    spi->begin();
}

void spiFuncCptr(const SPIClass* spi) {
    (void)spi;
}

} // proxy

} // api_test_namespace

test(spi_proxy_compatibility) {
    using namespace api_test_namespace::proxy;
    API_COMPILE(spiFuncRef(SPI));
    API_COMPILE(spiFuncCref(SPI));
    API_COMPILE(spiFuncPtr(&SPI));
    API_COMPILE(spiFuncCptr(&SPI));

#if Wiring_SPI1
    API_COMPILE(spiFuncRef(SPI1));
    API_COMPILE(spiFuncCref(SPI1));
    API_COMPILE(spiFuncPtr(&SPI1));
    API_COMPILE(spiFuncCptr(&SPI1));
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE(spiFuncRef(SPI2));
    API_COMPILE(spiFuncCref(SPI2));
    API_COMPILE(spiFuncPtr(&SPI2));
    API_COMPILE(spiFuncCptr(&SPI2));
#endif // Wiring_SPI2
}

test(spi)
{
    API_COMPILE(SPI.begin());
    API_COMPILE(SPI.begin(D0));
    API_COMPILE(SPI.begin(SPI_MODE_MASTER));
    API_COMPILE(SPI.begin(SPI_MODE_SLAVE));
    API_COMPILE(SPI.begin(SPI_MODE_MASTER, D0));
    API_COMPILE(SPI.end());
    API_COMPILE({ bool r = SPI.isEnabled(); (void)r; });

#if Wiring_SPI1
    API_COMPILE(SPI1.begin());
    API_COMPILE(SPI1.begin(D0));
    API_COMPILE(SPI1.begin(SPI_MODE_MASTER));
    API_COMPILE(SPI1.begin(SPI_MODE_SLAVE));
    API_COMPILE(SPI1.begin(SPI_MODE_MASTER, D0));
    API_COMPILE(SPI1.end());
    API_COMPILE({ bool r = SPI1.isEnabled(); (void)r; });
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE(SPI2.begin());
    API_COMPILE(SPI2.begin(D0));
    API_COMPILE(SPI2.begin(SPI_MODE_MASTER));
    API_COMPILE(SPI2.begin(SPI_MODE_SLAVE));
    API_COMPILE(SPI2.begin(SPI_MODE_MASTER, D0));
    API_COMPILE(SPI2.end());
    API_COMPILE({ bool r = SPI2.isEnabled(); (void)r; });
#endif // Wiring_SPI2
}

test(spi_conf) {
    API_COMPILE(SPI.setBitOrder(MSBFIRST));
    API_COMPILE(SPI.setDataMode(SPI_MODE0));

#if Wiring_SPI1
    API_COMPILE(SPI1.setBitOrder(MSBFIRST));
    API_COMPILE(SPI1.setDataMode(SPI_MODE0));
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE(SPI2.setBitOrder(MSBFIRST));
    API_COMPILE(SPI2.setDataMode(SPI_MODE0));
#endif // Wiring_SPI2
}

test(spi_clock)
{
    API_COMPILE(SPI.setClockSpeed(24, MHZ));
    API_COMPILE(SPI.setClockSpeed(24000000));
    API_COMPILE(SPI.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI.setClockDividerReference(SPI_CLK_ARDUINO));

#if Wiring_SPI1
    API_COMPILE(SPI1.setClockSpeed(24, MHZ));
    API_COMPILE(SPI1.setClockSpeed(24000000));
    API_COMPILE(SPI1.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI1.setClockDividerReference(SPI_CLK_ARDUINO));
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE(SPI2.setClockSpeed(24, MHZ));
    API_COMPILE(SPI2.setClockSpeed(24000000));
    API_COMPILE(SPI2.setClockDivider(SPI_CLOCK_DIV2));
    API_COMPILE(SPI2.setClockDividerReference(SPI_CLK_ARDUINO));
#endif // Wiring_SPI2
}

test(spi_transfer)
{
    API_COMPILE({ int32_t r = SPI.beginTransaction(); (void)r; });
    API_COMPILE({ int32_t r = SPI.beginTransaction(SPISettings(SPI_CLK_SYSTEM, SPI_MODE0, MSBFIRST)); (void)r; });
    API_COMPILE(SPI.transfer(0));
    API_COMPILE(SPI.transfer(NULL, NULL, 1, NULL));
    API_COMPILE(SPI.transferCancel());
    API_COMPILE(SPI.endTransaction());

#if Wiring_SPI1
    API_COMPILE({ int32_t r = SPI1.beginTransaction(); (void)r; });
    API_COMPILE({ int32_t r = SPI1.beginTransaction(SPISettings(SPI_CLK_SYSTEM, SPI_MODE0, MSBFIRST)); (void)r; });
    API_COMPILE(SPI1.transfer(0));
    API_COMPILE(SPI1.transfer(NULL, NULL, 1, NULL));
    API_COMPILE(SPI1.transferCancel());
    API_COMPILE(SPI1.endTransaction());
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE({ int32_t r = SPI2.beginTransaction(); (void)r; });
    API_COMPILE({ int32_t r = SPI2.beginTransaction(SPISettings(SPI_CLK_SYSTEM, SPI_MODE0, MSBFIRST)); (void)r; });
    API_COMPILE(SPI2.transfer(0));
    API_COMPILE(SPI2.transfer(NULL, NULL, 1, NULL));
    API_COMPILE(SPI2.transferCancel());
    API_COMPILE(SPI2.endTransaction());
#endif // Wiring_SPI2
}

test(spi_slave)
{
    API_COMPILE(SPI.onSelect(nullptr));
    API_COMPILE({ int32_t r = SPI.available(); (void)r; });

#if Wiring_SPI1
    API_COMPILE(SPI1.onSelect(nullptr));
    API_COMPILE({ int32_t r = SPI1.available(); (void)r; });
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE(SPI2.onSelect(nullptr));
    API_COMPILE({ int32_t r = SPI2.available(); (void)r; });
#endif // Wiring_SPI2
}

test(spi_lock)
{
    API_COMPILE({ bool r = SPI.trylock(); (void)r; });
    API_COMPILE({ int r = SPI.lock(); (void)r; });
    API_COMPILE(SPI.unlock());

#if Wiring_SPI1
    API_COMPILE({ bool r = SPI1.trylock(); (void)r; });
    API_COMPILE({ int r = SPI1.lock(); (void)r; });
    API_COMPILE(SPI1.unlock());
#endif // Wiring_SPI1

#if Wiring_SPI2
    API_COMPILE({ bool r = SPI2.trylock(); (void)r; });
    API_COMPILE({ int r = SPI2.lock(); (void)r; });
    API_COMPILE(SPI2.unlock());
#endif // Wiring_SPI2
}

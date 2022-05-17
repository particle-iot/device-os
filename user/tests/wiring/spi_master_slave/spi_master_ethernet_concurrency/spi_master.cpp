#include "application.h"
#include "unit-test/unit-test.h"
#include <functional>
#include "random.h"

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#define SPI_DELAY 5

#ifndef USE_SPI
#error Define USE_SPI
#endif // #ifndef USE_SPI

#ifndef USE_CS
#error Define USE_CS
#endif // #ifndef USE_CS

#if HAL_PLATFORM_NRF52840
#if (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_B5SOM)

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D8
#pragma message "Compiling for SPI, MY_CS set to D8"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#error "SPI2 not supported for asom, bsom and b5som"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#else // argon, boron

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D14
#pragma message "Compiling for SPI, MY_CS set to D14"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#error "SPI2 not supported for argon or boron"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#endif // #if (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_B5SOM)

#else

#error "Unsupported platform"

#endif // #if HAL_PLATFORM_NRF52840

#if defined(_SPI) && (USE_CS != 255)
#pragma message "Overriding default CS selection"
#undef MY_CS
#if (USE_CS == 0)
#define MY_CS A2
#pragma message "_CS pin set to A2"
#elif (USE_CS == 1)
#define MY_CS D5
#pragma message "_CS pin set to D5"
#elif (USE_CS == 2)
#define MY_CS D8
#pragma message "_CS pin set to D8"
#elif (USE_CS == 3)
#define MY_CS D14
#pragma message "_CS pin set to D14"
#elif (USE_CS == 4)
#define MY_CS C0
#pragma message "_CS pin set to C0"
#endif // (USE_CS == 0)
#endif // defined(_SPI) && (USE_CS != 255)

/*
 * Gen 3 SoM Wiring diagrams
 *
 * SPI/SPI1                       SPI1/SPI1
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)
 * Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master             Slave       Master              Slave
 * CS   D8  <-------> D5 CS       CS   D5 <---------> D5 CS
 * MISO D11 <-------> D4 MISO     MISO D4 <---------> D4 MISO
 * MOSI D12 <-------> D3 MOSI     MOSI D3 <---------> D3 MOSI
 * SCK  D13 <-------> D2 SCK      SCK  D2 <---------> D2 SCK
 *
 *********************************************************************************************
 *
 * Gen 3 Non-SoM Wiring diagrams
 *
 * SPI/SPI1                       SPI1/SPI1
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)
 * Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master             Slave       Master              Slave
 * CS   D14 <-------> D5 CS       CS   D5 <---------> D5 CS
 * MISO D11 <-------> D4 MISO     MISO D4 <---------> D4 MISO
 * MOSI D12 <-------> D3 MOSI     MOSI D3 <---------> D3 MOSI
 * SCK  D13 <-------> D2 SCK      SCK  D2 <---------> D2 SCK
 *
 *********************************************************************************************
 */

#ifdef MY_SPI

STARTUP(System.enableFeature(FEATURE_ETHERNET_DETECTION));

static uint8_t SPI_Master_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Master_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;

#ifdef stringify
#undef stringify
#endif
#ifdef __stringify
#undef __stringify
#endif
#define stringify(x) __stringify(x)
#define __stringify(x) #x

#ifndef UDP_ECHO_SERVER_HOSTNAME
#define UDP_ECHO_SERVER_HOSTNAME not_defined
#endif

const char udpEchoServerHostname[] = stringify(UDP_ECHO_SERVER_HOSTNAME);

static void SPI_Master_Configure()
{
    if (!MY_SPI.isEnabled()) {
        // Run SPI bus at 1 MHz, so that this test works reliably even if the
        // devices are connected with jumper wires
        MY_SPI.setClockSpeed(1, MHZ);
        MY_SPI.begin(MY_CS);

        // Clock dummy byte just in case
        (void)MY_SPI.transfer(0xff);
    }
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static void SPI_Master_Transfer_No_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length) {
    for(int count = 0; count < length; count++)
    {
        uint8_t tmp = MY_SPI.transfer(txbuf ? txbuf[count] : 0xff);
        if (rxbuf)
            rxbuf[count] = tmp;
    }
}

static void SPI_Master_Transfer_DMA(uint8_t* txbuf, uint8_t* rxbuf, int length, hal_spi_dma_user_callback cb) {
    if (cb) {
        DMA_Completed_Flag = 0;
        MY_SPI.transfer(txbuf, rxbuf, length, cb);
        while(DMA_Completed_Flag == 0);
        assertEqual(length, MY_SPI.available());
    } else {
        MY_SPI.transfer(txbuf, rxbuf, length, NULL);
        assertEqual(length, MY_SPI.available());
    }
}

bool SPI_Master_Slave_Change_Mode(uint8_t mode, uint8_t bitOrder, std::function<void(uint8_t*, uint8_t*, int)> transferFunc) {
    // Use previous settings
    SPI_Master_Configure();

    uint32_t requestedLength = 0;
    uint32_t switchMode = ((uint32_t)mode) | ((uint32_t)bitOrder << 8) | (0x8DC6 << 16);
    memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));

    memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));
    memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));

    // Select
    if (MY_SPI.beginTransaction()) {
        return false;
    }
    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);

    memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));
    memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t),
           (void*)&switchMode, sizeof(uint32_t));

    transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);

    digitalWrite(MY_CS, HIGH);
    delay(SPI_DELAY * 10);
    MY_SPI.endTransaction();

    bool ret = (strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

    // Apply new settings here
    MY_SPI.setDataMode(mode);
    MY_SPI.setBitOrder(bitOrder);
    SPI_Master_Configure();

    return ret;
}

void SPI_Master_Slave_Master_Test_Routine(std::function<void(uint8_t*, uint8_t*, int)> transferFunc, bool end = false) {
    uint32_t requestedLength = TRANSFER_LENGTH_2;
    SPI_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0 && !end)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(SPI_Master_Tx_Buffer, 0, sizeof(SPI_Master_Tx_Buffer));
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));

        // Select
        assertEqual(MY_SPI.beginTransaction(), 0);
        // Workaround for some platforms requiring the CS to be high when configuring
        // the DMA buffers
        digitalWrite(MY_CS, LOW);
        delay(SPI_DELAY);
        digitalWrite(MY_CS, HIGH);
        delay(SPI_DELAY);
        digitalWrite(MY_CS, LOW);

        memcpy(SPI_Master_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(SPI_Master_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        transferFunc(SPI_Master_Tx_Buffer, SPI_Master_Rx_Buffer, TRANSFER_LENGTH_1);
        digitalWrite(MY_CS, HIGH);
        MY_SPI.endTransaction();

        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1)) == 0);

        delay(SPI_DELAY);
        if (requestedLength == 0)
            break;

        assertEqual(MY_SPI.beginTransaction(), 0);
        digitalWrite(MY_CS, LOW);
        delay(SPI_DELAY);

        // Received a good first reply from Slave
        // Now read out requestedLength bytes
        memset(SPI_Master_Rx_Buffer, 0, sizeof(SPI_Master_Rx_Buffer));
        transferFunc(NULL, SPI_Master_Rx_Buffer, requestedLength);
        // Deselect
        digitalWrite(MY_CS, HIGH);
        delay(SPI_DELAY);
        MY_SPI.endTransaction();

        // Serial.print("< ");
        // Serial.println((const char *)SPI_Master_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Master_Rx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength) == 0);

        requestedLength--;
    }
}

test(00_SPI_Master_Slave_Master_Start_Ethernet) {
    // If server not defined, skip test
    if (!strcmp(udpEchoServerHostname, "not_defined") || !strcmp(udpEchoServerHostname, "")) {
        Serial.printlnf("Command line option UDP_ECHO_SERVER_HOSTNAME not defined! Usage: UDP_ECHO_SERVER_HOSTNAME=hostname make clean all TEST=...");
        skip();
        return;
    }
    Serial.printlnf("Using Echo Server: [%s]", udpEchoServerHostname);

    Serial.println("This is Master");
    Serial.println("Connecting to echo service over Ethernet");
    Ethernet.on();
    Ethernet.connect();
    waitFor(Ethernet.ready, 60000);
    assertTrue(Ethernet.ready());

    static volatile bool exit = false;
    static volatile bool ok = false;
    static auto okDone = []() -> bool {
        return ok;
    };


    auto thread = new Thread("ethernet_comms", [](void) -> os_thread_return_t {
        const uint16_t udpEchoPort = 40000;
        const size_t udpPayloadSize = 512;

        // Resolve UDP echo server hostname to ip address, so that DNS resolutions
        // no longer affect us after this point
        const auto udpEchoIp = Network.resolve(udpEchoServerHostname);
        if (!udpEchoIp) {
            return;
        }

        // Create UDP client
        std::unique_ptr<UDP> udp(new UDP());
        if (!udp) {
            return;
        }

        std::unique_ptr<uint8_t[]> sendBuffer(new uint8_t[udpPayloadSize]);
        if (!sendBuffer) {
            return;
        }
        std::unique_ptr<uint8_t[]> recvBuffer(new uint8_t[udpPayloadSize * 2]);
        if (!recvBuffer) {
            return;
        }

        udp->setBuffer(udpPayloadSize * 2, recvBuffer.get());
        udp->begin(udpEchoPort);

        particle::Random rand;

        while (!exit) {
            if (udp->parsePacket() > 0) {
                // Make sure we've received at least something back
                ok = true;
            }
            rand.gen((char*)sendBuffer.get(), udpPayloadSize);
            auto snd = udp->sendPacket(sendBuffer.get(), udpPayloadSize, udpEchoIp, udpEchoPort);
            (void)snd;
            delay(rand.gen<system_tick_t>() % 50);
        }
    }, OS_THREAD_PRIORITY_DEFAULT + 1);

    waitFor(okDone, 60000);
    if (!ok) {
        exit = true;
        thread->join();
        delete thread;
    }
    assertTrue((bool)ok);
}

/*
 * Default mode: SPI_MODE3, MSBFIRST
 */
test(01_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_Default_MODE3_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(02_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Default_MODE3_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(03_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_Default_MODE3_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE3, LSBFIRST
 */
test(04_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE3_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE3, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(05_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE3_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE3, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(06_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE3_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE3, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE0, MSBFIRST
 */
test(07_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE0_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(08_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE0_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(09_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE0_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE0, LSBFIRST
 */
test(10_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE0_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(11_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE0_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(12_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE0_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE0, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE1, MSBFIRST
 */
test(13_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE1_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(14_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE1_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(15_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE1_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE1, LSBFIRST
 */
test(16_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE1_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(17_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE1_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(18_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE1_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE1, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE2, MSBFIRST
 */
test(19_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE2_MSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(20_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE2_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(21_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE2_MSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, MSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

/*
 * SPI_MODE2, LSBFIRST
 */
test(22_SPI_Master_Slave_Master_Variable_Length_Transfer_No_DMA_MODE2_LSB)
{
    auto transferFunc = SPI_Master_Transfer_No_DMA;
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(23_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_MODE2_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, &SPI_DMA_Completed_Callback);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc);
}

test(24_SPI_Master_Slave_Master_Variable_Length_Transfer_DMA_Synchronous_MODE2_LSB)
{
    using namespace std::placeholders;
    auto transferFunc = std::bind(SPI_Master_Transfer_DMA, _1, _2, _3, (hal_spi_dma_user_callback)NULL);
    assertTrue(SPI_Master_Slave_Change_Mode(SPI_MODE2, LSBFIRST, transferFunc));
    SPI_Master_Slave_Master_Test_Routine(transferFunc, true);
}

#endif // #ifdef _SPI

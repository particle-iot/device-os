#include "application.h"
#include "unit-test/unit-test.h"

#define MASTER_TEST_MESSAGE "Hello from SPI Master!?"
#define SLAVE_TEST_MESSAGE_1  "SPI Slave is doing good"
#define SLAVE_TEST_MESSAGE_2  "All work and no play makes Jack a dull boy"
#define TRANSFER_LENGTH_1 (sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t) + sizeof(uint32_t))
#define TRANSFER_LENGTH_2 sizeof(SLAVE_TEST_MESSAGE_2)

#ifndef USE_SPI
#error Define USE_SPI
#endif // #ifndef USE_SPI

#ifndef USE_CS
#error Define USE_CS
#endif // #ifndef USE_CS

#if HAL_PLATFORM_NRF52840
#if (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_XSOM)

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D8
#pragma message "Compiling for SPI, MY_CS set to D8"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#error "SPI2 not supported for xenon-som, argon-som or boron-som"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#else // xenon, argon, boron

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D14
#pragma message "Compiling for SPI, MY_CS set to D14"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#error "SPI2 not supported for xenon, argon or boron"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#endif // #if (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_XSOM)

#else // Gen 2

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS A2
#pragma message "Compiling for SPI, MY_CS set to A2"
#elif (USE_SPI == 1)
#define MY_SPI SPI1
#define MY_CS D5
#pragma message "Compiling for SPI1, MY_CS set to D5"
#elif (USE_SPI == 2)
#define MY_SPI SPI2
#define MY_CS C0
#pragma message "Compiling for SPI2, MY_CS set to C0"
#else
#error "Not supported for Gen 2"
#endif // (USE_SPI == 0)

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
 * Gen 2 Wiring diagrams
 *
 * SPI/SPI                        SPI1/SPI                       SPI2/SPI
 * Master: SPI (USE_SPI=SPI)      Master: SPI1 (USE_SPI=SPI1)    Master: SPI2 (USE_SPI=SPI2)
 * Slave:  SPI (USE_SPI=SPI)      Slave:  SPI  (USE_SPI=SPI)     Slave:  SPI  (USE_SPI=SPI)
 *
 * Master              Slave      Master              Slave      Master              Slave
 * MOSI A5 <---------> A5 MOSI    MOSI D2 <---------> A5 MOSI    MOSI C1 <---------> A5 MOSI
 * MISO A4 <---------> A4 MISO    MISO D3 <---------> A4 MISO    MISO C2 <---------> A4 MISO
 * SCK  A3 <---------> A3 SCK     SCK  D4 <---------> A3 SCK     SCK  C3 <---------> A3 SCK
 * CS   A2 <---------> A2 CS      CS   D5 <---------> A2 CS      CS   C0 <---------> A2 CS
 *
 *********************************************************************************************
 *
 * SPI/SPI1                       SPI1/SPI1                      SPI2/SPI1
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)    Master: SPI2 (USE_SPI=SPI2)
 * Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master              Slave      Master              Slave      Master              Slave
 * MOSI A5 <---------> D2 MOSI    MOSI D2 <---------> D2 MOSI    MOSI C1 <---------> D2 MOSI
 * MISO A4 <---------> D3 MISO    MISO D3 <---------> D3 MISO    MISO C2 <---------> D3 MISO
 * SCK  A3 <---------> D4 SCK     SCK  D4 <---------> D4 SCK     SCK  C3 <---------> D4 SCK
 * CS   A2 <---------> D5 CS      CS   D5 <---------> D5 CS      CS   C0 <---------> D5 CS
 *
 *********************************************************************************************
 *
 * SPI/SPI2                       SPI1/SPI2                      SPI2/SPI2
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)    Master: SPI2 (USE_SPI=SPI2)
 * Slave:  SPI2 (USE_SPI=SPI2     Slave:  SPI2 (USE_SPI=SPI2)    Slave:  SPI2 (USE_SPI=SPI2)
 *
 * Master              Slave      Master              Slave      Master              Slave
 * MOSI A5 <---------> C1 MOSI    MOSI D2 <---------> C1 MOSI    MOSI C1 <---------> C1 MOSI
 * MISO A4 <---------> C2 MISO    MISO D3 <---------> C2 MISO    MISO C2 <---------> C2 MISO
 * SCK  A3 <---------> C3 SCK     SCK  D4 <---------> C3 SCK     SCK  C3 <---------> C3 SCK
 * CS   A2 <---------> C0 CS      CS   D5 <---------> C0 CS      CS   C0 <---------> C0 CS
 *
 *********************************************************************************************
 *
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

static uint8_t SPI_Slave_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Slave_Rx_Buffer[TRANSFER_LENGTH_2];
static volatile uint8_t DMA_Completed_Flag = 0;
static volatile uint8_t SPI_Selected_State = 0;

static void SPI_On_Select(uint8_t state)
{
    if (state)
        SPI_Selected_State = state;
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static inline void SPI_Transfer_DMA(uint8_t *tx, uint8_t *rx, int length, HAL_SPI_DMA_UserCallback cb)
{
    while (true)
    {
        DMA_Completed_Flag = cb ? 0 : 1;
        MY_SPI.transfer(tx, rx, length, cb);
        while(DMA_Completed_Flag == 0);
        if (MY_SPI.available() == 0)
        {
            /* 0 length transfer means that we got deselected by master and nothing got clocked in. Try again? */
            continue;
        }
        break;
    }
}

static void SPI_Init(uint8_t mode, uint8_t bitOrder) {
    if (MY_SPI.isEnabled()) {
        MY_SPI.end();
    }

    MY_SPI.onSelect(SPI_On_Select);
    MY_SPI.setBitOrder(bitOrder);
    MY_SPI.setDataMode(mode);

    MY_SPI.begin(SPI_MODE_SLAVE, MY_CS);
}

test(SPI_Master_Slave_Slave_Transfer)
{
    /* Test will alternate between asynchronous and synchronous SPI.transfer() */
    Serial.println("This is Slave");

    // Default SPI_MODE3, MSBFIRST
    SPI_Init(SPI_MODE3, MSBFIRST);

    uint32_t requestedLength = 0;

    uint32_t count = 0;

    while (true)
    {
        memset(SPI_Slave_Tx_Buffer, 0, sizeof(SPI_Slave_Tx_Buffer));
        memset(SPI_Slave_Rx_Buffer, 0, sizeof(SPI_Slave_Rx_Buffer));

        /* Preload SLAVE_TEST_MESSAGE_1 reply */
        memcpy(SPI_Slave_Tx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1));

        /* IMPORTANT: NOT waiting for Master to select us, as some of the platforms
         * require TX and RX buffers to be configured before CS goes low
         */
        // while(SPI_Selected_State == 0);
        // SPI_Selected_State = 0;

        /* Receive up to TRANSFER_LENGTH_2 bytes */
        SPI_Transfer_DMA(SPI_Slave_Tx_Buffer, SPI_Slave_Rx_Buffer, TRANSFER_LENGTH_2, count % 2 == 0 ? &SPI_DMA_Completed_Callback : NULL);
        /* Check how many bytes we have received */
        assertEqual(MY_SPI.available(), TRANSFER_LENGTH_1);

        // Serial.print("< ");
        // Serial.println((const char *)SPI_Slave_Rx_Buffer);
        assertTrue(strncmp((const char *)SPI_Slave_Rx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE)) == 0);

        requestedLength = *((uint32_t *)(SPI_Slave_Rx_Buffer + sizeof(MASTER_TEST_MESSAGE)));
        assertTrue((requestedLength >= 0 && requestedLength <= TRANSFER_LENGTH_2));

        if (requestedLength == 0) {
            // Check if Master wants us to switch mode
            uint32_t switchMode = *((uint32_t *)(SPI_Slave_Rx_Buffer + sizeof(MASTER_TEST_MESSAGE) + sizeof(uint32_t)));
            if (((switchMode >> 16) & 0xffff) == 0x8DC6) {
                uint8_t mode = switchMode & 0xff;
                uint8_t bitOrder = (switchMode >> 8) & 0xff;

                assertMoreOrEqual(mode, SPI_MODE0);
                assertLessOrEqual(mode, SPI_MODE3);

                assertTrue((bitOrder == MSBFIRST) || (bitOrder == LSBFIRST));

                Serial.printf("Switching SPI mode to MODE%1d %s\r\n", mode,
                              bitOrder == MSBFIRST ? "MSBFIRST" : "LSBFIRST");

                SPI_Init(mode, bitOrder);

                continue;
            } else {
                // Otherwise end the test
                break;
            }
        }

        memcpy(SPI_Slave_Tx_Buffer, SLAVE_TEST_MESSAGE_2, requestedLength);
        SPI_Slave_Tx_Buffer[requestedLength] = '\0';
        // Serial.print("> ");
        // Serial.println((const char *)SPI_Slave_Tx_Buffer);

        SPI_Transfer_DMA(SPI_Slave_Tx_Buffer, SPI_Slave_Rx_Buffer, requestedLength, count % 2 == 0 ? &SPI_DMA_Completed_Callback : NULL);
        /* Check that we have transferred requestedLength bytes as requested */
        assertEqual(MY_SPI.available(), requestedLength);

        /* Just a sanity check that we received only 0xff */
        for (int i = 0; i < requestedLength; i++)
        {
            assertEqual(SPI_Slave_Rx_Buffer[i], 0xff);
        }

        count++;
    }
}

#endif // #ifdef MY_SPI

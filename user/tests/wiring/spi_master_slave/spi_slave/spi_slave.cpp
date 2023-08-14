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

#if HAL_PLATFORM_RTL872X

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#if PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
#error "SPI not supported as slave for p2"
#endif // PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
#define MY_SPI SPI
#define MY_CS SS
#pragma message "Compiling for SPI, MY_CS set to SS"
#elif (USE_SPI == 1)
#if PLATFORM_ID == PLATFORM_MSOM
#error "SPI1 not supported as slave for MSoM"
#endif // PLATFORM_ID == PLATFORM_MSOM
#define MY_SPI SPI1
#define MY_CS SS1
#pragma message "Compiling for SPI1, MY_CS set to SS1"
#elif (USE_SPI == 2)
#error "SPI2 not supported for p2/TrackerM/MSoM"
#else
#error "Not supported for p2/TrackerM/MSoM"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#elif HAL_PLATFORM_NRF52840

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI, but SPI slave is not supported on Gen3
#error "SPI slave is not supported on SPI instance on Gen3 platforms. Please specify USE_SPI=SPI1."
#endif

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
#error "SPI2 not supported for asom, bsom or b5som"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#elif (PLATFORM_ID == PLATFORM_TRACKER)

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D0 // FIXME
#pragma message "Compiling for SPI, MY_CS set to D0"
#elif (USE_SPI == 2) || (USE_SPI == 1)
#error "SPI2 not supported for bsom and b5som"
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

#endif // HAL_PLATFORM_RTL872X

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
 *
 * P2 Wiring diagrams
 *
 * SPI1/SPI1                       SPI/SPI1 (SPI can't be used as slave)
 * Master: SPI1  (USE_SPI=SPI1)    Master: SPI (USE_SPI=SPI)
 * Slave:  SPI1  (USE_SPI=SPI1)    Slave:  SPI1 (USE_SPI=SPI1)
 *
 * Master             Slave       Master              Slave
 * CS   D5 <-------> D5 CS        CS   S3 <---------> D5 CS
 * MISO D4 <-------> D3 MISO      MISO S1 <---------> D3 MISO
 * MOSI D3 <-------> D2 MOSI      MOSI S0 <---------> D2 MOSI
 * SCK  D2 <-------> D4 SCK       SCK  S2 <---------> D4 SCK
 *
 *********************************************************************************************
 *
 * MSoM Wiring diagrams
 *
 * SPI/SPI                        SPI/SPI1 (SPI1 can't be used as slave)
 * Master: SPI  (USE_SPI=SPI)     Master: SPI1 (USE_SPI=SPI1)
 * Slave:  SPI  (USE_SPI=SPI)     Slave:  SPI (USE_SPI=SPI)
 *
 * Master             Slave       Master              Slave
 * CS   D8  <-------> D8  CS        CS   D3  <---------> D8  CS
 * MISO D13 <-------> D13 MISO      MISO D2  <---------> D13 MISO
 * MOSI D11 <-------> D11 MOSI      MOSI D10 <---------> D11 MOSI
 * SCK  D12 <-------> D12 SCK       SCK  D9  <---------> D12 SCK
 *
 *********************************************************************************************
 */

#ifdef MY_SPI

static uint8_t SPI_Slave_Tx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Slave_Rx_Buffer[TRANSFER_LENGTH_2];
static uint8_t SPI_Slave_Rx_Buffer_Supper[1024];
static volatile uint8_t DMA_Completed_Flag = 0;
static volatile uint8_t SPI_Selected_State = 0;

static const char* txString =
"urjlU1tW177HwJsR6TylreMKge225qyLaIizW5IhXHkWgTGpH2fZtm2Od20Ne3Q81fxfUl7zoFaF\
Z6smPzkpTGNSxGg7TCEiE2f19951tKxjFCB4Se86R4CaWW2YZF0mogirgsu2qRMGe4mC9QlJkCgXP\
bgSVV1mc2xsZcu4bj0pbmPIhxkuyAHe4cVK3gLpWEGTadtAn2k66rOFNBdfPaE0cUY3wwXlVQ9yDl\
OxexepQcC2WTrbUe4z85OSae9s8A6BwUCRBYYfEH01cnGCzYGCEOEm5jl4nJ3HqWckHI5K2NeWS4x\
EhkgMqG3RwfOTM85SQ7q7NLIhgprCTsBTzv2YpGgbAB7oSX0joGQHxfyndxIyCVIHknvEj1hynXtw\
uebA6i7JBFiGkk4AnRzk7v3dNjHt6weuYWtf6yj3aVzhbMaWFCR6HOKFc3i8XzBsnLTc4Uzft61a0\
qV8ZssHdHO7sbiojOmA37RkrNUFxX1aODUXWNEntkTylwvhxKpsAb6Lsopzve4ea2G17WpW62Z12x\
mNgTZQHOo3fCZDy8L7WfVwCJiJunHPXu9jw6g11NJFcpo2AakkZQDgUGZoeZgDB6GfRheAiurAEB5\
Ym4EVIQB9AvVBf4zY84R8D4bnfjwwLDwiZSo9y2Z5JsVQ0yRdqPdxv0cV2Kp0AaevITeubJseCXOg\
LkFiaeDTBoR7kyMyoJvJl4vjLmiV03RNSAl9JpZkBfTHzalZw8oaRHMMiTVVGdieJOIbANoaXyRbe\
xSYU1t5dOe8wxybwfBBlPIswpVJ45kXd4Bu8NCLXPAbgJCOVSlTQsfvzVKZykp9V1DBQ3PwyeBXJB\
QsLDslIOHOKbfqB8njXotpE3Dz46Wi6QtpinLsSiviZmz62qLW5Pd9M7SDCarrxFk8SBHyJl2KdjH\
5Lx1LmkW8gMiYquOctT9xhFNs406BxWrPcTc5kwaSJ6RJQyohQEJk9ojchrbSo4ucfZGQzEMBEIJs";

static void SPI_On_Select(uint8_t state)
{
    SPI_Selected_State = state;
}

static void SPI_DMA_Completed_Callback()
{
    DMA_Completed_Flag = 1;
}

static inline void SPI_Transfer_DMA(const uint8_t *tx, uint8_t *rx, int length, hal_spi_dma_user_callback cb)
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

test(00_SPI_Master_Slave_Slave_Transfer)
{
    /* Test will alternate between asynchronous and synchronous SPI.transfer() */
    Serial.println("This is Slave");

    // Default SPI_MODE3, MSBFIRST
    SPI_Init(SPI_MODE3, MSBFIRST);

    uint32_t requestedLength = 0;

    uint32_t count = 0;

    while (true)
    {
        int ret = hal_spi_sleep(MY_SPI.interface(), false, nullptr);
        // assertEqual(ret, (int)SYSTEM_ERROR_NONE); // Device may be awake
        memset(SPI_Slave_Tx_Buffer, 0, sizeof(SPI_Slave_Tx_Buffer));
        memset(SPI_Slave_Rx_Buffer, 0, sizeof(SPI_Slave_Rx_Buffer));

        /* Preload SLAVE_TEST_MESSAGE_1 reply */
        memcpy(SPI_Slave_Tx_Buffer, SLAVE_TEST_MESSAGE_1, sizeof(SLAVE_TEST_MESSAGE_1));

        /* IMPORTANT: NOT waiting for Master to select us, as some of the platforms
         * require TX and RX buffers to be configured before CS goes low
         */
        while(SPI_Selected_State == 0);

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

        ret = hal_spi_sleep(MY_SPI.interface(), true, nullptr);
        assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    }
}

test(01_SPI_Master_Slave_Slave_Receiption)
{
    /* IMPORTANT: NOT waiting for Master to select us, as some of the platforms
     * require TX and RX buffers to be configured before CS goes low
     */
    while(SPI_Selected_State == 0);

    memset(SPI_Slave_Rx_Buffer_Supper, 0, sizeof(SPI_Slave_Rx_Buffer_Supper));

    SPI_Transfer_DMA(nullptr, SPI_Slave_Rx_Buffer_Supper, sizeof(SPI_Slave_Rx_Buffer_Supper), NULL);
    /* Check how many bytes we have received */
    assertEqual(MY_SPI.available(), strlen(txString));

    // Serial.print("< ");
    // Serial.println((const char *)SPI_Slave_Rx_Buffer_Supper);
    assertTrue(strncmp((const char *)SPI_Slave_Rx_Buffer_Supper, txString, strlen(txString)) == 0);
}

test(02_SPI_Master_Slave_Slave_Const_String_Transfer_DMA)
{
    /* IMPORTANT: NOT waiting for Master to select us, as some of the platforms
    * require TX and RX buffers to be configured before CS goes low
    */
    while(SPI_Selected_State == 0);

    // FIXME: The rx_buffer has to be nullptr, otherwise, the the rx_buffer will overflow
    // as the length of the rx buffer given in this file is not equal to the length of tx data
    MY_SPI.transfer(txString, nullptr, strlen(txString), NULL);
    assertEqual(MY_SPI.available(), 0);
}

#endif // #ifdef MY_SPI

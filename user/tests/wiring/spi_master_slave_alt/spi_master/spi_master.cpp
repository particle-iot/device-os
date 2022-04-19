#include "application.h"
#include "unit-test/unit-test.h"
#include <functional>

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
#error "SPI2 not supported for asom, bsom or b5som"
#else
#error "Not supported for Gen 3"
#endif // (USE_SPI == 0 || USE_SPI == 255)

#elif (PLATFORM_ID == PLATFORM_TRACKER)

#if (USE_SPI == 0 || USE_SPI == 255) // default to SPI
#define MY_SPI SPI
#define MY_CS D0 // FIXME
#pragma message "Compiling for SPI, MY_CS set to D0"
#elif (USE_SPI == 1) || (USE_SPI == 2)
#error "SPI2 not supported for tracker"
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

#define DMA_MODE 1

static uint8_t rx_buffer[64];
static uint8_t tx_buffer[64];
static volatile uint32_t select_state = 0x00;
static volatile uint32_t transfer_state = 0x00;

void onTransferFinished() {
    transfer_state = 1;
}

void master_setup() {
    Serial.begin(9600);
    MY_SPI.setClockSpeed(1, MHZ);
    MY_SPI.setDataMode(SPI_MODE0);
    MY_SPI.begin(SPI_MODE_MASTER, MY_CS);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(tx_buffer, 0, sizeof(tx_buffer));
}

test(01_SPI_Master_Slave_Transfer_DMA_Default_MODE0_MSB)
{
    Serial.println("This is Master");
    master_setup();

    for (unsigned x = 0; x < sizeof(tx_buffer); x++) {

    // one transfer per 10 ms
    delay(10);

    // one spi transfer
    static int transferLength = sizeof(tx_buffer);
    if (++transferLength > (int)sizeof(tx_buffer)) {
        transferLength = 1;
    }
    for (uint8_t i = 0; i < transferLength; i++) {
        tx_buffer[i] = i;
    }

    memset(rx_buffer, 0, sizeof(rx_buffer));

    digitalWrite(MY_CS, LOW);
    delay(SPI_DELAY);

#if DMA_MODE == 1
    transfer_state = 0;
    MY_SPI.transfer(tx_buffer, rx_buffer, transferLength, onTransferFinished);
    while (transfer_state == 0) {
        // Serial.println("Waiting for transfer to complete");
        // delay(1000);
        Particle.process();
    }
#else
    for (int count = 0; count < transferLength; count++) {
        rx_buffer[count] = MY_SPI.transfer(tx_buffer[count]);
    }
#endif

    digitalWrite(MY_CS, HIGH);

    Serial.printf("Sent %d (0x%02x) bytes, got back:", transferLength, transferLength);
    Serial.println();
    for (int i = 0; i < transferLength; i++) {
        Serial.printf("%02x ", rx_buffer[i]);
    }
    Serial.println();

    if (memcmp(rx_buffer, tx_buffer, transferLength - 1) != 0) {
        Serial.println("Error - Data mismatch!");
        assertTrue(false); // force failure
    }

    } // for sizeof(tx_buffer) times

    assertTrue(true); // force pass
}

#endif // #ifdef MY_SPI


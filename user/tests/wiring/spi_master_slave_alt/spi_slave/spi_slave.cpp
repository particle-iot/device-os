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

#define DMA_MODE 1

static uint8_t rx_buffer[64];
static uint8_t tx_buffer[64];
static volatile uint32_t select_state = 0x00;
static volatile uint32_t transfer_state = 0x00;

void onTransferFinished() {
    transfer_state = 1;
}

void onSelect(uint8_t state) {
    if (state) {
        select_state = state;
    }
}

void slave_setup() {
    Serial.begin(9600);
    MY_SPI.onSelect(onSelect);
    MY_SPI.setClockSpeed(1, MHZ);
    MY_SPI.setDataMode(SPI_MODE0);
    MY_SPI.begin(SPI_MODE_SLAVE, MY_CS);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    for (uint8_t i = 0; i < sizeof(tx_buffer); i++) {
        tx_buffer[i] = i;
    }
}

test(01_SPI_Master_Slave_Slave_Transfer)
{
    Serial.println("This is Slave");
    slave_setup();

    while (1) {
        while (select_state == 0) {
            // Serial.println("Waiting for slave to be selected");
            // delay(1000);
            Particle.process();
        }
        select_state = 0;

        transfer_state = 0;
        MY_SPI.transfer(tx_buffer, rx_buffer, sizeof(rx_buffer), onTransferFinished);
        while (transfer_state == 0) {
            // Serial.println("Waiting for transfer to complete");
            // delay(1000);
            Particle.process();
        }

        int bytesReceived = MY_SPI.available();
        if (bytesReceived > 0) {
            Serial.printf("Received %d (0x%02x) bytes", bytesReceived, bytesReceived);
            Serial.println();
            for (int i = 0; i < bytesReceived; i++) {
                Serial.printf("%02x ", rx_buffer[i]);
            }
            Serial.println();

            if (memcmp(rx_buffer, tx_buffer, bytesReceived) != 0) {
                Serial.println("Error - Data mismatch!");
                assertTrue(false); // force failure
            }

            // If we got here with no failure, the test is over!
            if (bytesReceived == sizeof(rx_buffer)) {
                // Serial.println("Pass");
                break;
            }
        }
    }

    assertTrue(true); // force pass

}

#endif // MY_SPI

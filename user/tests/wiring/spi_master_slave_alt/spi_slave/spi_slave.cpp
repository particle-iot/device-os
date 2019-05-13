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

/**
 ******************************************************************************
 * @file    spi_fram.cpp
 * @authors Brett Walach
 * @version V1.0.0
 * @date    14-Oct-2014
 * @brief   SPI test application
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"

/*
 * SPI Test requires Spark MicroSD Shield with FM25CL64B-G or MB85RS64A FRAM populated for U3,
 *
 * or you may wire it up directly as follows:
 *
 *              8-Pin DIP FM25CL64B-G or MB85RS64A (8Ki)
 *                         ______
 *         (SS - D2) !CS -|o     |- Vdd   (3V3)
 *       (MISO - A4)  SO -|      |- !HOLD (TIE HIGH to 3V3)
 * (TIE HIGH to 3V3) !WP -|      |- SCK   (SCK - A3)
 *             (GND) Vss -|______|- SI    (MOSI - A5)
 *
 */


/*
 * FRAM COMMAND CODES
 */
#define CMD_WREN      0x06   // 0000 0110 Set Write Enable Latch
#define CMD_WRDI      0x04   // 0000 0100 Write Disable
#define CMD_RDSR      0x05   // 0000 0101 Read Status Register
#define CMD_WRSR      0x01   // 0000 0001 Write Status Register
#define CMD_READ      0x03   // 0000 0011 Read Memory Data
#define CMD_WRITE     0x02   // 0000 0010 Write Memory Data
#define FRAM_ADDR_MAX 0x1fff // Max address value (8Ki)

#if !defined(arraySize)
    #define arraySize(a)     (sizeof((a))/sizeof((a[0])))
#endif

static volatile bool SPI_DMA_TransferCompleted = false;

void FRAM_SPI_DMA_TransferComplete_Callback(void)
{
    SPI_DMA_TransferCompleted = true;
}

/*
 * Write to FRAM
 * cs_pin: chip select pin used for FRAM
 * addr: starting address
 * buf: pointer to data
 * count: data length.
 *        If this parameter is omitted, it is defaulted to one byte.
 * returns: 0 operation is successful
 *          -1 address out of range
 */
int8_t FRAMWrite(uint16_t cs_pin, uint16_t addr, uint8_t* buf, uint16_t count=1)
{
    if (addr > FRAM_ADDR_MAX) {
        return -1;
    }

    uint8_t addrMSB = (addr >> 8) & 0xff;
    uint8_t addrLSB = addr & 0xff;

    digitalWrite(cs_pin, LOW);
    SPI.transfer(CMD_WREN);  //write enable
    digitalWrite(cs_pin, HIGH);

    digitalWrite(cs_pin, LOW);
    SPI.transfer(CMD_WRITE); //write command
    SPI.transfer(addrMSB);
    SPI.transfer(addrLSB);

    /* Commented old method of single byte transfer */
    //for (uint16_t i=0; i<count; i++) SPI.transfer(buf[i]);

    /* Using SPI DMA transfer method to transfer buffer */
    SPI_DMA_TransferCompleted = false;
    SPI.transfer(buf, NULL, count, FRAM_SPI_DMA_TransferComplete_Callback);
    while(!SPI_DMA_TransferCompleted);

    digitalWrite(cs_pin, HIGH);

    return 0;
}

/*
 * Read from FRAM
 * cs_pin: chip select pin used for FRAM
 * addr: starting address
 * buf: pointer to data
 * count: data length.
 *        If this parameter is omitted, it is defaulted to one byte.
 * returns: 0 operation is successful
 *          -1 address out of range
 */
int8_t FRAMRead(uint16_t cs_pin, uint16_t addr, uint8_t* buf, uint16_t count=1)
{
    if (addr > FRAM_ADDR_MAX) {
        return -1;
    }

    uint8_t addrMSB = (addr >> 8) & 0xff;
    uint8_t addrLSB = addr & 0xff;

    digitalWrite(cs_pin, LOW);

    SPI.transfer(CMD_READ);
    SPI.transfer(addrMSB);
    SPI.transfer(addrLSB);

    /* Commented old method of single byte transfer */
    //for (uint16_t i=0; i<count; i++) buf[i] = SPI.transfer(0x7e);

    /* Using SPI DMA transfer method to transfer buffer */
    SPI_DMA_TransferCompleted = false;
    SPI.transfer(NULL, buf, count, FRAM_SPI_DMA_TransferComplete_Callback);
    while(!SPI_DMA_TransferCompleted);

    digitalWrite(cs_pin, HIGH);

    return 0;
}

test(SPI_Test1_isENABLED) {
    // When - Not setting up the SPI bus

    // Then - SPI bus is disabled
    assertFalse(SPI.isEnabled());

    // When - Setting up the SPI bus
    SPI.begin();

    // Then - SPI bus is enabled
    assertTrue(SPI.isEnabled());

    // When
    SPI.end();

    // Then - SPI bus is disabled
    assertFalse(SPI.isEnabled());
}

test(SPI_Test2_MODE0_MSBFIRST_ReadWriteSucceedsWithoutUserIntervention) {
    // MODE0 with Chip Select defaulted to A2, manually using D2 instead.

    // Test init
    pinMode(D2, OUTPUT);
    digitalWrite(D2, HIGH);

    // When - Setting up the SPI bus
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV4);  // 72MHz / 4MHz = 18MHz
    SPI.begin(); // Chip Select line will default to init A2 as OUTPUT, however we're manually using D2

    // Then - SPI bus is enabled
    assertTrue(SPI.isEnabled());

    //===========================================================

    // Init array
    int8_t status = 0;
    char buf1[256];
    char buf2[256];
    memset(buf1, 0xaa, 256);
    memset(buf2, 0x55, 256);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertNotEqual(buf1[i], buf2[i]);
    }

    // When - Write 256 0x00's to FRAM
    status = FRAMWrite(D2, 0x00, (uint8_t*)buf1, arraySize(buf1));
    assertTrue(status == 0);

    // Then - Read 256 0x00's back into buf2 and test for equality
    status = FRAMRead(D2, 0x00, (uint8_t*)buf2, arraySize(buf1));
    assertTrue(status == 0);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertEqual(buf1[i], buf2[i]);
    }

    //===========================================================

    // Seed buf1 with 0 - 255
    for (uint16_t i=0; i<256; i++)
    {
        buf1[i] = i;
    }

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        // Serial.print(buf1[i],HEX);
        // Serial.print(" != ");
        // Serial.println(buf2[i],HEX);
        if (i != 0xaa) {
            assertNotEqual(buf1[i], buf2[i]);
        }
        else {
            assertEqual(buf1[i], buf2[i]);
        }
    }

    // When - Write 256 values to FRAM
    status = FRAMWrite(D2, 0x00, (uint8_t*)buf1, arraySize(buf1));
    assertTrue(status == 0);

    // Then - Read 256 values back into buf2 and test for equality
    status = FRAMRead(D2, 0x00, (uint8_t*)buf2, arraySize(buf1));
    assertTrue(status == 0);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertEqual(buf1[i], buf2[i]);
    }

    //===========================================================

    // When
    SPI.end();

    // Then - SPI bus is disabled
    assertFalse(SPI.isEnabled());
}

test(SPI_Test3_MODE3_MSBFIRST_ReadWriteSucceedsWithoutUserIntervention) {
    // MODE3, with Chip Select forced to D2

    // When - Setting up the SPI bus
    SPI.setDataMode(SPI_MODE3);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV4);  // 72MHz / 4MHz = 18MHz
    SPI.begin(D2); // Chip Select forced to D2

    // Then - SPI bus is enabled
    assertTrue(SPI.isEnabled());

    //===========================================================

    // Init array
    int8_t status = 0;
    char buf1[256];
    char buf2[256];
    memset(buf1, 0xaa, 256);
    memset(buf2, 0x55, 256);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertNotEqual(buf1[i], buf2[i]);
    }

    // When - Write 256 0x00's to FRAM
    status = FRAMWrite(D2, 0x00, (uint8_t*)buf1, arraySize(buf1));
    assertTrue(status == 0);

    // Then - Read 256 0x00's back into buf2 and test for equality
    status = FRAMRead(D2, 0x00, (uint8_t*)buf2, arraySize(buf1));
    assertTrue(status == 0);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertEqual(buf1[i], buf2[i]);
    }

    //===========================================================

    // Seed buf1 with 0 - 255
    for (uint16_t i=0; i<256; i++)
    {
        buf1[i] = i;
    }

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        // Serial.print(buf1[i],HEX);
        // Serial.print(" != ");
        // Serial.println(buf2[i],HEX);
        if (i != 0xaa) {
            assertNotEqual(buf1[i], buf2[i]);
        }
        else {
            assertEqual(buf1[i], buf2[i]);
        }
    }

    // When - Write 256 values to FRAM
    status = FRAMWrite(D2, 0x00, (uint8_t*)buf1, arraySize(buf1));
    assertTrue(status == 0);

    // Then - Read 256 values back into buf2 and test for equality
    status = FRAMRead(D2, 0x00, (uint8_t*)buf2, arraySize(buf1));
    assertTrue(status == 0);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertEqual(buf1[i], buf2[i]);
    }

    //===========================================================

    // When
    SPI.end();

    // Then - SPI bus is disabled
    assertFalse(SPI.isEnabled());
}

static void SPI_Test4_Prepare_SPI() {
    // When - Setting up the SPI bus
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockSpeed(100000); // 100kHz
    SPI.begin(D2); // Chip Select line will default to init A2 as OUTPUT, however we're manually using D2

    // pinMode() doesn't support Open-drain mode with Pull-up enabled
    pinMode(MISO, AF_OUTPUT_DRAIN);
    pinMode(MOSI, AF_OUTPUT_DRAIN);

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    pin_t gpio_pin = PIN_MAP[MISO].gpio_pin;
    GPIO_TypeDef *gpio_port = PIN_MAP[MISO].gpio_peripheral;

    // Force MISO and MOSI as Open-drain with pull-up
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(gpio_port, &GPIO_InitStructure);

    gpio_pin = PIN_MAP[MOSI].gpio_pin;
    gpio_port = PIN_MAP[MOSI].gpio_peripheral;
    GPIO_InitStructure.GPIO_Pin = gpio_pin;
    GPIO_Init(gpio_port, &GPIO_InitStructure);
}

test(SPI_Test4_MODE0_MSBFIRST_ReadWriteDMAFinishesBeforeCallback) {
    // MODE0 with Chip Select defaulted to A2, manually using D2 instead.
    // Test init
    pinMode(D2, OUTPUT);
    digitalWrite(D2, HIGH);

    SPI_Test4_Prepare_SPI();
    // Then - SPI bus is enabled
    assertTrue(SPI.isEnabled());

    //===========================================================

    // Init array
    int8_t status = 0;
    char buf1[256];
    char buf2[256];
    // 0xaf works better here since at 100kHz we might have enough time only to
    // cause data corruption in last byte on LSB nibble
    memset(buf1, 0xaf, 256);
    memset(buf2, 0x55, 256);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertNotEqual(buf1[i], buf2[i]);
    }

    // When - Write 256 0xaf's to FRAM
    status = FRAMWrite(D2, 0x00, (uint8_t*)buf1, arraySize(buf1));
    assertTrue(status == 0);

    // Then - Read 256 0xaf's back into buf2 and test for equality
    status = FRAMRead(D2, 0x00, (uint8_t*)buf2, arraySize(buf1));
    // Disable SPI and force MISO to go low immediately
    SPI.end();
    digitalWrite(MISO, LOW);
    assertTrue(status == 0);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        // Serial.print(buf1[i],HEX);
        // Serial.print(" != ");
        // Serial.println(buf2[i],HEX);
        assertEqual(buf1[i], buf2[i]);
    }

    // Then - SPI bus is disabled
    assertFalse(SPI.isEnabled());

    //===========================================================

    // Seed buf1 with 0 - 255
    for (uint16_t i=0; i<256; i++)
    {
        buf1[i] = i;
    }

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        if (i != 0xaf) {
            assertNotEqual(buf1[i], buf2[i]);
        }
        else {
            assertEqual(buf1[i], buf2[i]);
        }
    }

    SPI_Test4_Prepare_SPI();
    // Then - SPI bus is enabled
    assertTrue(SPI.isEnabled());


    // When - Write 256 values to FRAM
    status = FRAMWrite(D2, 0x00, (uint8_t*)buf1, arraySize(buf1));
    assertTrue(status == 0);

    // Then - Read 256 values back into buf2 and test for equality
    status = FRAMRead(D2, 0x00, (uint8_t*)buf2, arraySize(buf1));
    // Disable SPI and force MISO to go low immediately
    SPI.end();
    digitalWrite(MISO, LOW);
    assertTrue(status == 0);

    for (uint16_t i=0; i<arraySize(buf1); i++)
    {
        assertEqual(buf1[i], buf2[i]);
    }

    //===========================================================

    // Then - SPI bus is disabled
    assertFalse(SPI.isEnabled());
}

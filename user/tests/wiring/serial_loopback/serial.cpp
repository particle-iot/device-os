/**
 ******************************************************************************
 * @file    serial.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   SERIAL test application
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
#if (PLATFORM_ID == 0)
#include "Serial2/Serial2.h"
#endif
#include "unit-test/unit-test.h"

SYSTEM_MODE(MANUAL);

/*
 * Serial1 Test requires TX to be jumpered to RX as follows:
 * valid for both Core and Photon
 *           WIRE
 * (TX) --==========-- (RX)
 *
 * Serial2 Test requires D0 to be jumpered to D1 as follows:
 * only on Core (PLATFORM_ID = PLATFORM_SPARK_CORE)
 *
 *           WIRE
 * (D0) --==========-- (D1)
 *
 */

test(SERIAL_ReadWriteSucceedsWithUserIntervention) {
    //The following code will test all the important USB Serial routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial.print("Type the following message and press Enter : ");
    Serial.println(test);
    serialReadLine(&Serial, message, 9, 10000);//10 sec timeout
    Serial.println("");
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteSucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
    Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity8N1SucceedsInLoopbackWithTxRxShorted) {
        //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_8N1);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity8E1SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_8E1);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity8O1SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_8O1);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity8N2SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_8N2);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity8E2SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_8E2);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity8O2SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_8O2);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity9N1SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_9N1);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteParity9N2SucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial1 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, SERIAL_9N2);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
	Serial1.end();
    // then
    assertTrue(strncmp(test, message, 5)==0);
}


#if (PLATFORM_ID == 0)
test(SERIAL2_ReadWriteSucceedsInLoopbackWithD0D1Shorted) {
    //The following code will test all the important USART Serial2 routines
    char test[] = "hello";
    char message[10];
    // when
    if (Serial2.isEnabled())
        Serial2.end();
    Serial2.begin(9600);
    Serial2.println(test);
    serialReadLine(&Serial2, message, 9, 1000);//1 sec timeout
    // then
    assertTrue(strncmp(test, message, 5)==0);

    Serial2.end();
}
#endif

test(SERIAL1_AvailableForWriteWorksCorrectly) {
    Serial1.begin(9600);
    assertEqual(Serial1.isEnabled(), true);

    // Initially there should be SERIAL_BUFFER_SIZE available in TX buffer
    assertEqual(Serial1.availableForWrite(), SERIAL_BUFFER_SIZE);

    // Disable Serial1 IRQ to prevent it from sending data
    NVIC_DisableIRQ(USART1_IRQn);
    // Write (SERIAL_BUFFER_SIZE / 2) bytes into TX buffer
    for (int i = 0; i < SERIAL_BUFFER_SIZE / 2; i++) {
        Serial1.write('a');
    }
    // There should be (SERIAL_BUFFER_SIZE / 2) bytes available in TX buffer
    assertEqual(Serial1.availableForWrite(), SERIAL_BUFFER_SIZE / 2);

    // Write (SERIAL_BUFFER_SIZE / 2 - 1) bytes into TX buffer
    for (int i = 0; i < SERIAL_BUFFER_SIZE / 2 - 1; i++) {
        Serial1.write('b');
    }
    // There should only be 1 byte available in TX buffer
    assertEqual(Serial1.availableForWrite(), 1);
    // Enable Serial1 IRQ again to send out the data from TX buffer
    NVIC_EnableIRQ(USART1_IRQn);
    delay(100);

    // There should be SERIAL_BUFFER_SIZE available in TX buffer again
    assertEqual(Serial1.availableForWrite(), SERIAL_BUFFER_SIZE);

    // At this point tx_buffer->head = (SERIAL_BUFFER_SIZE - 1), tx_buffer->tail = (SERIAL_BUFFER_SIZE - 1)
    // Now test that availableForWrite() returns correct results for cases where tx_buffer->head < tx_buffer->tail
    // Disable Serial1 IRQ again to prevent it from sending data
    NVIC_DisableIRQ(USART1_IRQn);
    // Write (SERIAL_BUFFER_SIZE / 2 + 1) bytes into TX buffer
    for (int i = 0; i < SERIAL_BUFFER_SIZE / 2 + 1; i++) {
        Serial1.write('c');
    }
    // There should be (SERIAL_BUFFER_SIZE / 2) bytes available in TX buffer
    assertEqual(Serial1.availableForWrite(), SERIAL_BUFFER_SIZE / 2);
    // Enable Serial1 IRQ again to send out the data from TX buffer
    NVIC_EnableIRQ(USART1_IRQn);
    delay(100);

    // There should be SERIAL_BUFFER_SIZE available in TX buffer again
    assertEqual(Serial1.availableForWrite(), SERIAL_BUFFER_SIZE);

    Serial1.end();
}

test(SERIAL1_LINMasterReadWriteBreakSucceedsInLoopbackWithTxRxShorted) {
    // Test for LIN mode
    // 1. The test will configure Serial1 in LIN Master mode (with 11 bit break detection
    // enabled as in Slave mode)
    // 2. Send and detect 2 consecutive breaks
    // 3. Send/recv message
    // 4. Send and detect a break
    // 5. Send/recv message
    // 6. Send and detect a break

    char test[] = "hello";
    char message[10];
    memset(message, 0, sizeof(message));
    // when
    if (Serial1.isEnabled())
        Serial1.end();
    Serial1.begin(9600, LIN_MASTER_13B | LIN_BREAK_10B);
    // then

    // Send 2 consecutive breaks
    assertFalse(Serial1.breakRx());
    Serial1.breakTx();
    delay(1);
    assertTrue(Serial1.breakRx());

    assertFalse(Serial1.breakRx());
    Serial1.breakTx();
    delay(1);
    assertTrue(Serial1.breakRx());

    // Send message
    while(Serial1.available())
        (void)Serial1.read();
    Serial1.println(test);
    Serial1.flush();
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
    Serial1.println(message);
    assertTrue(strncmp(test, message, 5)==0);

    // Send break
    assertFalse(Serial1.breakRx());
    Serial1.breakTx();
    delay(1);
    assertTrue(Serial1.breakRx());

    // Send message
    while(Serial1.available())
        (void)Serial1.read();
    Serial1.println(test);
    Serial1.flush();
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
    Serial1.println(message);
    assertTrue(strncmp(test, message, 5)==0);

    // Send break
    assertFalse(Serial1.breakRx());
    Serial1.breakTx();
    delay(1);
    assertTrue(Serial1.breakRx());

    Serial1.end();
}

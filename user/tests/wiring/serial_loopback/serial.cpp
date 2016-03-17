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
    Serial2.begin(9600);
    Serial2.println(test);
    serialReadLine(&Serial2, message, 9, 1000);//1 sec timeout
    // then
    assertTrue(strncmp(test, message, 5)==0);
}
#endif

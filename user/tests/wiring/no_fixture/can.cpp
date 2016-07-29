/**
 ******************************************************************************
 * @file    can.cpp
 * @authors Julien Vanier
 * @version V1.0.0
 * @date    10-Jan-2015
 * @brief   CAN bus test application
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#ifdef HAL_HAS_CAN_D1_D2

test(CAN_01_D1_D2_ReceivesTransmittedMessage) {
    CANChannel can(CAN_D1_D2);
    can.begin(500000, CAN_TEST_MODE);

    CANMessage tx;
    tx.id = 0x101;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
    assertEqual(rx.id, 0x101);
    assertEqual(rx.len, 1);
    assertEqual(rx.data[0], 10);
}

test(CAN_02_D1_D2_ReceivesAcceptedMessage) {
    CANChannel can(CAN_D1_D2);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only standard message 0x100
    can.addFilter(0x100, 0x7FF);

    CANMessage tx;
    tx.id = 0x100;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
}

test(CAN_03_D1_D2_DoesntReceiveFilteredMessage) {
    CANChannel can(CAN_D1_D2);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only standard message 0x100
    can.addFilter(0x100, 0x7FF);

    CANMessage tx;
    tx.id = 0x101;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, false);
}

test(CAN_04_D1_D2_ReceivesTransmittedExtendedMessage) {
    CANChannel can(CAN_D1_D2);
    can.begin(500000, CAN_TEST_MODE);

    CANMessage tx;
    tx.extended = true;
    tx.id = 0x3000;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
    assertEqual(rx.extended, true);
    assertEqual(rx.id, 0x3000);
    assertEqual(rx.len, 1);
    assertEqual(rx.data[0], 10);
}

test(CAN_05_D1_D2_ReceivesAcceptedExtendedMessage) {
    CANChannel can(CAN_D1_D2);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only extended message 0x3000
    can.addFilter(0x3000, 0x7FF, CAN_FILTER_EXTENDED);

    CANMessage tx;
    tx.extended = true;
    tx.id = 0x3000;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
}

test(CAN_06_D1_D2_DoesntReceiveFilteredExtendedMessage) {
    CANChannel can(CAN_D1_D2);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only extended message 0x3000
    can.addFilter(0x3000, 0x7FF, CAN_FILTER_EXTENDED);

    CANMessage tx;
    tx.extended = true;
    tx.id = 0x3001;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, false);
}

#endif // HAL_HAS_CAN_D1_D2

#ifdef HAL_HAS_CAN_C4_C5

test(CAN_07_C4_C5_ReceivesTransmittedMessage) {
    CANChannel can(CAN_C4_C5);
    can.begin(500000, CAN_TEST_MODE);

    CANMessage tx;
    tx.id = 0x101;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
    assertEqual(rx.id, 0x101);
    assertEqual(rx.len, 1);
    assertEqual(rx.data[0], 10);
}

test(CAN_08_C4_C5_ReceivesAcceptedMessage) {
    CANChannel can(CAN_C4_C5);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only standard message 0x100
    can.addFilter(0x100, 0x7FF);

    CANMessage tx;
    tx.id = 0x100;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
}

test(CAN_09_C4_C5_DoesntReceiveFilteredMessage) {
    CANChannel can(CAN_C4_C5);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only standard message 0x100
    can.addFilter(0x100, 0x7FF);

    CANMessage tx;
    tx.id = 0x101;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, false);
}

test(CAN_10_C4_C5_ReceivesTransmittedExtendedMessage) {
    CANChannel can(CAN_C4_C5);
    can.begin(500000, CAN_TEST_MODE);

    CANMessage tx;
    tx.extended = true;
    tx.id = 0x3000;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
    assertEqual(rx.extended, true);
    assertEqual(rx.id, 0x3000);
    assertEqual(rx.len, 1);
    assertEqual(rx.data[0], 10);
}

test(CAN_11_C4_C5_ReceivesAcceptedExtendedMessage) {
    CANChannel can(CAN_C4_C5);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only extended message 0x3000
    can.addFilter(0x3000, 0x7FF, CAN_FILTER_EXTENDED);

    CANMessage tx;
    tx.extended = true;
    tx.id = 0x3000;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, true);
}

test(CAN_12_C4_C5_DoesntReceiveFilteredExtendedMessage) {
    CANChannel can(CAN_C4_C5);
    can.begin(500000, CAN_TEST_MODE);
    // Accept only extended message 0x3000
    can.addFilter(0x3000, 0x7FF, CAN_FILTER_EXTENDED);

    CANMessage tx;
    tx.extended = true;
    tx.id = 0x3001;
    tx.len = 1;
    tx.data[0] = 10;
    can.transmit(tx);
    delay(1);
    CANMessage rx;
    bool hasMessage = can.receive(rx);
    can.end();

    assertEqual(hasMessage, false);
}

#endif // HAL_HAS_CAN_C4_C5

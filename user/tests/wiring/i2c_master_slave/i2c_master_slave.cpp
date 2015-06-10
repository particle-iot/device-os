/**
 ******************************************************************************
 * @file    i2c_master_slave.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    24-Mar-2015
 * @brief   I2C Master<=>Slave back-to-back test application
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#define MASTER_TEST_MESSAGE "Hello from I2C Master!!"
#define SLAVE_TEST_MESSAGE  "I2C Slave is doing good"
#define TEST_MESSAGE_SIZE   23

char Master_Received_Message[TEST_MESSAGE_SIZE];
char Slave_Received_Message[TEST_MESSAGE_SIZE];
int count = 0;

void I2C_Slave_ReceiveEvent_Callback(int howMany)
{
    count = 0;
    //loop through all available data
    while (Wire.available())
    {
        //store the received byte as character
        Slave_Received_Message[count++] = Wire.read();
    }
}

void I2C_Slave_RequestEvent_Callback(void)
{
    //respond with SLAVE_TEST_MESSAGE
    Wire.write(SLAVE_TEST_MESSAGE);
}

void I2C_Slave_Reader_Execute(void)
{
    //join i2c bus with address #4
    Wire.begin(4);

    //register event
    Wire.onReceive(I2C_Slave_ReceiveEvent_Callback);

    Serial.println("Start the Master Writer Now or hit a key to exit...");

    while(!Serial.available());

    assertTrue(strncmp(MASTER_TEST_MESSAGE, Slave_Received_Message, TEST_MESSAGE_SIZE)==0);
}

void I2C_Master_Writer_Execute(void)
{
    //join i2c bus as master
    Wire.begin();

    //transmit to slave device #4
    Wire.beginTransmission(4);

    //send MASTER_TEST_MESSAGE bytes
    Wire.write(MASTER_TEST_MESSAGE);

    // stop transmitting
    Wire.endTransmission();

    Serial.println("Start the Slave Reader First...");
}

void I2C_Slave_Writer_Execute(void)
{
    //join i2c bus with address #2
    Wire.begin(2);

    //register event
    Wire.onRequest(I2C_Slave_RequestEvent_Callback);

    Serial.println("Start the Master Reader Now...");
}

void I2C_Master_Reader_Execute(void)
{
    //join i2c bus as master
    Wire.begin();

    //request TEST_MESSAGE_SIZE bytes from slave device #2
    Wire.requestFrom(2, TEST_MESSAGE_SIZE);

    Serial.println("Start the Slave Writer First...");

    count = 0;
    while(count < TEST_MESSAGE_SIZE)
    {
        //slave may send less than requested
        if (Wire.available())
        {
            //store the received byte as character
            Master_Received_Message[count++] = Wire.read();
        }
    }

    assertTrue(strncmp(SLAVE_TEST_MESSAGE, Master_Received_Message, TEST_MESSAGE_SIZE)==0);
}

test(I2C_Master_Slave_All_OK)
{
    //I2C_Slave_Writer_Execute();
    //I2C_Master_Reader_Execute();

    //I2C_Slave_Reader_Execute();
    //I2C_Master_Writer_Execute();
}

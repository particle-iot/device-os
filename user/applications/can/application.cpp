/**
 ******************************************************************************
 * @file    application.cpp
 * @authors Brian Spranger
 * @version V1.0.1
 * @date    15 October 2015
 * @brief   CAN Test application
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "application.h"

int led2 = D7;

/* Function prototypes -------------------------------------------------------*/
void PrintCanMsg(CAN_Message_Struct *pMessage);

SYSTEM_MODE(MANUAL);

/* This function is called once at start up ----------------------------------*/
void setup()
{
	pinMode(led2, OUTPUT);
    Serial.begin(9600);
    Can1.begin(125000);

}

/* This function loops forever --------------------------------------------*/
void loop()
{
    CAN_Message_Struct Message;
	
	Message.Ext = true;
    Message.ID = 0x1234UL;
	Message.rtr = false;
    Message.Len = 8U;
    Message.Data[0]='h';
    Message.Data[1]='e';
	Message.Data[2]='l';
	Message.Data[3]='l';
	Message.Data[4]='0';
	Message.Data[5]=' ';
	Message.Data[6]='W';
	Message.Data[7]='!';
	Serial.println("Writing CAN");
	delay(500);
	digitalWrite(led2, HIGH);
	Can1.write(&Message);
	

	delay(500);
	digitalWrite(led2, LOW);
	if (1 == Can1.read(&Message))
	{
		Serial.println("Read CAN");
		PrintCanMsg(&Message);
	}
    
	
	
	
}

void PrintCanMsg(CAN_Message_Struct *pMessage)
{
	Serial.print("ID = ");
	Serial.print(pMessage->ID);
	Serial.print(" ");
	Serial.println(pMessage->Ext);
	Serial.print("Len = ");
	Serial.println(pMessage->Len);
	Serial.print("Data = ");
	Serial.print(pMessage->Data[0]);
	Serial.print(", ");
	Serial.print(pMessage->Data[1]);
	Serial.print(", ");
	Serial.print(pMessage->Data[2]);
	Serial.print(", ");
	Serial.print(pMessage->Data[3]);
	Serial.print(", ");
	Serial.print(pMessage->Data[4]);
	Serial.print(", ");
	Serial.print(pMessage->Data[5]);
	Serial.print(", ");
	Serial.print(pMessage->Data[6]);
	Serial.print(", ");
	Serial.println(pMessage->Data[7]);
}
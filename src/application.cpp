//#define TIMER_T
#if defined(TIMER_T)
#include "application.h"

uint16_t pin = D2;
uint32_t wait = 20;

void debug_output_(const char *p)
{
  static boolean once = false;
 if (!once)
   {
     once = true;
     Serial1.begin(115200);
   }

 Serial1.print(p);
}

void setup() {
  pinMode(D2,OUTPUT);
  digitalWrite(D2,HIGH);
}

#include <limits.h>
#define MAX_MICROS  (UINT_MAX / SYSTEM_US_TICKS)
void loop() {
    // Print out micros() timer so we know when it's coming
    //DEBUG(micros());
    // if greater than 59.6 seconds
    if(micros() > (MAX_MICROS - 1000000)) {
        digitalWrite(D2,LOW);

        // disable interrupts
        __disable_irq();

        // hard loop to wait for just the right time
        while(micros() < (MAX_MICROS - 200));
        uint32_t s = micros() ;


        // execute 20 well placed pulses with 20us delays and look for glitches
        // right when the DWT counter is going to wrap.
        // In our case, we see a very fast delay (1us) for several
        // iterations after the 8th pulse instead of the proper 20us delay.
        for(uint8_t x=0; x<30; x++) {
            PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin; // HIGH
            delayMicroseconds(5);
            PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin;  // LOW
            delayMicroseconds(5);
        }
        uint32_t t = micros() ;
        __enable_irq();
        DEBUG("start %d, end %d",s,t);
    }
}
#else
/**
 ******************************************************************************
 * @file    application.cpp
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    05-November-2013
 * @brief   Tinker application
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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


#define RENEW_INTERVAL      5*1000      // 30 secs
#define RETRY_INTERVAL      5*1000      // 10 secs
#define RESPONSE_INTERVAL   1*1000       // 1 sec
#define LET_IT_FILL_INTERVAL   3*1000       // 1 sec

TCPClient client;
char server[] = "nscdg.com";

// IO
int led = D2;

// Globals
volatile int state = 0;
volatile int wait = 0;
volatile int loopwait = 10;

volatile int tries = 0;
volatile int store = 0;
volatile int hash = 0;

volatile char command[32];
volatile int command_i=0;


#include "application.h"

uint8_t buffer[TCPCLIENT_BUF_MAX_SIZE+1]; // for EOT
int loops = 0;
int total = 0;
int wcl=2;


#define SERVER_IP 10,10,0,1
//------------------------------
#define SERVER_PORT 9999
#define WAIT_TIME 3000

UDP udpClient;
IPAddress serverAddress(SERVER_IP);

void setup2() {
  pinMode(D2,OUTPUT);
  pinMode(D1,OUTPUT);
}

system_tick_t lastMillis = (system_tick_t) -WAIT_TIME;

char buf[50];


void loop2() {
    if(millis() > lastMillis + WAIT_TIME){
        udpClient.begin(SERVER_PORT);
        udpClient.beginPacket(serverAddress,SERVER_PORT);
        snprintf(buf, sizeof(buf),"tick %ld", lastMillis);
        udpClient.write(buf);
        udpClient.endPacket();
        lastMillis = millis();
        DEBUG("tick ************ %ld",lastMillis);
        digitalWrite(D2,HIGH);
        delay(20);
        digitalWrite(D2,LOW);
        udpClient.stop();

  }
}


UDP Udp2;
void setup3()
{

    DEBUG("UDB RX");

    pinMode(led, OUTPUT);
    // Button resistorless
    state = 0;
    wait = RETRY_INTERVAL;
    Udp2.begin(8738);
    // Connecting
}

void loop3()
{
      delay(1);
      if (++loops == 100) {
          loops = 0;
        int packetSize = Udp2.parsePacket();
        DEBUG("parsePacket %d", packetSize);
        if(packetSize > 0)
        {
          int count = Udp2.read(buffer,sizeof(buffer));
          buffer[count] = '\0';
          DEBUG("read %d %s", count, buffer);
          total += count;
        }
      }
}




int bad_mod = 0;
int bad_every = 0;
void setup1()
{

    LOG("Test TCP BAD Every %d Usage!",bad_every);

    pinMode(led, OUTPUT);
    // Button resistorless
    state = 0;
    wait = RETRY_INTERVAL;

    // Connecting
}

void loop1()
{
    delay(1);
    switch(state){
        case 0:
            // Waiting for next time
            wait-=10;
            if(wait<0){
                wait = 0;
                state = 1;
            }
            break;
        case 1:
            // Connecting
            bad_mod++;
            DEBUG("connecting");
            total = 0;
            if (client.connect(server, 80)){
                state = 2;
            }else{
                DEBUG("connection failed state 1");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 2:
            // Requesting
            if(client.connected()){
                DEBUG (" Send");
                client.println("GET /t.php HTTP/1.0\r\n\r\n");
                if (bad_every  && ((bad_mod % bad_every) == 0))
                  {
                    DEBUG (" Sent but not Reading it!");
                    wait = 1000 * 18; // longer then spark com time
                    state = -2;
                  } else {
                      DEBUG (" Sent Doing Read");
                      wait = RETRY_INTERVAL;
                      state = -3;

                  }
            }else{
                DEBUG("connection lost state 2");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;

        case -2:
          if ((wait % 500) ==0) {
              DEBUG("Waiting client.status()=%d",client.status()) ;
          }
#if defined(GOOD)
          if (!client.status()) {
              state = 4;
              DEBUG("Waiting Aborted - Closing") ;
          }
#endif
          wait--;

          if(wait<0){
              wait = 0;
              state = 4;
          }
          break;

        case -3:
          if (client.available()) {
              DEBUG ("Ready");
              state = 3;
              loops = 0;
          }
          wait--;
          if(wait<0){
              wait = 0;
              state = 4;
          }
          break;

        case 3: {
          if(!client.connected()){
              wait = 1;
              state = 4;
          } else {
              // Receiving
            int count = client.available();
            if (count) {
                  DEBUG("client.available() %d", count);
                  // Print response to serial
                  DEBUG("client.peek %d", client.peek());

                  count = client.read(buffer, arraySize(buffer));
                  buffer[count] ='\0';
                  char *p = strstr((const char *)buffer,"0.1.2.3");
                  if (p)
                    {
                     total = -(p-(char*)buffer);
                    }
                  total += count;
                  DEBUG("client.read() %d", count);
                  debug_output_((const char*)buffer);
                  debug_output_("\r\n");
             }
          }
        }
            break;
        case 4:
          DEBUG("\r\n\r\n");
          DEBUG("%d total Bytes Read\r\n\r\n",total);
            // Disconnecting
            if(client.connected()){
                client.stop();
                DEBUG("connection closed state 4");
                wait = RENEW_INTERVAL;
                state = 0;
            }else{
                DEBUG("connection closed by server state 4");
                wait = RENEW_INTERVAL;
                state = 0;
            }
            break;
      }
}



/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

/* This function is called once at start up ----------------------------------*/
void setup()
{
	//Setup the Tinker application here
        DECLARE_SYS_HEALTH(PRESERVE_APP); // toDo LTE uses system policy inline as wrappers
        typedef void(*pv)(void);
#ifndef DIE
          int k = 0;
          pv p = 0;
          p();
#endif
#ifdef HANG
          __disable_irq();
          while(1) {};
#endif
	//Register all the Tinker functions

        setup1();
        setup2();
        setup3();
	Spark.function("digitalread", tinkerDigitalRead);
	Spark.function("digitalwrite", tinkerDigitalWrite);

	Spark.function("analogread", tinkerAnalogRead);
	Spark.function("analogwrite", tinkerAnalogWrite);

}

/* This function loops forever --------------------------------------------*/
void loop()
{
	loop1();
	loop2();
	loop3();
	//This will run in a loop
}

void debug_output_(const char *p)
{
  static boolean once = false;
 if (!once)
   {
     once = true;
     Serial1.begin(115200);
   }

 Serial1.print(p);
}


/*********************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, INPUT_PULLDOWN);
		return digitalRead(pinNumber);
	}
	else if (pin.startsWith("A"))
	{
		pinMode(pinNumber+10, INPUT_PULLDOWN);
		return digitalRead(pinNumber+10);
	}
	return -2;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerDigitalWrite(String command)
{
	bool value = 0;
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(command.substring(3,7) == "HIGH") value = 1;
	else if(command.substring(3,6) == "LOW") value = 0;
	else return -2;

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		digitalWrite(pinNumber, value);
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		digitalWrite(pinNumber+10, value);
		return 1;
	}
	else return -3;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, INPUT);
		return analogRead(pinNumber);
	}
	else if (pin.startsWith("A"))
	{
		pinMode(pinNumber+10, INPUT);
		return analogRead(pinNumber+10);
	}
	return -2;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogWrite
 * Description    : Writes an analog value (PWM) to the specified pin
 * Input          : Pin and Value (0 to 255)
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerAnalogWrite(String command)
{
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	String value = command.substring(3);

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		analogWrite(pinNumber, value.toInt());
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		analogWrite(pinNumber+10, value.toInt());
		return 1;
	}
	else return -2;
}
#endif

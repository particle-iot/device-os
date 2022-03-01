/**
 ******************************************************************************
 * @file    udp_ntp_client.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    10-April-2015
 * @brief   UDP NTP Client test application
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

unsigned int localPort = 8888;      // local port to listen for UDP packets

IPAddress timeServer;
// time-a.timefreq.bldrdoc.gov NTP server
// time-b.timefreq.bldrdoc.gov NTP server
// time-c.timefreq.bldrdoc.gov NTP server

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

static bool udpBound = false;

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer,NTP_PACKET_SIZE);
    Udp.endPacket();
}

unsigned long getNTPClientTime(void)
{
    unsigned long ntpEpochTime = 0;

    if(!udpBound)
    {
        Udp.begin(localPort);
        udpBound = true;
    }

    uint32_t _millis = millis();

    // timeout after 10 secs
    while(millis() - _millis < 10000)
    {
        // timeServer = Network.resolve("time-a.timefreq.bldrdoc.gov");
        timeServer = Network.resolve("0.pool.ntp.org");

        sendNTPpacket(timeServer); // send an NTP packet to a time server

        delay(1000);

        if (Udp.parsePacket())
        {
            // read the received packet into the buffer
            Udp.read(packetBuffer, NTP_PACKET_SIZE);

            // the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, extract the two words:
            unsigned long highWord = packetBuffer[40] << 8 | packetBuffer[41];
            unsigned long lowWord = packetBuffer[42] << 8 | packetBuffer[43];

            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;

            // convert NTP time into everyday time: subtract seventy years:
            ntpEpochTime = secsSince1900 - 2208988800UL;

            break;
        }
    }

    return ntpEpochTime;
}

void setup()
{
    while (!Serial.isConnected()) {
        Particle.process();
    }

    // Request time synchronization from the Spark Cloud
    Particle.syncTime();

    // get NTP time
    unsigned long ntpEpochTime = getNTPClientTime();

    // get spark time
    unsigned long sparkEpochTime = Time.now();

    // print time received from both the methods:
    //Serial.println(ntpEpochTime);
    //Serial.println(Time.timeStr(ntpEpochTime));

    //Serial.println(sparkEpochTime);
    //Serial.println(Time.timeStr(sparkEpochTime));

    //Test passes if the time difference is < 100 sec
    if (ntpEpochTime > sparkEpochTime)
    {
        if ( (ntpEpochTime - sparkEpochTime) < 100) {
            Serial.printlnf("PASS: %lu < 100", (ntpEpochTime - sparkEpochTime));
        } else {
            Serial.printlnf("FAIL: %lu >= 100", (ntpEpochTime - sparkEpochTime));
        }
    }
    else
    {
        if ( (sparkEpochTime - ntpEpochTime) < 100) {
            Serial.printlnf("PASS: %lu < 100", (sparkEpochTime - ntpEpochTime));
        } else {
            Serial.printlnf("FAIL: %lu >= 100", (sparkEpochTime - ntpEpochTime));
        }
    }
}
/**
 ******************************************************************************
 * @file    cloud.cpp
 * @authors Matthew McGowan
 * @date    21 January 2015
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

void idle()
{
    Spark.process();
}

bool disconnect(uint32_t timeout=15000)
{
    uint32_t start = millis();
    while (Spark.connected() && (millis()-start)<timeout) {
        Spark.disconnect();
        idle();
    }
    return !Spark.connected();
}

bool connect(uint32_t timeout=150000)
{
    uint32_t start = millis();
    while (!Spark.connected() && (millis()-start)<timeout) {
        Spark.connect();
        idle();
    }
    return Spark.connected();
}

test(Spark_Publish_Silently_Fails_When_Not_Connected) {
    disconnect();
    Spark.publish("hello", "world", 60, PRIVATE);
}

int not_connected_handler_count;
void Spark_Subscribe_When_Not_Connected_Handler(const char* topic, const char* data) {
    String deviceId = Spark.deviceID();
    Serial.println("event ***");
    if (data) Serial.println(data);
    if (data && !strcmp(data, deviceId.c_str())) {
        not_connected_handler_count++;
    }
}

test(Spark_Subscribe_When_Not_Connected) {
    disconnect();
    Spark.unsubscribe();
    not_connected_handler_count = 0;   
    const char* eventName = "test/Spark_Subscribe_When_Not_Connected";
    assertTrue(Spark.subscribe(eventName, Spark_Subscribe_When_Not_Connected_Handler));
    
    assertEqual(not_connected_handler_count, 0);
    connect();
    String deviceID = Spark.deviceID();
    Spark.publish(eventName, deviceID.c_str());
    
    long start = millis();
    while ((millis()-start)<30000 && !not_connected_handler_count)
        idle();
    
    assertEqual(not_connected_handler_count, 1);    
}

void Subscribe_To_Same_Event_Is_No_Op_Handler(const char* topic, const char* data) {
    
}

test(Subscribe_To_Same_Event_Is_No_Op) {
    int success = 0;
    disconnect();
    Spark.unsubscribe();            // unsubscribe all
    for (int i=0; i<10; i++) {
        if (Spark.subscribe("test/Subscribe_To_Same_Event_Is_No_Op", Subscribe_To_Same_Event_Is_No_Op_Handler))
            success++;
    }
    assertEqual(success,10);

    connect();
    // now see if we can subscribe to an additional event 
    not_connected_handler_count = 0;   
    const char* eventName = "test/Spark_Subscribe_When_Not_Connected2";
    assertTrue(Spark.subscribe(eventName, Spark_Subscribe_When_Not_Connected_Handler));
    assertEqual(not_connected_handler_count, 0);
    
    String deviceID = Spark.deviceID();
    Spark.publish(eventName, deviceID.c_str());
    
    long start = millis();
    while ((millis()-start)<30000 && !not_connected_handler_count)
        idle();
    
    assertEqual(not_connected_handler_count, 1);    
}


test(Spark_Unsubscribe) {
    disconnect();
    Spark.unsubscribe();
    not_connected_handler_count = 0;   
    const char* eventName = "test/Spark_Unsubscribe";
    assertTrue(Spark.subscribe(eventName, Spark_Subscribe_When_Not_Connected_Handler));
    
    assertEqual(not_connected_handler_count, 0);
    connect();
    String deviceID = Spark.deviceID();
    Spark.publish(eventName, deviceID.c_str());
    
    long start = millis();
    while ((millis()-start)<30000 && !not_connected_handler_count)
        idle();
    
    assertEqual(not_connected_handler_count, 1);    
    
    not_connected_handler_count = 0;
    Spark.unsubscribe();
    Spark.publish(eventName, deviceID.c_str());
    start = millis();
    while ((millis()-start)<10000 && !not_connected_handler_count)
        idle();

    // no further events received
    assertEqual(not_connected_handler_count, 0);        
}

/**
 * The event decoding logic modified the contents of the receive buffer, so that
 * the when looping a second time, the event name comprised only the first part up to the first slash.
 * E.g. the event "test/event" would be match like that against the first handler, but
 * subsequent handlers would be matched only against "test". 
 */
test(Spark_Second_Event_Handler_Not_Matched) {
    disconnect();
    Spark.unsubscribe();   
    connect();
    
    Spark.subscribe("test/event1", Subscribe_To_Same_Event_Is_No_Op_Handler);
    Spark.subscribe("test/event2", Spark_Subscribe_When_Not_Connected_Handler);
    not_connected_handler_count = 0;
    
    String deviceID = Spark.deviceID();    
    Spark.publish("test/event2", deviceID.c_str());
    
    // now wait for published event to be received
    long start = millis();
    while ((millis()-start)<30000 && !not_connected_handler_count)
        idle();
    
    assertEqual(not_connected_handler_count, 1);    
}
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

uint32_t publish_timeout = 90000;

void idle()
{
    Particle.process();
}

bool disconnect(uint32_t timeout=15000)
{
    uint32_t start = millis();
    while (Particle.connected() && (millis()-start)<timeout) {
        Particle.disconnect();
        idle();
    }
    return !Particle.connected();
}

bool connect(uint32_t timeout=150000)
{
    uint32_t start = millis();
    while (!Particle.connected() && (millis()-start)<timeout) {
        Particle.connect();
        idle();
    }
    return Particle.connected();
}

test(Spark_Publish_Silently_Fails_When_Not_Connected) {
    disconnect();
    Particle.publish("hello", "world", 60, PRIVATE);
}

int not_connected_handler_count;
void Spark_Subscribe_When_Not_Connected_Handler(const char* topic, const char* data) {
    String deviceId = Particle.deviceID();
    Serial.println("event ***");
    if (data) Serial.println(data);
    if (data && !strcmp(data, deviceId.c_str())) {
    		not_connected_handler_count++;
    }
}

test(Spark_Subscribe_When_Not_Connected_Recieves_Events_When_Connected) {
    disconnect();
    Particle.unsubscribe();
    not_connected_handler_count = 0;
    const char* eventName = "test/Spark_Subscribe_When_Not_Connected";
    assertTrue(Particle.subscribe(eventName, Spark_Subscribe_When_Not_Connected_Handler));

    assertEqual(not_connected_handler_count, 0);
    connect();
    String deviceID = Particle.deviceID();
    Particle.publish(eventName, deviceID.c_str());

    long start = millis();
    while ((millis()-start)<publish_timeout && !not_connected_handler_count)
        idle();

    assertEqual(not_connected_handler_count, 1);
}

void Subscribe_To_Same_Event_Is_No_Op_Handler(const char* topic, const char* data) {

}

test(Subscribe_To_Same_Event_Is_No_Op) {
    int success = 0;
    disconnect();
    Particle.unsubscribe();            // unsubscribe all
    for (int i=0; i<10; i++) {
        if (Particle.subscribe("test/Subscribe_To_Same_Event_Is_No_Op", Subscribe_To_Same_Event_Is_No_Op_Handler))
            success++;
    }
    assertEqual(success,10);

    connect();
    // now see if we can subscribe to an additional event
    not_connected_handler_count = 0;
    const char* eventName = "test/Spark_Subscribe_When_Not_Connected2";
    assertTrue(Particle.subscribe(eventName, Spark_Subscribe_When_Not_Connected_Handler));
    assertEqual(not_connected_handler_count, 0);

    String deviceID = Particle.deviceID();
    Particle.publish(eventName, deviceID.c_str());

    long start = millis();
    while ((millis()-start)<30000 && !not_connected_handler_count)
        idle();

    assertEqual(not_connected_handler_count, 1);
}


test(Spark_Unsubscribe) {
    disconnect();
    Particle.unsubscribe();
    not_connected_handler_count = 0;
    const char* eventName = "test/Spark_Unsubscribe";
    assertTrue(Particle.subscribe(eventName, Spark_Subscribe_When_Not_Connected_Handler));

    assertEqual(not_connected_handler_count, 0);
    connect();
    String deviceID = Particle.deviceID();
    Particle.publish(eventName, deviceID.c_str());

    long start = millis();
    while ((millis()-start)<publish_timeout && !not_connected_handler_count)
        idle();

    assertEqual(not_connected_handler_count, 1);

    not_connected_handler_count = 0;
    Particle.unsubscribe();
    Particle.publish(eventName, deviceID.c_str());
    start = millis();
    while ((millis()-start)<publish_timeout && !not_connected_handler_count)
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
    Particle.unsubscribe();
    connect();

    Particle.subscribe("test/event1", Subscribe_To_Same_Event_Is_No_Op_Handler);
    Particle.subscribe("test/event2", Spark_Subscribe_When_Not_Connected_Handler);
    not_connected_handler_count = 0;

    String deviceID = Particle.deviceID();
    Particle.publish("test/event2", deviceID.c_str());

    // now wait for published event to be received
    long start = millis();
    while ((millis()-start)<publish_timeout && !not_connected_handler_count)
        idle();

    assertEqual(not_connected_handler_count, 1);
}

class Subscriber {
  public:
    void subscribe() {
      assertTrue(Particle.subscribe("test/event3", &Subscriber::handler, this));
      // To make sure calling subscribe with a different handler is not a no op
      assertTrue(Particle.subscribe("test/event3", &Subscriber::handler2, this));
      assertTrue(Particle.subscribe("test/eventmine", &Subscriber::handler3, this, MY_DEVICES));

      receivedCount = 0;
      mineCount = 0;
    }
    void handler3(const char *eventName, const char *data) {
      mineCount++;
    }

    void handler(const char *eventName, const char *data) {
      receivedCount++;
    }
    void handler2(const char *eventName, const char *data) {
      receivedCount++;
    }
    int receivedCount;
    int mineCount;
} subscriber;

test(Subscribe_With_Object) {
    disconnect();
    Particle.unsubscribe();
    connect();

    subscriber.subscribe();

    String deviceID = Particle.deviceID();
    Particle.publish("test/event3");

    // now wait for published event to be received
    long start = millis();
    while ((millis()-start)<publish_timeout && !subscriber.receivedCount)
        idle();

    assertEqual(subscriber.receivedCount, 2);
}

/**
 * Subscribing to All events shows public events matching the name.
 */
test(all_events_subscription)
{
	disconnect();
	Particle.unsubscribe();
	connect();

	// public events
    subscriber.subscribe();

    Particle.publish("test/eventmine");	// my devices subscription
    Particle.publish("test/event3");

    // now wait for published event to be received
    long start = millis();
    while ((millis()-start)<publish_timeout && !subscriber.receivedCount)
        idle();

    // the public test/event3 is received by ALL_DEVICES subscription
    assertEqual(subscriber.receivedCount, 2);
    // the public test/event4 is not received by MY_DEVICES subscription
    assertEqual(subscriber.mineCount, 0);

}

/**
 * Subscribing to All events shows public events matching the name.
 */
test(mine_events_subscription)
{
	disconnect();
	Particle.unsubscribe();
	connect();

    subscriber.subscribe();

    Particle.publish("test/eventmine", "", PRIVATE);	// my devices subscription
    Particle.publish("test/event3", "", PRIVATE);

    // now wait for published event to be received
    long start = millis();
    while ((millis()-start)<publish_timeout && !subscriber.mineCount)
        idle();

    // the private test/event3 is not received by ALL_DEVICES subscription
    assertEqual(subscriber.receivedCount, 0);
    // the private test/event4 is received by MY_DEVICES subscription
    assertEqual(subscriber.mineCount, 1);
}


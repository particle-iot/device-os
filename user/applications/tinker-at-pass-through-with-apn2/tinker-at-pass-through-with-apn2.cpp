/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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
#include "stdarg.h"
#include "cellular_hal.h"

PRODUCT_ID(PLATFORM_ID);
PRODUCT_VERSION(2);

// #define PARTICLE_CLOUD 1

// AT commands received over this serial interface
#define MY_SERIAL Serial1

SerialLogHandler logHandler(115200, LOG_LEVEL_ALL);

// UDP Port used for two way communication
unsigned int localPort = 5549;
IPAddress serverIPaddress( 0,0,0,0 ); // IMPORTANT SET THIS!!!

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

bool echoTime = false;
bool pingTime = false;
bool ipaddrTime = false;

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

SYSTEM_MODE(SEMI_AUTOMATIC);

void serial_at_response_out(void* data, const char* msg)
{
   MY_SERIAL.print(msg); // AT command response sent back over Serial
}

STARTUP(
    cellular_at_response_handler_set(serial_at_response_out, NULL, NULL);
    // cellular_credentials_set("spark.telefonica.com", "", "", NULL);
    cellular_credentials_set("wireless.twilio.com", "", "", NULL);
);

void sendPing() {
    pingTime = true;
}

Timer pingMe(55000, sendPing);

void relayHandler(const char *event, const char *data)
{
    if (strcmp(data,"time")==0) {
        echoTime = true;
    }
    else if (strcmp(data,"pingoff")==0) {
        pingTime = false;
        pingMe.stop();
    }
    else if (strcmp(data,"pingon")==0) {
        pingTime = true;
        pingMe.start();
    }
    else if (strcmp(data,"ipaddr")==0) {
        ipaddrTime = true;
    }
    else {
        LOG(INFO,"period: %d", String(data).toInt());
        pingMe.changePeriod(String(data).toInt());
        pingTime = true;
    }
}

#define LOREM "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque ut elit nec mi bibendum mollis. Nam nec nisl mi. Donec dignissim iaculis purus, ut condimentum arcu semper quis. Phasellus efficitur ut arcu ac dignissim. In interdum sem id dictum luctus. Ut nec mattis sem. Nullam in aliquet lacus. Donec egestas nisi volutpat lobortis sodales. Aenean elementum magna ipsum, vitae pretium tellus lacinia eu. Phasellus commodo nisi at quam tincidunt, tempor gravida mauris facilisis. Duis tristique ligula ac pulvinar consectetur. Cras aliquam, leo ut eleifend molestie, arcu odio semper odio, quis sollicitudin metus libero et lorem. Donec venenatis congue commodo. Vivamus mattis elit metus, sed fringilla neque viverra eu. Phasellus leo urna, elementum vel pharetra sit amet, auctor non sapien. Phasellus at justo ac augue rutrum vulputate. In hac habitasse platea dictumst. Pellentesque nibh eros, placerat id laoreet sed, dapibus efficitur augue. Praesent pretium diam ac sem varius fermentum. Nunc suscipit dui risus sed"

void echoTest() {
    const char request[] =
        "POST /post HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/form-data; boundary=-------------aaaaaaaa\r\n"
        "Content-Length: 1124\r\n"
        "\r\n"
        "---------------aaaaaaaa\r\n"
        "Content-Disposition: form-data; name=\"field\"\r\n"
        "\r\n"
        LOREM "\r\n"
        "---------------aaaaaaaa--\r\n";
    const int requestSize = sizeof(request) - 1;

    // Cellular.connect();
    // if (!waitFor(Cellular.ready, 400000)) {
    //     System.sleep(SLEEP_MODE_SOFTPOWEROFF, 60); // attempt power cycling
    // }

    TCPClient c;
    int res = c.connect("httpbin.org", 80);
    (void)res;

    int sz = c.write((const uint8_t*)request, requestSize);
    if(sz != requestSize) {
        // increment the echo test error count if the request didn't send
        LOG(INFO,"\r\nSent request error!!!");
        // Serial1.println("\r\nSent request error!!!");
        return;
    }

    char* responseBuf = new char[2048];
    memset(responseBuf, 0, 2048);
    int responseSize = 0;
    uint32_t mil = millis();
    while(1) {
        while (c.available()) {
            responseBuf[responseSize++] = c.read();
        }
        if (!c.connected())
            break;
        if (millis() - mil >= 60000) {
            break;
        }
    }

    bool contains = false;
    if (responseSize > 0 && !c.connected()) {
        contains = strstr(responseBuf, LOREM) != nullptr;
    }

    delete responseBuf;

    // increment the echo test error count if the response doesn't match
    if (!contains) {
        LOG(INFO,"\r\nMatch error!!!");
        // Serial1.println("\r\nMatch error!!!");
    }
    else {
        LOG(INFO,"\r\nMatched :)");
        // Serial1.println("\r\nMatched :)");
    }
}

void blink_led() {
    digitalWrite(D7, !digitalRead(D7));
}

Timer blinky(100, blink_led);

void callHomeUDP()
{
    static String msg;
    static int count = 1;
    LOG(INFO,"UDP HELLO SENDING\r\n");
    Udp.beginPacket(serverIPaddress, localPort);
    msg = "hello from electron " + String(count++);
    Udp.write(msg);
    Udp.endPacket();
    LOG(INFO,"UDP HELLO SENT\r\n");
}

void checkUDP() {
  // Check if data has been received
  if (Udp.parsePacket() > 0) {

    // Read first char of data received
    char c = Udp.read();

    String temp = String(c);

    // Ignore other chars
    while(Udp.available()) {
        c = Udp.read();
        temp.concat(String(c));
    }

    // Log complete message
    LOG(INFO,"UDP MESSAGE:%s", temp.c_str());

    // Store sender ip and port
    IPAddress ipAddress = Udp.remoteIP();
    int port = Udp.remotePort();

    // Echo back data to sender
    Udp.beginPacket(ipAddress, port);
    Udp.write(temp.c_str());
    Udp.endPacket();
  }
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
    Serial.blockOnOverrun(true);
    Serial1.begin(115200);
    waitFor(Serial.isConnected,10000);
    Serial1.println("Electron U260 AT Interface");
    pinMode(D7, OUTPUT);

    Cellular.on();
    Cellular.command(100000,"AT+COPS=2\r\n");
    Cellular.command(100000,"AT+COPS=4,2,\"310260\"\r\n"); //TMO
    // Cellular.command(100000,"AT+COPS=4,2,\"310410\"\r\n"); //ATT
    // Cellular.command(100000,"AT+COPS=4,2,\"T-Mobile\"\r\n");
    Cellular.connect();
    waitUntil(Cellular.ready);

    pinMode(B0, INPUT_PULLUP);
    pinMode(B1, INPUT_PULLUP);
    pinMode(C0, INPUT_PULLUP);
    pinMode(C1, INPUT_PULLUP);
    // Particle.keepAlive(55 * 1);    // send a ping every 55 seconds
    Particle.keepAlive(2 * 60 * 60);    // send a ping every 2 hours
#ifdef PARTICLE_CLOUD
    //Register all the Tinker functions
    Particle.function("digitalread", tinkerDigitalRead);
    Particle.function("digitalwrite", tinkerDigitalWrite);

    Particle.function("analogread", tinkerAnalogRead);
    Particle.function("analogwrite", tinkerAnalogWrite);

    Particle.subscribe("20170118in", relayHandler);
    Particle.connect();
    waitUntil(Particle.connected);
#endif
    // pingMe.start();
    blinky.start();

    Udp.begin(localPort);
    callHomeUDP();
}

void processATcommands()
{
    static String cmd = "";
    static bool echo_commands = true;
    static bool assist = false;
    if (MY_SERIAL.available() > 0) {
        char c = MY_SERIAL.read();
        if (c == '\r') {
            MY_SERIAL.println();
            if(cmd == ":echo1;" || cmd == ":ECHO1;") {
                echo_commands = true;
            }
            else if(cmd == ":echo0;" || cmd == ":ECHO0;") {
                echo_commands = false;
            }
            else if(cmd == ":assist1;" || cmd == ":ASSIST1;") {
                assist = true;
            }
            else if(cmd == ":assist0;" || cmd == ":ASSIST0;") {
                assist = false;
            }
            else if(cmd != "") {
                Cellular.command("%s\r\n", cmd.c_str());
            }
            cmd = "";
        }
        else if (assist && c == 27) { // ESC
            if (cmd.length() > 0 && echo_commands) {
                MY_SERIAL.print('\r');
                for (uint32_t x=0; x<cmd.length(); x++) {
                    MY_SERIAL.print(' ');
                }
                MY_SERIAL.print('\r');
            }
            cmd = "";
        }
        else if (assist && (c == 8 || c == 127)) { // BACKSPACE
            if (cmd.length() > 0) {
                cmd.remove(cmd.length()-1, 1);
                if (echo_commands) {
                    MY_SERIAL.print("\b \b");
                }
            }
        }
        else {
            cmd += c;
            if (echo_commands) MY_SERIAL.print(c);
        }
    }
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    if (digitalRead(B1)==LOW) {
        callHomeUDP();
        delay(1000);
    }
    if (echoTime || digitalRead(B0)==LOW) {
        echoTime = false;
        // Particle.publish("20170118out", String::format("%.3f",millis()/1000.0),NO_ACK);
        Particle.publish("20170118out", String::format("%.3f",millis()/1000.0));
        // Particle.publish(NULL, NO_ACK);
        delay(1000);
    }
    if (pingTime && digitalRead(C0)==HIGH) {
        pingTime = false;
        // Particle.publish("20170118out", String::format("%.3f",millis()/1000.0),NO_ACK);
        Particle.publish("20170118out", String::format("%.3f",millis()/1000.0));
        // Particle.publish(NULL, NO_ACK);
    }
    if (digitalRead(C1)==LOW) {
        Particle.publish("spark/device/session/end","", PRIVATE);
        // echoTest();
    }
    if (ipaddrTime) {
        ipaddrTime = false;
        Particle.publish("20170118out", String(Cellular.localIP()),NO_ACK);
    }

    processATcommands();

    checkUDP();
}

/*******************************************************************************
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
    if (pinNumber < 0 || pinNumber > 7) return -1;

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
#if Wiring_Cellular
    else if (pin.startsWith("B"))
    {
        if (pinNumber > 5) return -3;
        pinMode(pinNumber+24, INPUT_PULLDOWN);
        return digitalRead(pinNumber+24);
    }
    else if (pin.startsWith("C"))
    {
        if (pinNumber > 5) return -4;
        pinMode(pinNumber+30, INPUT_PULLDOWN);
        return digitalRead(pinNumber+30);
    }
#endif
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
    if (pinNumber < 0 || pinNumber > 7) return -1;

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
#if Wiring_Cellular
    else if(command.startsWith("B"))
    {
        if (pinNumber > 5) return -4;
        pinMode(pinNumber+24, OUTPUT);
        digitalWrite(pinNumber+24, value);
        return 1;
    }
    else if(command.startsWith("C"))
    {
        if (pinNumber > 5) return -5;
        pinMode(pinNumber+30, OUTPUT);
        digitalWrite(pinNumber+30, value);
        return 1;
    }
#endif
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
    if (pinNumber < 0 || pinNumber > 7) return -1;

    if(pin.startsWith("D"))
    {
        return -3;
    }
    else if (pin.startsWith("A"))
    {
        return analogRead(pinNumber+10);
    }
#if Wiring_Cellular
    else if (pin.startsWith("B"))
    {
        if (pinNumber < 2 || pinNumber > 5) return -3;
        return analogRead(pinNumber+24);
    }
#endif
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
    String value = command.substring(3);

    if(command.substring(0,2) == "TX")
    {
        pinMode(TX, OUTPUT);
        analogWrite(TX, value.toInt());
        return 1;
    }
    else if(command.substring(0,2) == "RX")
    {
        pinMode(RX, OUTPUT);
        analogWrite(RX, value.toInt());
        return 1;
    }

    //convert ascii to integer
    int pinNumber = command.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits

    if (pinNumber < 0 || pinNumber > 7) return -1;

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
    else if(command.substring(0,2) == "TX")
    {
        pinMode(TX, OUTPUT);
        analogWrite(TX, value.toInt());
        return 1;
    }
    else if(command.substring(0,2) == "RX")
    {
        pinMode(RX, OUTPUT);
        analogWrite(RX, value.toInt());
        return 1;
    }
#if Wiring_Cellular
    else if (command.startsWith("B"))
    {
        if (pinNumber > 3) return -3;
        pinMode(pinNumber+24, OUTPUT);
        analogWrite(pinNumber+24, value.toInt());
        return 1;
    }
    else if (command.startsWith("C"))
    {
        if (pinNumber < 4 || pinNumber > 5) return -4;
        pinMode(pinNumber+30, OUTPUT);
        analogWrite(pinNumber+30, value.toInt());
        return 1;
    }
#endif
    else return -2;
}

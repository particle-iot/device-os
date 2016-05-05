#include "application.h"

// telnet defaults to port 23
TCPServer server = TCPServer(23);
TCPClient client;

void setup()
{
  // Make sure your Serial Terminal app is closed before powering your device
  Serial.begin(9600);
  Serial.println("echo server");

 // start listening for clients
  server.begin();
}

void loop()
{
  // Now open your Serial Terminal, and hit any key to continue!
  if (Serial.available()) {
      char c = Serial.read();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.SSID());
  }


    if (client.connected()) {
    // echo all available bytes back to the client
    while (client.available()) {
      server.write(client.read());
    }
  } else {
    // if no client is yet connected, check for a new connection
    client = server.available();
  }
}
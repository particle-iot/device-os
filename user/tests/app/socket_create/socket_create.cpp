/*
 * Problem: On the Electron you can create TCP or UDP sockets and once you get to 7 total,
 * the socket_create() function won’t return valid handles.  This will not knock
 * the Electron off the cloud.  If you use disconnect from the cloud, then use up
 * all the sockets, then try to reconnect, it attempts to recover, can’t, so it
 * disconnects and resets the Cellular module which generates a URC for socket closed
 * remotely, but the system did not free up the sockets because they were never set
 * to a .connected status.  Each time the system then tries to allocate a new socket
 * for the Cloud, it finds that they are all in use so the system is stuck with all
 * sockets open (but not really, they were remotely closed).
 *
 * Solution: Force socketFree() when the socket is remotely closed.
 *
 * Test: disconnect from the Cloud, and use up all available sockets.  Then reconnect
 * and watch the system attempt to recover by power cycling the modem which causes
 * all sockets to be remotely disconnected.  They are freed and the Cloud socket can
 * be created and connected.
 */
#include "application.h"
#include "socket_hal.h"

// ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, INFO_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
SerialDebugOutput debugOutput(9600, ALL_LEVEL);

SYSTEM_MODE(AUTOMATIC);
int port = 9000;

void setup()
{
    Serial.begin(9600);
    Serial.println("PRESS ENTER");
    while (!Serial.available()) Particle.process();
    while (Serial.available()) Serial.read(); // Flush the input buffer
}

void loop()
{
    if (Serial.available() > 0)
    {
        char c = Serial.read();
        Serial.printf("Hey, you said \'%c\', so I'm gunna: ", c);
        if (c == 't') {
            Serial.print("Create a TCP socket: ");
            int socket_handle = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, port++, NIF_DEFAULT);
            if (socket_handle_valid(socket_handle)) {
                Serial.printlnf("%d TCP socket created!", socket_handle);
            }
            else {
                Serial.printlnf("%d TCP socket not created!", socket_handle);
            }
        }
        if (c == 'u') {
            Serial.print("Create a UDP socket: ");
            int socket_handle = socket_create(AF_INET, SOCK_STREAM, IPPROTO_UDP, port++, NIF_DEFAULT);
            if (socket_handle_valid(socket_handle)) {
                Serial.printlnf("%d UDP socket created!", socket_handle);
            }
            else {
                Serial.printlnf("%d UDP socket not created!", socket_handle);
            }
        }
        else if(c == 'C') {
            Particle.connect();
        }
        else if(c == 'c') {
            Particle.disconnect();
        }
        else {
            Serial.println("ignore you because you're not speaking my language!");
        }
        while (Serial.available()) Serial.read(); // Flush the input buffer
    }

}

#include <string.h>
#include "spark_wiring_usbserial.h"

typedef void (*ConnectCallback)(const char *ssid, const char *passowrd);

class WiFiCredentialsReader
{
  public:
    WiFiCredentialsReader(ConnectCallback connect_callback);
    void read(void);

  private:
    USBSerial serial;
    ConnectCallback connect_callback;
    char ssid[33];
    char password[33];

    void print(const char *s);
    void read_line(char *dst);
    void read_available_char(bool &reading_line, char *p);
    void flush_input();
};

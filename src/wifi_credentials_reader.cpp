#include "wifi_credentials_reader.h"

WiFiCredentialsReader::WiFiCredentialsReader(USBSerial &serial, ConnectCallback connect_callback)
{
  memset(ssid, 0, 33);
  memset(password, 0, 33);
  this->connect_callback = connect_callback;
  this->serial = &serial;
  this->serial->begin(9600);
}

void WiFiCredentialsReader::read(void)
{
  if (0 < serial->available())
  {
    if ('w' == serial->read())
    {
      print("SSID: ");
      read_line(ssid);

      print("Password: ");
      read_line(password);

      connect_callback(ssid, password);
    }
  }
}


/* private methods */

inline void WiFiCredentialsReader::print(const char *s)
{
  for (size_t i = 0; i < strlen(s); ++i)
    serial->write(s[i]);
}

inline void WiFiCredentialsReader::read_line(char *dst)
{
  bool reading_line = true;
  char *p = dst;
  while (reading_line && p - dst < 32)
  {
    read_available_char(reading_line, p);
  }
  flush_input(32 == p - dst);
}

inline void WiFiCredentialsReader::read_available_char(bool &reading_line, char *p)
{
  if (0 < serial->available())
  {
    char c = serial->read();
    reading_line = ('\n' != c && '\r' != c);
    if (reading_line)
    {
      serial->write(c);
      *p++ = c;
    }
    else
    {
      serial->write('\r');
      serial->write('\n');
    }
  }
}

inline void WiFiCredentialsReader::flush_input(bool write_newline)
{
  if (write_newline)
  {
    serial->write('\r');
    serial->write('\n');
  }

  while (serial->available())
    serial->read();
}

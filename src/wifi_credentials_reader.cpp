#include "wifi_credentials_reader.h"

WiFiCredentialsReader::WiFiCredentialsReader(ConnectCallback connect_callback)
{
  this->connect_callback = connect_callback;
  serial.begin(9600);
}

void WiFiCredentialsReader::read(void)
{
  if (0 < serial.available())
  {
    char c = serial.read();
    if ('w' == c)
    {
      memset(ssid, 0, 33);
      memset(password, 0, 33);

      print("SSID: ");
      read_line(ssid);

      print("Password: ");
      read_line(password);

      print("Thanks! Wait about 7 seconds while I test that...\r\n");

      connect_callback(ssid, password);

      print("Awesome. Now hang on 8 more seconds while I save\r\n");
      print("these credentials, and then we'll connect!\r\n");
      print("If you see a pulsing cyan light, your Spark Core\r\n");
      print("has connected to the Cloud and is ready to go!\r\n");
      print("If your LED flashes red or you encounter any other problems,\r\n");
      print("visit https://www.spark.io/support to debug.\r\n");
      print("\r\n    Spark <3 you!\r\n\r\n");
    }
    else if ('i' == c)
    {
      char id[12];
      memcpy(id, (char *)ID1, 12);
      print("Your core id is ");
      char hex_digit;
      for (int i = 0; i < 12; ++i)
      {
        hex_digit = 48 + (id[i] >> 4);
        if (57 < hex_digit)
          hex_digit += 39;
        serial.write(hex_digit);
        hex_digit = id[i] & 0xf;
        if (57 < hex_digit)
          hex_digit += 39;
        serial.write(hex_digit);
      }
      print("\r\n");
    }
  }
}


/* private methods */

void WiFiCredentialsReader::print(const char *s)
{
  for (size_t i = 0; i < strlen(s); ++i)
  {
    serial.write(s[i]);
    Delay(1); // ridonkulous, but required
  }
}

void WiFiCredentialsReader::read_line(char *dst)
{
  char c = 0, i = 0;
  while (1)
  {
    if (0 < serial.available())
    {
      c = serial.read();
      if (i == 32 || c == '\r' || c == '\n')
      {
        *dst = '\0';
        break;
      }
      serial.write(c);
      *dst++ = c;
      ++i;
    }
  }
  print("\r\n");
  while (0 < serial.available())
    serial.read();
}

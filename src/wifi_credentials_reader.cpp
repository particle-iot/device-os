#include "wifi_credentials_reader.h"

WiFiCredentialsReader::WiFiCredentialsReader(ConnectCallback connect_callback)
{
  memset(ssid, 0, 33);
  memset(password, 0, 33);
  this->connect_callback = connect_callback;
  serial.begin(9600);
}

void WiFiCredentialsReader::read(void)
{
  if (0 < serial.available())
  {
    if ('w' == serial.read())
    {
      print("SSID: ");
      read_line(ssid);

      print("Password: ");
      read_line(password);

      print("Thanks! Wait about 7 seconds while I test that...\r\n");
      connect_callback(ssid, password);
      print("Awesome. Now hang on 8 more seconds while I save, and then we'll connect!\r\n");
      print("\r\n    Spark <3 you!\r\n\r\n");
    }
  }
}


/* private methods */

inline void WiFiCredentialsReader::print(const char *s)
{
  for (size_t i = 0; i < strlen(s); ++i)
    serial.write(s[i]);
}

inline void WiFiCredentialsReader::read_line(char *dst)
{
	char c = 0, i = 0;
	while(1)
	{
		if(serial.available())
		{
			c = serial.read();
			serial.write(c);
			if(i == 32 || (c == '\r' || c == '\n'))
			{
				*dst = '\0';
				break;
			}
			*dst++ = c;
			i++;
		}
	}
	serial.print("\r\n");
}

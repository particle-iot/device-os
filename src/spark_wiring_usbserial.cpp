/*
  SoftwareSerial library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#include "spark_wiring_usbserial.h"

//
// Constructor
//
USBSerial::USBSerial()
{
}

//
// Public methods
//

void USBSerial::begin(long speed)
{
	USB_USART_Init(speed);
}

void USBSerial::end()
{
	//To Do
}


// Read data from buffer
int USBSerial::read()
{
	return USB_USART_Receive_Data();
}

int USBSerial::available()
{
	return USB_USART_Available_Data();
}

size_t USBSerial::write(uint8_t byte)
{
	USB_USART_Send_Data(byte);

	return 1;
}

void USBSerial::flush()
{
	//To Do
}

int USBSerial::peek()
{
	return -1;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

USBSerial Serial;

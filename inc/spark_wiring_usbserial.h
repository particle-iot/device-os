/*
  SoftwareSerial library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_USBSERIAL_H
#define __SPARK_WIRING_USBSERIAL_H

#include "spark_wiring_stream.h"

class USBSerial : public Stream
{
private:

public:
	// public methods
	USBSerial();

	void begin(long speed);
	void end();
	int peek();

	virtual size_t write(uint8_t byte);
	virtual int read();
	virtual int available();
	virtual void flush();

	using Print::write;
};

extern void USB_USART_Init(uint32_t baudRate);
extern uint8_t USB_USART_Available_Data(void);
extern int32_t USB_USART_Receive_Data(void);
extern void USB_USART_Send_Data(uint8_t Data);

extern USBSerial Serial;

#endif

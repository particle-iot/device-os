/*
  HardwareSerial library
  API declarations borrowed from Arduino & Wiring
  Modified by Spark
 */

#ifndef __SPARK_WIRING_USARTSERIAL_H
#define __SPARK_WIRING_USARTSERIAL_H

#include "spark_wiring_stream.h"

class USARTSerial : public Stream
{
  private:
	static USART_InitTypeDef USART_InitStructure;

	static bool USARTSerial_Enabled;
  public:
    USARTSerial();
    void begin(unsigned long);
    void begin(unsigned long, uint8_t);
    void end();

    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);

    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }

    using Print::write; // pull in write(str) and write(buf, size) from Print

    operator bool();

	static bool isEnabled(void);

};

extern USARTSerial Serial1;

#endif

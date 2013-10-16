#ifndef SPARK_WIRING_HARDWARE_SERIAL_H
#define SPARK_WIRING_HARDWARE_SERIAL_H


#include "stm32f10x.h"
#include "platform_config.h"
#include "spark_utilities.h"

#include "spark_wiring_stream.h"
#include "spark_wiring.h"

/*
Serial.begin
Serial.end
Serial.available

Serial.read
Serial.write
Serial.print
Serial.println
Serial.peek
Serial.flush

*/

struct ring_buffer;

class HardwareSerial : public Stream
{
	private:
		ring_buffer *_rx_buffer;
		ring_buffer *_tx_buffer;
		volatile uint8_t *_ubrrh;
		volatile uint8_t *_ubrrl;
		volatile uint8_t *_ucsra;
		volatile uint8_t *_ucsrb;
		volatile uint8_t *_ucsrc;
		volatile uint8_t *_udr;
		uint8_t _rxen;
		uint8_t _txen;
		uint8_t _rxcie;
		uint8_t _udrie;
		uint8_t _u2x;
		bool transmitting;

	public:
		void begin(uint32_t);
    	void begin(uint32_t, uint8_t);
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
};

extern HardwareSerial SerialW;

#endif
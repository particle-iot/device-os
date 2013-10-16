#ifndef SPARK_WIRING_HARDWARE_SERIAL_H
#define SPARK_WIRING_HARDWARE_SERIAL_H

#include "stm32f10x.h"
#include "platform_config.h"
#include "spark_utilities.h"

#include "spark_wiring_stream.h"

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

class HardwareSerial : public Stream
{
	private:

	public:
		void begin(uint32_t);
    	void begin(uint32_t, uint8_t);
    	void end();
    	virtual uint8_t available(void);


};

#endif
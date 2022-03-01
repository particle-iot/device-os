#ifndef __LIB_SERIAL2_H
#define __LIB_SERIAL2_H

#include "spark_wiring_usartserial.h"

#if Wiring_Serial2

// instantiate Serial2

USARTSerial& __fetch_global_Serial2()
{
	static USARTSerial serial2(HAL_USART_SERIAL2, acquireSerial2Buffer());
	return serial2;
}

void serialEventRun2()
{
    __handleSerialEvent(Serial2, serialEvent2);
}

#endif

#endif



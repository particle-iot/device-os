#ifndef __LIB_SERIAL3_H
#define __LIB_SERIAL3_H

#include "spark_wiring_usartserial.h"
#if Wiring_Serial3

// instantiate Serial3

USARTSerial& __fetch_global_Serial3()
{
	static USARTSerial serial3(HAL_USART_SERIAL3, acquireSerial3Buffer());
	return serial3;
}

void serialEventRun3()
{
    __handleSerialEvent(Serial3, serialEvent3);
}

#endif
#endif

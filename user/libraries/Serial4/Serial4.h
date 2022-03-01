#ifndef __LIB_SERIAL4_H
#define __LIB_SERIAL4_H

#include "spark_wiring_usartserial.h"

#if Wiring_Serial4

// instantiate Serial4
USARTSerial& __fetch_global_Serial4()
{
	static USARTSerial serial4(HAL_USART_SERIAL4, acquireSerial4Buffer());
	return serial4;
}

void serialEventRun4()
{
    __handleSerialEvent(Serial4, serialEvent4);
}

#endif
#endif

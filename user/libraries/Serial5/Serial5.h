#ifndef __LIB_SERIAL5_H
#define __LIB_SERIAL5_H

#include "spark_wiring_usartserial.h"

#if Wiring_Serial5

// instantiate Serial5
USARTSerial& __fetch_global_Serial5()
{
	static USARTSerial serial5(HAL_USART_SERIAL5, acquireSerial5Buffer());
	return serial5;
}

void serialEventRun5()
{
    __handleSerialEvent(Serial5, serialEvent5);
}

#endif
#endif

#ifndef __LIB_SERIAL2_H
#define __LIB_SERIAL2_H

#include "spark_wiring_usartserial.h"

#if PLATFORM_ID != 88 // Duo: Serial2 is instantiated in spark_wiring_usartserial.cpp

#if Wiring_Serial2

// instantiate Serial2
static Ring_Buffer serial2_rx_buffer;
static Ring_Buffer serial2_tx_buffer;

USARTSerial& __fetch_global_Serial2()
{
	static USARTSerial serial2(HAL_USART_SERIAL2, &serial2_rx_buffer, &serial2_tx_buffer);
	return serial2;
}


void serialEventRun2()
{
    __handleSerialEvent(Serial2, serialEvent2);
}

#endif // Wiring_Serial2

#endif // PLATFORM_ID != 88

#endif // __LIB_SERIAL2_H



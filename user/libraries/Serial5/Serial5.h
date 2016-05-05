#ifndef __LIB_SERIAL5_H
#define __LIB_SERIAL5_H

#include "spark_wiring_usartserial.h"

#if Wiring_Serial5

// instantiate Serial5
static Ring_Buffer serial5_rx_buffer;
static Ring_Buffer serial5_tx_buffer;

USARTSerial& __fetch_global_Serial5()
{
	static USARTSerial serial5(HAL_USART_SERIAL5, &serial5_rx_buffer, &serial5_tx_buffer);
	return serial5;
}


void serialEventRun5()
{
    __handleSerialEvent(Serial5, serialEvent5);
}

#endif
#endif

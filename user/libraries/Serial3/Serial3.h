#ifndef __LIB_SERIAL3_H
#define __LIB_SERIAL3_H

#include "spark_wiring_usartserial.h"
#if Wiring_Serial3

// instantiate Serial3
static Ring_Buffer serial3_rx_buffer;
static Ring_Buffer serial3_tx_buffer;


USARTSerial& __fetch_global_Serial3()
{
	static USARTSerial serial3(HAL_USART_SERIAL3, &serial3_rx_buffer, &serial3_tx_buffer);
	return serial3;
}



void serialEventRun3()
{
    __handleSerialEvent(Serial3, serialEvent3);
}

#endif
#endif

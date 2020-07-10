#ifndef __LIB_SERIAL4_H
#define __LIB_SERIAL4_H

#include "spark_wiring_usartserial.h"

#if Wiring_Serial4

// instantiate Serial4
static hal_usart_ring_buffer_t serial4_rx_buffer;
static hal_usart_ring_buffer_t serial4_tx_buffer;

USARTSerial& __fetch_global_Serial4()
{
	static USARTSerial serial4(HAL_USART_SERIAL4, &serial4_rx_buffer, &serial4_tx_buffer);
	return serial4;
}

void serialEventRun4()
{
    __handleSerialEvent(Serial4, serialEvent4);
}

#endif
#endif

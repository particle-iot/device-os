#ifndef __LIB_SERIAL4_H
#define __LIB_SERIAL4_H

#include "spark_wiring_usartserial.h"

// instantiate Serial4
static Ring_Buffer serial4_rx_buffer;
static Ring_Buffer serial4_tx_buffer;

USARTSerial Serial4(HAL_USART_SERIAL4, &serial4_rx_buffer, &serial4_tx_buffer);

void serialEventRun4()
{
    __handleSerialEvent(Serial4, serialEvent4);
}

#endif

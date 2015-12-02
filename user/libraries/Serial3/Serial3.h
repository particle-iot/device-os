#ifndef __LIB_SERIAL3_H
#define __LIB_SERIAL3_H

#include "spark_wiring_usartserial.h"

// instantiate Serial3
static Ring_Buffer serial3_rx_buffer;
static Ring_Buffer serial3_tx_buffer;

USARTSerial Serial3(HAL_USART_SERIAL3, &serial3_rx_buffer, &serial3_tx_buffer);

void serialEventRun3()
{
    __handleSerialEvent(Serial3, serialEvent3);
}


#endif

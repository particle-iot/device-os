#ifndef __LIB_SERIAL5_H
#define __LIB_SERIAL5_H

#include "spark_wiring_usartserial.h"

// instantiate Serial5
static Ring_Buffer serial5_rx_buffer;
static Ring_Buffer serial5_tx_buffer;

USARTSerial Serial5(HAL_USART_SERIAL5, &serial5_rx_buffer, &serial5_tx_buffer);

void serialEventRun5()
{
    __handleSerialEvent(Serial5, serialEvent5);
}


#endif

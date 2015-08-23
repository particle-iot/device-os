#ifndef __LIB_SERIAL2_H
#define __LIB_SERIAL2_H

#include "particle_wiring_usartserial.h"

// instantiate Serial2
static Ring_Buffer serial2_rx_buffer;
static Ring_Buffer serial2_tx_buffer;

USARTSerial Serial2(HAL_USART_SERIAL2, &serial2_rx_buffer, &serial2_tx_buffer);

#endif



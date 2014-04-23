/**
 ******************************************************************************
 * @file    spark_wiring_usartserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_usartserial.c module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef __SPARK_WIRING_USARTSERIAL_H
#define __SPARK_WIRING_USARTSERIAL_H

#include "spark_wiring_stream.h"

#define SERIAL_BUFFER_SIZE 64

typedef struct Ring_Buffer
{
  unsigned char buffer[SERIAL_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} Ring_Buffer;

typedef enum USART_Num_Def {
  USART_D1_D0 = 0,
  USART_TX_RX
} USART_Num_Def;

#define TOTAL_USARTS 2
#define REMAP_NONE 0
typedef struct STM32_USART_Info {
  USART_TypeDef* usart_peripheral;
  __IO uint32_t* usart_apbReg;
  uint32_t usart_clock_en; 

  IRQn usart_int_n;

  uint16_t usart_tx_pin;
  uint16_t usart_rx_pin;

  uint32_t usart_pin_remap;

  // Buffer pointers. These need to beglobal for IRQ handler access
  Ring_Buffer* usart_tx_buffer;
  Ring_Buffer* usart_rx_buffer;

} STM32_USART_Info;

extern STM32_Pin_Info PIN_MAP[];

class USARTSerial : public Stream
{
  private:
    static USART_InitTypeDef USART_InitStructure;
    static bool USARTSerial_Enabled;
    bool transmitting;
    Ring_Buffer _rx_buffer;
    Ring_Buffer _tx_buffer;
    int umapIndex; // USART_MAP index

  public:
    USARTSerial(USART_Num_Def usartNum);
    virtual ~USARTSerial() {};
    void begin(unsigned long);
    void begin(unsigned long, uint8_t);
    void end();

    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);

    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }

    using Print::write; // pull in write(str) and write(buf, size) from Print

    operator bool();

	static bool isEnabled(void);

};

extern USARTSerial Serial1;  // USART2 on PA2/3
extern USARTSerial Serial2; // USART1 on alternate PB6/7 (Spark D1, D0)

#endif

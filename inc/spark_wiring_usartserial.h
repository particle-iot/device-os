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
  USART_TX_RX =0,
  USART_D1_D0
} USART_Num_Def;
#define TOTAL_USARTS 2

#define GPIO_Remap_None 0

typedef struct STM32_USART_Info {
  USART_TypeDef* usart_peripheral;
  __IO uint32_t* usart_apbReg;
  uint32_t usart_clock_en; 

  IRQn usart_int_n;

  uint16_t usart_tx_pin;
  uint16_t usart_rx_pin;

  uint32_t usart_pin_remap;

  // Buffer pointers. These need to be global for IRQ handler access
  Ring_Buffer* usart_tx_buffer;
  Ring_Buffer* usart_rx_buffer;

} STM32_USART_Info;
extern STM32_USART_Info USART_MAP[TOTAL_USARTS];

extern STM32_Pin_Info PIN_MAP[];

/* configuration masks to specify USART_InitTypeDef using Serial.begin(baud, config) 
 *
 * Bit mask allocation [Bit 8 (MSB) .. 1 (LSB)]
 * Bit 8: unused (could enable/disable rx/tx in the future)
 * Bit 7: Word Length (Data bits + Parity Bit) [0 = 8 bits, 1 = 9 bits]
 * Bits 5-6: Hardware Flow Control [0(0b00) = None, 1(0b01) = RTS, 2(0b10) = CTS, 3(0b11) = RTS + CTS]
 * Bits 3-4: Parity Bits [0(0b00) = None, 1(0b01) = Even, 2(0b10) = Odd]
 * Bits 1-2: Stop Bits [0(0b00) = 1 stop bit, 1(0b01) = 0.5 stop bits, 2(0b10) = 2 stop bits, 3(0b11) = 1.5 stop bits]

 * Note: The STM32 only supports 8-bit and 9-bit word lengths.  Word lengths includes a parity bit if even/odd parity is used (e.g. 8 data bits + odd parity requires 9-bit word lengths)
*/

// stop bit masks
const uint16_t USARTSerial_StopBits[4] = {USART_StopBits_1, USART_StopBits_0_5, USART_StopBits_2, USART_StopBits_1};
#define SB_MASK(config) (config & 0x03)
#define SB_1    0
#define SB_0_5  1
#define SB_2    2
#define SB_1_5  3

// parity bit masks
const uint16_t USARTSerial_Parity[3] = {USART_Parity_No, USART_Parity_Even, USART_Parity_Odd};
#define PAR_MASK(config) ((config & 0x0C)>>2)
#define PAR_NONE  (0 << 2)
#define PAR_EVEN  (1 << 2)
#define PAR_ODD   (2 << 2)

// control flow masks
const uint16_t USARTSerial_FlowControl[4] = {USART_HardwareFlowControl_None, USART_HardwareFlowControl_RTS, USART_HardwareFlowControl_CTS, USART_HardwareFlowControl_RTS_CTS};
#define FLOW_MASK(config) ((config & 0x30) >> 4)
#define FLOW_NONE     (0 << 4)
#define FLOW_RTS      (1 << 4)
#define FLOW_CTS      (2 << 4)
#define FLOW_RTS_CTS  (3 << 4)

// word lengths masks
const uint16_t USARTSerial_WordLength[2] = {USART_WordLength_8b, USART_WordLength_9b};
#define WL_MASK(config) ((config & 0x40) >> 6)
#define WL_8  (0 << 6)
#define WL_9  (1 << 6)

// some commond configurations
#define SERIAL_8N1 ((uint8_t) (FLOW_NONE | WL_8 | PAR_NONE | SB_1))
#define SERIAL_8N2 ((uint8_t) (FLOW_NONE | WL_8 | PAR_NONE | SB_2))

class USARTSerial : public Stream
{
  private:
    static USART_InitTypeDef USART_InitStructure;
    static bool USARTSerial_Enabled;
    bool transmitting;
    Ring_Buffer _rx_buffer;
    Ring_Buffer _tx_buffer;
    STM32_USART_Info *usartMap; // pointer to USART_MAP[] containing USART peripheral register locations (etc)

  public:
    USARTSerial(STM32_USART_Info *usartMapPtr);
    virtual ~USARTSerial() {};
    void begin(unsigned long baud) {begin(baud, SERIAL_8N1);}
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

extern USARTSerial Serial1; // USART2 on PA2/3 (Spark TX, RX)
extern USARTSerial Serial2; // USART1 on alternate PB6/7 (Spark D1, D0)

#endif

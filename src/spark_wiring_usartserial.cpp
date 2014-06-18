/**
 ******************************************************************************
 * @file    spark_wiring_usartserial.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring hardware serial module
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

#include "spark_wiring_usartserial.h"

/*
 * USART mapping
 */
STM32_USART_Info USART_MAP[TOTAL_USARTS] =
{
  /*
   * USRAT_peripheral (USART1-USART2; not using others)
   * clock control register (APB1ENR or APB1ENR)
   * clock enable bit value (RCC_APB2Periph_USART1 or RCC_APB2Periph_USART2)
   * interrupt number (USART1_IRQn or USART2_IRQn)
   * TX pin
   * RX pin
   * GPIO Remap (RCC_APB2Periph_USART2 or GPIO_Remap_None )
   * <tx_buffer pointer> used internally and does not appear below
   * <rx_buffer pointer> used internally and does not appear below
   */
  { USART2, &RCC->APB1ENR, RCC_APB1Periph_USART2, USART2_IRQn, TX, RX, GPIO_Remap_None },
  { USART1, &RCC->APB2ENR, RCC_APB2Periph_USART1, USART1_IRQn, D1, D0, GPIO_Remap_USART1 }
};


inline void store_char(unsigned char c, Ring_Buffer *buffer)
{
        unsigned i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

	if (i != buffer->tail)
	{
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

// Initialize Class Variables //////////////////////////////////////////////////
USART_InitTypeDef USARTSerial::USART_InitStructure;
bool USARTSerial::USARTSerial_Enabled = false;

// Constructors ////////////////////////////////////////////////////////////////

USARTSerial::USARTSerial(STM32_USART_Info *usartMapPtr)
{
        usartMap = usartMapPtr;

        usartMap->usart_rx_buffer = &_rx_buffer;
        usartMap->usart_tx_buffer = &_tx_buffer;

        memset(&_rx_buffer, 0, sizeof(Ring_Buffer));
        memset(&_tx_buffer, 0, sizeof(Ring_Buffer));

        transmitting = false;
}

// Public Methods //////////////////////////////////////////////////////////////

void USARTSerial::begin(unsigned long baud)
{
	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock
        *usartMap->usart_apbReg |=  usartMap->usart_clock_en;

	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = usartMap->usart_int_n;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART2_IRQ_PRIORITY;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

        // Configure USART Rx as input floating
        pinMode(usartMap->usart_rx_pin, INPUT);

        // Configure USART Tx as alternate function push-pull
        pinMode(usartMap->usart_tx_pin, AF_OUTPUT_PUSHPULL);

        // Remap USARTn to alternate pins EG. USART1 to pins TX/PB6, RX/PB7
        GPIO_PinRemapConfig(usartMap->usart_pin_remap, ENABLE);

	// USART default configuration
	// USART configured as follow:
	// - BaudRate = (set baudRate as 9600 baud)
	// - Word Length = 8 Bits
	// - One Stop Bit
	// - No parity
	// - Hardware flow control disabled (RTS and CTS signals)
	// - Receive and transmit enabled
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	// Configure USART
	USART_Init(usartMap->usart_peripheral, &USART_InitStructure);

	// Enable USART Receive and Transmit interrupts
	USART_ITConfig(usartMap->usart_peripheral, USART_IT_RXNE, ENABLE);
	USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, ENABLE);

	// Enable the USART
	USART_Cmd(usartMap->usart_peripheral, ENABLE);

	USARTSerial_Enabled = true;
	transmitting = false;
}

// TODO
void USARTSerial::begin(unsigned long baud, byte config)
{

}

void USARTSerial::end()
{
	// wait for transmission of outgoing data
	while (_tx_buffer.head != _tx_buffer.tail);

	// Disable USART Receive and Transmit interrupts
	USART_ITConfig(usartMap->usart_peripheral, USART_IT_RXNE, DISABLE);
	USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, DISABLE);

	// Disable the USART
	USART_Cmd(usartMap->usart_peripheral, DISABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Disable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = usartMap->usart_int_n;
        NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

	NVIC_Init(&NVIC_InitStructure);

	// clear any received data
	_rx_buffer.head = _rx_buffer.tail;

        // null ring buffer pointers
        usartMap->usart_tx_buffer = NULL;
        usartMap->usart_rx_buffer = NULL;

        // Undo any pin re-mapping done for this USART
        GPIO_PinRemapConfig(usartMap->usart_pin_remap, DISABLE);

	USARTSerial_Enabled = false;
}

int USARTSerial::available(void)
{
	return (unsigned int)(SERIAL_BUFFER_SIZE + _rx_buffer.head - _rx_buffer.tail) % SERIAL_BUFFER_SIZE;
}

int USARTSerial::peek(void)
{
	if (_rx_buffer.head == _rx_buffer.tail)
	{
		return -1;
	}
	else
	{
		return _rx_buffer.buffer[_rx_buffer.tail];
	}
}

int USARTSerial::read(void)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if (_rx_buffer.head == _rx_buffer.tail)
	{
		return -1;
	}
	else
	{
		unsigned char c = _rx_buffer.buffer[_rx_buffer.tail];
		_rx_buffer.tail = (unsigned int)(_rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
		return c;
	}
}

void USARTSerial::flush()
{
	// Loop until USART DR register is empty
	while ( _tx_buffer.head != _tx_buffer.tail );
	// Loop until last frame transmission complete
	while (transmitting && (USART_GetFlagStatus(usartMap->usart_peripheral, USART_FLAG_TC) == RESET));
	transmitting = false;
}

size_t USARTSerial::write(uint8_t c)
{

        // interrupts are off and data in queue;
        if ((USART_GetITStatus(usartMap->usart_peripheral, USART_IT_TXE) == RESET)
            && _tx_buffer.head != _tx_buffer.tail) {
            // Get him busy
            USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, ENABLE);
        }

        unsigned i = (_tx_buffer.head + 1) % SERIAL_BUFFER_SIZE;

	// If the output buffer is full, there's nothing for it other than to
        // wait for the interrupt handler to empty it a bit
        //         no space so       or  Called Off Panic with interrupt off get the message out!
        //         make space                     Enter Polled IO mode
        while (i == _tx_buffer.tail || ((__get_PRIMASK() & 1) && _tx_buffer.head != _tx_buffer.tail) ) {
            // Interrupts are on but they are not being serviced because this was called from a higher
            // Priority interrupt

            if (USART_GetITStatus(usartMap->usart_peripheral, USART_IT_TXE) && USART_GetFlagStatus(usartMap->usart_peripheral, USART_FLAG_TXE))
            {
                // protect for good measure
                USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, DISABLE);
                // Write out a byte
                USART_SendData(usartMap->usart_peripheral,  _tx_buffer.buffer[_tx_buffer.tail++]);
                _tx_buffer.tail %= SERIAL_BUFFER_SIZE;
                // unprotect
                USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, ENABLE);
            }
        }

        _tx_buffer.buffer[_tx_buffer.head] = c;
        _tx_buffer.head = i;
	transmitting = true;
        USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, ENABLE);


	return 1;
}

USARTSerial::operator bool() {
	return true;
}

// Shared Interrupt Handler for USART2/Serial1 and USART1/Serial2
// WARNING: This function MUST remain reentrance compliant -- no local static variables etc.
static void USART_Interrupt_Handler(STM32_USART_Info *usartMap)
{
  if(USART_GetITStatus(usartMap->usart_peripheral, USART_IT_RXNE) != RESET)
  {
    // Read byte from the receive data register
    unsigned char c = USART_ReceiveData(usartMap->usart_peripheral);
    store_char(c, usartMap->usart_rx_buffer);
  }

  if(USART_GetITStatus(usartMap->usart_peripheral, USART_IT_TXE) != RESET)
  {
    // Write byte to the transmit data register
    if (usartMap->usart_tx_buffer->head == usartMap->usart_tx_buffer->tail)
    {
      // Buffer empty, so disable the USART Transmit interrupt
      USART_ITConfig(usartMap->usart_peripheral, USART_IT_TXE, DISABLE);
    }
    else
    {
      // There is more data in the output buffer. Send the next byte
      USART_SendData(usartMap->usart_peripheral, usartMap->usart_tx_buffer->buffer[usartMap->usart_tx_buffer->tail++]);
      usartMap->usart_tx_buffer->tail %= SERIAL_BUFFER_SIZE;
    }
  }
}

// Serial1 interrupt handler
// Serial1 uses standard Sparkcore pins PA2/TX(TX), PA3/RX(RX)
/*******************************************************************************
* Function Name  : Wiring_USART2_Interrupt_Handler (Declared as weak in stm32_it.cpp)
* Description    : This function handles USART2 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Wiring_USART2_Interrupt_Handler(void)
{
  USART_Interrupt_Handler(&USART_MAP[USART_TX_RX]);
}

// Serial2 interrupt handler
// Serial2 uses alternate function pins PB6/D1(TX), PB7/D0(RX)
/*******************************************************************************
* Function Name  : Wiring_USART1_Interrupt_Handler (Declared as weak in stm32_it.cpp)
* Description    : This function handles USART1 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Wiring_USART1_Interrupt_Handler(void)
{
  USART_Interrupt_Handler(&USART_MAP[USART_D1_D0]);
}

bool USARTSerial::isEnabled() {
	return USARTSerial_Enabled;
}


// Preinstantiate Objects //////////////////////////////////////////////////////

USARTSerial Serial1(&USART_MAP[USART_TX_RX]);
// optional Serial2 is instantiated from libraries/Serial2/Serial2.h

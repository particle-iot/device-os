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
   * rx_buffer pointer
   * tx_buffer pointer
   * interrupt number (USART1_IRQn or USART2_IRQn)
   */
  { USART1, RCC->APB2ENR, RCC_APB2Periph_USART1, NULL, NULL, USART1_IRQn },
  { USART2, RCC->APB1ENR, RCC_APB1Periph_USART2, NULL, NULL, USART2_IRQn }
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

USARTSerial::USARTSerial(USART_Num_Def usartNum)
{
  STM32_USART_Info* myUSART = &USART_MAP[usartNum];

//        myUSART->usart_rx_buffer = (Ring_Buffer *)malloc(sizeof(Ring_Buffer));
//        myUSART->usart_rx_buffer = (Ring_Buffer *)malloc(sizeof(Ring_Buffer));
//        _rx_buffer = myUSART->usart_rx_buffer;
//        _tx_buffer = myUSART->usart_tx_buffer;
  //      memset(_rx_buffer, 0, sizeof(Ring_Buffer));
  //      memset(_tx_buffer, 0, sizeof(Ring_Buffer));

        _rx_buffer = &__rx_buffer;
        _tx_buffer = &__tx_buffer;

        myUSART->usart_rx_buffer = _rx_buffer;
        myUSART->usart_tx_buffer = _tx_buffer;

        _rx_buffer->buffer[0] = '\0';
        _rx_buffer->head = _rx_buffer->tail = 0;
        _tx_buffer->buffer[0] = '\0';
        _tx_buffer->head = _tx_buffer->tail = 0;

	transmitting = false;
}

// Public Methods //////////////////////////////////////////////////////////////

void USARTSerial::begin(unsigned long baud)
{
	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART2_IRQ_PRIORITY;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

	// Configure USART Rx as input floating
	pinMode(RX, INPUT);

	// Configure USART Tx as alternate function push-pull
	pinMode(TX, AF_OUTPUT_PUSHPULL);

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
	USART_Init(USART2, &USART_InitStructure);

	// Enable USART Receive and Transmit interrupts
	//XXX USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	//XXX USART_ITConfig(USART2, USART_IT_TXE, ENABLE);

	// Enable the USART
	USART_Cmd(USART2, ENABLE);

	USARTSerial_Enabled = true;
	transmitting = false;
}

void USARTSerial::begin(unsigned long baud, byte config)
{

}

void USARTSerial::end()
{
	// wait for transmission of outgoing data
	while (_tx_buffer->head != _tx_buffer->tail);

	// Disable USART Receive and Transmit interrupts
	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);

	// Disable the USART
	USART_Cmd(USART2, DISABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Disable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

	NVIC_Init(&NVIC_InitStructure);

	// clear any received data
	_rx_buffer->head = _rx_buffer->tail;

	USARTSerial_Enabled = false;
}

int USARTSerial::available(void)
{
	return (unsigned int)(SERIAL_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

int USARTSerial::peek(void)
{
	if (_rx_buffer->head == _rx_buffer->tail)
	{
		return -1;
	}
	else
	{
		return _rx_buffer->buffer[_rx_buffer->tail];
	}
}

int USARTSerial::read(void)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if (_rx_buffer->head == _rx_buffer->tail)
	{
		return -1;
	}
	else
	{
		unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
		_rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
		return c;
	}
}

void USARTSerial::flush()
{
	// Loop until USART DR register is empty
	while ( _tx_buffer->head != _tx_buffer->tail );
	// Loop until last frame transmission complete
	while (transmitting && (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET));
	transmitting = false;
}

size_t USARTSerial::write(uint8_t c)
{

        // interrupts are off and data in queue;
        if ((USART_GetITStatus(USART1, USART_IT_TXE) == RESET)
            && _tx_buffer->head != _tx_buffer->tail) {
            // Get him busy
            USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
        }

        unsigned i = (_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE;

	// If the output buffer is full, there's nothing for it other than to
        // wait for the interrupt handler to empty it a bit
        //         no space so       or  Called Off Panic with interrupt off get the message out!
        //         make space                     Enter Polled IO mode
        while (i == _tx_buffer->tail || ((__get_PRIMASK() & 1) && _tx_buffer->head != _tx_buffer->tail) ) {
            // Interrupts are on but they are not being serviced because this was called from a higher
            // Priority interrupt

            if (USART_GetITStatus(USART1, USART_IT_TXE) && USART_GetFlagStatus(USART1, USART_FLAG_TXE))
            {
                // protect for good measure
                USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
                // Write out a byte
                USART_SendData(USART1,  _tx_buffer->buffer[_tx_buffer->tail++]);
                _tx_buffer->tail %= SERIAL_BUFFER_SIZE;
                // unprotect
                USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
            }
        }

        _tx_buffer->buffer[_tx_buffer->head] = c;
        _tx_buffer->head = i;
	transmitting = true;
        USART_ITConfig(USART1, USART_IT_TXE, ENABLE);


	return 1;
}

USARTSerial::operator bool() {
	return true;
}

static void Spark_USART_Interrupt_Handler(USART_Num_Def usartNum)
{
  STM32_USART_Info* myUSART = &USART_MAP[usartNum];
  Ring_Buffer* rx_buffer = USART_MAP[usartNum].usart_rx_buffer;
  Ring_Buffer* tx_buffer = USART_MAP[usartNum].usart_tx_buffer;

  if(USART_GetITStatus(USART_MAP[usartNum].usart_peripheral, USART_IT_RXNE) != RESET)
  {
    // Read byte from the receive data register
    unsigned char c = USART_ReceiveData(myUSART->usart_peripheral);
    store_char(c, rx_buffer);
  }

  if(USART_GetITStatus(myUSART->usart_peripheral, USART_IT_TXE) != RESET)
  {
    // Write byte to the transmit data register
    if (tx_buffer->head == tx_buffer->tail)
    {
      // Buffer empty, so disable the USART Transmit interrupt
      USART_ITConfig(myUSART->usart_peripheral, USART_IT_TXE, DISABLE);
    }
    else
    {
      // There is more data in the output buffer. Send the next byte
      USART_SendData(myUSART->usart_peripheral, tx_buffer->buffer[tx_buffer->tail++]);
      tx_buffer->tail %= SERIAL_BUFFER_SIZE;
    }
  }
}

// Serial2 interrupt handler
/*******************************************************************************
* Function Name  : Wiring_USART2_Interrupt_Handler (Declared as weak in stm32_it.cpp)
* Description    : This function handles USART2 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Wiring_USART2_Interrupt_Handler(void)
{
  Spark_USART_Interrupt_Handler(USART_2);
}

// Serial2 interrupt handler
/*******************************************************************************
* Function Name  : Wiring_USART1_Interrupt_Handler (Declared as weak in stm32_it.cpp)
* Description    : This function handles USART1 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Wiring_USART1_Interrupt_Handler(void)
{
  Spark_USART_Interrupt_Handler(USART_1);
}

bool USARTSerial::isEnabled() {
	return USARTSerial_Enabled;
}

/*************************************************************/
/*** Serial2, using alternate function pins TX/PB6, RX/PB7 ***/

void USARTSerial2::begin(unsigned long baud)
{
	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock and GPIOB
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOB, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART2_IRQ_PRIORITY; // same priority as USART2 (Serial1)
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

	// Configure USART Rx as input floating
	pinMode(D0, INPUT);

	// Configure USART Tx as alternate function push-pull
	pinMode(D1, AF_OUTPUT_PUSHPULL);

        // Remap USART1 to alternate pins TX/PB6, RX/PB7
        GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

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
	USART_Init(USART1, &USART_InitStructure);

	// Enable USART Receive and Transmit interrupts
	//XXX USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	//XXX USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

	// Enable the USART
	USART_Cmd(USART1, ENABLE);

	USARTSerial_Enabled = true;
	transmitting = false;
}

void USARTSerial2::begin(unsigned long baud, byte config)
{

}

void USARTSerial2::end()
{
	// wait for transmission of outgoing data
	while (_tx_buffer->head != _tx_buffer->tail);

	// Disable USART Receive and Transmit interrupts
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);

	// Disable the USART
	USART_Cmd(USART1, DISABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Disable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

	NVIC_Init(&NVIC_InitStructure);

	// clear any received data
	_rx_buffer->head = _rx_buffer->tail;

        // un-map alt. fnction pins PB6/D1, PB7/D0
        AFIO->MAPR &= ~AFIO_MAPR_USART1_REMAP;

	USARTSerial_Enabled = false;
}
// Preinstantiate Objects //////////////////////////////////////////////////////

USARTSerial Serial1(USART_2);
USARTSerial2 Serial2(USART_1);


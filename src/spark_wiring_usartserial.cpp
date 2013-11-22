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

// Initialize Class Variables //////////////////////////////////////////////////
USART_InitTypeDef USARTSerial::USART_InitStructure;

bool USARTSerial::USARTSerial_Enabled = false;

// Constructors ////////////////////////////////////////////////////////////////

USARTSerial::USARTSerial()
{
}

// Public Methods //////////////////////////////////////////////////////////////

void USARTSerial::begin(unsigned long baud)
{
	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// Enable USART Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

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

	// Enable the USART
	USART_Cmd(USART2, ENABLE);

	USARTSerial_Enabled = true;
}

void USARTSerial::begin(unsigned long baud, byte config)
{

}

void USARTSerial::end()
{
	// Disable the USART
	USART_Cmd(USART2, DISABLE);

	USARTSerial_Enabled = false;
}

int USARTSerial::available(void)
{
	// Check if the USART Receive Data Register is not empty
	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
		return 1;
	else
		return 0;
}

int USARTSerial::peek(void)
{
    return -1;
}

int USARTSerial::read(void)
{
	// Return the received byte
	return USART_ReceiveData(USART2);
}

void USARTSerial::flush()
{
	//To Do
}

size_t USARTSerial::write(uint8_t c)
{
	// Send one byte from USART
	USART_SendData(USART2, c);

	// Loop until USART DR register is empty
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
	{
	}

	return 1;
}

USARTSerial::operator bool() {
	return true;
}

/*******************************************************************************
* Function Name  : Wiring_USART2_Interrupt_Handler (Declared as weak in stm32_it.cpp)
* Description    : This function handles USART2 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
//void Wiring_USART2_Interrupt_Handler(void)
//{
//	//To Do
//}

bool USARTSerial::isEnabled() {
	return USARTSerial_Enabled;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

USARTSerial Serial1;

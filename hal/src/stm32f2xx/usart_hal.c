/**
 ******************************************************************************
 * @file    usart_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    17-Dec-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "stm32f2xx.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum USART_Num_Def {
	USART_TX_RX = 0,
	USART_RGBG_RGBB = 1
#if PLATFORM_ID == 10 // Electron
	,USART_TXD_UC_RXD_UC = 2
	,USART_C3_C2 = 3
    ,USART_C1_C0 = 4
#endif
} USART_Num_Def;

/* Private macro -------------------------------------------------------------*/
#define USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS 0  //Enabling this => 1 is not working at present

/* Private variables ---------------------------------------------------------*/
typedef struct STM32_USART_Info {
	USART_TypeDef* usart_peripheral;

	__IO uint32_t* usart_apbReg;
	uint32_t usart_clock_en;

	int32_t usart_int_n;

	uint16_t usart_tx_pin;
	uint16_t usart_rx_pin;

	uint8_t usart_tx_pinsource;
	uint8_t usart_rx_pinsource;

	uint8_t usart_af_map;

	// Buffer pointers. These need to be global for IRQ handler access
	Ring_Buffer* usart_tx_buffer;
	Ring_Buffer* usart_rx_buffer;

	bool usart_enabled;
	bool usart_transmitting;
} STM32_USART_Info;

/*
 * USART mapping
 */
STM32_USART_Info USART_MAP[TOTAL_USARTS] =
{
		/*
		 * USART_peripheral (USARTx/UARTx; not using others)
		 * clock control register (APBxENR)
		 * clock enable bit value (RCC_APBxPeriph_USARTx/RCC_APBxPeriph_UARTx)
		 * interrupt number (USARTx_IRQn/UARTx_IRQn)
		 * TX pin
		 * RX pin
		 * TX pin source
		 * RX pin source
		 * GPIO AF map (GPIO_AF_USARTx/GPIO_AF_UARTx)
		 * <tx_buffer pointer> used internally and does not appear below
		 * <rx_buffer pointer> used internally and does not appear below
		 * <usart enabled> used internally and does not appear below
		 * <usart transmitting> used internally and does not appear below
		 */
		{ USART1, &RCC->APB2ENR, RCC_APB2Periph_USART1, USART1_IRQn, TX, RX, GPIO_PinSource9, GPIO_PinSource10, GPIO_AF_USART1 }, // USART 1
		{ USART2, &RCC->APB1ENR, RCC_APB1Periph_USART2, USART2_IRQn, RGBG, RGBB, GPIO_PinSource2, GPIO_PinSource3, GPIO_AF_USART2 } // USART 2
#if PLATFORM_ID == 10 // Electron
		,{ USART3, &RCC->APB1ENR, RCC_APB1Periph_USART3, USART3_IRQn, TXD_UC, RXD_UC, GPIO_PinSource10, GPIO_PinSource11, GPIO_AF_USART3 } // USART 3
        ,{ UART4, &RCC->APB1ENR, RCC_APB1Periph_UART4, UART4_IRQn, C3, C2, GPIO_PinSource10, GPIO_PinSource11, GPIO_AF_UART4 } // UART 4
        ,{ UART5, &RCC->APB1ENR, RCC_APB1Periph_UART5, UART5_IRQn, C1, C0, GPIO_PinSource12, GPIO_PinSource2, GPIO_AF_UART5 } // UART 5
#endif
};

static USART_InitTypeDef USART_InitStructure;
static STM32_USART_Info *usartMap[TOTAL_USARTS]; // pointer to USART_MAP[] containing USART peripheral register locations (etc)

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

inline void store_char(unsigned char c, Ring_Buffer *buffer) __attribute__((always_inline));
inline void store_char(unsigned char c, Ring_Buffer *buffer)
{
	unsigned i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

	if (i != buffer->tail)
	{
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
	if(serial == HAL_USART_SERIAL1)
	{
		usartMap[serial] = &USART_MAP[USART_TX_RX];
	}
	else if(serial == HAL_USART_SERIAL2)
	{
		usartMap[serial] = &USART_MAP[USART_RGBG_RGBB];
	}
#if PLATFORM_ID == 10 // Electron
	else if(serial == HAL_USART_SERIAL3)
	{
		usartMap[serial] = &USART_MAP[USART_TXD_UC_RXD_UC];
	}
    else if(serial == HAL_USART_SERIAL4)
    {
        usartMap[serial] = &USART_MAP[USART_C3_C2];
    }
    else if(serial == HAL_USART_SERIAL5)
    {
        usartMap[serial] = &USART_MAP[USART_C1_C0];
    }
#endif

	usartMap[serial]->usart_rx_buffer = rx_buffer;
	usartMap[serial]->usart_tx_buffer = tx_buffer;

	memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(Ring_Buffer));
	memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(Ring_Buffer));

	usartMap[serial]->usart_enabled = false;
	usartMap[serial]->usart_transmitting = false;
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
	USART_DeInit(usartMap[serial]->usart_peripheral);

	// Configure USART Rx and Tx as alternate function push-pull, and enable GPIOA clock
	HAL_Pin_Mode(usartMap[serial]->usart_rx_pin, AF_OUTPUT_PUSHPULL);
	HAL_Pin_Mode(usartMap[serial]->usart_tx_pin, AF_OUTPUT_PUSHPULL);
#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS    // Electron
	if (serial == HAL_USART_SERIAL3)
	{
	    // Configure USART RTS and CTS as alternate function push-pull
	    HAL_Pin_Mode(RTS_UC, AF_OUTPUT_PUSHPULL);
	    HAL_Pin_Mode(CTS_UC, AF_OUTPUT_PUSHPULL);
	}
#endif

	// Enable USART Clock
	*usartMap[serial]->usart_apbReg |=  usartMap[serial]->usart_clock_en;

	// Connect USART pins to AFx
	STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
	GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->usart_rx_pin].gpio_peripheral, usartMap[serial]->usart_rx_pinsource, usartMap[serial]->usart_af_map);
	GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_peripheral, usartMap[serial]->usart_tx_pinsource, usartMap[serial]->usart_af_map);
#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS    // Electron
    if (serial == HAL_USART_SERIAL3)
    {
        GPIO_PinAFConfig(PIN_MAP[RTS_UC].gpio_peripheral, PIN_MAP[RTS_UC].gpio_pin_source, usartMap[serial]->usart_af_map);
        GPIO_PinAFConfig(PIN_MAP[CTS_UC].gpio_peripheral, PIN_MAP[CTS_UC].gpio_pin_source, usartMap[serial]->usart_af_map);
    }
#endif

	// NVIC Configuration
	NVIC_InitTypeDef NVIC_InitStructure;
	// Enable the USART Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->usart_int_n;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// USART default configuration
	// USART configured as follow:
	// - BaudRate = (set baudRate as 9600 baud)
	// - Word Length = 8 Bits
	// - One Stop Bit
	// - No parity
	// - Hardware flow control disabled for Serial1/2/4/5
    // - Hardware flow control enabled (RTS and CTS signals) for Serial3
	// - Receive and transmit enabled
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS    // Electron
    if (serial == HAL_USART_SERIAL3)
    {
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    }
#endif

	// Configure USART
	USART_Init(usartMap[serial]->usart_peripheral, &USART_InitStructure);

	// Enable the USART
	USART_Cmd(usartMap[serial]->usart_peripheral, ENABLE);

	usartMap[serial]->usart_enabled = true;
	usartMap[serial]->usart_transmitting = false;

	// Enable USART Receive and Transmit interrupts
	USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);
	USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_RXNE, ENABLE);
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    // wait for transmission of outgoing data
    while (usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail);

    // Disable the USART
    USART_Cmd(usartMap[serial]->usart_peripheral, DISABLE);

    // Deinitialise USART
    USART_DeInit(usartMap[serial]->usart_peripheral);

    // Disable USART Receive and Transmit interrupts
    USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_RXNE, DISABLE);
    USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);

    NVIC_InitTypeDef NVIC_InitStructure;

    // Disable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->usart_int_n;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

    NVIC_Init(&NVIC_InitStructure);

    // Disable USART Clock
    *usartMap[serial]->usart_apbReg &= ~usartMap[serial]->usart_clock_en;

    // clear any received data
    usartMap[serial]->usart_rx_buffer->head = usartMap[serial]->usart_rx_buffer->tail;

    // Undo any pin re-mapping done for this USART
    // ...

    memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(Ring_Buffer));
    memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(Ring_Buffer));

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
	// interrupts are off and data in queue;
	if ((USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) == RESET)
			&& usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail) {
		// Get him busy
		USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);
	}

	unsigned i = (usartMap[serial]->usart_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE;

	// If the output buffer is full, there's nothing for it other than to
	// wait for the interrupt handler to empty it a bit
	//         no space so       or  Called Off Panic with interrupt off get the message out!
	//         make space                     Enter Polled IO mode
	while (i == usartMap[serial]->usart_tx_buffer->tail || ((__get_PRIMASK() & 1) && usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail) ) {
		// Interrupts are on but they are not being serviced because this was called from a higher
		// Priority interrupt

		if (USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) && USART_GetFlagStatus(usartMap[serial]->usart_peripheral, USART_FLAG_TXE))
		{
			// protect for good measure
			USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);
			// Write out a byte
			USART_SendData(usartMap[serial]->usart_peripheral,  usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->tail++]);
			usartMap[serial]->usart_tx_buffer->tail %= SERIAL_BUFFER_SIZE;
			// unprotect
			USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);
		}
	}

	usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->head] = data;
	usartMap[serial]->usart_tx_buffer->head = i;
	usartMap[serial]->usart_transmitting = true;
	USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);

	return 1;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
	return (unsigned int)(SERIAL_BUFFER_SIZE + usartMap[serial]->usart_rx_buffer->head - usartMap[serial]->usart_rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if (usartMap[serial]->usart_rx_buffer->head == usartMap[serial]->usart_rx_buffer->tail)
	{
		return -1;
	}
	else
	{
		unsigned char c = usartMap[serial]->usart_rx_buffer->buffer[usartMap[serial]->usart_rx_buffer->tail];
		usartMap[serial]->usart_rx_buffer->tail = (unsigned int)(usartMap[serial]->usart_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
		return c;
	}
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
	if (usartMap[serial]->usart_rx_buffer->head == usartMap[serial]->usart_rx_buffer->tail)
	{
		return -1;
	}
	else
	{
		return usartMap[serial]->usart_rx_buffer->buffer[usartMap[serial]->usart_rx_buffer->tail];
	}
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
	// Loop until USART DR register is empty
	while ( usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail );
	// Loop until last frame transmission complete
	while (usartMap[serial]->usart_transmitting && (USART_GetFlagStatus(usartMap[serial]->usart_peripheral, USART_FLAG_TC) == RESET));
	usartMap[serial]->usart_transmitting = false;
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
	return usartMap[serial]->usart_enabled;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{
	USART_HalfDuplexCmd(usartMap[serial]->usart_peripheral, Enable ? ENABLE : DISABLE);
}

// Shared Interrupt Handler for USART2/Serial1 and USART1/Serial2
// WARNING: This function MUST remain reentrance compliant -- no local static variables etc.
static void HAL_USART_Handler(HAL_USART_Serial serial)
{
	if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_RXNE) != RESET)
	{
		// Read byte from the receive data register
		unsigned char c = USART_ReceiveData(usartMap[serial]->usart_peripheral);
		store_char(c, usartMap[serial]->usart_rx_buffer);
	}

	if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) != RESET)
	{
		// Write byte to the transmit data register
		if (usartMap[serial]->usart_tx_buffer->head == usartMap[serial]->usart_tx_buffer->tail)
		{
			// Buffer empty, so disable the USART Transmit interrupt
			USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);
		}
		else
		{
			// There is more data in the output buffer. Send the next byte
			USART_SendData(usartMap[serial]->usart_peripheral, usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->tail++]);
			usartMap[serial]->usart_tx_buffer->tail %= SERIAL_BUFFER_SIZE;
		}
	}

	// // If Overrun occurs, clear the OVR condition
	// if (USART_GetFlagStatus(usartMap[serial]->usart_peripheral, USART_FLAG_ORE) != RESET)
	// {
	// 	(void)USART_ReceiveData(usartMap[serial]->usart_peripheral);
	// 	USART_ClearITPendingBit (usartMap[serial]->usart_peripheral, USART_IT_ORE);
	// }
}

// Serial1 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART1_Handler
 * Description    : This function handles USART1 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART1_Handler(void)
{
	HAL_USART_Handler(HAL_USART_SERIAL1);
}

// Serial2 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART2_Handler
 * Description    : This function handles USART2 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART2_Handler(void)
{
	HAL_USART_Handler(HAL_USART_SERIAL2);
}

#if PLATFORM_ID == 10 // Only Electron
#if 0
// Serial3 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART3_Handler
 * Description    : This function handles USART3 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART3_Handler(void)
{
	HAL_USART_Handler(HAL_USART_SERIAL3);
}
#endif

// Serial4 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART4_Handler
 * Description    : This function handles UART4 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART4_Handler(void)
{
    HAL_USART_Handler(HAL_USART_SERIAL4);
}

// Serial5 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART5_Handler
 * Description    : This function handles UART5 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART5_Handler(void)
{
    HAL_USART_Handler(HAL_USART_SERIAL5);
}
#endif

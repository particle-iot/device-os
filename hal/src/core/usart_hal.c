/**
 ******************************************************************************
 * @file    usart_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
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
#include "gpio_hal.h"
#include "stm32f10x.h"
#include <string.h>
#include "interrupts_hal.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum USART_Num_Def {
  USART_TX_RX = 0,
  USART_D1_D0 = 1
} USART_Num_Def;

/* Private define ------------------------------------------------------------*/
#define GPIO_Remap_None 0

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
typedef struct STM32_USART_Info {
  USART_TypeDef* usart_peripheral;
  __IO uint32_t* usart_apbReg;
  uint32_t usart_clock_en;

  int32_t usart_int_n;

  uint16_t usart_tx_pin;
  uint16_t usart_rx_pin;

  uint32_t usart_pin_remap;

  uint16_t usart_cts_pin;
  uint16_t usart_rts_pin;

  // Buffer pointers. These need to be global for IRQ handler access
  Ring_Buffer* usart_tx_buffer;
  Ring_Buffer* usart_rx_buffer;

  bool usart_enabled;
  bool usart_transmitting;

  uint32_t usart_config;
} STM32_USART_Info;

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
     * CTS pin
     * RTS pin
     * <tx_buffer pointer> used internally and does not appear below
     * <rx_buffer pointer> used internally and does not appear below
     * <usart enabled> used internally and does not appear below
     * <usart transmitting> used internally and does not appear below
     */
    { USART2, &RCC->APB1ENR, RCC_APB1Periph_USART2, USART2_IRQn, TX, RX, GPIO_Remap_None, A0, A1 },
    { USART1, &RCC->APB2ENR, RCC_APB2Periph_USART1, USART1_IRQn, D1, D0, GPIO_Remap_USART1 }
};

static USART_InitTypeDef USART_InitStructure;
// pointer to USART_MAP[] containing USART peripheral register locations (etc)
static STM32_USART_Info *usartMap[TOTAL_USARTS];

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

inline void store_char(uint16_t c, Ring_Buffer *buffer) __attribute__((always_inline));
inline void store_char(uint16_t c, Ring_Buffer *buffer)
{
  unsigned i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

  if (i != buffer->tail)
  {
    buffer->buffer[buffer->head] = c;
    buffer->head = i;
  }
}

static uint8_t HAL_USART_Calculate_Word_Length(uint32_t config, uint8_t noparity);
static uint32_t HAL_USART_Calculate_Data_Bits_Mask(uint32_t config);
static uint8_t HAL_USART_Validate_Config(uint32_t config);
static void HAL_USART_Configure_Transmit_Receive(HAL_USART_Serial serial, uint8_t transmit, uint8_t receive);
static void HAL_USART_Configure_Pin_Modes(HAL_USART_Serial serial, uint32_t config);

uint8_t HAL_USART_Calculate_Word_Length(uint32_t config, uint8_t noparity)
{
  // STM32F1 USARTs support only 8-bit or 9-bit communication, however
  // the parity bit is included in the total word length, so for 8E1 mode
  // the total word length would be 9 bits.
  uint8_t wlen = 0;
  switch (config & SERIAL_DATA_BITS) {
    case SERIAL_DATA_BITS_7:
      wlen += 7;
      break;
    case SERIAL_DATA_BITS_8:
      wlen += 8;
      break;
    case SERIAL_DATA_BITS_9:
      wlen += 9;
      break;
  }

  if ((config & SERIAL_PARITY) && !noparity)
    wlen++;

  if (wlen > 9 || wlen < (noparity ? 7 : 8))
    wlen = 0;

  return wlen;
}

uint32_t HAL_USART_Calculate_Data_Bits_Mask(uint32_t config)
{
  return (1 << HAL_USART_Calculate_Word_Length(config, 1)) - 1;
}

uint8_t HAL_USART_Validate_Config(uint32_t config)
{
  // Total word length should be either 8 or 9 bits
  if (HAL_USART_Calculate_Word_Length(config, 0) == 0)
    return 0;

  // Either No, Even or Odd parity
  if ((config & SERIAL_PARITY) == (SERIAL_PARITY_EVEN | SERIAL_PARITY_ODD))
    return 0;

  if ((config & SERIAL_HALF_DUPLEX) && (config & LIN_MODE))
    return 0;

  if (config & LIN_MODE)
  {
    // Either Master or Slave mode
    // Break detection can still be enabled in both Master and Slave modes
    if ((config & LIN_MODE_MASTER) && (config & LIN_MODE_SLAVE))
      return 0;
    switch (config & LIN_BREAK_BITS)
    {
      case LIN_BREAK_13B:
      case LIN_BREAK_10B:
      case LIN_BREAK_11B:
        break;
      default:
        return 0;
    }
  }

  return 1;
}

void HAL_USART_Configure_Transmit_Receive(HAL_USART_Serial serial, uint8_t transmit, uint8_t receive)
{
  uint32_t toset = 0;
  if (transmit) {
    toset |= ((uint32_t)USART_CR1_TE);
  }
  if (receive) {
    toset |= ((uint32_t)USART_CR1_RE);
  }
  uint32_t tmp = usartMap[serial]->usart_peripheral->CR1;
  if ((tmp & ((uint32_t)(USART_CR1_TE | USART_CR1_RE))) != toset) {
    tmp &= ~((uint32_t)(USART_CR1_TE | USART_CR1_RE));
    tmp |= toset;
    usartMap[serial]->usart_peripheral->CR1 = tmp;
  }
}

void HAL_USART_Configure_Pin_Modes(HAL_USART_Serial serial, uint32_t config)
{
  // Configure USART Rx as input floating
  HAL_Pin_Mode(usartMap[serial]->usart_rx_pin, INPUT);
  if ((config & SERIAL_HALF_DUPLEX) == 0) {
    // Configure USART Tx as alternate function push-pull
    HAL_Pin_Mode(usartMap[serial]->usart_tx_pin, AF_OUTPUT_PUSHPULL);
  } else if ((config & SERIAL_OPEN_DRAIN)) {
    // Half-duplex with open drain
    HAL_Pin_Mode(usartMap[serial]->usart_tx_pin, AF_OUTPUT_DRAIN);
  } else {
    // Half-duplex with push-pull
    /* RM0008 27.3.10:
     * TX is always released when no data is transmitted. Thus, it acts as a standard IO in idle
     * or in reception. It means that the IO must be configured so that TX is configured as
     * floating input (or output high open-drain) when not driven by the USART.
     */
    HAL_Pin_Mode(usartMap[serial]->usart_tx_pin, AF_OUTPUT_PUSHPULL);
  }
  // Remap USARTn to alternate pins EG. USART1 to pins TX/PB6, RX/PB7
  GPIO_PinRemapConfig(usartMap[serial]->usart_pin_remap, ENABLE);
}

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
  if(serial == HAL_USART_SERIAL1)
  {
    usartMap[serial] = &USART_MAP[USART_TX_RX];
  }
  else if(serial == HAL_USART_SERIAL2)
  {
    usartMap[serial] = &USART_MAP[USART_D1_D0];
  }

  usartMap[serial]->usart_rx_buffer = rx_buffer;
  usartMap[serial]->usart_tx_buffer = tx_buffer;

  memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(Ring_Buffer));
  memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(Ring_Buffer));

  usartMap[serial]->usart_enabled = false;
  usartMap[serial]->usart_transmitting = false;
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
  HAL_USART_BeginConfig(serial, baud, 0, 0); //Default serial configuration is 8N1
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void *ptr)
{
  // Verify UART configuration, exit if it's invalid.
  if (!HAL_USART_Validate_Config(config)) {
    usartMap[serial]->usart_enabled = false;
    return;
  }

  usartMap[serial]->usart_enabled = false;

  // AFIO clock enable
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  // Enable USART Clock
  *usartMap[serial]->usart_apbReg |=  usartMap[serial]->usart_clock_en;

  NVIC_InitTypeDef NVIC_InitStructure;

  // Enable the USART Interrupt
  NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->usart_int_n;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;//USART2_IRQ_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);

  HAL_USART_Configure_Pin_Modes(serial, config);

  // USART default configuration
  // USART configured as follow:
  // - BaudRate = (set baudRate as 9600 baud)
  // - Word Length = 8 Bits
  // - One Stop Bit
  // - No parity
  // - Hardware flow control disabled (RTS and CTS signals)
  // - Receive and transmit enabled
  // USART configuration
  USART_InitStructure.USART_BaudRate = baud;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  switch (config & SERIAL_FLOW_CONTROL) {
    case SERIAL_FLOW_CONTROL_CTS:
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
      break;
    case SERIAL_FLOW_CONTROL_RTS:
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
      break;
    case SERIAL_FLOW_CONTROL_RTS_CTS:
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
      break;
    case SERIAL_FLOW_CONTROL_NONE:
    default:
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
      break;
  }

  if (serial == HAL_USART_SERIAL2) {
    // USART1 supports hardware flow control, but RTS and CTS pins are not exposed
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  }

  switch (USART_InitStructure.USART_HardwareFlowControl) {
    case USART_HardwareFlowControl_CTS:
      HAL_Pin_Mode(usartMap[serial]->usart_cts_pin, AF_OUTPUT_PUSHPULL);
      break;
    case USART_HardwareFlowControl_RTS:
      HAL_Pin_Mode(usartMap[serial]->usart_rts_pin, AF_OUTPUT_PUSHPULL);
      break;
    case USART_HardwareFlowControl_RTS_CTS:
      HAL_Pin_Mode(usartMap[serial]->usart_cts_pin, AF_OUTPUT_PUSHPULL);
      HAL_Pin_Mode(usartMap[serial]->usart_rts_pin, AF_OUTPUT_PUSHPULL);
      break;
    case USART_HardwareFlowControl_None:
    default:
      break;
  }

  // Stop bit configuration.
  switch (config & SERIAL_STOP_BITS) {
    case SERIAL_STOP_BITS_1: // 1 stop bit
      USART_InitStructure.USART_StopBits = USART_StopBits_1;
      break;
    case SERIAL_STOP_BITS_2: // 2 stop bits
      USART_InitStructure.USART_StopBits = USART_StopBits_2;
      break;
    case SERIAL_STOP_BITS_0_5: // 0.5 stop bits
      USART_InitStructure.USART_StopBits = USART_StopBits_0_5;
      break;
    case SERIAL_STOP_BITS_1_5: // 1.5 stop bits
      USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
      break;
  }

  // Data bits configuration
  switch (HAL_USART_Calculate_Word_Length(config, 0)) {
    case 8:
      USART_InitStructure.USART_WordLength = USART_WordLength_8b;
      break;
    case 9:
      USART_InitStructure.USART_WordLength = USART_WordLength_9b;
      break;
  }

  // Parity configuration
  switch (config & SERIAL_PARITY) {
    case SERIAL_PARITY_NO:
      USART_InitStructure.USART_Parity = USART_Parity_No;
      break;
    case SERIAL_PARITY_EVEN:
      USART_InitStructure.USART_Parity = USART_Parity_Even;
      break;
    case SERIAL_PARITY_ODD:
      USART_InitStructure.USART_Parity = USART_Parity_Odd;
      break;
  }

  // Disable LIN mode just in case
  USART_LINCmd(usartMap[serial]->usart_peripheral, DISABLE);

  // Configure USART
  USART_Init(usartMap[serial]->usart_peripheral, &USART_InitStructure);

  // LIN configuration
  if (config & LIN_MODE) {
    // Enable break detection
    switch(config & LIN_BREAK_BITS) {
      case LIN_BREAK_10B:
        USART_LINBreakDetectLengthConfig(usartMap[serial]->usart_peripheral, USART_LINBreakDetectLength_10b);
        break;
      case LIN_BREAK_11B:
        USART_LINBreakDetectLengthConfig(usartMap[serial]->usart_peripheral, USART_LINBreakDetectLength_11b);
        break;
    }
  }


  // Enable USART Receive and Transmit interrupts
  USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_RXNE, ENABLE);
  USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);

  usartMap[serial]->usart_config = config;
  if (config & SERIAL_HALF_DUPLEX) {
    HAL_USART_Half_Duplex(serial, ENABLE);
  }

  USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TC, DISABLE);

  // Enable the USART
  USART_Cmd(usartMap[serial]->usart_peripheral, ENABLE);

  if (config & LIN_MODE) {
    USART_LINCmd(usartMap[serial]->usart_peripheral, ENABLE);
  }

  usartMap[serial]->usart_enabled = true;
  usartMap[serial]->usart_transmitting = false;
}

void HAL_USART_End(HAL_USART_Serial serial)
{
  // wait for transmission of outgoing data
  while (usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail);

  // Disable the USART
  USART_Cmd(usartMap[serial]->usart_peripheral, DISABLE);

  // Switch pins to INPUT
  HAL_Pin_Mode(usartMap[serial]->usart_rx_pin, INPUT);
  HAL_Pin_Mode(usartMap[serial]->usart_tx_pin, INPUT);

  // Disable LIN mode
  USART_LINCmd(usartMap[serial]->usart_peripheral, DISABLE);

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
  GPIO_PinRemapConfig(usartMap[serial]->usart_pin_remap, DISABLE);

  memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(Ring_Buffer));
  memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(Ring_Buffer));

  usartMap[serial]->usart_enabled = false;
  usartMap[serial]->usart_transmitting = false;
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
  int32_t tail = usartMap[serial]->usart_tx_buffer->tail;
  int32_t available = SERIAL_BUFFER_SIZE - (usartMap[serial]->usart_tx_buffer->head >= tail ?
    usartMap[serial]->usart_tx_buffer->head - tail :
    (SERIAL_BUFFER_SIZE + usartMap[serial]->usart_tx_buffer->head - tail) - 1);

  return available;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
  return HAL_USART_Write_NineBitData(serial, data);
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data)
{
  // Remove any bits exceeding data bits configured
  data &= HAL_USART_Calculate_Data_Bits_Mask(usartMap[serial]->usart_config);
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
  while (i == usartMap[serial]->usart_tx_buffer->tail || ((__get_PRIMASK() & 1) &&
    usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail) ) {
    // Interrupts are on but they are not being serviced because this was called from a higher
    // Priority interrupt

    if (USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) &&
      USART_GetFlagStatus(usartMap[serial]->usart_peripheral, USART_FLAG_TXE))
    {
      // protect for good measure
      USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);
      // Write out a byte
      USART_SendData(usartMap[serial]->usart_peripheral,
        usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->tail++]);
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
  return (unsigned int)(SERIAL_BUFFER_SIZE + usartMap[serial]->usart_rx_buffer->head -
    usartMap[serial]->usart_rx_buffer->tail) % SERIAL_BUFFER_SIZE;
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
    uint16_t c = usartMap[serial]->usart_rx_buffer->buffer[usartMap[serial]->usart_rx_buffer->tail];
    usartMap[serial]->usart_rx_buffer->tail =
      (unsigned int)(usartMap[serial]->usart_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
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
  while (usartMap[serial]->usart_transmitting &&
    (USART_GetFlagStatus(usartMap[serial]->usart_peripheral, USART_FLAG_TC) == RESET));
  usartMap[serial]->usart_transmitting = false;
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
  return usartMap[serial]->usart_enabled;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{
  if (HAL_USART_Is_Enabled(serial)) {
    USART_Cmd(usartMap[serial]->usart_peripheral, DISABLE);
  }
  if (Enable) {
    usartMap[serial]->usart_config |= SERIAL_HALF_DUPLEX;
  } else {
    usartMap[serial]->usart_config &= ~(SERIAL_HALF_DUPLEX);
  }
  HAL_USART_Configure_Pin_Modes(serial, usartMap[serial]->usart_config);
  // These bits need to be cleared according to the reference manual
  usartMap[serial]->usart_peripheral->CR2 &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);
  usartMap[serial]->usart_peripheral->CR3 &= ~(USART_CR3_IREN | USART_CR3_SCEN);
  USART_HalfDuplexCmd(usartMap[serial]->usart_peripheral, Enable ? ENABLE : DISABLE);
  if (HAL_USART_Is_Enabled(serial)) {
    USART_Cmd(usartMap[serial]->usart_peripheral, ENABLE);
  }

}

void HAL_USART_Send_Break(HAL_USART_Serial serial, void* reserved)
{
  int32_t state = HAL_disable_irq();
  while((usartMap[serial]->usart_peripheral->CR1 & USART_CR1_SBK) == SET);
  USART_SendBreak(usartMap[serial]->usart_peripheral);
  while((usartMap[serial]->usart_peripheral->CR1 & USART_CR1_SBK) == SET);
  HAL_enable_irq(state);
}

uint8_t HAL_USART_Break_Detected(HAL_USART_Serial serial)
{
  if (USART_GetFlagStatus(usartMap[serial]->usart_peripheral, USART_FLAG_LBD)) {
    // Clear LBD flag
    USART_ClearFlag(usartMap[serial]->usart_peripheral, USART_FLAG_LBD);
    return 1;
  }

  return 0;
}

// Shared Interrupt Handler for USART2/Serial1 and USART1/Serial2
// WARNING: This function MUST remain reentrance compliant -- no local static variables etc.
static void HAL_USART_Handler(HAL_USART_Serial serial)
{
  if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_RXNE) != RESET)
  {
    // Read byte from the receive data register
    uint16_t c = USART_ReceiveData(usartMap[serial]->usart_peripheral);
    // Remove parity bits from data
    c &= HAL_USART_Calculate_Data_Bits_Mask(usartMap[serial]->usart_config);
    store_char(c, usartMap[serial]->usart_rx_buffer);
  }

  uint8_t noecho = (usartMap[serial]->usart_config & (SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO)) == (SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);

  if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TC) != RESET) {
    if (noecho) {
      USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TC, DISABLE);
      HAL_USART_Configure_Transmit_Receive(serial, 0, 1);
    }
  }

  if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) != RESET)
  {
    // Write byte to the transmit data register
    if (usartMap[serial]->usart_tx_buffer->head == usartMap[serial]->usart_tx_buffer->tail)
    {
      // Buffer empty, so disable the USART Transmit interrupt
      USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);
      if (noecho) {
        USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TC, ENABLE);
      }
    }
    else
    {
      if (noecho) {
        HAL_USART_Configure_Transmit_Receive(serial, 1, 0);
      }

      // There is more data in the output buffer. Send the next byte
      USART_SendData(usartMap[serial]->usart_peripheral,
        usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->tail++]);
      usartMap[serial]->usart_tx_buffer->tail %= SERIAL_BUFFER_SIZE;
    }
  }
}

// Serial1 interrupt handler
// Serial1 uses standard Sparkcore pins PA2/TX(TX), PA3/RX(RX)
/*******************************************************************************
 * Function Name  : HAL_USART2_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles USART2 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART2_Handler(void)
{
  HAL_USART_Handler(HAL_USART_SERIAL1);
}

// Serial2 interrupt handler
// Serial2 uses alternate function pins PB6/D1(TX), PB7/D0(RX)
/*******************************************************************************
 * Function Name  : HAL_USART1_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles USART1 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART1_Handler(void)
{
  HAL_USART_Handler(HAL_USART_SERIAL2);
}

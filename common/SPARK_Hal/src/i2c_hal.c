/**
 ******************************************************************************
 * @file    i2c_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define BUFFER_LENGTH   32
#define EVENT_TIMEOUT   100

#define TRANSMITTER     0x00
#define RECEIVER        0x01

/* Private variables ---------------------------------------------------------*/
static I2C_InitTypeDef I2C_InitStructure;

static uint32_t I2C_ClockSpeed = CLOCK_SPEED_100KHZ;
static bool I2C_Enabled = false;

static uint8_t rxBuffer[BUFFER_LENGTH];
static uint8_t rxBufferIndex = 0;
static uint8_t rxBufferLength = 0;

static uint8_t txAddress = 0;
static uint8_t txBuffer[BUFFER_LENGTH];
static uint8_t txBufferIndex = 0;
static uint8_t txBufferLength = 0;

static uint8_t transmitting = 0;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void (*user_onRequest)(void);
static void (*user_onReceive)(int);

//Initializes DMA channel used by the I2C1 peripheral based on Direction
static void TwoWire_DMAConfig(uint8_t *pBuffer, uint32_t BufferSize, uint32_t Direction)
{
  DMA_InitTypeDef  DMA_InitStructure;

  /* Configure the DMA Tx/Rx Channel with the buffer address and the buffer size */
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)0x40005410;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pBuffer;
  DMA_InitStructure.DMA_BufferSize = (uint32_t)BufferSize;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

  if (Direction == TRANSMITTER)
  {
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    /* Disable DMA TX Channel */
    DMA_Cmd(DMA1_Channel6, DISABLE);
    /* DMA1 channel6 configuration */
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);
  }
  else
  {
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    /* Disable DMA RX Channel */
    DMA_Cmd(DMA1_Channel7, DISABLE);
    /* DMA1 channel7 configuration */
    DMA_Init(DMA1_Channel7, &DMA_InitStructure);
  }
}

void HAL_I2C_Set_Speed(uint32_t speed)
{
  I2C_ClockSpeed = speed;
}

void HAL_I2C_Stretch_Clock(bool stretch)
{
  if(stretch == true)
  {
    I2C_StretchClockCmd(I2C1, ENABLE);
  }
  else
  {
    I2C_StretchClockCmd(I2C1, DISABLE);
  }
}

void HAL_I2C_Begin(I2C_Mode mode, uint8_t address)
{
  rxBufferIndex = 0;
  rxBufferLength = 0;

  txBufferIndex = 0;
  txBufferLength = 0;

  /* Enable I2C1 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
  /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  HAL_Pin_Mode(SCL, AF_OUTPUT_DRAIN);
  HAL_Pin_Mode(SDA, AF_OUTPUT_DRAIN);

  if(mode != I2C_MODE_MASTER)
  {
    NVIC_InitTypeDef  NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);
  }

  I2C_DeInit(I2C1);

  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = address << 1;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_ClockSpeed;
  I2C_Init(I2C1, &I2C_InitStructure);

  I2C_Cmd(I2C1, ENABLE);

  if(mode != I2C_MODE_MASTER)
  {
    I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
  }

  I2C_Enabled = true;
}

void HAL_I2C_End(void)
{
  if(I2C_Enabled != false)
  {
    I2C_Cmd(I2C1, DISABLE);

    I2C_Enabled = false;
  }
}

uint32_t HAL_I2C_Request_Data(uint8_t address, uint8_t quantity, uint8_t stop)
{
  uint32_t _millis;

  // clamp to buffer length
  if(quantity > BUFFER_LENGTH)
  {
    quantity = BUFFER_LENGTH;
  }

  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);

  _millis = HAL_Timer_Get_Milli_Seconds();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis)) return 0;
  }

  /* Send Slave address for read */
  I2C_Send7bitAddress(I2C1, address, I2C_Direction_Receiver);

  _millis = HAL_Timer_Get_Milli_Seconds();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis)) return 0;
  }

  TwoWire_DMAConfig(rxBuffer, quantity, RECEIVER);

  /* Enable DMA NACK automatic generation */
  I2C_DMALastTransferCmd(I2C1, ENABLE);

  /* Enable I2C DMA request */
  I2C_DMACmd(I2C1, ENABLE);

  /* Enable DMA RX Channel */
  DMA_Cmd(DMA1_Channel7, ENABLE);

  /* Wait until DMA Transfer Complete */
  _millis = HAL_Timer_Get_Milli_Seconds();
  while(!DMA_GetFlagStatus(DMA1_FLAG_TC7))
  {
    if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis)) break;
  }

  /* Disable DMA RX Channel */
  DMA_Cmd(DMA1_Channel7, DISABLE);

  /* Disable I2C DMA request */
  I2C_DMACmd(I2C1, DISABLE);

  /* Clear DMA RX Transfer Complete Flag */
  DMA_ClearFlag(DMA1_FLAG_TC7);

  /* Send STOP Condition */
  if(stop == true)
  {
    /* Send STOP condition */
    I2C_GenerateSTOP(I2C1, ENABLE);
  }

  /* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C1, ENABLE);

  // set rx buffer iterator vars
  rxBufferIndex = 0;
  rxBufferLength = quantity - DMA_GetCurrDataCounter(DMA1_Channel7);

  return rxBufferLength;
}

void HAL_I2C_Begin_Transmission(uint8_t address)
{
  // indicate that we are transmitting
  transmitting = 1;
  // set address of targeted slave
  txAddress = address;
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
}

uint8_t HAL_I2C_End_Transmission(uint8_t stop)
{
  uint32_t _millis;

  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);

  _millis = HAL_Timer_Get_Milli_Seconds();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis)) return 4;
  }

  /* Send Slave address for write */
  I2C_Send7bitAddress(I2C1, txAddress, I2C_Direction_Transmitter);

  _millis = HAL_Timer_Get_Milli_Seconds();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis)) return 4;
  }

  TwoWire_DMAConfig(txBuffer, txBufferLength+1, TRANSMITTER);

  /* Enable DMA NACK automatic generation */
  I2C_DMALastTransferCmd(I2C1, ENABLE);

  /* Enable I2C DMA request */
  I2C_DMACmd(I2C1, ENABLE);

  /* Enable DMA TX Channel */
  DMA_Cmd(DMA1_Channel6, ENABLE);

  /* Wait until DMA Transfer Complete */
  _millis = HAL_Timer_Get_Milli_Seconds();
  while(!DMA_GetFlagStatus(DMA1_FLAG_TC6))
  {
    if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis)) return 4;
  }

  /* Disable DMA TX Channel */
  DMA_Cmd(DMA1_Channel6, DISABLE);

  /* Disable I2C DMA request */
  I2C_DMACmd(I2C1, DISABLE);

  /* Clear DMA TX Transfer Complete Flag */
  DMA_ClearFlag(DMA1_FLAG_TC6);

  /* Send STOP Condition */
  if(stop == true)
  {
    /* Send STOP condition */
    I2C_GenerateSTOP(I2C1, ENABLE);
  }

  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;

  // indicate that we are done transmitting
  transmitting = 0;

  return 0;
}

uint32_t HAL_I2C_Write_Data(uint8_t data)
{
  if(transmitting)
  {
    // in master/slave transmitter mode
    // don't bother if buffer is full
    if(txBufferLength >= BUFFER_LENGTH)
    {
      return 0;
    }
    // put byte in tx buffer
    txBuffer[txBufferIndex++] = data;
    // update amount in buffer
    txBufferLength = txBufferIndex;
  }

  return 1;
}

int32_t HAL_I2C_Available_Data(void)
{
  return rxBufferLength - rxBufferIndex;
}

int32_t HAL_I2C_Read_Data(void)
{
  int value = -1;

  // get each successive byte on each call
  if(rxBufferIndex < rxBufferLength)
  {
    value = rxBuffer[rxBufferIndex++];
  }

  return value;
}

int32_t HAL_I2C_Peek_Data(void)
{
  int value = -1;

  if(rxBufferIndex < rxBufferLength)
  {
    value = rxBuffer[rxBufferIndex];
  }

  return value;
}

void HAL_I2C_Flush_Data(void)
{
  // XXX: to be implemented.
}

bool HAL_I2C_Is_Enabled(void)
{
 return I2C_Enabled;
}

void HAL_I2C_Set_Callback_On_Receive(void (*function)(int))
{
  user_onReceive = function;
}

void HAL_I2C_Set_Callback_On_Request(void (*function)(void))
{
  user_onRequest = function;
}

void HAL_I2C1_EV_Interrupt_Handler(void)
{
  //Slave Event Handler
  __IO uint32_t SR1Register = 0;
  __IO uint32_t SR2Register = 0;

  /* Read the I2C1 SR1 and SR2 status registers */
  SR1Register = I2C1->SR1;
  SR2Register = I2C1->SR2;

  /* If I2C1 is slave (MSL flag = 0) */
  if ((SR2Register &0x0001) != 0x0001)
  {
    /* If ADDR = 1: EV1 */
    if ((SR1Register & 0x0002) == 0x0002)
    {
      /* Clear SR1Register and SR2Register variables to prepare for next IT */
      SR1Register = 0;
      SR2Register = 0;

      // indicate that we are transmitting
      transmitting = 1;
      // reset tx buffer iterator vars
      txBufferIndex = 0;
      txBufferLength = 0;
      // set rx buffer iterator vars
      rxBufferIndex = 0;
      rxBufferLength = 0;

      if(NULL != user_onRequest)
      {
        // alert user program
        user_onRequest();
      }

      TwoWire_DMAConfig(txBuffer, txBufferLength+1, TRANSMITTER);
      /* Enable DMA NACK automatic generation */
      I2C_DMALastTransferCmd(I2C1, ENABLE);
      /* Enable I2C DMA request */
      I2C_DMACmd(I2C1, ENABLE);
      /* Enable DMA TX Channel */
      DMA_Cmd(DMA1_Channel6, ENABLE);

      TwoWire_DMAConfig(rxBuffer, BUFFER_LENGTH, RECEIVER);
      /* Enable DMA NACK automatic generation */
      I2C_DMALastTransferCmd(I2C1, ENABLE);
      /* Enable I2C DMA request */
      I2C_DMACmd(I2C1, ENABLE);
      /* Enable DMA RX Channel */
      DMA_Cmd(DMA1_Channel7, ENABLE);
    }

    /* If STOPF = 1: EV4 (Slave has detected a STOP condition on the bus */
    if (( SR1Register & 0x0010) == 0x0010)
    {
      I2C1->CR1 |= ((uint16_t)0x0001);
      SR1Register = 0;
      SR2Register = 0;

      // indicate that we are done transmitting
      transmitting = 0;
      // set rx buffer iterator vars
      rxBufferIndex = 0;
      rxBufferLength = BUFFER_LENGTH - DMA_GetCurrDataCounter(DMA1_Channel7);

      if(NULL != user_onReceive)
      {
        // alert user program
        user_onReceive(rxBufferLength);
      }
    }
  }
}

void HAL_I2C1_ER_Interrupt_Handler(void)
{
  __IO uint32_t SR1Register = 0;

  /* Read the I2C1 status register */
  SR1Register = I2C1->SR1;

  /* If AF = 1 */
  if ((SR1Register & 0x0400) == 0x0400)
  {
    I2C1->SR1 &= 0xFBFF;
    SR1Register = 0;
  }

  /* If ARLO = 1 */
  if ((SR1Register & 0x0200) == 0x0200)
  {
    I2C1->SR1 &= 0xFBFF;
    SR1Register = 0;
  }

  /* If BERR = 1 */
  if ((SR1Register & 0x0100) == 0x0100)
  {
    I2C1->SR1 &= 0xFEFF;
    SR1Register = 0;
  }

  /* If OVR = 1 */
  if ((SR1Register & 0x0800) == 0x0800)
  {
    I2C1->SR1 &= 0xF7FF;
    SR1Register = 0;
  }
}

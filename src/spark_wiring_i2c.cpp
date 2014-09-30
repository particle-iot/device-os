/**
 ******************************************************************************
 * @file    spark_wiring_i2c.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
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

#include "spark_wiring_i2c.h"

// Initialize Class Variables //////////////////////////////////////////////////
I2C_InitTypeDef TwoWire::I2C_InitStructure;

uint32_t TwoWire::I2C_ClockSpeed = CLOCK_SPEED_100KHZ;
bool TwoWire::I2C_EnableDMAMode = false;
bool TwoWire::I2C_SetAsSlave = false;
bool TwoWire::I2C_Enabled = false;

uint8_t TwoWire::rxBuffer[BUFFER_LENGTH];
uint8_t TwoWire::rxBufferIndex = 0;
uint8_t TwoWire::rxBufferLength = 0;

uint8_t TwoWire::txAddress = 0;
uint8_t TwoWire::txBuffer[BUFFER_LENGTH];
uint8_t TwoWire::txBufferIndex = 0;
uint8_t TwoWire::txBufferLength = 0;

uint8_t TwoWire::transmitting = 0;
void (*TwoWire::user_onRequest)(void);
void (*TwoWire::user_onReceive)(int);

#define TRANSMITTER             0x00
#define RECEIVER                0x01

//Initializes DMA channel used by the I2C1 peripheral based on Direction
void TwoWire_DMAConfig(uint8_t *pBuffer, uint32_t BufferSize, uint32_t Direction)
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

// Constructors ////////////////////////////////////////////////////////////////

TwoWire::TwoWire()
{
}

// Public Methods //////////////////////////////////////////////////////////////

//setSpeed() should be called before begin() else default to 100KHz
void TwoWire::setSpeed(uint32_t clockSpeed)
{
  I2C_ClockSpeed = clockSpeed;
}

//enableDMAMode(true) should be called before begin() else default polling mode used
void TwoWire::enableDMAMode(bool enableDMAMode)
{
  I2C_EnableDMAMode = enableDMAMode;
}

void TwoWire::stretchClock(bool stretch)
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

void TwoWire::begin(void)
{
  rxBufferIndex = 0;
  rxBufferLength = 0;

  txBufferIndex = 0;
  txBufferLength = 0;

  /* Enable I2C1 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

  if(I2C_EnableDMAMode)
  {
    /* Enable DMA1 clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  }

  pinMode(SCL, AF_OUTPUT_DRAIN);
  pinMode(SDA, AF_OUTPUT_DRAIN);

  I2C_DeInit(I2C1);

  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  if(I2C_SetAsSlave != true)
  {
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  }
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_ClockSpeed;
  I2C_Init(I2C1, &I2C_InitStructure);

  I2C_Cmd(I2C1, ENABLE);

  I2C_Enabled = true;
}

void TwoWire::begin(uint8_t address)
{
  NVIC_InitTypeDef  NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
  NVIC_Init(&NVIC_InitStructure);

  I2C_SetAsSlave = true;

  I2C_InitStructure.I2C_OwnAddress1 = address << 1;

  begin();

  I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
}

void TwoWire::begin(int address)
{
  begin((uint8_t)address);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
  system_tick_t _millis;
  uint8_t bytesRead = 0;

  // clamp to buffer length
  if(quantity > BUFFER_LENGTH)
  {
    quantity = BUFFER_LENGTH;
  }

  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);

  _millis = millis();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if(EVENT_TIMEOUT < (millis() - _millis)) return 0;
  }

  /* Send Slave address for read */
  I2C_Send7bitAddress(I2C1, address, I2C_Direction_Receiver);

  _millis = millis();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if(EVENT_TIMEOUT < (millis() - _millis)) return 0;
  }

  if(I2C_EnableDMAMode)
  {
    TwoWire_DMAConfig(rxBuffer, quantity, RECEIVER);

    /* Enable DMA NACK automatic generation */
    I2C_DMALastTransferCmd(I2C1, ENABLE);

    /* Enable I2C DMA request */
    I2C_DMACmd(I2C1, ENABLE);

    /* Enable DMA RX Channel */
    DMA_Cmd(DMA1_Channel7, ENABLE);

    /* Wait until DMA Transfer Complete */
    _millis = millis();
    while(!DMA_GetFlagStatus(DMA1_FLAG_TC7))
    {
      if(EVENT_TIMEOUT < (millis() - _millis)) break;
    }

    /* Disable DMA RX Channel */
    DMA_Cmd(DMA1_Channel7, DISABLE);

    /* Disable I2C DMA request */
    I2C_DMACmd(I2C1, DISABLE);

    /* Clear DMA RX Transfer Complete Flag */
    DMA_ClearFlag(DMA1_FLAG_TC7);

    /* Send STOP Condition */
    if(sendStop == true)
    {
      /* Send STOP condition */
      I2C_GenerateSTOP(I2C1, ENABLE);
    }

    bytesRead = quantity - DMA_GetCurrDataCounter(DMA1_Channel7);
  }
  else
  {
    /* perform blocking read into buffer */
    uint8_t *pBuffer = rxBuffer;
    uint8_t numByteToRead = quantity;

    /* While there is data to be read */
    _millis = millis();
    while(numByteToRead && (EVENT_TIMEOUT > (millis() - _millis)))
    {
      if(numByteToRead == 1 && sendStop == true)
      {
        /* Disable Acknowledgement */
        I2C_AcknowledgeConfig(I2C1, DISABLE);

        /* Send STOP Condition */
        I2C_GenerateSTOP(I2C1, ENABLE);
      }

      if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
      {
        /* Read a byte from the Slave */
        *pBuffer = I2C_ReceiveData(I2C1);

        bytesRead++;

        /* Point to the next location where the byte read will be saved */
        pBuffer++;

        /* Decrement the read bytes counter */
        numByteToRead--;

        /* Reset timeout to our last read */
        _millis = millis();
      }
    }
  }

  /* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C1, ENABLE);

  // set rx buffer iterator vars
  rxBufferIndex = 0;
  rxBufferLength = bytesRead;

  return bytesRead;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
}

void TwoWire::beginTransmission(uint8_t address)
{
  // indicate that we are transmitting
  transmitting = 1;
  // set address of targeted slave
  txAddress = address;
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
}

void TwoWire::beginTransmission(int address)
{
  beginTransmission((uint8_t)address);
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to 
//	perform a repeated start. 
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(uint8_t sendStop)
{
  uint32_t _millis;

  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);

  _millis = millis();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
  }

  /* Send Slave address for write */
  I2C_Send7bitAddress(I2C1, txAddress, I2C_Direction_Transmitter);

  _millis = millis();
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
  }

  if(I2C_EnableDMAMode)
  {
    TwoWire_DMAConfig(txBuffer, txBufferLength+1, TRANSMITTER);

    /* Enable DMA NACK automatic generation */
    I2C_DMALastTransferCmd(I2C1, ENABLE);

    /* Enable I2C DMA request */
    I2C_DMACmd(I2C1, ENABLE);

    /* Enable DMA TX Channel */
    DMA_Cmd(DMA1_Channel6, ENABLE);

    /* Wait until DMA Transfer Complete */
    _millis = millis();
    while(!DMA_GetFlagStatus(DMA1_FLAG_TC6))
    {
      if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
    }

    /* Disable DMA TX Channel */
    DMA_Cmd(DMA1_Channel6, DISABLE);

    /* Disable I2C DMA request */
    I2C_DMACmd(I2C1, DISABLE);

    /* Clear DMA TX Transfer Complete Flag */
    DMA_ClearFlag(DMA1_FLAG_TC6);
  }
  else
  {
    uint8_t *pBuffer = txBuffer;
    uint8_t NumByteToWrite = txBufferLength;

    /* While there is data to be written */
    while(NumByteToWrite--)
    {
      /* Send the current byte to slave */
      I2C_SendData(I2C1, *pBuffer);

      /* Point to the next byte to be written */
      pBuffer++;

      _millis = millis();
      while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
      {
        if(EVENT_TIMEOUT < (millis() - _millis)) return 4;
      }
    }
  }

  /* Send STOP Condition */
  if(sendStop == true)
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

//	This provides backwards compatibility with the original
//	definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire::endTransmission(void)
{
  return endTransmission(true);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(uint8_t data)
{
  if(transmitting)
  {
    // in master/slave transmitter mode
    // don't bother if buffer is full
    if(txBufferLength >= BUFFER_LENGTH)
    {
      setWriteError();
      return 0;
    }
    // put byte in tx buffer
    txBuffer[txBufferIndex++] = data;
    // update amount in buffer
    txBufferLength = txBufferIndex;
  }

  return 1;
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
  if(transmitting)
  {
    // in master/slave transmitter mode
    for(size_t i = 0; i < quantity; ++i)
    {
      write(data[i]);
    }
  }

  return quantity;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::available(void)
{
  return rxBufferLength - rxBufferIndex;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::read(void)
{
  int value = -1;

  // get each successive byte on each call
  if(rxBufferIndex < rxBufferLength)
  {
    value = rxBuffer[rxBufferIndex++];
  }

  return value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::peek(void)
{
  int value = -1;

  if(rxBufferIndex < rxBufferLength)
  {
    value = rxBuffer[rxBufferIndex];
  }

  return value;
}

void TwoWire::flush(void)
{
  // XXX: to be implemented.
}

// sets function called on slave write
void TwoWire::onReceive( void (*function)(int) )
{
  user_onReceive = function;
}

// sets function called on slave read
void TwoWire::onRequest( void (*function)(void) )
{
  user_onRequest = function;
}

void TwoWire::slaveEventHandler(void)
{
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

/*******************************************************************************
 * Function Name  : Wiring_I2C1_EV_Interrupt_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Event interrupt request(Only for Slave mode).
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Wiring_I2C1_EV_Interrupt_Handler(void)
{
  TwoWire::slaveEventHandler();
}

/*******************************************************************************
 * Function Name  : Wiring_I2C1_ER_Interrupt_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Error interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Wiring_I2C1_ER_Interrupt_Handler(void)
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

bool TwoWire::isEnabled()
{
  return I2C_Enabled;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

TwoWire Wire = TwoWire();


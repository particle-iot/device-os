/**
 ******************************************************************************
 * @file    i2c3_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    24-Dec-2014
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
#include "i2c3_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "pinmap_impl.h"
#include <stddef.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define BUFFER_LENGTH   32
#define EVENT_TIMEOUT   100*1000

#define TRANSMITTER     0x00
#define RECEIVER        0x01

inline system_tick_t isr_safe_micros()
{
    return HAL_Timer_Get_Micro_Seconds();
}

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
static void (*callback_onRequest)(void);
static void (*callback_onReceive)(int);

void HAL_I2C3_Set_Speed(uint32_t speed)
{
    I2C_ClockSpeed = speed;
}

void HAL_I2C3_Enable_DMA_Mode(bool enable)
{
    /* Presently I2C Master mode uses polling and I2C Slave mode uses Interrupt */
}

void HAL_I2C3_Stretch_Clock(bool stretch)
{
    if(stretch == true)
    {
        I2C_StretchClockCmd(I2C3, ENABLE);
    }
    else
    {
        I2C_StretchClockCmd(I2C3, DISABLE);
    }
}

void HAL_I2C3_Begin(bool mode, uint8_t address)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    rxBufferIndex = 0;
    rxBufferLength = 0;

    txBufferIndex = 0;
    txBufferLength = 0;

    /* Enable I2C3 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE);

    /* Connect I2C3 pins to AF4 */
    GPIO_PinAFConfig(PIN_MAP[PM_SCL_UC].gpio_peripheral, PIN_MAP[PM_SCL_UC].gpio_pin_source, GPIO_AF_I2C3);
    GPIO_PinAFConfig(PIN_MAP[PM_SDA_UC].gpio_peripheral, PIN_MAP[PM_SDA_UC].gpio_pin_source, GPIO_AF_I2C3);

    HAL_Pin_Mode(PM_SCL_UC, AF_OUTPUT_DRAIN);
    HAL_Pin_Mode(PM_SDA_UC, AF_OUTPUT_DRAIN);

    if(mode != 0) //execute following if in slave mode
    {
        NVIC_InitTypeDef  NVIC_InitStructure;

        NVIC_InitStructure.NVIC_IRQChannel = I2C3_EV_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = I2C3_ER_IRQn;
        NVIC_Init(&NVIC_InitStructure);
    }

    I2C_DeInit(I2C3);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = address << 1;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_ClockSpeed;
    I2C_Init(I2C3, &I2C_InitStructure);

    I2C_Cmd(I2C3, ENABLE);

    if(mode != 0) //execute following if in slave mode
    {
        I2C_ITConfig(I2C3, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, ENABLE);
    }

    I2C_Enabled = true;
}

void HAL_I2C3_End(void)
{
    if(I2C_Enabled != false)
    {
        I2C_Cmd(I2C3, DISABLE);

        I2C_Enabled = false;
    }
}

uint32_t HAL_I2C3_Request_Data(uint8_t address, uint8_t quantity, uint8_t stop)
{
    uint32_t _micros;
    uint8_t bytesRead = 0;

    // clamp to buffer length
    if(quantity > BUFFER_LENGTH)
    {
        quantity = BUFFER_LENGTH;
    }

    /* Send START condition */
    I2C_GenerateSTART(I2C3, ENABLE);

    _micros = isr_safe_micros();
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if(EVENT_TIMEOUT < (isr_safe_micros() - _micros)) return 0;
    }

    /* Send Slave address for read */
    I2C_Send7bitAddress(I2C3, address << 1, I2C_Direction_Receiver);

    _micros = isr_safe_micros();
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        if(EVENT_TIMEOUT < (isr_safe_micros() - _micros))
        {
            /* Send STOP Condition */
            I2C_GenerateSTOP(I2C3, ENABLE);
            return 0;
        }
    }

    /* perform blocking read into buffer */
    uint8_t *pBuffer = rxBuffer;
    uint8_t numByteToRead = quantity;

    /* While there is data to be read */
    _micros = isr_safe_micros();
    while(numByteToRead && (EVENT_TIMEOUT > (isr_safe_micros() - _micros)))
    {
        if(numByteToRead == 1 && stop == true)
        {
            /* Disable Acknowledgement */
            I2C_AcknowledgeConfig(I2C3, DISABLE);

            /* Send STOP Condition */
            I2C_GenerateSTOP(I2C3, ENABLE);
        }

        if(I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            /* Read a byte from the Slave */
            *pBuffer = I2C_ReceiveData(I2C3);

            bytesRead++;

            /* Point to the next location where the byte read will be saved */
            pBuffer++;

            /* Decrement the read bytes counter */
            numByteToRead--;

            /* Reset timeout to our last read */
            _micros = isr_safe_micros();
        }
    }

    /* Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(I2C3, ENABLE);

    // set rx buffer iterator vars
    rxBufferIndex = 0;
    rxBufferLength = bytesRead;

    return bytesRead;
}

void HAL_I2C3_Begin_Transmission(uint8_t address)
{
    // indicate that we are transmitting
    transmitting = 1;
    // set address of targeted slave
    txAddress = address << 1;
    // reset tx buffer iterator vars
    txBufferIndex = 0;
    txBufferLength = 0;
}

uint8_t HAL_I2C3_End_Transmission(uint8_t stop)
{
    uint32_t _micros;

    /* Send START condition */
    I2C_GenerateSTART(I2C3, ENABLE);

    _micros = isr_safe_micros();
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if(EVENT_TIMEOUT < (isr_safe_micros() - _micros)) return 4;
    }

    /* Send Slave address for write */
    I2C_Send7bitAddress(I2C3, txAddress, I2C_Direction_Transmitter);

    _micros = isr_safe_micros();
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if(EVENT_TIMEOUT < (isr_safe_micros() - _micros))
        {
            /* Send STOP Condition */
            I2C_GenerateSTOP(I2C3, ENABLE);
            return 4;
        }
    }

    uint8_t *pBuffer = txBuffer;
    uint8_t NumByteToWrite = txBufferLength;

    /* While there is data to be written */
    while(NumByteToWrite--)
    {
        /* Send the current byte to slave */
        I2C_SendData(I2C3, *pBuffer);

        /* Point to the next byte to be written */
        pBuffer++;

        _micros = isr_safe_micros();
        while (!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        {
            if(EVENT_TIMEOUT < (isr_safe_micros() - _micros)) return 4;
        }
    }

    /* Send STOP Condition */
    if(stop == true)
    {
        /* Send STOP condition */
        I2C_GenerateSTOP(I2C3, ENABLE);
    }

    // reset tx buffer iterator vars
    txBufferIndex = 0;
    txBufferLength = 0;

    // indicate that we are done transmitting
    transmitting = 0;

    return 0;
}

uint32_t HAL_I2C3_Write_Data(uint8_t data)
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

int32_t HAL_I2C3_Available_Data(void)
{
    return rxBufferLength - rxBufferIndex;
}

int32_t HAL_I2C3_Read_Data(void)
{
    int value = -1;

    // get each successive byte on each call
    if(rxBufferIndex < rxBufferLength)
    {
        value = rxBuffer[rxBufferIndex++];
    }

    return value;
}

int32_t HAL_I2C3_Peek_Data(void)
{
    int value = -1;

    if(rxBufferIndex < rxBufferLength)
    {
        value = rxBuffer[rxBufferIndex];
    }

    return value;
}

void HAL_I2C3_Flush_Data(void)
{
    // XXX: to be implemented.
}

bool HAL_I2C3_Is_Enabled(void)
{
    return I2C_Enabled;
}

void HAL_I2C3_Set_Callback_On_Receive(void (*function)(int))
{
    callback_onReceive = function;
}

void HAL_I2C3_Set_Callback_On_Request(void (*function)(void))
{
    callback_onRequest = function;
}

/**
 * @brief  This function handles I2C3 Error interrupt request.
 * @param  None
 * @retval None
 */
void I2C3_ER_irq(void)
{
    /* Read SR1 register to get I2C error */
    if ((I2C_ReadRegister(I2C3, I2C_Register_SR1) & 0xFF00) != 0x00)
    {
        /* Clears error flags */
        I2C3->SR1 &= 0x00FF;
    }
}

/**
 * @brief  This function handles I2C3 event interrupt request.
 * @param  None
 * @retval None
 */
void I2C3_EV_irq(void)
{
    /* Process Last I2C Event */
    switch (I2C_GetLastEvent(I2C3))
    {
    /********** Slave Transmitter Events ************/

    /* Check on EV1 */
    case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:
        transmitting = 1;
        txBufferIndex = 0;
        txBufferLength = 0;

        if(NULL != callback_onRequest)
        {
            // alert user program
            callback_onRequest();
        }

        txBufferIndex = 0;
        break;

    /* Check on EV3 */
    case I2C_EVENT_SLAVE_BYTE_TRANSMITTING:
    case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:
        if (txBufferIndex < txBufferLength)
        {
            I2C_SendData(I2C3, txBuffer[txBufferIndex++]);
        }
        break;

    /*********** Slave Receiver Events *************/

    /* check on EV1*/
    case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:
        rxBufferIndex = 0;
        rxBufferLength = 0;
        break;

    /* Check on EV2*/
    case I2C_EVENT_SLAVE_BYTE_RECEIVED:
    case (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_SR1_BTF):
        rxBuffer[rxBufferIndex++] = I2C_ReceiveData(I2C3);
        break;

    /* Check on EV4 */
    case I2C_EVENT_SLAVE_STOP_DETECTED:
        /* software sequence to clear STOPF */
        I2C_GetFlagStatus(I2C3, I2C_FLAG_STOPF);
        I2C_Cmd(I2C3, ENABLE);

        rxBufferLength = rxBufferIndex;
        rxBufferIndex = 0;

        if(NULL != callback_onReceive)
        {
            // alert user program
            callback_onReceive(rxBufferLength);
        }
        break;

    default:
        break;
    }
}

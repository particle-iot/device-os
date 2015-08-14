/**
 ******************************************************************************
 * @file    i2c_hal.c
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
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "pinmap_impl.h"
#include <stddef.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define BUFFER_LENGTH   (I2C_BUFFER_LENGTH)
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
static void (*callback_onRequest)(void);
static void (*callback_onReceive)(int);

static void HAL_I2C_SoftwareReset(void)
{
    /* Disable the I2C peripheral */
    I2C_Cmd(I2C1, DISABLE);

    /* Reset all I2C registers */
    I2C_SoftwareResetCmd(I2C1, ENABLE);
    I2C_SoftwareResetCmd(I2C1, DISABLE);

    /* Enable the I2C peripheral */
    I2C_Cmd(I2C1, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C1, &I2C_InitStructure);
}

void HAL_I2C_Set_Speed(uint32_t speed)
{
    I2C_ClockSpeed = speed;
}

void HAL_I2C_Enable_DMA_Mode(bool enable)
{
    /* Presently I2C Master mode uses polling and I2C Slave mode uses Interrupt */
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
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    rxBufferIndex = 0;
    rxBufferLength = 0;

    txBufferIndex = 0;
    txBufferLength = 0;

    /* Enable I2C1 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* Enable I2C1 reset state */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);

    /* Release I2C1 from reset state */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

    /* Connect I2C1 pins to AF4 */
    GPIO_PinAFConfig(PIN_MAP[SCL].gpio_peripheral, PIN_MAP[SCL].gpio_pin_source, GPIO_AF_I2C1);
    GPIO_PinAFConfig(PIN_MAP[SDA].gpio_peripheral, PIN_MAP[SDA].gpio_pin_source, GPIO_AF_I2C1);

    HAL_Pin_Mode(SCL, AF_OUTPUT_DRAIN);
    HAL_Pin_Mode(SDA, AF_OUTPUT_DRAIN);

    NVIC_InitTypeDef  NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    if(mode != I2C_MODE_MASTER)
    {
        NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
        NVIC_Init(&NVIC_InitStructure);
    }

    /* Commented I2C_DeInit() since Enable/Release I2C Reset State is done above */
    //I2C_DeInit(I2C1);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = address << 1;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_ClockSpeed;

    /*
        From STM32 peripheral library notes:
        ====================================
        If an error occurs (ie. I2C error flags are set besides to the monitored
        flags), the I2C_CheckEvent() function may return SUCCESS despite
        the communication hold or corrupted real state.
        In this case, it is advised to use error interrupts to monitor
        the error events and handle them in the interrupt IRQ handler.
     */
    I2C_ITConfig(I2C1, I2C_IT_ERR, ENABLE);

    if(mode != I2C_MODE_MASTER)
    {
        I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
    }

    /* Enable the I2C peripheral */
    I2C_Cmd(I2C1, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C1, &I2C_InitStructure);

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
    uint8_t bytesRead = 0;

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
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset();

            return 0;
        }
    }

    /* Send Slave address for read */
    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* Send STOP Condition */
            I2C_GenerateSTOP(I2C1, ENABLE);

            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset();

            return 0;
        }
    }

    /* perform blocking read into buffer */
    uint8_t *pBuffer = rxBuffer;
    uint8_t numByteToRead = quantity;

    /* While there is data to be read */
    while(numByteToRead)
    {
        if(numByteToRead < 2 && stop == true)
        {
            /* Disable Acknowledgement */
            I2C_AcknowledgeConfig(I2C1, DISABLE);

            /* Send STOP Condition */
            I2C_GenerateSTOP(I2C1, ENABLE);
        }

        /* Wait for the byte to be received */
        _millis = HAL_Timer_Get_Milli_Seconds();
        //while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
        while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset();

                return 0;
            }
        }

        /* Read the byte from the Slave */
        *pBuffer = I2C_ReceiveData(I2C1);

        bytesRead++;

        /* Point to the next location where the byte read will be saved */
        pBuffer++;

        /* Decrement the read bytes counter */
        numByteToRead--;

        /* Wait to make sure that STOP control bit has been cleared */
        _millis = HAL_Timer_Get_Milli_Seconds();
        while(I2C1->CR1 & I2C_CR1_STOP)
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset();

                return 0;
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

void HAL_I2C_Begin_Transmission(uint8_t address)
{
    // indicate that we are transmitting
    transmitting = 1;
    // set address of targeted slave
    txAddress = address << 1;
    // reset tx buffer iterator vars
    txBufferIndex = 0;
    txBufferLength = 0;
}

uint8_t HAL_I2C_End_Transmission(uint8_t stop)
{
    uint32_t _millis;

    _millis = HAL_Timer_Get_Milli_Seconds();
    /* While the I2C Bus is busy */
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset();

            return 1;
        }
    }

    /* Send START condition */
    I2C_GenerateSTART(I2C1, ENABLE);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset();

            return 2;
        }
    }

    /* Send Slave address for write */
    I2C_Send7bitAddress(I2C1, txAddress, I2C_Direction_Transmitter);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* Send STOP Condition */
            I2C_GenerateSTOP(I2C1, ENABLE);

            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset();

            return 3;
        }
    }

    uint8_t *pBuffer = txBuffer;
    uint8_t NumByteToWrite = txBufferLength;

    /* While there is data to be written */
    while(NumByteToWrite--)
    {
        /* Send the current byte to slave */
        I2C_SendData(I2C1, *pBuffer);

        /* Point to the next byte to be written */
        pBuffer++;

        _millis = HAL_Timer_Get_Milli_Seconds();
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset();

                return 4;
            }
        }

        _millis = HAL_Timer_Get_Milli_Seconds();
        while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET)
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset();

                return 5;
            }
        }
    }

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
    callback_onReceive = function;
}

void HAL_I2C_Set_Callback_On_Request(void (*function)(void))
{
    callback_onRequest = function;
}

/**
 * @brief  This function handles I2C1 Error interrupt request.
 * @param  None
 * @retval None
 */
void I2C1_ER_irq(void)
{
#if 0 //Checks whether specified I2C interrupt has occurred and clear IT pending bit.
    /* Check on I2C Time out flag and clear it */
    if (I2C_GetITStatus(I2C1, I2C_IT_TIMEOUT))
    {
      I2C_ClearITPendingBit(I2C1, I2C_IT_TIMEOUT);
    }

    /* Check on I2C Arbitration Lost flag and clear it */
    if (I2C_GetITStatus(I2C1, I2C_IT_ARLO))
    {
      I2C_ClearITPendingBit(I2C1, I2C_IT_ARLO);
    }

    /* Check on I2C Overrun/Underrun error flag and clear it */
    if (I2C_GetITStatus(I2C1, I2C_IT_OVR))
    {
      I2C_ClearITPendingBit(I2C1, I2C_IT_OVR);
    }

    /* Check on I2C Acknowledge failure error flag and clear it */
    if (I2C_GetITStatus(I2C1, I2C_IT_AF))
    {
      I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
    }

    /* Check on I2C Bus error flag and clear it */
    if (I2C_GetITStatus(I2C1, I2C_IT_BERR))
    {
      I2C_ClearITPendingBit(I2C1, I2C_IT_BERR);
    }
#else //Clear all error pending bits without worrying about specific error interrupt bit
    /* Read SR1 register to get I2C error */
    if ((I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0xFF00) != 0x00)
    {
        /* Clear I2C error flags */
        I2C1->SR1 &= 0x00FF;

        //I2C_SoftwareResetCmd() and/or I2C_GenerateStop() ???
    }
#endif
}

/**
 * @brief  This function handles I2C1 event interrupt request.
 * @param  None
 * @retval None
 */
void I2C1_EV_irq(void)
{
    /* Process Last I2C Event */
    switch (I2C_GetLastEvent(I2C1))
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
            I2C_SendData(I2C1, txBuffer[txBufferIndex++]);
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
        rxBuffer[rxBufferIndex++] = I2C_ReceiveData(I2C1);
        break;

    /* Check on EV4 */
    case I2C_EVENT_SLAVE_STOP_DETECTED:
        /* software sequence to clear STOPF */
        I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF);
        I2C_Cmd(I2C1, ENABLE);

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

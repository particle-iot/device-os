/**
 ******************************************************************************
 * @file    i2c_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    26-Aug-2015
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

/* Private define ------------------------------------------------------------*/
#if PLATFORM_ID == 10 // Electron
#define TOTAL_I2C   3
#else
#define TOTAL_I2C   1
#endif

/* Private typedef -----------------------------------------------------------*/
typedef enum I2C_Num_Def {
    I2C1_D0_D1 = 0
#if PLATFORM_ID == 10 // Electron
   ,I2C3_C4_C5 = 1
   ,I2C3_PM_SDA_SCL = 2
#endif
} I2C_Num_Def;

/* Private variables ---------------------------------------------------------*/
typedef struct STM32_I2C_Info {
    I2C_TypeDef* I2C_Peripheral;

    __IO uint32_t* I2C_RCC_APBRegister;
    uint32_t I2C_RCC_APBClockEnable;

    uint8_t I2C_ER_IRQn;
    uint8_t I2C_EV_IRQn;

    uint16_t I2C_SDA_Pin;
    uint16_t I2C_SCL_Pin;

    uint8_t I2C_AF_Mapping;

    I2C_InitTypeDef I2C_InitStructure;

    uint32_t I2C_ClockSpeed;
    bool I2C_Enabled;

    uint8_t rxBuffer[BUFFER_LENGTH];
    uint8_t rxBufferIndex;
    uint8_t rxBufferLength;

    uint8_t txAddress;
    uint8_t txBuffer[BUFFER_LENGTH];
    uint8_t txBufferIndex;
    uint8_t txBufferLength;

    uint8_t transmitting;

    void (*callback_onRequest)(void);
    void (*callback_onReceive)(int);
} STM32_I2C_Info;

/*
 * I2C mapping
 */
STM32_I2C_Info I2C_MAP[TOTAL_I2C] =
{
        { I2C1, &RCC->APB1ENR, RCC_APB1Periph_I2C1, I2C1_ER_IRQn, I2C1_EV_IRQn, D0, D1, GPIO_AF_I2C1 }
#if PLATFORM_ID == 10 // Electron
       ,{ I2C1, &RCC->APB1ENR, RCC_APB1Periph_I2C1, I2C1_ER_IRQn, I2C1_EV_IRQn, C4, C5, GPIO_AF_I2C1 }
       ,{ I2C3, &RCC->APB1ENR, RCC_APB1Periph_I2C3, I2C3_ER_IRQn, I2C3_EV_IRQn, PM_SDA_UC, PM_SCL_UC, GPIO_AF_I2C3 }
#endif
};

static STM32_I2C_Info *i2cMap[TOTAL_I2C]; // pointer to I2C_MAP[] containing I2C peripheral info

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void HAL_I2C_SoftwareReset(HAL_I2C_Interface i2c)
{
    /* Disable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);

    /* Reset all I2C registers */
    I2C_SoftwareResetCmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);
    I2C_SoftwareResetCmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);

    /* Enable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(i2cMap[i2c]->I2C_Peripheral, &i2cMap[i2c]->I2C_InitStructure);
}

void HAL_I2C_Init(HAL_I2C_Interface i2c, void* reserved)
{
    if(i2c == HAL_I2C_INTERFACE1)
    {
        i2cMap[i2c] = &I2C_MAP[I2C1_D0_D1];
}
#if PLATFORM_ID == 10 // Electron
    else if(i2c == HAL_I2C_INTERFACE2)
    {
        i2cMap[i2c] = &I2C_MAP[I2C3_C4_C5];
    }
    else if(i2c == HAL_I2C_INTERFACE3)
    {
        i2cMap[i2c] = &I2C_MAP[I2C3_PM_SDA_SCL];
    }
#endif

    i2cMap[i2c]->I2C_ClockSpeed = CLOCK_SPEED_100KHZ;
    i2cMap[i2c]->I2C_Enabled = false;

    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = 0;

    i2cMap[i2c]->txAddress = 0;
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;

    i2cMap[i2c]->transmitting = 0;
}

void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved)
{
    i2cMap[i2c]->I2C_ClockSpeed = speed;
}

void HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable, void* reserved)
{
    /* Presently I2C Master mode uses polling and I2C Slave mode uses Interrupt */
}

void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved)
{
    if(stretch == true)
    {
        I2C_StretchClockCmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);
    }
    else
    {
        I2C_StretchClockCmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);
    }
}

void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = 0;

    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;

    /* Enable I2C clock */
    *i2cMap[i2c]->I2C_RCC_APBRegister |= i2cMap[i2c]->I2C_RCC_APBClockEnable;

    /* Enable and Release I2C Reset State */
    I2C_DeInit(i2cMap[i2c]->I2C_Peripheral);

    /* Connect I2C pins to respective AF */
    GPIO_PinAFConfig(PIN_MAP[i2cMap[i2c]->I2C_SCL_Pin].gpio_peripheral, PIN_MAP[i2cMap[i2c]->I2C_SCL_Pin].gpio_pin_source, i2cMap[i2c]->I2C_AF_Mapping);
    GPIO_PinAFConfig(PIN_MAP[i2cMap[i2c]->I2C_SDA_Pin].gpio_peripheral, PIN_MAP[i2cMap[i2c]->I2C_SDA_Pin].gpio_pin_source, i2cMap[i2c]->I2C_AF_Mapping);

    HAL_Pin_Mode(i2cMap[i2c]->I2C_SCL_Pin, AF_OUTPUT_DRAIN);
    HAL_Pin_Mode(i2cMap[i2c]->I2C_SDA_Pin, AF_OUTPUT_DRAIN);

        NVIC_InitTypeDef  NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = i2cMap[i2c]->I2C_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

    if(mode != I2C_MODE_MASTER)
    {
        NVIC_InitStructure.NVIC_IRQChannel = i2cMap[i2c]->I2C_EV_IRQn;
        NVIC_Init(&NVIC_InitStructure);
    }

    i2cMap[i2c]->I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    i2cMap[i2c]->I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    i2cMap[i2c]->I2C_InitStructure.I2C_OwnAddress1 = address << 1;
    i2cMap[i2c]->I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    i2cMap[i2c]->I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2cMap[i2c]->I2C_InitStructure.I2C_ClockSpeed = i2cMap[i2c]->I2C_ClockSpeed;

    /*
        From STM32 peripheral library notes:
        ====================================
        If an error occurs (ie. I2C error flags are set besides to the monitored
        flags), the I2C_CheckEvent() function may return SUCCESS despite
        the communication hold or corrupted real state.
        In this case, it is advised to use error interrupts to monitor
        the error events and handle them in the interrupt IRQ handler.
     */
    I2C_ITConfig(i2cMap[i2c]->I2C_Peripheral, I2C_IT_ERR, ENABLE);

    if(mode != I2C_MODE_MASTER)
    {
        I2C_ITConfig(i2cMap[i2c]->I2C_Peripheral, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
    }

    /* Enable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(i2cMap[i2c]->I2C_Peripheral, &i2cMap[i2c]->I2C_InitStructure);

    i2cMap[i2c]->I2C_Enabled = true;
}

void HAL_I2C_End(HAL_I2C_Interface i2c, void* reserved)
{
    if(i2cMap[i2c]->I2C_Enabled != false)
    {
        I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);

        i2cMap[i2c]->I2C_Enabled = false;
    }
}

uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved)
{
    uint32_t _millis;
    uint8_t bytesRead = 0;

    // clamp to buffer length
    if(quantity > BUFFER_LENGTH)
    {
        quantity = BUFFER_LENGTH;
    }

    /* Send START condition */
    I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);

            return 0;
        }
    }

    /* Send Slave address for read */
    I2C_Send7bitAddress(i2cMap[i2c]->I2C_Peripheral, address << 1, I2C_Direction_Receiver);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* Send STOP Condition */
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);

            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);

            return 0;
        }
    }

    /* perform blocking read into buffer */
    uint8_t *pBuffer = i2cMap[i2c]->rxBuffer;
    uint8_t numByteToRead = quantity;

    /* While there is data to be read */
    while(numByteToRead)
    {
        if(numByteToRead < 2 && stop == true)
        {
            /* Disable Acknowledgement */
            I2C_AcknowledgeConfig(i2cMap[i2c]->I2C_Peripheral, DISABLE);

            /* Send STOP Condition */
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);
        }

        /* Wait for the byte to be received */
        _millis = HAL_Timer_Get_Milli_Seconds();
        //while(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_BYTE_RECEIVED))
        while(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_RXNE) == RESET)
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset(i2c);

                return 0;
            }
        }

        /* Read the byte from the Slave */
        *pBuffer = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);

            bytesRead++;

            /* Point to the next location where the byte read will be saved */
            pBuffer++;

            /* Decrement the read bytes counter */
            numByteToRead--;

        /* Wait to make sure that STOP control bit has been cleared */
            _millis = HAL_Timer_Get_Milli_Seconds();
        while(i2cMap[i2c]->I2C_Peripheral->CR1 & I2C_CR1_STOP)
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset(i2c);

                return 0;
        }
    }
    }

    /* Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    // set rx buffer iterator vars
    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = bytesRead;

    return bytesRead;
}

void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address, void* reserved)
{
    // indicate that we are transmitting
    i2cMap[i2c]->transmitting = 1;
    // set address of targeted slave
    i2cMap[i2c]->txAddress = address << 1;
    // reset tx buffer iterator vars
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;
}

uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop, void* reserved)
{
    uint32_t _millis;

    _millis = HAL_Timer_Get_Milli_Seconds();
    /* While the I2C Bus is busy */
    while(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BUSY))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);

            return 1;
        }
    }

    /* Send START condition */
    I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);

            return 2;
        }
    }

    /* Send Slave address for write */
    I2C_Send7bitAddress(i2cMap[i2c]->I2C_Peripheral, i2cMap[i2c]->txAddress, I2C_Direction_Transmitter);

    _millis = HAL_Timer_Get_Milli_Seconds();
    while(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
            /* Send STOP Condition */
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);

            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);

            return 3;
        }
    }

    uint8_t *pBuffer = i2cMap[i2c]->txBuffer;
    uint8_t NumByteToWrite = i2cMap[i2c]->txBufferLength;

    /* While there is data to be written */
    while(NumByteToWrite--)
    {
        /* Send the current byte to slave */
        I2C_SendData(i2cMap[i2c]->I2C_Peripheral, *pBuffer);

        /* Point to the next byte to be written */
        pBuffer++;

        _millis = HAL_Timer_Get_Milli_Seconds();
        while(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset(i2c);

                return 4;
            }
        }

        _millis = HAL_Timer_Get_Milli_Seconds();
        while(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BTF) == RESET)
        {
            if(EVENT_TIMEOUT < (HAL_Timer_Get_Milli_Seconds() - _millis))
        {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset(i2c);

                return 5;
            }
        }
    }

    /* Send STOP Condition */
    if(stop == true)
    {
        /* Send STOP condition */
        I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);
    }

    // reset tx buffer iterator vars
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;

    // indicate that we are done transmitting
    i2cMap[i2c]->transmitting = 0;

    return 0;
}

uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data, void* reserved)
{
    if(i2cMap[i2c]->transmitting)
    {
        // in master/slave transmitter mode
        // don't bother if buffer is full
        if(i2cMap[i2c]->txBufferLength >= BUFFER_LENGTH)
        {
            return 0;
        }
        // put byte in tx buffer
        i2cMap[i2c]->txBuffer[i2cMap[i2c]->txBufferIndex++] = data;
        // update amount in buffer
        i2cMap[i2c]->txBufferLength = i2cMap[i2c]->txBufferIndex;
    }

    return 1;
}

int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c, void* reserved)
{
    return i2cMap[i2c]->rxBufferLength - i2cMap[i2c]->rxBufferIndex;
}

int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c, void* reserved)
{
    int value = -1;

    // get each successive byte on each call
    if(i2cMap[i2c]->rxBufferIndex < i2cMap[i2c]->rxBufferLength)
    {
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex++];
    }

    return value;
}

int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c, void* reserved)
{
    int value = -1;

    if(i2cMap[i2c]->rxBufferIndex < i2cMap[i2c]->rxBufferLength)
    {
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex];
    }

    return value;
}

void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c, void* reserved)
{
    // XXX: to be implemented.
}

bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c, void* reserved)
{
    return i2cMap[i2c]->I2C_Enabled;
}

void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int), void* reserved)
{
    i2cMap[i2c]->callback_onReceive = function;
}

void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void), void* reserved)
{
    i2cMap[i2c]->callback_onRequest = function;
}

static void HAL_I2C_ER_InterruptHandler(HAL_I2C_Interface i2c)
{
#if 0 //Checks whether specified I2C interrupt has occurred and clear IT pending bit.
    /* Check on I2C Time out flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_TIMEOUT))
    {
      I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_TIMEOUT);
    }

    /* Check on I2C Arbitration Lost flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_ARLO))
    {
      I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_ARLO);
    }

    /* Check on I2C Overrun/Underrun error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_OVR))
    {
      I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_OVR);
    }

    /* Check on I2C Acknowledge failure error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_AF))
    {
      I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_AF);
    }

    /* Check on I2C Bus error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_BERR))
    {
      I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_BERR);
    }
#else //Clear all error pending bits without worrying about specific error interrupt bit
    /* Read SR1 register to get I2C error */
    if ((I2C_ReadRegister(i2cMap[i2c]->I2C_Peripheral, I2C_Register_SR1) & 0xFF00) != 0x00)
    {
        /* Clear I2C error flags */
        i2cMap[i2c]->I2C_Peripheral->SR1 &= 0x00FF;
    }
#endif

    //I2C_SoftwareResetCmd() and/or I2C_GenerateStop() ???
}

/**
 * @brief  This function handles I2C1 Error interrupt request.
 * @param  None
 * @retval None
 */
void I2C1_ER_irq(void)
{
    HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE1);
}

#if PLATFORM_ID == 10 // Electron
/**
 * @brief  This function handles I2C3 Error interrupt request.
 * @param  None
 * @retval None
 */
void I2C3_ER_irq(void)
{
    HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE3);
}
#endif

static void HAL_I2C_EV_InterruptHandler(HAL_I2C_Interface i2c)
{
    /* Process Last I2C Event */
    switch (I2C_GetLastEvent(i2cMap[i2c]->I2C_Peripheral))
    {
    /********** Slave Transmitter Events ************/

    /* Check on EV1 */
    case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:
        i2cMap[i2c]->transmitting = 1;
        i2cMap[i2c]->txBufferIndex = 0;
        i2cMap[i2c]->txBufferLength = 0;

        if(NULL != i2cMap[i2c]->callback_onRequest)
        {
            // alert user program
            i2cMap[i2c]->callback_onRequest();
        }

        i2cMap[i2c]->txBufferIndex = 0;
        break;

    /* Check on EV3 */
    case I2C_EVENT_SLAVE_BYTE_TRANSMITTING:
    case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:
        if (i2cMap[i2c]->txBufferIndex < i2cMap[i2c]->txBufferLength)
        {
            I2C_SendData(i2cMap[i2c]->I2C_Peripheral, i2cMap[i2c]->txBuffer[i2cMap[i2c]->txBufferIndex++]);
        }
        break;

    /*********** Slave Receiver Events *************/

    /* check on EV1*/
    case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:
        i2cMap[i2c]->rxBufferIndex = 0;
        i2cMap[i2c]->rxBufferLength = 0;
        break;

    /* Check on EV2*/
    case I2C_EVENT_SLAVE_BYTE_RECEIVED:
    case (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_SR1_BTF):
        i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex++] = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
        break;

    /* Check on EV4 */
    case I2C_EVENT_SLAVE_STOP_DETECTED:
        /* software sequence to clear STOPF */
        I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_STOPF);
        I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        i2cMap[i2c]->rxBufferLength = i2cMap[i2c]->rxBufferIndex;
        i2cMap[i2c]->rxBufferIndex = 0;

        if(NULL != i2cMap[i2c]->callback_onReceive)
        {
            // alert user program
            i2cMap[i2c]->callback_onReceive(i2cMap[i2c]->rxBufferLength);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief  This function handles I2C1 event interrupt request.
 * @param  None
 * @retval None
 */
void I2C1_EV_irq(void)
{
    HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE1);
}

#if PLATFORM_ID == 10 // Electron
/**
 * @brief  This function handles I2C3 event interrupt request.
 * @param  None
 * @retval None
 */
void I2C3_EV_irq(void)
{
    HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE3);
}
#endif

// On the Photon/P1 the I2C interface selector was added after the first release.
// So these compatibility functions are needed for older firmware

void HAL_I2C_Set_Speed_v1(uint32_t speed) { HAL_I2C_Set_Speed(HAL_I2C_INTERFACE1, speed, NULL); }
void HAL_I2C_Enable_DMA_Mode_v1(bool enable) { HAL_I2C_Enable_DMA_Mode(HAL_I2C_INTERFACE1, enable, NULL); }
void HAL_I2C_Stretch_Clock_v1(bool stretch) { HAL_I2C_Stretch_Clock(HAL_I2C_INTERFACE1, stretch, NULL); }
void HAL_I2C_Begin_v1(I2C_Mode mode, uint8_t address) { HAL_I2C_Begin(HAL_I2C_INTERFACE1, mode, address, NULL); }
void HAL_I2C_End_v1() { HAL_I2C_End(HAL_I2C_INTERFACE1, NULL); }
uint32_t HAL_I2C_Request_Data_v1(uint8_t address, uint8_t quantity, uint8_t stop) { return HAL_I2C_Request_Data(HAL_I2C_INTERFACE1, address, quantity, stop, NULL); }
void HAL_I2C_Begin_Transmission_v1(uint8_t address) { HAL_I2C_Begin_Transmission(HAL_I2C_INTERFACE1, address, NULL); }
uint8_t HAL_I2C_End_Transmission_v1(uint8_t stop) { return HAL_I2C_End_Transmission(HAL_I2C_INTERFACE1, stop, NULL); }
uint32_t HAL_I2C_Write_Data_v1(uint8_t data) { return HAL_I2C_Write_Data(HAL_I2C_INTERFACE1, data, NULL); }
int32_t HAL_I2C_Available_Data_v1(void) { return HAL_I2C_Available_Data(HAL_I2C_INTERFACE1, NULL); }
int32_t HAL_I2C_Read_Data_v1(void) { return HAL_I2C_Read_Data(HAL_I2C_INTERFACE1, NULL); }
int32_t HAL_I2C_Peek_Data_v1(void) { return HAL_I2C_Peek_Data(HAL_I2C_INTERFACE1, NULL); }
void HAL_I2C_Flush_Data_v1(void) { HAL_I2C_Flush_Data(HAL_I2C_INTERFACE1, NULL); }
bool HAL_I2C_Is_Enabled_v1(void) { return HAL_I2C_Is_Enabled(HAL_I2C_INTERFACE1, NULL); }
void HAL_I2C_Set_Callback_On_Receive_v1(void (*function)(int)) { HAL_I2C_Set_Callback_On_Receive(HAL_I2C_INTERFACE1, function, NULL); }
void HAL_I2C_Set_Callback_On_Request_v1(void (*function)(void)) { HAL_I2C_Set_Callback_On_Request(HAL_I2C_INTERFACE1, function, NULL); }


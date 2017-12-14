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
#include "service_debug.h"
#include "interrupts_hal.h"
#include "delay_hal.h"
#include "concurrent_hal.h"

#ifdef LOG_SOURCE_CATEGORY
LOG_SOURCE_CATEGORY("hal.i2c")
#endif // LOG_SOURCE_CATEGORY

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define BUFFER_LENGTH   (I2C_BUFFER_LENGTH)
#define EVENT_TIMEOUT   100000 // 100ms

#define TRANSMITTER     0x00
#define RECEIVER        0x01

/* Private define ------------------------------------------------------------*/
#if PLATFORM_ID == 10 // Electron
#define TOTAL_I2C   3
#else
#define TOTAL_I2C   1
#endif

#define I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED_NO_ADDR ((uint32_t)0x00070080)

#define WAIT_TIMED(what) ({ \
    uint32_t _micros = HAL_Timer_Get_Micro_Seconds();                           \
    bool res = true;                                                            \
    while((what))                                                               \
    {                                                                           \
        int32_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);                 \
        bool nok = ((EVENT_TIMEOUT < dt)                                         \
                   && (what))                                                   \
                   || (dt < 0);                                                   \
        if (nok)                                                                \
        {                                                                       \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})


/* Private typedef -----------------------------------------------------------*/
typedef enum I2C_Num_Def {
    I2C1_D0_D1 = 0
#if PLATFORM_ID == 10 // Electron
   ,I2C3_C4_C5 = 1
   ,I2C3_PM_SDA_SCL = 2
#endif
} I2C_Num_Def;

typedef enum I2C_Transaction_Ending_Condition {
    I2C_ENDING_UNKNOWN,
    I2C_ENDING_STOP,
    I2C_ENDING_START
} I2C_Transaction_Ending_Condition;

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

    I2C_Mode mode;
    volatile bool ackFailure;
    volatile uint8_t prevEnding;
    uint8_t clkStretchingEnabled;

    os_mutex_recursive_t mutex;
} STM32_I2C_Info;

/*
 * I2C mapping
 */
STM32_I2C_Info I2C_MAP[TOTAL_I2C] =
{
        { I2C1, &RCC->APB1ENR, RCC_APB1Periph_I2C1, I2C1_ER_IRQn, I2C1_EV_IRQn, D0, D1, GPIO_AF_I2C1, .mutex = NULL }
#if PLATFORM_ID == 10 // Electron
       ,{ I2C1, &RCC->APB1ENR, RCC_APB1Periph_I2C1, I2C1_ER_IRQn, I2C1_EV_IRQn, C4, C5, GPIO_AF_I2C1, .mutex = NULL }
       ,{ I2C3, &RCC->APB1ENR, RCC_APB1Periph_I2C3, I2C3_ER_IRQn, I2C3_EV_IRQn, PM_SDA_UC, PM_SCL_UC, GPIO_AF_I2C3, .mutex = NULL }
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

    /* Clear all I2C interrupt error flags, and re-enable them */
    I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_SMBALERT | I2C_IT_PECERR |
            I2C_IT_TIMEOUT | I2C_IT_ARLO | I2C_IT_OVR | I2C_IT_BERR | I2C_IT_AF);
    I2C_ITConfig(i2cMap[i2c]->I2C_Peripheral, I2C_IT_ERR, ENABLE);

    /* Re-enable Event and Buffer interrupts in Slave mode */
    if(i2cMap[i2c]->mode == I2C_MODE_SLAVE)
    {
        I2C_ITConfig(i2cMap[i2c]->I2C_Peripheral, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
    }

    /* Enable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(i2cMap[i2c]->I2C_Peripheral, &i2cMap[i2c]->I2C_InitStructure);

    i2cMap[i2c]->prevEnding = I2C_ENDING_UNKNOWN;
}

void HAL_I2C_Init(HAL_I2C_Interface i2c, void* reserved)
{
    if(i2c == HAL_I2C_INTERFACE1)
    {
        i2cMap[i2c] = &I2C_MAP[I2C1_D0_D1];
    }
#if PLATFORM_ID == 10 // Electron
    if(i2c == HAL_I2C_INTERFACE2 || i2c == HAL_I2C_INTERFACE1)
    {
    	   // these are co-dependent so initialize both
       i2cMap[HAL_I2C_INTERFACE1] = &I2C_MAP[I2C1_D0_D1];
       i2cMap[HAL_I2C_INTERFACE2] = &I2C_MAP[I2C3_C4_C5];
    }
    else if(i2c == HAL_I2C_INTERFACE3)
    {
        i2cMap[i2c] = &I2C_MAP[I2C3_PM_SDA_SCL];
    }
#endif

#if PLATFORM_ID == 10
    // For now we only enable this for PMIC I2C bus
    if (i2c == HAL_I2C_INTERFACE3) {
        os_thread_scheduling(false, NULL);
        if (i2cMap[i2c]->mutex == NULL) {
            os_mutex_recursive_create(&i2cMap[i2c]->mutex);
        }
        HAL_I2C_Acquire(i2c, NULL);
        os_thread_scheduling(true, NULL);
    }
#endif // PLATFORM_ID == 10

    i2cMap[i2c]->I2C_ClockSpeed = CLOCK_SPEED_100KHZ;
    i2cMap[i2c]->I2C_Enabled = false;

    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = 0;

    i2cMap[i2c]->txAddress = 0;
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;

    i2cMap[i2c]->transmitting = 0;

    i2cMap[i2c]->ackFailure = false;
    i2cMap[i2c]->prevEnding = I2C_ENDING_UNKNOWN;

    i2cMap[i2c]->clkStretchingEnabled = 1;
    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    i2cMap[i2c]->I2C_ClockSpeed = speed;
    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable, void* reserved)
{
    /* Presently I2C Master mode uses polling and I2C Slave mode uses Interrupt */
}

void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    if(stretch == true)
    {
        I2C_StretchClockCmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);
    }
    else
    {
        I2C_StretchClockCmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);
    }

    i2cMap[i2c]->clkStretchingEnabled = stretch;
    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

#if PLATFORM_ID == 10
    /*
     * On Electron both I2C_INTERFACE1 and I2C_INTERFACE2 use the same peripheral - I2C1,
     * but on different pins. We cannot enable both of them at the same time.
     */
    if (i2c == HAL_I2C_INTERFACE1 || i2c == HAL_I2C_INTERFACE2) {
        HAL_I2C_Interface dependent = (i2c == HAL_I2C_INTERFACE1 ? HAL_I2C_INTERFACE2 : HAL_I2C_INTERFACE1);
        if (HAL_I2C_Is_Enabled(dependent, NULL) == true) {
            // Unfortunately we cannot return an error code here
            HAL_I2C_Release(i2c, NULL);
            return;
        }
    }
#endif

    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = 0;

    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;

    i2cMap[i2c]->mode = mode;
    i2cMap[i2c]->ackFailure = false;
    i2cMap[i2c]->prevEnding = I2C_ENDING_UNKNOWN;

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

    if(i2cMap[i2c]->mode != I2C_MODE_MASTER)
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

    if(i2cMap[i2c]->mode != I2C_MODE_MASTER)
    {
        I2C_ITConfig(i2cMap[i2c]->I2C_Peripheral, I2C_IT_EVT | I2C_IT_BUF, ENABLE);

        HAL_I2C_Stretch_Clock(i2c, i2cMap[i2c]->clkStretchingEnabled, NULL);
    }

    /* Enable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(i2cMap[i2c]->I2C_Peripheral, &i2cMap[i2c]->I2C_InitStructure);

    i2cMap[i2c]->I2C_Enabled = true;
    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_End(HAL_I2C_Interface i2c, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    if(i2cMap[i2c]->I2C_Enabled != false)
    {
        I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);

        i2cMap[i2c]->I2C_Enabled = false;
    }
    HAL_I2C_Release(i2c, NULL);
}

uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    uint8_t bytesRead = 0;
    int state;

    /* Implementation based on ST AN2824
     * http://www.st.com/st-web-ui/static/active/jp/resource/technical/document/application_note/CD00209826.pdf
     */

    // clamp to buffer length
    if(quantity > BUFFER_LENGTH)
    {
        quantity = BUFFER_LENGTH;
    }

    // Pre-configure ACK/NACK
    I2C_AcknowledgeConfig(i2cMap[i2c]->I2C_Peripheral, ENABLE);
    I2C_NACKPositionConfig(i2cMap[i2c]->I2C_Peripheral, I2C_NACKPosition_Current);

    if (i2cMap[i2c]->prevEnding != I2C_ENDING_START)
    {
        /* While the I2C Bus is busy */
        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BUSY)))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }

        /* Send START condition */
        I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        if (!WAIT_TIMED(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_MODE_SELECT)))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }

    }

    /* Initialized variables here to minimize delays
     * between sending of slave addr and read loop
     */
    uint8_t *pBuffer = i2cMap[i2c]->rxBuffer;
    uint8_t numByteToRead = quantity;

    /* Ensure ackFailure flag is cleared */
    i2cMap[i2c]->ackFailure = false;

    /* Send Slave address for read */
    I2C_Send7bitAddress(i2cMap[i2c]->I2C_Peripheral, address << 1, I2C_Direction_Receiver);

    if((!WAIT_TIMED(!I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_ADDR) && !i2cMap[i2c]->ackFailure)) ||
        i2cMap[i2c]->ackFailure)
    {
        /* Send STOP Condition */
        I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        /* Wait to make sure that STOP control bit has been cleared */
        if (!WAIT_TIMED(i2cMap[i2c]->I2C_Peripheral->CR1 & I2C_CR1_STOP))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
        }

        /* Ensure ackFailure flag is cleared */
        i2cMap[i2c]->ackFailure = false;
        HAL_I2C_Release(i2c, NULL);
        return 0;
    }

    if (quantity == 1)
    {
        I2C_AcknowledgeConfig(i2cMap[i2c]->I2C_Peripheral, DISABLE);

        state = HAL_disable_irq();
        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->I2C_Peripheral->SR2;
        if (stop)
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);
        else
            I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);
        HAL_enable_irq(state);

        // Wait for RXNE
        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_RXNE) == RESET))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }

        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
        bytesRead = 1;
    }
    else if (quantity == 2)
    {
        I2C_NACKPositionConfig(i2cMap[i2c]->I2C_Peripheral, I2C_NACKPosition_Next);

        state = HAL_disable_irq();
        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->I2C_Peripheral->SR2;
        I2C_AcknowledgeConfig(i2cMap[i2c]->I2C_Peripheral, DISABLE);
        HAL_enable_irq(state);

        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BTF) == RESET))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }

        state = HAL_disable_irq();
        if (stop)
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);
        else
            I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
        HAL_enable_irq(state);

        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
        bytesRead = 2;
    }
    else if (quantity > 2)
    {
        /*
         * This case differs a bit from AN2824 for STM32F2:
         * For N >2 -byte reception, from N-2 data reception
         * - Wait until BTF = 1 (data N-2 in DR, data N-1 in shift register, SCL stretched low until
         *   data N-2 is read)
         * - Set ACK low
         * - Read data N-2
         * - Wait until BTF = 1 (data N-1 in DR, data N in shift register, SCL stretched low until a
         *   data N-1 is read)
         * - Set STOP high
         * - Read data N-1 and N
         */

        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->I2C_Peripheral->SR2;
        while(numByteToRead > 3)
        {
            if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BTF) == RESET))
            {
                /* SW Reset the I2C Peripheral */
                HAL_I2C_SoftwareReset(i2c);
                HAL_I2C_Release(i2c, NULL);
                return 0;
            }

            *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
            bytesRead++;
            numByteToRead--;
        }

        // Last 3 bytes
        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BTF) == RESET))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }

        I2C_AcknowledgeConfig(i2cMap[i2c]->I2C_Peripheral, DISABLE);

        // Byte N-2
        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BTF) == RESET))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }

        if (stop)
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);
        else
            I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        // Byte N-1
        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);

        // Byte N
        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);

        bytesRead += 3;
        numByteToRead = 0;
    }
    else
    {
        // Zero-byte read
        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->I2C_Peripheral->SR2;
        if (stop)
            I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);
        else
            I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);
    }

    if (stop)
    {
        /* Wait to make sure that STOP control bit has been cleared */
        if (!WAIT_TIMED(i2cMap[i2c]->I2C_Peripheral->CR1 & I2C_CR1_STOP))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }
        i2cMap[i2c]->prevEnding = I2C_ENDING_STOP;
    }
    else
    {
        i2cMap[i2c]->prevEnding = I2C_ENDING_START;

        if (!WAIT_TIMED(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_MODE_SELECT)))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }
    }

    // set rx buffer iterator vars
    i2cMap[i2c]->rxBufferIndex = 0;
    i2cMap[i2c]->rxBufferLength = bytesRead;
    HAL_I2C_Release(i2c, NULL);

    return bytesRead;
}

void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    // indicate that we are transmitting
    i2cMap[i2c]->transmitting = 1;
    // set address of targeted slave
    i2cMap[i2c]->txAddress = address << 1;
    // reset tx buffer iterator vars
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;
    HAL_I2C_Release(i2c, NULL);
}

uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    if (i2cMap[i2c]->prevEnding != I2C_ENDING_START)
    {
        /* While the I2C Bus is busy */
        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BUSY)))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 1;
        }

        /* Send START condition */
        I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        if (!WAIT_TIMED(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_MODE_SELECT)))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 2;
        }
    }

    /* Ensure ackFailure flag is cleared */
    i2cMap[i2c]->ackFailure = false;

    /* Send Slave address for write */
    I2C_Send7bitAddress(i2cMap[i2c]->I2C_Peripheral, i2cMap[i2c]->txAddress, I2C_Direction_Transmitter);

    if((!WAIT_TIMED(!I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_ADDR) && !i2cMap[i2c]->ackFailure)) ||
        i2cMap[i2c]->ackFailure)
    {
        /* Send STOP Condition */
        I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        /* Wait to make sure that STOP control bit has been cleared */
        if (!WAIT_TIMED(i2cMap[i2c]->I2C_Peripheral->CR1 & I2C_CR1_STOP))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
        }

        /* Ensure ackFailure flag is cleared */
        i2cMap[i2c]->ackFailure = false;
        HAL_I2C_Release(i2c, NULL);
        return 3;
    }

    if (!WAIT_TIMED(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED_NO_ADDR)))
    {
        /* SW Reset the I2C Peripheral */
        HAL_I2C_SoftwareReset(i2c);
        HAL_I2C_Release(i2c, NULL);
        return 31;
    }

    uint8_t *pBuffer = i2cMap[i2c]->txBuffer;
    uint8_t NumByteToWrite = i2cMap[i2c]->txBufferLength;

    if (NumByteToWrite)
    {
        // Send first byte
        I2C_SendData(i2cMap[i2c]->I2C_Peripheral, *pBuffer);
        pBuffer++;
        NumByteToWrite--;
    }

    /* While there is data to be written */
    while(NumByteToWrite--)
    {
        if (!WAIT_TIMED(I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_BTF) == RESET))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 5;
        }

        I2C_SendData(i2cMap[i2c]->I2C_Peripheral, *(pBuffer++));
    }

    /* Send STOP Condition */
    if(stop == true)
    {
        /* Send STOP condition */
        I2C_GenerateSTOP(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        if (!WAIT_TIMED(i2cMap[i2c]->I2C_Peripheral->CR1 & I2C_CR1_STOP))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 6;
        }
        i2cMap[i2c]->prevEnding = I2C_ENDING_STOP;
    }
    else
    {
        i2cMap[i2c]->prevEnding = I2C_ENDING_START;

        /* Send START condition */
        I2C_GenerateSTART(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        if (!WAIT_TIMED(!I2C_CheckEvent(i2cMap[i2c]->I2C_Peripheral, I2C_EVENT_MASTER_MODE_SELECT)))
        {
            /* SW Reset the I2C Peripheral */
            HAL_I2C_SoftwareReset(i2c);
            HAL_I2C_Release(i2c, NULL);
            return 7;
        }
    }

    // reset tx buffer iterator vars
    i2cMap[i2c]->txBufferIndex = 0;
    i2cMap[i2c]->txBufferLength = 0;

    // indicate that we are done transmitting
    i2cMap[i2c]->transmitting = 0;
    HAL_I2C_Release(i2c, NULL);

    return 0;
}

uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    if(i2cMap[i2c]->transmitting)
    {
        // in master/slave transmitter mode
        // don't bother if buffer is full
        if(i2cMap[i2c]->txBufferLength >= BUFFER_LENGTH)
        {
            HAL_I2C_Release(i2c, NULL);
            return 0;
        }
        // put byte in tx buffer
        i2cMap[i2c]->txBuffer[i2cMap[i2c]->txBufferIndex++] = data;
        // update amount in buffer
        i2cMap[i2c]->txBufferLength = i2cMap[i2c]->txBufferIndex;
    }
    HAL_I2C_Release(i2c, NULL);
    return 1;
}

int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    int32_t available = i2cMap[i2c]->rxBufferLength - i2cMap[i2c]->rxBufferIndex;
    HAL_I2C_Release(i2c, NULL);
    return available;
}

int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    int value = -1;

    // get each successive byte on each call
    if(i2cMap[i2c]->rxBufferIndex < i2cMap[i2c]->rxBufferLength)
    {
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex++];
    }
    HAL_I2C_Release(i2c, NULL);
    return value;
}

int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    int value = -1;

    if(i2cMap[i2c]->rxBufferIndex < i2cMap[i2c]->rxBufferLength)
    {
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex];
    }
    HAL_I2C_Release(i2c, NULL);
    return value;
}

void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c, void* reserved)
{
    // XXX: to be implemented.
}

bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    bool en = i2cMap[i2c]->I2C_Enabled;
    HAL_I2C_Release(i2c, NULL);
    return en;
}

void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int), void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    i2cMap[i2c]->callback_onReceive = function;
    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void), void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    i2cMap[i2c]->callback_onRequest = function;
    HAL_I2C_Release(i2c, NULL);
}

uint8_t HAL_I2C_Reset(HAL_I2C_Interface i2c, uint32_t reserved, void* reserved1)
{
    HAL_I2C_Acquire(i2c, NULL);
    if (HAL_I2C_Is_Enabled(i2c, NULL)) {
        HAL_I2C_End(i2c, NULL);

        HAL_Pin_Mode(i2cMap[i2c]->I2C_SDA_Pin, INPUT_PULLUP); //Turn SCA into high impedance input
        HAL_Pin_Mode(i2cMap[i2c]->I2C_SCL_Pin, OUTPUT); //Turn SCL into a normal GPO
        HAL_GPIO_Write(i2cMap[i2c]->I2C_SCL_Pin, 1); // Start idle HIGH

        //Generate 9 pulses on SCL to tell slave to release the bus
        for(int i=0; i <9; i++)
        {
            HAL_GPIO_Write(i2cMap[i2c]->I2C_SCL_Pin, 0);
            HAL_Delay_Microseconds(100);
            HAL_GPIO_Write(i2cMap[i2c]->I2C_SCL_Pin, 1);
            HAL_Delay_Microseconds(100);
        }

        //Change SCL to be an input
        HAL_Pin_Mode(i2cMap[i2c]->I2C_SCL_Pin, INPUT_PULLUP);

        HAL_I2C_Begin(i2c, i2cMap[i2c]->mode, i2cMap[i2c]->I2C_InitStructure.I2C_OwnAddress1 >> 1, NULL);
        HAL_Delay_Milliseconds(50);
        HAL_I2C_Release(i2c, NULL);
        return 0;
    }
    HAL_I2C_Release(i2c, NULL);
    return 1;
}

static void HAL_I2C_ER_InterruptHandler(HAL_I2C_Interface i2c)
{
    /* Useful for debugging, diable to save time */
    // DEBUG_D("ER:0x%04x\r\n",I2C_ReadRegister(i2cMap[i2c]->I2C_Peripheral, I2C_Register_SR1) & 0xFF00);
#if 0
    //Check whether specified I2C interrupt has occurred and clear IT pending bit.

    /* Check on I2C SMBus Alert flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_SMBALERT))
    {
        I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_SMBALERT);
    }

    /* Check on I2C PEC error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_PECERR))
    {
        I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_PECERR);
    }

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

    /* Check on I2C Bus error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_BERR))
    {
        I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_BERR);
    }
#else
    /* Save I2C Acknowledge failure error flag, then clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->I2C_Peripheral, I2C_IT_AF))
    {
        i2cMap[i2c]->ackFailure = true;
        I2C_ClearITPendingBit(i2cMap[i2c]->I2C_Peripheral, I2C_IT_AF);
    }

    /* Now clear all remaining error pending bits without worrying about specific error
     * interrupt bit. This is faster than checking and clearing each one.
     */

    /* Read SR1 register to get I2C error */
    if ((I2C_ReadRegister(i2cMap[i2c]->I2C_Peripheral, I2C_Register_SR1) & 0xFF00) != 0x00)
    {
        /* Clear I2C error flags */
        i2cMap[i2c]->I2C_Peripheral->SR1 &= 0x00FF;
    }
#endif

    /* We could call I2C_SoftwareResetCmd() and/or I2C_GenerateStop() from this error handler,
     * but for now only ACK failure is handled in code.
     */
}

/**
 * @brief  This function handles I2C1 Error interrupt request.
 * @param  None
 * @retval None
 */
void I2C1_ER_irq(void)
{
#if PLATFORM_ID == 10 // Electron
    if (HAL_I2C_Is_Enabled(HAL_I2C_INTERFACE1, NULL))
    {
        HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE1);
    }
    else if (HAL_I2C_Is_Enabled(HAL_I2C_INTERFACE2, NULL))
    {
        HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE2);
    }
#else
    HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE1);
#endif
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
    /* According to reference manual Figure 219 (http://www.st.com/web/en/resource/technical/document/reference_manual/CD00225773.pdf):
     * 3. After checking the SR1 register content, the user should perform the complete clearing sequence for each
     * flag found set.
     * Thus, for ADDR and STOPF flags, the following sequence is required inside the I2C interrupt routine:
     * READ SR1
     * if (ADDR == 1) {READ SR1; READ SR2}
     * if (STOPF == 1) {READ SR1; WRITE CR1}
     * The purpose is to make sure that both ADDR and STOPF flags are cleared if both are found set
     */
    uint32_t sr1 = I2C_ReadRegister(i2cMap[i2c]->I2C_Peripheral, I2C_Register_SR1);

    /* EV4 */
    if (sr1 & I2C_EVENT_SLAVE_STOP_DETECTED)
    {
        /* software sequence to clear STOPF */
        I2C_GetFlagStatus(i2cMap[i2c]->I2C_Peripheral, I2C_FLAG_STOPF);
        // Restore clock stretching settings
        // This will also clear EV4
        I2C_StretchClockCmd(i2cMap[i2c]->I2C_Peripheral, i2cMap[i2c]->clkStretchingEnabled ? ENABLE : DISABLE);
        //I2C_Cmd(i2cMap[i2c]->I2C_Peripheral, ENABLE);

        i2cMap[i2c]->rxBufferLength = i2cMap[i2c]->rxBufferIndex;
        i2cMap[i2c]->rxBufferIndex = 0;

        if(NULL != i2cMap[i2c]->callback_onReceive)
        {
            // alert user program
            i2cMap[i2c]->callback_onReceive(i2cMap[i2c]->rxBufferLength);
        }
    }

    uint32_t st = I2C_GetLastEvent(i2cMap[i2c]->I2C_Peripheral);

    /* Process Last I2C Event */
    switch (st)
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
        else
        {
            // If no data is loaded into DR register and clock stretching is enabled,
            // the device will continue pulling SCL low. To avoid that, disable clock stretching
            // when the tx buffer is exhausted to release SCL.
            I2C_StretchClockCmd(i2cMap[i2c]->I2C_Peripheral, DISABLE);
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
        // Prevent RX buffer overflow
        if (i2cMap[i2c]->rxBufferIndex < BUFFER_LENGTH)
            i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxBufferIndex++] = I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
        else
            (void)I2C_ReceiveData(i2cMap[i2c]->I2C_Peripheral);
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
#if PLATFORM_ID == 10 // Electron
    if (HAL_I2C_Is_Enabled(HAL_I2C_INTERFACE1, NULL))
    {
        HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE1);
    }
    else if (HAL_I2C_Is_Enabled(HAL_I2C_INTERFACE2, NULL))
    {
        HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE2);
    }
#else
    HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE1);
#endif
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

int32_t HAL_I2C_Acquire(HAL_I2C_Interface i2c, void* reserved)
{
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = i2cMap[i2c]->mutex;
        if (mutex) {
            return os_mutex_recursive_lock(mutex);
        }
    }
    return -1;
}

int32_t HAL_I2C_Release(HAL_I2C_Interface i2c, void* reserved)
{
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = i2cMap[i2c]->mutex;
        if (mutex) {
            return os_mutex_recursive_unlock(mutex);
        }
    }
    return -1;
}

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


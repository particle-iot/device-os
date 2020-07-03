/******************************************************************************
 * @file    i2c_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    26-Aug-2015
 * @brief
 ******************************************************************************/
/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Includes ------------------------------------------------------------------*/
#include "i2c_hal.h"

#include <stddef.h>
#include <stdlib.h>

#include "gpio_hal.h"
#include "timer_hal.h"
#include "pinmap_impl.h"
#include "platforms.h"
#include "service_debug.h"
#include "system_error.h"
#include "system_tick_hal.h"
#include "interrupts_hal.h"
#include "delay_hal.h"
#include "concurrent_hal.h"
#include "check.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif // MIN

#ifdef LOG_SOURCE_CATEGORY
LOG_SOURCE_CATEGORY("hal.i2c")
#endif // LOG_SOURCE_CATEGORY

/* Private macro -------------------------------------------------------------*/
#define TRANSMITTER     0x00
#define RECEIVER        0x01

/* Private define ------------------------------------------------------------*/
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define TOTAL_I2C   3
#else
#define TOTAL_I2C   1
#endif

#define I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED_NO_ADDR ((uint32_t)0x00070080)

#define WAIT_TIMED(timeout_ms, what) ({ \
    system_tick_t _micros = HAL_Timer_Get_Micro_Seconds();                      \
    bool res = true;                                                            \
    while ((what)) {                                                            \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);           \
        bool nok = (((timeout_ms * 1000) < dt) && (what));                      \
        if (nok) {                                                              \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})


/* Private typedef -----------------------------------------------------------*/
typedef enum i2c_ports_t {
    I2C1_D0_D1 = 0
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
   ,I2C3_C4_C5 = 1
   ,I2C3_PM_SDA_SCL = 2
#endif
} i2c_ports_t;

typedef enum i2c_transmission_ending_condition_t {
    I2C_ENDING_UNKNOWN,
    I2C_ENDING_STOP,
    I2C_ENDING_START
} i2c_transmission_ending_condition_t;

/* Private variables ---------------------------------------------------------*/
typedef struct stm32_i2c_info_t {
    I2C_TypeDef* peripheral;

    __IO uint32_t* RCC_APBRegister;
    uint32_t RCC_APBClockEnable;

    uint8_t ER_IRQn;
    uint8_t EV_IRQn;

    uint16_t sdaPin;
    uint16_t sclPin;

    uint8_t afMapping;

    I2C_InitTypeDef initStructure;

    uint32_t clockSpeed;
    volatile hal_i2c_state_t state;

    uint8_t* rxBuffer;
    size_t rxBufferSize;
    size_t rxIndexHead;
    size_t rxIndexTail;

    uint8_t txAddress;
    uint8_t* txBuffer;
    size_t txBufferSize;
    size_t txIndexHead;
    size_t txIndexTail;

    uint8_t transmitting;

    void (*callback_onRequest)(void);
    void (*callback_onReceive)(int);

    hal_i2c_mode_t mode;
    volatile bool ackFailure;
    volatile uint8_t prevEnding;
    uint8_t clkStretchingEnabled;

    os_mutex_recursive_t mutex;

    hal_i2c_transmission_config_t transferConfig;
} stm32_i2c_info_t;

/*
 * I2C mapping
 */
stm32_i2c_info_t I2C_MAP[TOTAL_I2C] = {
    { I2C1, &RCC->APB1ENR, RCC_APB1Periph_I2C1, I2C1_ER_IRQn, I2C1_EV_IRQn, D0, D1, GPIO_AF_I2C1, .mutex = NULL }
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
   ,{ I2C1, &RCC->APB1ENR, RCC_APB1Periph_I2C1, I2C1_ER_IRQn, I2C1_EV_IRQn, C4, C5, GPIO_AF_I2C1, .mutex = NULL }
   ,{ I2C3, &RCC->APB1ENR, RCC_APB1Periph_I2C3, I2C3_ER_IRQn, I2C3_EV_IRQn, PM_SDA_UC, PM_SCL_UC, GPIO_AF_I2C3, .mutex = NULL }
#endif
};

static stm32_i2c_info_t *i2cMap[TOTAL_I2C] = { // pointer to I2C_MAP[] containing I2C peripheral info
    &I2C_MAP[I2C1_D0_D1]
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
   ,&I2C_MAP[I2C3_C4_C5]
   ,&I2C_MAP[I2C3_PM_SDA_SCL]
#endif
};

/* Private function prototypes -----------------------------------------------*/
static void softwareReset(hal_i2c_interface_t i2c) {
    /* Disable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->peripheral, DISABLE);

    /* Reset all I2C registers */
    I2C_SoftwareResetCmd(i2cMap[i2c]->peripheral, ENABLE);
    I2C_SoftwareResetCmd(i2cMap[i2c]->peripheral, DISABLE);

    /* Clear all I2C interrupt error flags, and re-enable them */
    I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_SMBALERT | I2C_IT_PECERR |
            I2C_IT_TIMEOUT | I2C_IT_ARLO | I2C_IT_OVR | I2C_IT_BERR | I2C_IT_AF);
    I2C_ITConfig(i2cMap[i2c]->peripheral, I2C_IT_ERR, ENABLE);

    /* Re-enable Event and Buffer interrupts in Slave mode */
    if (i2cMap[i2c]->mode == I2C_MODE_SLAVE) {
        I2C_ITConfig(i2cMap[i2c]->peripheral, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
    }

    /* Enable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->peripheral, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(i2cMap[i2c]->peripheral, &i2cMap[i2c]->initStructure);

    i2cMap[i2c]->prevEnding = I2C_ENDING_UNKNOWN;
}

static void setConfigOrDefault(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config) {
    memset(&i2cMap[i2c]->transferConfig, 0, sizeof(i2cMap[i2c]->transferConfig));
    if (config) {
        memcpy(&i2cMap[i2c]->transferConfig, config, MIN((size_t)config->size, sizeof(i2cMap[i2c]->transferConfig)));
    } else {
        hal_i2c_transmission_config_t c = {
            .size = sizeof(hal_i2c_transmission_config_t),
            .version = 0,
            .address = 0xff,
            .reserved = {0},
            .quantity = 0,
            .timeout_ms = HAL_I2C_DEFAULT_TIMEOUT_MS,
            .flags = HAL_I2C_TRANSMISSION_FLAG_STOP
        };
        i2cMap[i2c]->transferConfig = c;
    }
}

static bool isConfigValid(const hal_i2c_config_t* config) {
    if ((config == NULL) ||
            (config->rx_buffer == NULL ||
             config->rx_buffer_size == 0 ||
             config->tx_buffer == NULL ||
             config->tx_buffer_size == 0)) {
        return false;
    }
    return true;
}

static bool isBuffersInitialized(hal_i2c_interface_t i2c) {
    if (i2cMap[i2c]->rxBuffer == NULL ||
            i2cMap[i2c]->rxBufferSize == 0 ||
            i2cMap[i2c]->txBuffer == NULL ||
            i2cMap[i2c]->txBufferSize == 0) {
        return false;
    }
    return true;
}

int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config) {
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
    // For now we only enable this for PMIC I2C bus
    if (i2c == HAL_I2C_INTERFACE3) {
        os_thread_scheduling(false, NULL);
        if (i2cMap[i2c]->mutex == NULL) {
            os_mutex_recursive_create(&i2cMap[i2c]->mutex);
        } else {
            // Already initialized
            os_thread_scheduling(true, NULL);
            return SYSTEM_ERROR_NONE;
        }
        hal_i2c_lock(i2c, NULL);
        os_thread_scheduling(true, NULL);
    }
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

    // Initialize internal data structure
    if (isConfigValid(config)) {
        i2cMap[i2c]->rxBuffer = config->rx_buffer;
        i2cMap[i2c]->rxBufferSize = config->rx_buffer_size;
        i2cMap[i2c]->txBuffer = config->tx_buffer;
        i2cMap[i2c]->txBufferSize = config->tx_buffer_size;
    } else if (!isBuffersInitialized(i2c)) {
        i2cMap[i2c]->rxBuffer = (uint8_t*)malloc(I2C_BUFFER_LENGTH);
        i2cMap[i2c]->rxBufferSize = I2C_BUFFER_LENGTH;
        i2cMap[i2c]->txBuffer = (uint8_t*)malloc(I2C_BUFFER_LENGTH);
        i2cMap[i2c]->txBufferSize = I2C_BUFFER_LENGTH;
        SPARK_ASSERT(i2cMap[i2c]->rxBuffer && i2cMap[i2c]->txBuffer);
    }

    // Initialize I2C state
    i2cMap[i2c]->clockSpeed = CLOCK_SPEED_100KHZ;
    i2cMap[i2c]->state = HAL_I2C_STATE_DISABLED;

    i2cMap[i2c]->rxIndexHead = 0;
    i2cMap[i2c]->rxIndexTail = 0;

    i2cMap[i2c]->txAddress = 0;
    i2cMap[i2c]->txIndexHead = 0;
    i2cMap[i2c]->txIndexTail = 0;

    i2cMap[i2c]->transmitting = 0;

    i2cMap[i2c]->ackFailure = false;
    i2cMap[i2c]->prevEnding = I2C_ENDING_UNKNOWN;

    i2cMap[i2c]->clkStretchingEnabled = 1;
    memset((void *)i2cMap[i2c]->rxBuffer, 0, i2cMap[i2c]->rxBufferSize);
    memset((void *)i2cMap[i2c]->txBuffer, 0, i2cMap[i2c]->txBufferSize);

    hal_i2c_unlock(i2c, NULL);
    return SYSTEM_ERROR_NONE;
}

void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    i2cMap[i2c]->clockSpeed = speed;
    hal_i2c_unlock(i2c, NULL);
}

void hal_i2c_enable_dma_mode(hal_i2c_interface_t i2c, bool enable, void* reserved) {
    /* Presently I2C Master mode uses polling and I2C Slave mode uses Interrupt */
}

void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    if (stretch) {
        I2C_StretchClockCmd(i2cMap[i2c]->peripheral, ENABLE);
    } else {
        I2C_StretchClockCmd(i2cMap[i2c]->peripheral, DISABLE);
    }

    i2cMap[i2c]->clkStretchingEnabled = stretch;
    hal_i2c_unlock(i2c, NULL);
}

void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    /*
     * On Electron both I2C_INTERFACE1 and I2C_INTERFACE2 use the same peripheral - I2C1,
     * but on different pins. We cannot enable both of them at the same time.
     */
    if (i2c == HAL_I2C_INTERFACE1 || i2c == HAL_I2C_INTERFACE2) {
        hal_i2c_interface_t dependent = (i2c == HAL_I2C_INTERFACE1 ? HAL_I2C_INTERFACE2 : HAL_I2C_INTERFACE1);
        if (hal_i2c_is_enabled(dependent, NULL) == true) {
            // Unfortunately we cannot return an error code here
            hal_i2c_unlock(i2c, NULL);
            return;
        }
    }
#endif

    if (i2cMap[i2c]->state == HAL_I2C_STATE_DISABLED) {
        i2cMap[i2c]->rxIndexHead = 0;
        i2cMap[i2c]->rxIndexTail = 0;
        i2cMap[i2c]->txIndexHead = 0;
        i2cMap[i2c]->txIndexTail = 0;
        memset((void *)i2cMap[i2c]->rxBuffer, 0, i2cMap[i2c]->rxBufferSize);
        memset((void *)i2cMap[i2c]->txBuffer, 0, i2cMap[i2c]->txBufferSize);
    }

    i2cMap[i2c]->mode = mode;
    i2cMap[i2c]->ackFailure = false;
    i2cMap[i2c]->prevEnding = I2C_ENDING_UNKNOWN;

    /* Enable I2C clock */
    *i2cMap[i2c]->RCC_APBRegister |= i2cMap[i2c]->RCC_APBClockEnable;

    /* Enable and Release I2C Reset State */
    I2C_DeInit(i2cMap[i2c]->peripheral);

    /* Connect I2C pins to respective AF */
    GPIO_PinAFConfig(PIN_MAP[i2cMap[i2c]->sclPin].gpio_peripheral, PIN_MAP[i2cMap[i2c]->sclPin].gpio_pin_source, i2cMap[i2c]->afMapping);
    GPIO_PinAFConfig(PIN_MAP[i2cMap[i2c]->sdaPin].gpio_peripheral, PIN_MAP[i2cMap[i2c]->sdaPin].gpio_pin_source, i2cMap[i2c]->afMapping);

    HAL_Pin_Mode(i2cMap[i2c]->sclPin, AF_OUTPUT_DRAIN);
    HAL_Pin_Mode(i2cMap[i2c]->sdaPin, AF_OUTPUT_DRAIN);

    NVIC_InitTypeDef  nvicInitStructure;

    nvicInitStructure.NVIC_IRQChannel = i2cMap[i2c]->ER_IRQn;
    nvicInitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    nvicInitStructure.NVIC_IRQChannelSubPriority = 0;
    nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicInitStructure);

    if (i2cMap[i2c]->mode != I2C_MODE_MASTER) {
        nvicInitStructure.NVIC_IRQChannel = i2cMap[i2c]->EV_IRQn;
        NVIC_Init(&nvicInitStructure);
    }

    i2cMap[i2c]->initStructure.I2C_Mode = I2C_Mode_I2C;
    i2cMap[i2c]->initStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    i2cMap[i2c]->initStructure.I2C_OwnAddress1 = address << 1;
    i2cMap[i2c]->initStructure.I2C_Ack = I2C_Ack_Enable;
    i2cMap[i2c]->initStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2cMap[i2c]->initStructure.I2C_ClockSpeed = i2cMap[i2c]->clockSpeed;

    /*
        From STM32 peripheral library notes:
        ====================================
        If an error occurs (ie. I2C error flags are set besides to the monitored
        flags), the I2C_CheckEvent() function may return SUCCESS despite
        the communication hold or corrupted real state.
        In this case, it is advised to use error interrupts to monitor
        the error events and handle them in the interrupt IRQ handler.
     */
    I2C_ITConfig(i2cMap[i2c]->peripheral, I2C_IT_ERR, ENABLE);

    if (i2cMap[i2c]->mode != I2C_MODE_MASTER) {
        I2C_ITConfig(i2cMap[i2c]->peripheral, I2C_IT_EVT | I2C_IT_BUF, ENABLE);

        hal_i2c_stretch_clock(i2c, i2cMap[i2c]->clkStretchingEnabled, NULL);
    }

    /* Enable the I2C peripheral */
    I2C_Cmd(i2cMap[i2c]->peripheral, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(i2cMap[i2c]->peripheral, &i2cMap[i2c]->initStructure);

    i2cMap[i2c]->state = HAL_I2C_STATE_ENABLED;
    hal_i2c_unlock(i2c, NULL);
}

void hal_i2c_end(hal_i2c_interface_t i2c, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    if (i2cMap[i2c]->state != HAL_I2C_STATE_DISABLED) {
        I2C_Cmd(i2cMap[i2c]->peripheral, DISABLE);
        i2cMap[i2c]->state = HAL_I2C_STATE_DISABLED;
    }
    hal_i2c_unlock(i2c, NULL);
}

uint32_t hal_i2c_request(hal_i2c_interface_t i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved) {
    hal_i2c_transmission_config_t conf = {
        .size = sizeof(hal_i2c_transmission_config_t),
        .version = 0,
        .address = address,
        .reserved = {0},
        .quantity = quantity,
        .timeout_ms = HAL_I2C_DEFAULT_TIMEOUT_MS,
        .flags = stop ? HAL_I2C_TRANSMISSION_FLAG_STOP : 0
    };
    return hal_i2c_request_ex(i2c, &conf, NULL);
}

int32_t hal_i2c_request_ex(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config, void* reserved) {
    if (!config) {
        return 0;
    }

    hal_i2c_lock(i2c, NULL);
    size_t bytesRead = 0;
    size_t quantity = config->quantity;
    int state;

    /* Implementation based on ST AN2824
     * http://www.st.com/st-web-ui/static/active/jp/resource/technical/document/application_note/CD00209826.pdf
     */

    // clamp to buffer length
    if (quantity > i2cMap[i2c]->rxBufferSize) {
        quantity = i2cMap[i2c]->rxBufferSize;
    }

    // Pre-configure ACK/NACK
    I2C_AcknowledgeConfig(i2cMap[i2c]->peripheral, ENABLE);
    I2C_NACKPositionConfig(i2cMap[i2c]->peripheral, I2C_NACKPosition_Current);

    if (i2cMap[i2c]->prevEnding != I2C_ENDING_START) {
        /* While the I2C Bus is busy */
        if (!WAIT_TIMED(config->timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BUSY))) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }

        /* Send START condition */
        I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);

        if (!WAIT_TIMED(config->timeout_ms, !I2C_CheckEvent(i2cMap[i2c]->peripheral, I2C_EVENT_MASTER_MODE_SELECT))) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }
    }

    /* Initialized variables here to minimize delays
     * between sending of slave addr and read loop
     */
    uint8_t *pBuffer = i2cMap[i2c]->rxBuffer;
    size_t numByteToRead = quantity;

    /* Ensure ackFailure flag is cleared */
    i2cMap[i2c]->ackFailure = false;

    /* Send Slave address for read */
    I2C_Send7bitAddress(i2cMap[i2c]->peripheral, config->address << 1, I2C_Direction_Receiver);

    if ((!WAIT_TIMED(config->timeout_ms, !I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_ADDR) && !i2cMap[i2c]->ackFailure)) || i2cMap[i2c]->ackFailure) {
        /* Send STOP Condition */
        I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);

        /* Wait to make sure that STOP control bit has been cleared */
        if (!WAIT_TIMED(config->timeout_ms, i2cMap[i2c]->peripheral->CR1 & I2C_CR1_STOP)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
        }

        /* Ensure ackFailure flag is cleared */
        i2cMap[i2c]->ackFailure = false;
        hal_i2c_unlock(i2c, NULL);
        return 0;
    }

    if (quantity == 1) {
        I2C_AcknowledgeConfig(i2cMap[i2c]->peripheral, DISABLE);

        state = HAL_disable_irq();
        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->peripheral->SR2;
        if (config->flags & HAL_I2C_TRANSMISSION_FLAG_STOP) {
            I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);
        } else {
            I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);
        }
        HAL_enable_irq(state);

        // Wait for RXNE
        if (!WAIT_TIMED(config->timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_RXNE) == RESET)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }

        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);
        bytesRead = 1;
    } else if (quantity == 2) {
        I2C_NACKPositionConfig(i2cMap[i2c]->peripheral, I2C_NACKPosition_Next);

        state = HAL_disable_irq();
        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->peripheral->SR2;
        I2C_AcknowledgeConfig(i2cMap[i2c]->peripheral, DISABLE);
        HAL_enable_irq(state);

        if (!WAIT_TIMED(config->timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BTF) == RESET)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }

        state = HAL_disable_irq();
        if (config->flags & HAL_I2C_TRANSMISSION_FLAG_STOP) {
            I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);
        } else {
            I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);
        }

        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);
        HAL_enable_irq(state);

        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);
        bytesRead = 2;
    } else if (quantity > 2) {
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
        (void)i2cMap[i2c]->peripheral->SR2;
        while (numByteToRead > 3) {
            if (!WAIT_TIMED(config->timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BTF) == RESET)) {
                /* SW Reset the I2C Peripheral */
                softwareReset(i2c);
                hal_i2c_unlock(i2c, NULL);
                return 0;
            }

            *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);
            bytesRead++;
            numByteToRead--;
        }

        // Last 3 bytes
        if (!WAIT_TIMED(config->timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BTF) == RESET)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }

        I2C_AcknowledgeConfig(i2cMap[i2c]->peripheral, DISABLE);

        // Byte N-2
        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);
        if (!WAIT_TIMED(config->timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BTF) == RESET)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }

        if (config->flags & HAL_I2C_TRANSMISSION_FLAG_STOP) {
            I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);
        } else {
            I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);
        }

        // Byte N-1
        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);
        // Byte N
        *(pBuffer++) = I2C_ReceiveData(i2cMap[i2c]->peripheral);

        bytesRead += 3;
        numByteToRead = 0;
    } else {
        // Zero-byte read
        // Clear I2C_FLAG_ADDR flag by reading SR2
        (void)i2cMap[i2c]->peripheral->SR2;
        if (config->flags & HAL_I2C_TRANSMISSION_FLAG_STOP) {
            I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);
        } else {
            I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);
        }
    }

    if (config->flags & HAL_I2C_TRANSMISSION_FLAG_STOP) {
        /* Wait to make sure that STOP control bit has been cleared */
        if (!WAIT_TIMED(config->timeout_ms, i2cMap[i2c]->peripheral->CR1 & I2C_CR1_STOP)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }
        i2cMap[i2c]->prevEnding = I2C_ENDING_STOP;
    } else {
        i2cMap[i2c]->prevEnding = I2C_ENDING_START;

        if (!WAIT_TIMED(config->timeout_ms, !I2C_CheckEvent(i2cMap[i2c]->peripheral, I2C_EVENT_MASTER_MODE_SELECT))) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }
    }

    // set rx buffer iterator vars
    i2cMap[i2c]->rxIndexHead = 0;
    i2cMap[i2c]->rxIndexTail = bytesRead;
    hal_i2c_unlock(i2c, NULL);

    return bytesRead;
}

void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config) {
    hal_i2c_lock(i2c, NULL);
    // indicate that we are transmitting
    i2cMap[i2c]->transmitting = 1;
    // set address of targeted slave
    i2cMap[i2c]->txAddress = address << 1;
    // reset tx buffer iterator vars
    i2cMap[i2c]->txIndexHead = 0;
    i2cMap[i2c]->txIndexTail = 0;
    setConfigOrDefault(i2c, config);
    hal_i2c_unlock(i2c, NULL);
}

uint8_t hal_i2c_end_transmission(hal_i2c_interface_t i2c, uint8_t stop, void* reserved) {
    hal_i2c_lock(i2c, NULL);

    if (i2cMap[i2c]->transferConfig.address != 0xff) {
        stop = i2cMap[i2c]->transferConfig.flags & HAL_I2C_TRANSMISSION_FLAG_STOP;
    }

    if (i2cMap[i2c]->prevEnding != I2C_ENDING_START) {
        /* While the I2C Bus is busy */
        if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BUSY))) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 1;
        }

        /* Send START condition */
        I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);

        if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, !I2C_CheckEvent(i2cMap[i2c]->peripheral, I2C_EVENT_MASTER_MODE_SELECT))) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 2;
        }
    }

    /* Ensure ackFailure flag is cleared */
    i2cMap[i2c]->ackFailure = false;

    /* Send Slave address for write */
    I2C_Send7bitAddress(i2cMap[i2c]->peripheral, i2cMap[i2c]->txAddress, I2C_Direction_Transmitter);

    if ((!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, !I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_ADDR) && !i2cMap[i2c]->ackFailure)) || i2cMap[i2c]->ackFailure) {
        /* Send STOP Condition */
        I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);

        /* Wait to make sure that STOP control bit has been cleared */
        if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, i2cMap[i2c]->peripheral->CR1 & I2C_CR1_STOP)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
        }

        /* Ensure ackFailure flag is cleared */
        i2cMap[i2c]->ackFailure = false;
        hal_i2c_unlock(i2c, NULL);
        return 3;
    }

    if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, !I2C_CheckEvent(i2cMap[i2c]->peripheral, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED_NO_ADDR))) {
        /* SW Reset the I2C Peripheral */
        softwareReset(i2c);
        hal_i2c_unlock(i2c, NULL);
        return 31;
    }

    uint8_t *pBuffer = i2cMap[i2c]->txBuffer;
    size_t NumByteToWrite = i2cMap[i2c]->txIndexTail;

    if (NumByteToWrite) {
        // Send first byte
        I2C_SendData(i2cMap[i2c]->peripheral, *pBuffer);
        pBuffer++;
        NumByteToWrite--;
    }

    /* While there is data to be written */
    while (NumByteToWrite--) {
        if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_BTF) == RESET)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 5;
        }
        I2C_SendData(i2cMap[i2c]->peripheral, *(pBuffer++));
    }

    /* Send STOP Condition */
    if (stop) {
        /* Send STOP condition */
        I2C_GenerateSTOP(i2cMap[i2c]->peripheral, ENABLE);

        if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, i2cMap[i2c]->peripheral->CR1 & I2C_CR1_STOP)) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 6;
        }
        i2cMap[i2c]->prevEnding = I2C_ENDING_STOP;
    } else {
        i2cMap[i2c]->prevEnding = I2C_ENDING_START;

        /* Send START condition */
        I2C_GenerateSTART(i2cMap[i2c]->peripheral, ENABLE);

        if (!WAIT_TIMED(i2cMap[i2c]->transferConfig.timeout_ms, !I2C_CheckEvent(i2cMap[i2c]->peripheral, I2C_EVENT_MASTER_MODE_SELECT))) {
            /* SW Reset the I2C Peripheral */
            softwareReset(i2c);
            hal_i2c_unlock(i2c, NULL);
            return 7;
        }
    }

    // reset tx buffer iterator vars
    i2cMap[i2c]->txIndexHead = 0;
    i2cMap[i2c]->txIndexTail = 0;

    // indicate that we are done transmitting
    i2cMap[i2c]->transmitting = 0;
    hal_i2c_unlock(i2c, NULL);

    return 0;
}

uint32_t hal_i2c_write(hal_i2c_interface_t i2c, uint8_t data, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    if (i2cMap[i2c]->transmitting) {
        // in master/slave transmitter mode
        // don't bother if buffer is full
        if (i2cMap[i2c]->txIndexTail >= i2cMap[i2c]->txBufferSize) {
            hal_i2c_unlock(i2c, NULL);
            return 0;
        }
        // put byte in tx buffer
        i2cMap[i2c]->txBuffer[i2cMap[i2c]->txIndexHead++] = data;
        // update amount in buffer
        i2cMap[i2c]->txIndexTail = i2cMap[i2c]->txIndexHead;
    }
    hal_i2c_unlock(i2c, NULL);
    return 1;
}

int32_t hal_i2c_available(hal_i2c_interface_t i2c, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    int32_t available = i2cMap[i2c]->rxIndexTail - i2cMap[i2c]->rxIndexHead;
    hal_i2c_unlock(i2c, NULL);
    return available;
}

int32_t hal_i2c_read(hal_i2c_interface_t i2c, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    int value = -1;

    // get each successive byte on each call
    if (i2cMap[i2c]->rxIndexHead < i2cMap[i2c]->rxIndexTail) {
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxIndexHead++];
    }
    hal_i2c_unlock(i2c, NULL);
    return value;
}

int32_t hal_i2c_peek(hal_i2c_interface_t i2c, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    int value = -1;

    if (i2cMap[i2c]->rxIndexHead < i2cMap[i2c]->rxIndexTail) {
        value = i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxIndexHead];
    }
    hal_i2c_unlock(i2c, NULL);
    return value;
}

void hal_i2c_flush(hal_i2c_interface_t i2c, void* reserved) {
    // XXX: to be implemented.
}

bool hal_i2c_is_enabled(hal_i2c_interface_t i2c, void* reserved) {
    return i2cMap[i2c]->state == HAL_I2C_STATE_ENABLED;
}

void hal_i2c_set_callback_on_received(hal_i2c_interface_t i2c, void (*function)(int), void* reserved) {
    hal_i2c_lock(i2c, NULL);
    i2cMap[i2c]->callback_onReceive = function;
    hal_i2c_unlock(i2c, NULL);
}

void hal_i2c_set_callback_on_requested(hal_i2c_interface_t i2c, void (*function)(void), void* reserved) {
    hal_i2c_lock(i2c, NULL);
    i2cMap[i2c]->callback_onRequest = function;
    hal_i2c_unlock(i2c, NULL);
}

uint8_t hal_i2c_reset(hal_i2c_interface_t i2c, uint32_t reserved, void* reserved1) {
    hal_i2c_lock(i2c, NULL);
    if (hal_i2c_is_enabled(i2c, NULL)) {
        hal_i2c_end(i2c, NULL);

        HAL_Pin_Mode(i2cMap[i2c]->sdaPin, INPUT_PULLUP); //Turn SCA into high impedance input
        HAL_Pin_Mode(i2cMap[i2c]->sclPin, OUTPUT); //Turn SCL into a normal GPO
        HAL_GPIO_Write(i2cMap[i2c]->sclPin, 1); // Start idle HIGH

        //Generate 9 pulses on SCL to tell slave to release the bus
        for (int i = 0; i < 9; i++) {
            HAL_GPIO_Write(i2cMap[i2c]->sclPin, 0);
            HAL_Delay_Microseconds(100);
            HAL_GPIO_Write(i2cMap[i2c]->sclPin, 1);
            HAL_Delay_Microseconds(100);
        }

        //Change SCL to be an input
        HAL_Pin_Mode(i2cMap[i2c]->sclPin, INPUT_PULLUP);

        hal_i2c_begin(i2c, i2cMap[i2c]->mode, i2cMap[i2c]->initStructure.I2C_OwnAddress1 >> 1, NULL);
        HAL_Delay_Milliseconds(50);
        hal_i2c_unlock(i2c, NULL);
        return 0;
    }
    hal_i2c_unlock(i2c, NULL);
    return 1;
}

static void HAL_I2C_ER_InterruptHandler(hal_i2c_interface_t i2c) {
    /* Useful for debugging, diable to save time */
    // DEBUG_D("ER:0x%04x\r\n",I2C_ReadRegister(i2cMap[i2c]->peripheral, I2C_Register_SR1) & 0xFF00);
#if 0
    //Check whether specified I2C interrupt has occurred and clear IT pending bit.

    /* Check on I2C SMBus Alert flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_SMBALERT)) {
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_SMBALERT);
    }

    /* Check on I2C PEC error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_PECERR)) {
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_PECERR);
    }

    /* Check on I2C Time out flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_TIMEOUT)) {
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_TIMEOUT);
    }

    /* Check on I2C Arbitration Lost flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_ARLO)) {
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_ARLO);
    }

    /* Check on I2C Overrun/Underrun error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_OVR)) {
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_OVR);
    }

    /* Check on I2C Bus error flag and clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_BERR)) {
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_BERR);
    }
#else
    /* Save I2C Acknowledge failure error flag, then clear it */
    if (I2C_GetITStatus(i2cMap[i2c]->peripheral, I2C_IT_AF)) {
        i2cMap[i2c]->ackFailure = true;
        I2C_ClearITPendingBit(i2cMap[i2c]->peripheral, I2C_IT_AF);
    }

    /* Now clear all remaining error pending bits without worrying about specific error
     * interrupt bit. This is faster than checking and clearing each one.
     */

    /* Read SR1 register to get I2C error */
    if ((I2C_ReadRegister(i2cMap[i2c]->peripheral, I2C_Register_SR1) & 0xFF00) != 0x00) {
        /* Clear I2C error flags */
        i2cMap[i2c]->peripheral->SR1 &= 0x00FF;
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
void I2C1_ER_irq(void) {
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
    if (hal_i2c_is_enabled(HAL_I2C_INTERFACE1, NULL)) {
        HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE1);
    } else if (hal_i2c_is_enabled(HAL_I2C_INTERFACE2, NULL)) {
        HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE2);
    }
#else
    HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE1);
#endif
}

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
/**
 * @brief  This function handles I2C3 Error interrupt request.
 * @param  None
 * @retval None
 */
void I2C3_ER_irq(void) {
    HAL_I2C_ER_InterruptHandler(HAL_I2C_INTERFACE3);
}
#endif

static void HAL_I2C_EV_InterruptHandler(hal_i2c_interface_t i2c) {
    /* According to reference manual Figure 219 (http://www.st.com/web/en/resource/technical/document/reference_manual/CD00225773.pdf):
     * 3. After checking the SR1 register content, the user should perform the complete clearing sequence for each
     * flag found set.
     * Thus, for ADDR and STOPF flags, the following sequence is required inside the I2C interrupt routine:
     * READ SR1
     * if (ADDR == 1) {READ SR1; READ SR2}
     * if (STOPF == 1) {READ SR1; WRITE CR1}
     * The purpose is to make sure that both ADDR and STOPF flags are cleared if both are found set
     */
    uint32_t sr1 = I2C_ReadRegister(i2cMap[i2c]->peripheral, I2C_Register_SR1);

    /* EV4 */
    if (sr1 & I2C_EVENT_SLAVE_STOP_DETECTED) {
        /* software sequence to clear STOPF */
        I2C_GetFlagStatus(i2cMap[i2c]->peripheral, I2C_FLAG_STOPF);
        // Restore clock stretching settings
        // This will also clear EV4
        I2C_StretchClockCmd(i2cMap[i2c]->peripheral, i2cMap[i2c]->clkStretchingEnabled ? ENABLE : DISABLE);
        //I2C_Cmd(i2cMap[i2c]->peripheral, ENABLE);

        i2cMap[i2c]->rxIndexTail = i2cMap[i2c]->rxIndexHead;
        i2cMap[i2c]->rxIndexHead = 0;

        if (NULL != i2cMap[i2c]->callback_onReceive) {
            // alert user program
            i2cMap[i2c]->callback_onReceive(i2cMap[i2c]->rxIndexTail);
        }
    }

    uint32_t st = I2C_GetLastEvent(i2cMap[i2c]->peripheral);

    /* Process Last I2C Event */
    switch (st) {
        /********** Slave Transmitter Events ************/
        /* Check on EV1 */
        case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED: {
            i2cMap[i2c]->transmitting = 1;
            i2cMap[i2c]->txIndexHead = 0;
            i2cMap[i2c]->txIndexTail = 0;
            if (NULL != i2cMap[i2c]->callback_onRequest) {
                // alert user program
                i2cMap[i2c]->callback_onRequest();
            }
            i2cMap[i2c]->txIndexHead = 0;
            break;
        }
        /* Check on EV3 */
        case I2C_EVENT_SLAVE_BYTE_TRANSMITTING:
        case I2C_EVENT_SLAVE_BYTE_TRANSMITTED: {
            if (i2cMap[i2c]->txIndexHead < i2cMap[i2c]->txIndexTail) {
                I2C_SendData(i2cMap[i2c]->peripheral, i2cMap[i2c]->txBuffer[i2cMap[i2c]->txIndexHead++]);
            } else {
                // If no data is loaded into DR register and clock stretching is enabled,
                // the device will continue pulling SCL low. To avoid that, disable clock stretching
                // when the tx buffer is exhausted to release SCL.
                I2C_StretchClockCmd(i2cMap[i2c]->peripheral, DISABLE);
                __DSB();
                __ISB();
                I2C_StretchClockCmd(i2cMap[i2c]->peripheral, i2cMap[i2c]->clkStretchingEnabled ? ENABLE : DISABLE);
            }
            break;
        }

        /*********** Slave Receiver Events *************/
        /* check on EV1*/
        case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED: {
            i2cMap[i2c]->rxIndexHead = 0;
            i2cMap[i2c]->rxIndexTail = 0;
            break;
        }

        /* Check on EV2*/
        case I2C_EVENT_SLAVE_BYTE_RECEIVED:
        case (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_SR1_BTF): {
            // Prevent RX buffer overflow
            if (i2cMap[i2c]->rxIndexHead < i2cMap[i2c]->rxBufferSize) {
                i2cMap[i2c]->rxBuffer[i2cMap[i2c]->rxIndexHead++] = I2C_ReceiveData(i2cMap[i2c]->peripheral);
            } else {
                (void)I2C_ReceiveData(i2cMap[i2c]->peripheral);
            }
            break;
        }

        default: {
            break;
        }
    }
}

/**
 * @brief  This function handles I2C1 event interrupt request.
 * @param  None
 * @retval None
 */
void I2C1_EV_irq(void) {
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
    if (hal_i2c_is_enabled(HAL_I2C_INTERFACE1, NULL)) {
        HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE1);
    } else if (hal_i2c_is_enabled(HAL_I2C_INTERFACE2, NULL)) {
        HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE2);
    }
#else
    HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE1);
#endif
}

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
/**
 * @brief  This function handles I2C3 event interrupt request.
 * @param  None
 * @retval None
 */
void I2C3_EV_irq(void) {
    HAL_I2C_EV_InterruptHandler(HAL_I2C_INTERFACE3);
}
#endif

int32_t hal_i2c_lock(hal_i2c_interface_t i2c, void* reserved) {
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = i2cMap[i2c]->mutex;
        if (mutex) {
            return os_mutex_recursive_lock(mutex);
        }
    }
    return -1;
}

int32_t hal_i2c_unlock(hal_i2c_interface_t i2c, void* reserved) {
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = i2cMap[i2c]->mutex;
        if (mutex) {
            return os_mutex_recursive_unlock(mutex);
        }
    }
    return -1;
}

int hal_i2c_sleep(hal_i2c_interface_t i2c, bool sleep, void* reserved) {
    hal_i2c_lock(i2c, NULL);
    if (sleep) {
        // Suspend I2C
        if (i2cMap[i2c]->state != HAL_I2C_STATE_ENABLED) {
            hal_i2c_unlock(i2c, NULL);
            return SYSTEM_ERROR_INVALID_STATE;
        }
        hal_i2c_end(i2c, NULL);
        i2cMap[i2c]->state = HAL_I2C_STATE_SUSPENDED;
    } else {
        // Restore I2C
        if (i2cMap[i2c]->state != HAL_I2C_STATE_SUSPENDED) {
            hal_i2c_unlock(i2c, NULL);
            return SYSTEM_ERROR_INVALID_STATE;
        }
        hal_i2c_begin(i2c, i2cMap[i2c]->mode, i2cMap[i2c]->initStructure.I2C_OwnAddress1 >> 1, NULL);
    }
    hal_i2c_unlock(i2c, NULL);
    return SYSTEM_ERROR_NONE;
}

// On the Photon/P1 the I2C interface selector was added after the first release.
// So these compatibility functions are needed for older firmware

void hal_i2c_set_speed_deprecated(uint32_t speed) { hal_i2c_set_speed(HAL_I2C_INTERFACE1, speed, NULL); }
void hal_i2c_enable_dma_mode_deprecated(bool enable) { hal_i2c_enable_dma_mode(HAL_I2C_INTERFACE1, enable, NULL); }
void hal_i2c_stretch_clock_deprecated(bool stretch) { hal_i2c_stretch_clock(HAL_I2C_INTERFACE1, stretch, NULL); }
void hal_i2c_begin_deprecated(hal_i2c_mode_t mode, uint8_t address) { hal_i2c_begin(HAL_I2C_INTERFACE1, mode, address, NULL); }
void hal_i2c_end_deprecated() { hal_i2c_end(HAL_I2C_INTERFACE1, NULL); }
uint32_t hal_i2c_request_deprecated(uint8_t address, uint8_t quantity, uint8_t stop) { return hal_i2c_request(HAL_I2C_INTERFACE1, address, quantity, stop, NULL); }
void hal_i2c_begin_transmission_deprecated(uint8_t address) { hal_i2c_begin_transmission(HAL_I2C_INTERFACE1, address, NULL); }
uint8_t hal_i2c_end_transmission_deprecated(uint8_t stop) { return hal_i2c_end_transmission(HAL_I2C_INTERFACE1, stop, NULL); }
uint32_t hal_i2c_write_deprecated(uint8_t data) { return hal_i2c_write(HAL_I2C_INTERFACE1, data, NULL); }
int32_t hal_i2c_available_deprecated(void) { return hal_i2c_available(HAL_I2C_INTERFACE1, NULL); }
int32_t hal_i2c_read_deprecated(void) { return hal_i2c_read(HAL_I2C_INTERFACE1, NULL); }
int32_t hal_i2c_peek_deprecated(void) { return hal_i2c_peek(HAL_I2C_INTERFACE1, NULL); }
void hal_i2c_flush_deprecated(void) { hal_i2c_flush(HAL_I2C_INTERFACE1, NULL); }
bool hal_i2c_is_enabled_deprecated(void) { return hal_i2c_is_enabled(HAL_I2C_INTERFACE1, NULL); }
void hal_i2c_set_callback_on_received_deprecated(void (*function)(int)) { hal_i2c_set_callback_on_received(HAL_I2C_INTERFACE1, function, NULL); }
void hal_i2c_set_callback_on_requested_deprecated(void (*function)(void)) { hal_i2c_set_callback_on_requested(HAL_I2C_INTERFACE1, function, NULL); }


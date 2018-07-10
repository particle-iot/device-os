/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include <string.h>
#include "nrfx_twim.h"
#include "nrf_gpio.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "platforms.h"
#include "concurrent_hal.h"
#include "interrupts_hal.h"
#include "pinmap_impl.h"
#include "logging.h"

#define TOTAL_I2C                   2

#define BUFFER_LENGTH               I2C_BUFFER_LENGTH 
#define I2C_IRQ_PRIORITY            APP_IRQ_PRIORITY_LOWEST


/* TWI instance. */
static nrfx_twim_t m_twi0 = NRFX_TWIM_INSTANCE(0);
static nrfx_twim_t m_twi1 = NRFX_TWIM_INSTANCE(1);

typedef struct {
    nrfx_twim_t             *instance;
    uint8_t                 scl_pin;
    uint8_t                 sda_pin;

    bool                    enabled;
    volatile bool           transmitting;
    I2C_Mode                mode;
    uint32_t                speed;

    uint8_t                 address;

    uint8_t                 rx_buf[BUFFER_LENGTH];
    uint8_t                 rx_buf_index;
    uint8_t                 rx_buf_length;
    uint8_t                 tx_buf[BUFFER_LENGTH];
    uint8_t                 tx_buf_index;
    uint8_t                 tx_buf_length;

    os_mutex_recursive_t    mutex;

    void (*callback_on_request)(void);
    void (*callback_on_receive)(int);
} nrf5x_i2c_info_t;

nrf5x_i2c_info_t m_i2c_map[TOTAL_I2C] = {
    {&m_twi0, SCL, SDA},
#if PLATFORM_ID == PLATFORM_BORON
    {&m_twi1, PMIC_SCL, PMIC_SDA}
#else
    {&m_twi1, D3, D2}
#endif
};

static void twi_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
    uint32_t inst_num = (uint32_t)p_context;

    switch (p_event->type)
    {
        case NRFX_TWIM_EVT_DONE:
            if (p_event->xfer_desc.type == NRFX_TWIM_XFER_RX)
            {
                // TODO: slave mode receive 
            }
            m_i2c_map[inst_num].transmitting = false;
            break;

        case NRFX_TWIM_EVT_ADDRESS_NACK:
            m_i2c_map[inst_num].transmitting = false;
            break;
        case NRFX_TWIM_EVT_DATA_NACK:
            m_i2c_map[inst_num].transmitting = false;
            break;
        default:
            break;
    }
}

static int twi_uinit(HAL_I2C_Interface i2c)
{
    if (m_i2c_map[i2c].enabled)
    {
        nrfx_twim_uninit(m_i2c_map[i2c].instance);
    }

    // Reset pin function
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    PIN_MAP[m_i2c_map[i2c].scl_pin].pin_func = PF_NONE;
    PIN_MAP[m_i2c_map[i2c].sda_pin].pin_func = PF_NONE;

    return 0;
}

static int twi_init(HAL_I2C_Interface i2c)
{
    ret_code_t err_code;
    nrf_twim_frequency_t nrf_frequency = (m_i2c_map[i2c].speed == CLOCK_SPEED_400KHZ) ? NRF_TWIM_FREQ_400K : NRF_TWIM_FREQ_100K;

    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    
    const nrfx_twim_config_t twi_config = {
       .scl                = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[m_i2c_map[i2c].scl_pin].gpio_port, PIN_MAP[m_i2c_map[i2c].scl_pin].gpio_pin), 
       .sda                = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[m_i2c_map[i2c].sda_pin].gpio_port, PIN_MAP[m_i2c_map[i2c].sda_pin].gpio_pin),
       .frequency          = nrf_frequency,
       .interrupt_priority = I2C_IRQ_PRIORITY,
       .hold_bus_uninit    = false
    };

    void *p_context = (void *)i2c;
    err_code = nrfx_twim_init(m_i2c_map[i2c].instance, &twi_config, twi_handler, p_context);
    SPARK_ASSERT(err_code == NRF_SUCCESS);

    nrfx_twim_enable(m_i2c_map[i2c].instance);

    // Set pin function
    PIN_MAP[m_i2c_map[i2c].scl_pin].pin_func = PF_I2C;
    PIN_MAP[m_i2c_map[i2c].sda_pin].pin_func = PF_I2C;

    return 0;
}

void HAL_I2C_Init(HAL_I2C_Interface i2c, void* reserved)
{
    if (m_i2c_map[i2c].enabled)
    {
        twi_uinit(i2c);
    }

    os_thread_scheduling(false, NULL);
    if (m_i2c_map[i2c].mutex == NULL) 
    {
        os_mutex_recursive_create(&m_i2c_map[i2c].mutex);
    }

    HAL_I2C_Acquire(i2c, NULL);
    os_thread_scheduling(true, NULL);

    m_i2c_map[i2c].enabled = false;
    m_i2c_map[i2c].mode = I2C_MODE_MASTER;
    m_i2c_map[i2c].transmitting = false;
    m_i2c_map[i2c].speed = CLOCK_SPEED_100KHZ;
    m_i2c_map[i2c].rx_buf_index = 0;
    m_i2c_map[i2c].rx_buf_length = 0;
    m_i2c_map[i2c].tx_buf_index = 0;
    m_i2c_map[i2c].tx_buf_length = 0;
    memset(m_i2c_map[i2c].rx_buf, 0 , BUFFER_LENGTH);
    memset(m_i2c_map[i2c].tx_buf, 0 , BUFFER_LENGTH);

    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_Set_Speed(HAL_I2C_Interface i2c, uint32_t speed, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    m_i2c_map[i2c].speed = speed;
    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_Stretch_Clock(HAL_I2C_Interface i2c, bool stretch, void* reserved)
{
    // always enabled
}

void HAL_I2C_Begin(HAL_I2C_Interface i2c, I2C_Mode mode, uint8_t address, void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);

    // TODO: to support slave mode 

    twi_uinit(i2c);

    m_i2c_map[i2c].rx_buf_index = 0;
    m_i2c_map[i2c].rx_buf_length = 0;
    m_i2c_map[i2c].tx_buf_index = 0;
    m_i2c_map[i2c].tx_buf_length = 0;
    memset(m_i2c_map[i2c].rx_buf, 0 , BUFFER_LENGTH);
    memset(m_i2c_map[i2c].tx_buf, 0 , BUFFER_LENGTH);

    twi_init(i2c);
    m_i2c_map[i2c].enabled = true;

    HAL_I2C_Release(i2c, NULL);
}

void HAL_I2C_End(HAL_I2C_Interface i2c,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    if (m_i2c_map[i2c].enabled)
    {
        twi_uinit(i2c);
        m_i2c_map[i2c].enabled = false;
    }
    HAL_I2C_Release(i2c, NULL);
}

uint32_t HAL_I2C_Request_Data(HAL_I2C_Interface i2c, uint8_t address, uint8_t quantity, uint8_t stop,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    uint32_t err_code;

    // clamp to buffer length
    if(quantity > BUFFER_LENGTH)
    {
        quantity = BUFFER_LENGTH;
    }

    m_i2c_map[i2c].transmitting = true;
    m_i2c_map[i2c].address = address;
    err_code = nrfx_twim_rx(m_i2c_map[i2c].instance, m_i2c_map[i2c].address, m_i2c_map[i2c].rx_buf, quantity);
    if (err_code)
    {
        m_i2c_map[i2c].transmitting = false;
        quantity = 0;
    }

    while (m_i2c_map[i2c].transmitting);

    m_i2c_map[i2c].rx_buf_index = 0;
    m_i2c_map[i2c].rx_buf_length = quantity;
    HAL_I2C_Release(i2c, NULL);

    return quantity;
}

void HAL_I2C_Begin_Transmission(HAL_I2C_Interface i2c, uint8_t address,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    m_i2c_map[i2c].address = address;
    m_i2c_map[i2c].tx_buf_index = 0;
    m_i2c_map[i2c].tx_buf_length = 0;
    HAL_I2C_Release(i2c, NULL);
}

uint8_t HAL_I2C_End_Transmission(HAL_I2C_Interface i2c, uint8_t stop,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    m_i2c_map[i2c].transmitting = true;
    uint32_t err_code = nrfx_twim_tx(m_i2c_map[i2c].instance, m_i2c_map[i2c].address, m_i2c_map[i2c].tx_buf, 
                                       m_i2c_map[i2c].tx_buf_length, stop);
    if (err_code)
    {
        m_i2c_map[i2c].transmitting = false;
    }

    while (m_i2c_map[i2c].transmitting);

    HAL_I2C_Release(i2c, NULL);
    return 0;
}

uint32_t HAL_I2C_Write_Data(HAL_I2C_Interface i2c, uint8_t data,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);

    // in master/slave transmitter mode
    // don't bother if buffer is full
    if (m_i2c_map[i2c].tx_buf_length >= BUFFER_LENGTH)
    {
        HAL_I2C_Release(i2c, NULL);
        return 0;
    }
    // put byte in tx buffer
    m_i2c_map[i2c].tx_buf[m_i2c_map[i2c].tx_buf_index++] = data;
    // update amount in buffer
    m_i2c_map[i2c].tx_buf_length = m_i2c_map[i2c].tx_buf_index;

    HAL_I2C_Release(i2c, NULL);
    return 1;
}

int32_t HAL_I2C_Available_Data(HAL_I2C_Interface i2c,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    int32_t available = m_i2c_map[i2c].rx_buf_length - m_i2c_map[i2c].rx_buf_index;
    HAL_I2C_Release(i2c, NULL);
    return available;
}

int32_t HAL_I2C_Read_Data(HAL_I2C_Interface i2c,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    int value = -1;

    // get each successive byte on each call
    if (m_i2c_map[i2c].rx_buf_index < m_i2c_map[i2c].rx_buf_length)
    {
        value = m_i2c_map[i2c].rx_buf[m_i2c_map[i2c].rx_buf_index++];
    }
    HAL_I2C_Release(i2c, NULL);
    return value;
}

int32_t HAL_I2C_Peek_Data(HAL_I2C_Interface i2c,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    int value = -1;

    if(m_i2c_map[i2c].rx_buf_index < m_i2c_map[i2c].rx_buf_length)
    {
        value = m_i2c_map[i2c].rx_buf[m_i2c_map[i2c].rx_buf_index];
    }
    HAL_I2C_Release(i2c, NULL);
    return value;
}

void HAL_I2C_Flush_Data(HAL_I2C_Interface i2c,void* reserved)
{
    m_i2c_map[i2c].rx_buf_index = 0;
    m_i2c_map[i2c].rx_buf_length = 0;
    m_i2c_map[i2c].tx_buf_index = 0;
    m_i2c_map[i2c].tx_buf_length = 0;
}

bool HAL_I2C_Is_Enabled(HAL_I2C_Interface i2c,void* reserved)
{
    HAL_I2C_Acquire(i2c, NULL);
    bool en = m_i2c_map[i2c].enabled;
    HAL_I2C_Release(i2c, NULL);
    return en;
}

void HAL_I2C_Set_Callback_On_Receive(HAL_I2C_Interface i2c, void (*function)(int),void* reserved)
{
    // TODO: slave mode
}

void HAL_I2C_Set_Callback_On_Request(HAL_I2C_Interface i2c, void (*function)(void),void* reserved)
{
    // TODO: slave mode
}

/*******************************************************************************
 * Function Name  : HAL_I2C1_EV_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles I2C1 Event interrupt request(Only for Slave mode).
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_I2C1_EV_Handler(void)
{
    // TODO: slave mode
}

void HAL_I2C_Enable_DMA_Mode(HAL_I2C_Interface i2c, bool enable,void* reserved)
{
    // nrfx driver use DMA to send data
}

uint8_t HAL_I2C_Reset(HAL_I2C_Interface i2c, uint32_t reserved, void* reserved1)
{
  return 0;
}

int32_t HAL_I2C_Acquire(HAL_I2C_Interface i2c, void* reserved)
{
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = m_i2c_map[i2c].mutex;
        if (mutex) {
            return os_mutex_recursive_lock(mutex);
        }
    }
    return -1;
}

int32_t HAL_I2C_Release(HAL_I2C_Interface i2c, void* reserved)
{
    if (!HAL_IsISR()) {
        os_mutex_recursive_t mutex = m_i2c_map[i2c].mutex;
        if (mutex) {
            return os_mutex_recursive_unlock(mutex);
        }
    }
    return -1;
}

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
#include "sdk_common.h"
#include "nrfx.h"
#include "nrf_drv_uart.h"
#include "nrf_gpio.h"
#include "nrf_assert.h"
#include "app_fifo.h"
#include "pinmap_impl.h"
#include "usart_hal.h"
#include "logging.h"
  
#define UART_BUFF_SIZE              (SERIAL_BUFFER_SIZE * 2)

// only support even parity(SERIAL_PARITY_EVEN)
#define IS_PARITY_VALID(config)     (((config & SERIAL_PARITY) == SERIAL_PARITY_EVEN) || ((config & SERIAL_PARITY) == SERIAL_PARITY_NO))
#define IS_PARITY_ENABLED(config)   (((config & SERIAL_PARITY) == SERIAL_PARITY_EVEN) ? true : false)
// only support both CTS and RTS(SERIAL_FLOW_CONTROL_RTS_CTS)
#define IS_HWFC_VALID(config)       (((config & SERIAL_FLOW_CONTROL) == SERIAL_FLOW_CONTROL_RTS_CTS) || ((config & SERIAL_FLOW_CONTROL) == SERIAL_FLOW_CONTROL_NONE))
#define IS_HWFC_ENABLED(config)     (((config & SERIAL_FLOW_CONTROL) == SERIAL_FLOW_CONTROL_RTS_CTS) ? true : false)
// only support one stop bit 
#define IS_STOP_BITS_VALID(config)  ((config & SERIAL_STOP_BITS) == SERIAL_STOP_BITS_1)
// only support one 8 data bits
#define IS_DATA_BITS_VALID(config)  ((config & SERIAL_DATA_BITS) == SERIAL_DATA_BITS_8)

typedef struct {
    nrf_drv_uart_t          *instance;
    app_irq_priority_t      priority;
    uint8_t                 tx_pin;
    uint8_t                 rx_pin;
    uint8_t                 cts_pin;
    uint8_t                 rts_pin;

    app_fifo_t              rx_fifo;
    app_fifo_t              tx_fifo;
    uint8_t                 rx_buffer[1];
    uint8_t                 tx_buffer[1];

    bool                    enabled;
    bool                    rx_ovf;
    uint32_t                uart_config;
} nrf5x_uart_info_t;

static nrf_drv_uart_t m_uarte0 = NRF_DRV_UART_INSTANCE(0);
static nrf_drv_uart_t m_uarte1 = NRF_DRV_UART_INSTANCE(1);

static nrf5x_uart_info_t m_uart_map[TOTAL_USARTS] = 
{
    {&m_uarte0, APP_IRQ_PRIORITY_LOWEST, TX, RX, CTS, RTS},
    {&m_uarte1, APP_IRQ_PRIORITY_LOWEST, TX1, RX1, CTS1, RTS1}
};

#define FIFO_LENGTH(p_fifo)     fifo_length(p_fifo)  /**< Macro for calculating the FIFO length. */
#define IS_FIFO_FULL(p_fifo)    fifo_full(p_fifo)
static __INLINE uint32_t fifo_length(app_fifo_t * p_fifo)
{
    uint32_t tmp = p_fifo->read_pos;
    return p_fifo->write_pos - tmp;
}
static __INLINE bool fifo_full(app_fifo_t * p_fifo)
{
    return (FIFO_LENGTH(p_fifo) > p_fifo->buf_size_mask);
}

static nrf_uart_baudrate_t get_nrf_baudrate(uint32_t baud)
{
    static const struct {
        uint32_t baud;
        nrf_uart_baudrate_t nrf_baud;
    } baudrate_map[] = {
        { 1200,     NRF_UARTE_BAUDRATE_1200    }, 
        { 2400,     NRF_UARTE_BAUDRATE_2400    }, 
        { 4800,     NRF_UARTE_BAUDRATE_4800    }, 
        { 9600,     NRF_UARTE_BAUDRATE_9600    }, 
        { 14400,    NRF_UARTE_BAUDRATE_14400   }, 
        { 19200,    NRF_UARTE_BAUDRATE_19200   }, 
        { 28800,    NRF_UARTE_BAUDRATE_28800   }, 
        { 38400,    NRF_UARTE_BAUDRATE_38400   }, 
        { 57600,    NRF_UARTE_BAUDRATE_57600   }, 
        { 76800,    NRF_UARTE_BAUDRATE_76800   }, 
        { 115200,   NRF_UARTE_BAUDRATE_115200  }, 
        { 230400,   NRF_UARTE_BAUDRATE_230400  }, 
        { 250000,   NRF_UARTE_BAUDRATE_250000  }, 
        { 460800,   NRF_UARTE_BAUDRATE_460800  }, 
        { 921600,   NRF_UARTE_BAUDRATE_921600  }, 
        { 1000000,  NRF_UARTE_BAUDRATE_1000000 }
    };

    nrf_uart_baudrate_t nrf_baudtate = NRF_UARTE_BAUDRATE_115200;

    if (baud < 1200 || baud > 1000000)
    {
        // return default baudrate
        return nrf_baudtate;
    }

    for (uint32_t i = 1; i < sizeof(baudrate_map) / sizeof(baudrate_map[0]); i++)
    {
        if (baud < baudrate_map[i].baud)
        {
            nrf_baudtate = baudrate_map[i - 1].nrf_baud;
            break;
        }
    }
    
    return nrf_baudtate;
}

static void uart_event_handler(nrf_drv_uart_event_t * p_event, void* p_context)
{
    uint32_t instance_num = (uint32_t)p_context;

    switch (p_event->type)
    {
        case NRF_DRV_UART_EVT_RX_DONE:
            // Write received byte to FIFO.
            app_fifo_put(&m_uart_map[instance_num].rx_fifo, p_event->data.rxtx.p_data[0]);

            // Start new RX if size in buffer.
            if (FIFO_LENGTH(&m_uart_map[instance_num].rx_fifo) <= m_uart_map[instance_num].rx_fifo.buf_size_mask)
            {
                nrf_drv_uart_rx(m_uart_map[instance_num].instance, m_uart_map[instance_num].rx_buffer, 1);
            }
            else
            {
                // Overflow in RX FIFO.
                m_uart_map[instance_num].rx_ovf = true;
            }
            break;

        case NRF_DRV_UART_EVT_ERROR:
            nrf_drv_uart_rx(m_uart_map[instance_num].instance, m_uart_map[instance_num].rx_buffer, 1);
            break;

        case NRF_DRV_UART_EVT_TX_DONE:
            // Get next byte from FIFO.
            if (app_fifo_get(&m_uart_map[instance_num].tx_fifo, m_uart_map[instance_num].tx_buffer) == NRF_SUCCESS)
            {
                nrf_drv_uart_tx(m_uart_map[instance_num].instance, m_uart_map[instance_num].tx_buffer, 1);
            }
            break;

        default:
            break;
    }
}

static uint32_t uart_init(HAL_USART_Serial serial, uint32_t baud)
{
    if (m_uart_map[serial].tx_fifo.p_buf == NULL || m_uart_map[serial].rx_fifo.p_buf == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    uint32_t err_code;
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    nrf_drv_uart_config_t config = {
        .pseltxd            = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[m_uart_map[serial].tx_pin].gpio_port, PIN_MAP[m_uart_map[serial].tx_pin].gpio_pin),              
        .pselrxd            = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[m_uart_map[serial].rx_pin].gpio_port, PIN_MAP[m_uart_map[serial].rx_pin].gpio_pin),                      
        .pselcts            = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[m_uart_map[serial].cts_pin].gpio_port, PIN_MAP[m_uart_map[serial].cts_pin].gpio_pin),                      
        .pselrts            = (uint32_t)NRF_GPIO_PIN_MAP(PIN_MAP[m_uart_map[serial].rts_pin].gpio_port, PIN_MAP[m_uart_map[serial].rts_pin].gpio_pin),                     
        .p_context          = (void *)((int)m_uart_map[serial].instance->inst_idx),                                              
        .hwfc               = IS_HWFC_ENABLED(m_uart_map[serial].uart_config) ? NRF_UART_HWFC_ENABLED : NRF_UART_HWFC_DISABLED,
        .parity             = IS_PARITY_ENABLED(m_uart_map[serial].uart_config) ? NRF_UART_PARITY_INCLUDED : NRF_UART_PARITY_EXCLUDED,
        .baudrate           = get_nrf_baudrate(baud),
        .interrupt_priority = m_uart_map[serial].priority,                  
        NRF_DRV_UART_DEFAULT_CONFIG_USE_EASY_DMA                                 
    };

    err_code = nrf_drv_uart_init(m_uart_map[serial].instance, &config, uart_event_handler);
    SPARK_ASSERT(err_code == NRF_SUCCESS);

    // Turn on receiver 
    nrf_drv_uart_rx(m_uart_map[serial].instance, m_uart_map[serial].rx_buffer, 1);

    // set pin mode
    HAL_Set_Pin_Function(m_uart_map[serial].tx_pin, PF_UART);
    HAL_Set_Pin_Function(m_uart_map[serial].rx_pin, PF_UART);
    if (IS_HWFC_ENABLED(m_uart_map[serial].uart_config))
    {
        HAL_Set_Pin_Function(m_uart_map[serial].cts_pin, PF_UART);
        HAL_Set_Pin_Function(m_uart_map[serial].rts_pin, PF_UART);
    }

    return NRF_SUCCESS;
}

static void uart_uninit(HAL_USART_Serial serial)
{
    nrf_drv_uart_uninit(m_uart_map[serial].instance);

    // reset pin mode
    HAL_Set_Pin_Function(m_uart_map[serial].tx_pin, PF_NONE);
    HAL_Set_Pin_Function(m_uart_map[serial].rx_pin, PF_NONE);
    if (IS_HWFC_ENABLED(m_uart_map[serial].uart_config))
    {
        HAL_Set_Pin_Function(m_uart_map[serial].cts_pin, PF_NONE);
        HAL_Set_Pin_Function(m_uart_map[serial].rts_pin, PF_NONE);
    }
}

static uint32_t uart_flush(HAL_USART_Serial serial)
{
    uint32_t err_code;

    err_code = app_fifo_flush(&m_uart_map[serial].rx_fifo);
    VERIFY_SUCCESS(err_code);

    err_code = app_fifo_flush(&m_uart_map[serial].tx_fifo);
    VERIFY_SUCCESS(err_code);

    return NRF_SUCCESS;
}

static uint32_t uart_get(HAL_USART_Serial serial, uint8_t * p_byte)
{
    ASSERT(p_byte);
    bool rx_ovf = m_uart_map[serial].rx_ovf;

    ret_code_t err_code = app_fifo_get(&m_uart_map[serial].rx_fifo, p_byte);

    // If FIFO was full new request to receive one byte was not scheduled. Must be done here.
    if (rx_ovf)
    {
        m_uart_map[serial].rx_ovf = false;
        uint32_t uart_err_code = nrf_drv_uart_rx(m_uart_map[serial].instance, m_uart_map[serial].rx_buffer, 1);

        // RX resume should never fail.
        SPARK_ASSERT(uart_err_code == NRF_SUCCESS);
    }

    return err_code;
}

static uint32_t uart_put(HAL_USART_Serial serial, uint8_t byte)
{
    uint32_t err_code;
    err_code = app_fifo_put(&m_uart_map[serial].tx_fifo, byte);
    if (err_code == NRF_SUCCESS)
    {
        // The new byte has been added to FIFO. It will be picked up from there
        // (in 'uart_event_handler') when all preceding bytes are transmitted.
        // But if UART is not transmitting anything at the moment, we must start
        // a new transmission here.
        if (!nrf_drv_uart_tx_in_progress(m_uart_map[serial].instance))
        {
            // This operation should be almost always successful, since we've
            // just added a byte to FIFO, but if some bigger delay occurred
            // (some heavy interrupt handler routine has been executed) since
            // that time, FIFO might be empty already.
            if (app_fifo_get(&m_uart_map[serial].tx_fifo, m_uart_map[serial].tx_buffer) == NRF_SUCCESS)
            {
                err_code = nrf_drv_uart_tx(m_uart_map[serial].instance, m_uart_map[serial].tx_buffer, 1);
            }
        }
    }
    return err_code;
}

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
    uint32_t ret;

    if (m_uart_map[serial].enabled)
    {
        uart_uninit(serial);
    }

    memset(tx_buffer, 0, sizeof(Ring_Buffer));
    memset(rx_buffer, 0, sizeof(Ring_Buffer));
    
    ret = app_fifo_init(&m_uart_map[serial].tx_fifo, (uint8_t *)tx_buffer->buffer, UART_BUFF_SIZE);
    SPARK_ASSERT(ret == NRF_SUCCESS);
    ret = app_fifo_init(&m_uart_map[serial].rx_fifo, (uint8_t *)rx_buffer->buffer, UART_BUFF_SIZE);
    SPARK_ASSERT(ret == NRF_SUCCESS);

    m_uart_map[serial].enabled = false;
    m_uart_map[serial].rx_ovf = false;
    m_uart_map[serial].uart_config = 0;
}  


void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{   
    HAL_USART_BeginConfig(serial, baud, 0, 0);
}

bool HAL_USART_Validate_Config(uint32_t config)
{
	// Total word length should be 8 bits
	if (!IS_DATA_BITS_VALID(config))
		return false;

	// Either No or Even 
	if (!IS_PARITY_VALID(config))
		return false;

    // Either one or two stop bits
    if (!IS_STOP_BITS_VALID(config))
        return false;

    // Not support half duplex mode
	if (config & SERIAL_HALF_DUPLEX)
		return false;

    // Not support lin mode
	if (config & LIN_MODE)
        return false;

	return true;
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void *ptr)
{
    // Verify UART configuration, exit if it's invalid.
    if (!HAL_USART_Validate_Config(config))
    {
        return;
    }

    if (m_uart_map[serial].enabled)
    {
        uart_uninit(serial);
    }

    m_uart_map[serial].uart_config = config;
    uart_init(serial, baud); 
    m_uart_map[serial].enabled = true;
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    // Wait for transmission of outgoing data
    while (FIFO_LENGTH(&m_uart_map[serial].tx_fifo));
    
    uart_uninit(serial);

    app_fifo_flush(&m_uart_map[serial].tx_fifo);
    app_fifo_flush(&m_uart_map[serial].rx_fifo);

    m_uart_map[serial].enabled = false;
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    if (!m_uart_map[serial].enabled)
    {
        return 0;
    }
    
    // FIXME: lower layer driver send data by using interrupt， couldn't use 
    //        blocking transmission mode
    // interrupt is diable(enter criticle) 
    // while ((__get_PRIMASK() & 1) && FIFO_LENGTH(m_uart_map[serial].tx_fifo)) {}
    if ((__get_PRIMASK() & 1))
    {
        return 0;
    }

    if (IS_FIFO_FULL(&m_uart_map[serial].tx_fifo))
    {
        while (IS_FIFO_FULL(&m_uart_map[serial].tx_fifo));
    }

    uart_put(serial, data);

    return 1;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    return FIFO_LENGTH(&m_uart_map[serial].rx_fifo);
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    uint8_t data = 0;

    if (HAL_USART_Available_Data(serial) == 0)
    {
        return -1;
    }
    else
    {
        uart_get(serial, &data);
        return data;
    }
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{    
    uint8_t data = 0;

    if (HAL_USART_Available_Data(serial) == 0)
    {
        return -1;
    }
    else
    {
        app_fifo_peek(&m_uart_map[serial].rx_fifo, 0, &data);
        return data;
    }
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
    // Loop until UART send buffer is empty
    while (FIFO_LENGTH(&m_uart_map[serial].tx_fifo));

    uart_flush(serial);
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
    return m_uart_map[serial].enabled;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{
    // not support
    return;
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
    return UART_BUFF_SIZE - FIFO_LENGTH(&m_uart_map[serial].tx_fifo);
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data)
{
    return HAL_USART_Write_Data(serial, data);
}

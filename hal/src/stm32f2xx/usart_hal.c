/******************************************************************************
 * @file    usart_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    17-Dec-2014
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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "stm32f2xx.h"
#include <string.h>
#include "interrupts_hal.h"
#include "check.h"
#include "system_error.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum usart_ports_t {
    USART_TX_RX = 0,
    USART_RGBG_RGBB = 1
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    ,USART_TXD_UC_RXD_UC = 2
    ,USART_C3_C2 = 3
    ,USART_C1_C0 = 4
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
} usart_ports_t;

/* Private macro -------------------------------------------------------------*/
#define USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS 0  //Enabling this => 1 is not working at present

typedef struct usart_config_t {
    uint32_t baud_rate;
    uint32_t config;
} usart_config_t;

/* Private variables ---------------------------------------------------------*/
typedef struct stm32_usart_info_t {
    USART_TypeDef* peripheral;

    __IO uint32_t* apb_reg;
    uint32_t clock_enabled;

    int32_t int_n;

    uint16_t tx_pin;
    uint16_t rx_pin;

    uint8_t tx_pinsource;
    uint8_t rx_pinsource;

    uint8_t af_map;

    uint8_t cts_pin;
    uint8_t rts_pin;

    // Buffer pointers. These need to be global for IRQ handler access
    hal_usart_ring_buffer_t* tx_buffer;
    hal_usart_ring_buffer_t* rx_buffer;

    bool configured;
    volatile bool enabled;
    volatile bool suspended;
    bool transmitting;

    usart_config_t conf;
} stm32_usart_info_t;

/*
 * USART mapping
 */
stm32_usart_info_t USART_MAP[TOTAL_USARTS] = {
    /*
     * USART_peripheral (USARTx/UARTx; not using others)
     * clock control register (APBxENR)
     * clock enable bit value (RCC_APBxPeriph_USARTx/RCC_APBxPeriph_UARTx)
     * interrupt number (USARTx_IRQn/UARTx_IRQn)
     * TX pin
     * RX pin
     * TX pin source
     * RX pin source
     * GPIO AF map (GPIO_AF_USARTx/GPIO_AF_UARTx)
     * CTS pin
     * RTS pin
     * <tx_buffer pointer> used internally and does not appear below
     * <rx_buffer pointer> used internally and does not appear below
     * <usart enabled> used internally and does not appear below
     * <usart transmitting> used internally and does not appear below
     */
    { USART1, &RCC->APB2ENR, RCC_APB2Periph_USART1, USART1_IRQn, TX,     RX,     GPIO_PinSource9,  GPIO_PinSource10, GPIO_AF_USART1 }, // USART 1
    { USART2, &RCC->APB1ENR, RCC_APB1Periph_USART2, USART2_IRQn, RGBG,   RGBB,   GPIO_PinSource2,  GPIO_PinSource3,  GPIO_AF_USART2, A7,     RGBR } // USART 2
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
   ,{ USART3, &RCC->APB1ENR, RCC_APB1Periph_USART3, USART3_IRQn, TXD_UC, RXD_UC, GPIO_PinSource10, GPIO_PinSource11, GPIO_AF_USART3, CTS_UC, RTS_UC } // USART 3
   ,{ UART4,  &RCC->APB1ENR, RCC_APB1Periph_UART4,  UART4_IRQn,  C3,     C2,     GPIO_PinSource10, GPIO_PinSource11, GPIO_AF_UART4 } // UART 4
   ,{ UART5,  &RCC->APB1ENR, RCC_APB1Periph_UART5,  UART5_IRQn,  C1,     C0,     GPIO_PinSource12, GPIO_PinSource2,  GPIO_AF_UART5 } // UART 5
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
};

static USART_InitTypeDef usartInitStructure;
static stm32_usart_info_t *usartMap[TOTAL_USARTS]; // pointer to USART_MAP[] containing USART peripheral register locations (etc)

/* Private function prototypes -----------------------------------------------*/
inline void storeChar(uint16_t c, hal_usart_ring_buffer_t *buffer) __attribute__((always_inline));
inline void storeChar(uint16_t c, hal_usart_ring_buffer_t *buffer) {
    unsigned i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

    if (i != buffer->tail) {
        buffer->buffer[buffer->head] = c;
        buffer->head = i;
    }
}

static uint8_t calculateWordLength(uint32_t config, uint8_t noparity);
static uint32_t calculateDataBitsMask(uint32_t config);
static uint8_t validateConfig(uint32_t config);
static void configureTransmitReceive(hal_usart_interface_t serial, uint8_t transmit, uint8_t receive);
static void configurePinsMode(hal_usart_interface_t serial, uint32_t config);
static void usartEndImpl(hal_usart_interface_t serial);

uint8_t calculateWordLength(uint32_t config, uint8_t noparity) {
    // STM32F2 USARTs support only 8-bit or 9-bit communication, however
    // the parity bit is included in the total word length, so for 8E1 mode
    // the total word length would be 9 bits.
    uint8_t wlen = 0;
    switch (config & SERIAL_DATA_BITS) {
        case SERIAL_DATA_BITS_7: {
            wlen += 7;
            break;
        }
        case SERIAL_DATA_BITS_8: {
            wlen += 8;
            break;
        }
        case SERIAL_DATA_BITS_9: {
            wlen += 9;
            break;
        }
    }

    if ((config & SERIAL_PARITY) && !noparity) {
        wlen++;
    }

    if (wlen > 9 || wlen < (noparity ? 7 : 8)) {
        wlen = 0;
    }

    return wlen;
}

uint32_t calculateDataBitsMask(uint32_t config) {
    return (1 << calculateWordLength(config, 1)) - 1;
}

uint8_t validateConfig(uint32_t config) {
    // Total word length should be either 8 or 9 bits
    if (calculateWordLength(config, 0) == 0) {
        return 0;
    }
    // Either No, Even or Odd parity
    if ((config & SERIAL_PARITY) == (SERIAL_PARITY_EVEN | SERIAL_PARITY_ODD)) {
        return 0;
    }
    if ((config & SERIAL_HALF_DUPLEX) && (config & LIN_MODE)) {
        return 0;
    }

    if (config & LIN_MODE) {
        // Either Master or Slave mode
        // Break detection can still be enabled in both Master and Slave modes
        if ((config & LIN_MODE_MASTER) && (config & LIN_MODE_SLAVE)) {
            return 0;
        }
        switch (config & LIN_BREAK_BITS) {
            case LIN_BREAK_13B:
            case LIN_BREAK_10B:
            case LIN_BREAK_11B: {
                break;
            }
            default: {
                return 0;
            }
        }
    }

    return 1;
}

void configureTransmitReceive(hal_usart_interface_t serial, uint8_t transmit, uint8_t receive) {
    uint32_t toset = 0;
    if (transmit) {
        toset |= ((uint32_t)USART_CR1_TE);
    }
    if (receive) {
        toset |= ((uint32_t)USART_CR1_RE);
    }
    uint32_t tmp = usartMap[serial]->peripheral->CR1;
    if ((tmp & ((uint32_t)(USART_CR1_TE | USART_CR1_RE))) != toset) {
        tmp &= ~((uint32_t)(USART_CR1_TE | USART_CR1_RE));
        tmp |= toset;
        usartMap[serial]->peripheral->CR1 = tmp;
    }
}

void configurePinsMode(hal_usart_interface_t serial, uint32_t config) {
    if ((config & SERIAL_HALF_DUPLEX) == 0) {
        // Full duplex with push-pull
        HAL_Pin_Mode(usartMap[serial]->rx_pin, AF_OUTPUT_PUSHPULL);
        HAL_Pin_Mode(usartMap[serial]->tx_pin, AF_OUTPUT_PUSHPULL);
    } else if ((config & SERIAL_OPEN_DRAIN)) {
        // Half-duplex with open drain
        HAL_Pin_Mode(usartMap[serial]->rx_pin, INPUT);
        HAL_Pin_Mode(usartMap[serial]->tx_pin, AF_OUTPUT_DRAIN);
    } else {
        // Half-duplex with push-pull
        /* RM0033 24.3.10:
         * The TX pin is always released when no data is transmitted. Thus, it acts as a standard
         * I/O in idle or in reception. It means that the I/O must be configured so that TX is
         * configured as floating input (or output high open-drain) when not driven by the USART
         */
        HAL_Pin_Mode(usartMap[serial]->rx_pin, INPUT);
        HAL_Pin_Mode(usartMap[serial]->tx_pin, AF_OUTPUT_PUSHPULL);
        if (config & SERIAL_TX_PULL_UP) {
            // We ought to allow enabling pull-up or pull-down through HAL somehow
            Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
            pin_t gpio_pin = PIN_MAP[usartMap[serial]->tx_pin].gpio_pin;
            GPIO_TypeDef *gpio_port = PIN_MAP[usartMap[serial]->tx_pin].gpio_peripheral;
            GPIO_InitTypeDef GPIO_InitStructure = {0};
            GPIO_InitStructure.GPIO_Pin = gpio_pin;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
            GPIO_Init(gpio_port, &GPIO_InitStructure);
        }
    }
}

void usartEndImpl(hal_usart_interface_t serial) {
    // Wait for transmission of outgoing data
    while (usartMap[serial]->tx_buffer->head != usartMap[serial]->tx_buffer->tail);

    // Disable the USART
    USART_Cmd(usartMap[serial]->peripheral, DISABLE);

    // Switch pins to INPUT
    HAL_Pin_Mode(usartMap[serial]->rx_pin, INPUT);
    HAL_Pin_Mode(usartMap[serial]->tx_pin, INPUT_PULLUP);

    // Disable LIN mode
    USART_LINCmd(usartMap[serial]->peripheral, DISABLE);

    // Deinitialise USART
    USART_DeInit(usartMap[serial]->peripheral);

    // Disable USART Receive and Transmit interrupts
    USART_ITConfig(usartMap[serial]->peripheral, USART_IT_RXNE, DISABLE);
    USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, DISABLE);

    NVIC_InitTypeDef NVIC_InitStructure;

    // Disable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->int_n;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

    NVIC_Init(&NVIC_InitStructure);

    // Disable USART Clock
    *usartMap[serial]->apb_reg &= ~usartMap[serial]->clock_enabled;

    // clear any received data
    usartMap[serial]->rx_buffer->head = usartMap[serial]->rx_buffer->tail;
    // Undo any pin re-mapping done for this USART
    // ...
    memset(usartMap[serial]->rx_buffer, 0, sizeof(hal_usart_ring_buffer_t));
    memset(usartMap[serial]->tx_buffer, 0, sizeof(hal_usart_ring_buffer_t));

    usartMap[serial]->enabled = false;
    usartMap[serial]->transmitting = false;
}

void hal_usart_init(hal_usart_interface_t serial, hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer) {
    usartMap[serial]->configured = false;

    if (serial == HAL_USART_SERIAL1) {
        usartMap[serial] = &USART_MAP[USART_TX_RX];
    } else if (serial == HAL_USART_SERIAL2) {
        usartMap[serial] = &USART_MAP[USART_RGBG_RGBB];
    }
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    else if (serial == HAL_USART_SERIAL3) {
        usartMap[serial] = &USART_MAP[USART_TXD_UC_RXD_UC];
    } else if (serial == HAL_USART_SERIAL4) {
        usartMap[serial] = &USART_MAP[USART_C3_C2];
    } else if (serial == HAL_USART_SERIAL5) {
        usartMap[serial] = &USART_MAP[USART_C1_C0];
    }
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

    usartMap[serial]->rx_buffer = rx_buffer;
    usartMap[serial]->tx_buffer = tx_buffer;

    memset(usartMap[serial]->rx_buffer, 0, sizeof(hal_usart_ring_buffer_t));
    memset(usartMap[serial]->tx_buffer, 0, sizeof(hal_usart_ring_buffer_t));

    usartMap[serial]->enabled = false;
    usartMap[serial]->transmitting = false;

    usartMap[serial]->configured = true;
}

void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud) {
    hal_usart_begin_config(serial, baud, 0, 0); // Default serial configuration is 8N1
}

void hal_usart_begin_config(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void *ptr) {
    if (!usartMap[serial]->configured) {
        return;
    }
    // Verify UART configuration, exit if it's invalid.
    if (!validateConfig(config)) {
        usartMap[serial]->enabled = false;
        return;
    }

    usartMap[serial]->suspended = false;

    USART_DeInit(usartMap[serial]->peripheral);

    usartMap[serial]->enabled = false;

    configurePinsMode(serial, config);

    // Enable USART Clock
    *usartMap[serial]->apb_reg |=  usartMap[serial]->clock_enabled;

    // Connect USART pins to AFx
    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
    GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->rx_pin].gpio_peripheral, usartMap[serial]->rx_pinsource, usartMap[serial]->af_map);
    GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->tx_pin].gpio_peripheral, usartMap[serial]->tx_pinsource, usartMap[serial]->af_map);

    // NVIC Configuration
    NVIC_InitTypeDef NVIC_InitStructure;
    // Enable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->int_n;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // USART default configuration
    // USART configured as follow:
    // - BaudRate = (set baudRate as 9600 baud)
    // - Word Length = 8 Bits
    // - One Stop Bit
    // - No parity
    // - Hardware flow control disabled for Serial1/2/4/5
    // - Hardware flow control enabled (RTS and CTS signals) for Serial3
    // - Receive and transmit enabled
    // USART configuration
    usartInitStructure.USART_BaudRate = baud;
    usartInitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    // Flow control configuration
    switch (config & SERIAL_FLOW_CONTROL) {
        case SERIAL_FLOW_CONTROL_CTS: {
            usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
            break;
        }
        case SERIAL_FLOW_CONTROL_RTS: {
            usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
            break;
        }
        case SERIAL_FLOW_CONTROL_RTS_CTS: {
            usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
            break;
        }
        case SERIAL_FLOW_CONTROL_NONE:
        default: {
            usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
            break;
        }
    }

    if (serial == HAL_USART_SERIAL1) {
        // USART1 supports hardware flow control, but RTS and CTS pins are not exposed
        usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    }

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    if (serial == HAL_USART_SERIAL4 || serial == HAL_USART_SERIAL5) {
        // UART4 and UART5 do not support hardware flow control
        usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    }
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS    // Electron
    if (serial == HAL_USART_SERIAL3) {
        usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    }
#endif

    switch (usartInitStructure.USART_HardwareFlowControl) {
        case USART_HardwareFlowControl_CTS: {
            HAL_Pin_Mode(usartMap[serial]->cts_pin, AF_OUTPUT_PUSHPULL);
            GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->cts_pin].gpio_peripheral, PIN_MAP[usartMap[serial]->cts_pin].gpio_pin_source, usartMap[serial]->af_map);
            break;
        }
        case USART_HardwareFlowControl_RTS: {
            HAL_Pin_Mode(usartMap[serial]->rts_pin, AF_OUTPUT_PUSHPULL);
            GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->rts_pin].gpio_peripheral, PIN_MAP[usartMap[serial]->rts_pin].gpio_pin_source, usartMap[serial]->af_map);
            break;
        }
        case USART_HardwareFlowControl_RTS_CTS: {
            HAL_Pin_Mode(usartMap[serial]->cts_pin, AF_OUTPUT_PUSHPULL);
            HAL_Pin_Mode(usartMap[serial]->rts_pin, AF_OUTPUT_PUSHPULL);
            GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->cts_pin].gpio_peripheral, PIN_MAP[usartMap[serial]->cts_pin].gpio_pin_source, usartMap[serial]->af_map);
            GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->rts_pin].gpio_peripheral, PIN_MAP[usartMap[serial]->rts_pin].gpio_pin_source, usartMap[serial]->af_map);
            break;
        }
        case USART_HardwareFlowControl_None:
        default: {
            break;
        }
    }

    // Stop bit configuration.
    switch (config & SERIAL_STOP_BITS) {
        case SERIAL_STOP_BITS_1: { // 1 stop bit
            usartInitStructure.USART_StopBits = USART_StopBits_1;
            break;
        }
        case SERIAL_STOP_BITS_2: { // 2 stop bits
            usartInitStructure.USART_StopBits = USART_StopBits_2;
            break;
        }
        case SERIAL_STOP_BITS_0_5: { // 0.5 stop bits
            usartInitStructure.USART_StopBits = USART_StopBits_0_5;
            break;
        }
        case SERIAL_STOP_BITS_1_5: { // 1.5 stop bits
            usartInitStructure.USART_StopBits = USART_StopBits_1_5;
            break;
        }
        default: {
            break;
        }
    }

    // Data bits configuration
    switch (calculateWordLength(config, 0)) {
        case 8: {
            usartInitStructure.USART_WordLength = USART_WordLength_8b;
            break;
        }
        case 9: {
            usartInitStructure.USART_WordLength = USART_WordLength_9b;
            break;
        }
        default: {
            break;
        }
    }

    // Parity configuration
    switch (config & SERIAL_PARITY) {
        case SERIAL_PARITY_NO: {
            usartInitStructure.USART_Parity = USART_Parity_No;
            break;
        }
        case SERIAL_PARITY_EVEN: {
            usartInitStructure.USART_Parity = USART_Parity_Even;
            break;
        }
        case SERIAL_PARITY_ODD: {
            usartInitStructure.USART_Parity = USART_Parity_Odd;
            break;
        }
        default: {
            break;
        }
    }

    // Disable LIN mode just in case
    USART_LINCmd(usartMap[serial]->peripheral, DISABLE);

    // Configure USART
    USART_Init(usartMap[serial]->peripheral, &usartInitStructure);

    // LIN configuration
    if (config & LIN_MODE) {
        // Enable break detection
        switch (config & LIN_BREAK_BITS) {
            case LIN_BREAK_10B: {
                USART_LINBreakDetectLengthConfig(usartMap[serial]->peripheral, USART_LINBreakDetectLength_10b);
                break;
            }
            case LIN_BREAK_11B: {
                USART_LINBreakDetectLengthConfig(usartMap[serial]->peripheral, USART_LINBreakDetectLength_11b);
                break;
            }
            default: {
                break;
            }
        }
    }

    usartMap[serial]->conf.baud_rate = baud;
    usartMap[serial]->conf.config = config;
    if (config & SERIAL_HALF_DUPLEX) {
        hal_usart_half_duplex(serial, ENABLE);
    }

    USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TC, DISABLE);

    // Enable the USART
    USART_Cmd(usartMap[serial]->peripheral, ENABLE);

    if (config & LIN_MODE) {
        USART_LINCmd(usartMap[serial]->peripheral, ENABLE);
    }

    usartMap[serial]->enabled = true;
    usartMap[serial]->transmitting = false;

    // Enable USART Receive and Transmit interrupts
    USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, ENABLE);
    USART_ITConfig(usartMap[serial]->peripheral, USART_IT_RXNE, ENABLE);
}

void hal_usart_end(hal_usart_interface_t serial) {
    usartMap[serial]->suspended = false;
    usartEndImpl(serial);
}

uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data) {
    return hal_usart_write_nine_bits(serial, data);
}

uint32_t hal_usart_write_nine_bits(hal_usart_interface_t serial, uint16_t data) {
    // Remove any bits exceeding data bits configured
    data &= calculateDataBitsMask(usartMap[serial]->conf.config);
    // interrupts are off and data in queue;
    if ((USART_GetITStatus(usartMap[serial]->peripheral, USART_IT_TXE) == RESET)
        && usartMap[serial]->tx_buffer->head != usartMap[serial]->tx_buffer->tail) {
        // Get him busy
        USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, ENABLE);
    }

    unsigned i = (usartMap[serial]->tx_buffer->head + 1) % SERIAL_BUFFER_SIZE;

    // If the output buffer is full, there's nothing for it other than to
    // wait for the interrupt handler to empty it a bit
    // no space so or Called Off Panic with interrupt off get the message out!
    // make space Enter Polled IO mode
    while (i == usartMap[serial]->tx_buffer->tail || ((__get_PRIMASK() & 1) && usartMap[serial]->tx_buffer->head != usartMap[serial]->tx_buffer->tail) ) {
        // Interrupts are on but they are not being serviced because this was called from a higher
        // Priority interrupt
        if (USART_GetITStatus(usartMap[serial]->peripheral, USART_IT_TXE) && USART_GetFlagStatus(usartMap[serial]->peripheral, USART_FLAG_TXE)) {
            // protect for good measure
            USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, DISABLE);
            // Write out a byte
            USART_SendData(usartMap[serial]->peripheral,  usartMap[serial]->tx_buffer->buffer[usartMap[serial]->tx_buffer->tail++]);
            usartMap[serial]->tx_buffer->tail %= SERIAL_BUFFER_SIZE;
            // unprotect
            USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, ENABLE);
        }
    }

    usartMap[serial]->tx_buffer->buffer[usartMap[serial]->tx_buffer->head] = data;
    usartMap[serial]->tx_buffer->head = i;
    usartMap[serial]->transmitting = true;
    USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, ENABLE);

    return 1;
}

int32_t hal_usart_available(hal_usart_interface_t serial) {
    return (unsigned int)(SERIAL_BUFFER_SIZE + usartMap[serial]->rx_buffer->head - usartMap[serial]->rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial) {
    int32_t tail = usartMap[serial]->tx_buffer->tail;
    int32_t available = SERIAL_BUFFER_SIZE - (usartMap[serial]->tx_buffer->head >= tail ?
                                              usartMap[serial]->tx_buffer->head - tail :
                                              (SERIAL_BUFFER_SIZE + usartMap[serial]->tx_buffer->head - tail) - 1);

    return available;
}


int32_t hal_usart_read(hal_usart_interface_t serial) {
    // if the head isn't ahead of the tail, we don't have any characters
    if (usartMap[serial]->rx_buffer->head == usartMap[serial]->rx_buffer->tail) {
        return -1;
    } else {
        uint16_t c = usartMap[serial]->rx_buffer->buffer[usartMap[serial]->rx_buffer->tail];
        usartMap[serial]->rx_buffer->tail = (unsigned int)(usartMap[serial]->rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
        return c;
    }
}

int32_t hal_usart_peek(hal_usart_interface_t serial) {
    if (usartMap[serial]->rx_buffer->head == usartMap[serial]->rx_buffer->tail) {
        return -1;
    } else {
        return usartMap[serial]->rx_buffer->buffer[usartMap[serial]->rx_buffer->tail];
    }
}

void hal_usart_flush(hal_usart_interface_t serial) {
    // Loop until USART DR register is empty
    while ( usartMap[serial]->tx_buffer->head != usartMap[serial]->tx_buffer->tail );
    // Loop until last frame transmission complete
    while (usartMap[serial]->transmitting && (USART_GetFlagStatus(usartMap[serial]->peripheral, USART_FLAG_TC) == RESET));
    usartMap[serial]->transmitting = false;
}

bool hal_usart_is_enabled(hal_usart_interface_t serial) {
    return usartMap[serial]->enabled;
}

void hal_usart_half_duplex(hal_usart_interface_t serial, bool enable) {
    if (hal_usart_is_enabled(serial)) {
        USART_Cmd(usartMap[serial]->peripheral, DISABLE);
    }

    if (enable) {
        usartMap[serial]->conf.config |= SERIAL_HALF_DUPLEX;
    } else {
        usartMap[serial]->conf.config &= ~(SERIAL_HALF_DUPLEX);
    }
    configurePinsMode(serial, usartMap[serial]->conf.config);

    // These bits need to be cleared according to the reference manual
    usartMap[serial]->peripheral->CR2 &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);
    usartMap[serial]->peripheral->CR3 &= ~(USART_CR3_IREN | USART_CR3_SCEN);
    USART_HalfDuplexCmd(usartMap[serial]->peripheral, enable ? ENABLE : DISABLE);


    if (hal_usart_is_enabled(serial)) {
        USART_Cmd(usartMap[serial]->peripheral, ENABLE);
    }
}

void hal_usart_send_break(hal_usart_interface_t serial, void* reserved) {
    int32_t state = HAL_disable_irq();
    while((usartMap[serial]->peripheral->CR1 & USART_CR1_SBK) == SET);
    USART_SendBreak(usartMap[serial]->peripheral);
    while((usartMap[serial]->peripheral->CR1 & USART_CR1_SBK) == SET);
    HAL_enable_irq(state);
}

uint8_t hal_usart_break_detected(hal_usart_interface_t serial) {
    if (USART_GetFlagStatus(usartMap[serial]->peripheral, USART_FLAG_LBD)) {
        // Clear LBD flag
        USART_ClearFlag(usartMap[serial]->peripheral, USART_FLAG_LBD);
        return 1;
    }
    return 0;
}

// Shared Interrupt Handler for USART2/Serial1 and USART1/Serial2
// WARNING: This function MUST remain reentrance compliant -- no local static variables etc.
static void usartIntHandler(hal_usart_interface_t serial) {
    if (USART_GetITStatus(usartMap[serial]->peripheral, USART_IT_RXNE) != RESET) {
        // Read byte from the receive data register
        uint16_t c = USART_ReceiveData(usartMap[serial]->peripheral);
        // Remove parity bits from data
        c &= calculateDataBitsMask(usartMap[serial]->conf.config);
        storeChar(c, usartMap[serial]->rx_buffer);
    }

    uint8_t noecho = (usartMap[serial]->conf.config & (SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO)) == (SERIAL_HALF_DUPLEX | SERIAL_HALF_DUPLEX_NO_ECHO);

    if (USART_GetITStatus(usartMap[serial]->peripheral, USART_IT_TC) != RESET) {
        if (noecho) {
            USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TC, DISABLE);
            configureTransmitReceive(serial, 0, 1);
        }
    }

    if (USART_GetITStatus(usartMap[serial]->peripheral, USART_IT_TXE) != RESET) {
        // Write byte to the transmit data register
        if (usartMap[serial]->tx_buffer->head == usartMap[serial]->tx_buffer->tail) {
            // Buffer empty, so disable the USART Transmit interrupt
            USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TXE, DISABLE);
            if (noecho) {
                USART_ITConfig(usartMap[serial]->peripheral, USART_IT_TC, ENABLE);
            }
        } else {
            if (noecho) {
                configureTransmitReceive(serial, 1, 0);
            }
            // There is more data in the output buffer. Send the next byte
            USART_SendData(usartMap[serial]->peripheral, usartMap[serial]->tx_buffer->buffer[usartMap[serial]->tx_buffer->tail++]);
            usartMap[serial]->tx_buffer->tail %= SERIAL_BUFFER_SIZE;
        }
    }

    if (USART_GetFlagStatus(usartMap[serial]->peripheral, USART_FLAG_ORE) != RESET) {
        // If Overrun flag is still set, clear it
        (void)USART_ReceiveData(usartMap[serial]->peripheral);
    }
}

int hal_usart_sleep(hal_usart_interface_t serial, bool sleep, void* reserved) {
    if (sleep) {
        CHECK_TRUE(usartMap[serial]->enabled, SYSTEM_ERROR_NONE);
        CHECK_FALSE(usartMap[serial]->suspended, SYSTEM_ERROR_NONE);
        usartMap[serial]->suspended = true;
        hal_usart_flush(serial);
        usartEndImpl(serial);
    } else {
        CHECK_TRUE(usartMap[serial]->configured, SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(usartMap[serial]->suspended, SYSTEM_ERROR_NONE);
        hal_usart_begin_config(serial, usartMap[serial]->conf.baud_rate, usartMap[serial]->conf.config, NULL);
    }
    return SYSTEM_ERROR_NONE;
}

// Serial1 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART1_Handler
 * Description    : This function handles USART1 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART1_Handler(void) {
    usartIntHandler(HAL_USART_SERIAL1);
}

// Serial2 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART2_Handler
 * Description    : This function handles USART2 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART2_Handler(void) {
    usartIntHandler(HAL_USART_SERIAL2);
}

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#if 0
// Serial3 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART3_Handler
 * Description    : This function handles USART3 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART3_Handler(void) {
    usartIntHandler(HAL_USART_SERIAL3);
}
#endif

// Serial4 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART4_Handler
 * Description    : This function handles UART4 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART4_Handler(void) {
    usartIntHandler(HAL_USART_SERIAL4);
}

// Serial5 interrupt handler
/*******************************************************************************
 * Function Name  : HAL_USART5_Handler
 * Description    : This function handles UART5 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART5_Handler(void) {
    usartIntHandler(HAL_USART_SERIAL5);
}

#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

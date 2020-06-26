/******************************************************************************
 * @file    spi_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    24-Dec-2014
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
#include <stddef.h>
#include "spi_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"
#include "interrupts_hal.h"
#include "debug.h"
#include "check.h"

/* Private define ------------------------------------------------------------*/
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define TOTAL_SPI   3
#else
#define TOTAL_SPI   2
#endif

/* Private variables ---------------------------------------------------------*/
typedef struct stm32_spi_info_t {
    SPI_TypeDef* peripheral;

    __IO uint32_t* rcc_apb_register;
    uint32_t rcc_apb_clock_enable;

    __IO uint32_t* rcc_ahb_register;
    uint32_t rcc_ahb_clock_enable;

    uint32_t dma_channel;

    DMA_Stream_TypeDef* tx_dma_stream;
    DMA_Stream_TypeDef* rx_dma_stream;

    uint8_t tx_dma_stream_irqn;
    uint8_t rx_dma_stream_irqn;
    uint32_t tx_dma_stream_tc_event;
    uint32_t rx_dma_stream_tc_event;

    uint8_t sck_pin;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t ss_pin;

    uint8_t af_mapping;
} stm32_spi_info_t;

typedef struct spi_state_t {
    SPI_InitTypeDef init_structure;

    bool bit_order_set;
    bool data_mode_set;
    bool clock_divider_set;
    volatile bool enabled;
    volatile bool suspended;

    hal_spi_dma_user_callback dma_user_callback;

    uint16_t ss_pin;
    hal_spi_mode_t mode;
    hal_spi_select_user_callback select_user_callback;
    uint32_t dma_last_transfer_length;
    uint32_t dma_current_transfer_length;
    volatile uint8_t ss_state;
    uint8_t dma_configured;
    volatile uint8_t dma_aborted;

    __attribute__((aligned(4))) uint8_t temp_memory_rx;
    __attribute__((aligned(4))) uint8_t temp_memory_tx;
} spi_state_t;

static void spiOnSelectedHandler(void *data);

/*
 * SPI mapping
 */
const stm32_spi_info_t spiMap[TOTAL_SPI] = {
    { SPI1, &RCC->APB2ENR, RCC_APB2Periph_SPI1, &RCC->AHB1ENR, RCC_AHB1Periph_DMA2, DMA_Channel_3,
      DMA2_Stream5, DMA2_Stream2, DMA2_Stream5_IRQn, DMA2_Stream2_IRQn, DMA_IT_TCIF5, DMA_IT_TCIF2, SCK, MISO, MOSI, SS, GPIO_AF_SPI1 },
    { SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0,
      DMA1_Stream7, DMA1_Stream2, DMA1_Stream7_IRQn, DMA1_Stream2_IRQn, DMA_IT_TCIF7, DMA_IT_TCIF2, D4, D3, D2, D5, GPIO_AF_SPI3  }
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
   ,{ SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0,
      DMA1_Stream7, DMA1_Stream2, DMA1_Stream7_IRQn, DMA1_Stream2_IRQn, DMA_IT_TCIF7, DMA_IT_TCIF2, C3, C2, C1, C0, GPIO_AF_SPI3  }
#endif
};

/* Private typedef -----------------------------------------------------------*/
typedef enum spi_ports_t {
    SPI1_A3_A4_A5 = 0,
    SPI3_D4_D3_D2 = 1
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
   ,SPI3_C3_C2_C1 = 2
#endif
} spi_ports_t;

spi_state_t spiState[TOTAL_SPI];


static void spiDmaConfig(hal_spi_interface_t spi, void* tx_buffer, void* rx_buffer, uint32_t length) {
    DMA_InitTypeDef dmaInitStructure;
    NVIC_InitTypeDef nvicInitStructure;

    /* Deinitialize DMA Streams */
    DMA_DeInit(spiMap[spi].tx_dma_stream);
    DMA_DeInit(spiMap[spi].rx_dma_stream);

    dmaInitStructure.DMA_BufferSize = length;
    dmaInitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dmaInitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    dmaInitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dmaInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dmaInitStructure.DMA_Mode = DMA_Mode_Normal;

    dmaInitStructure.DMA_PeripheralBaseAddr = (uint32_t)&spiMap[spi].peripheral->DR;
    dmaInitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    dmaInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dmaInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmaInitStructure.DMA_Priority = DMA_Priority_High;

    dmaInitStructure.DMA_Channel = spiMap[spi].dma_channel;

    spiState[spi].temp_memory_tx = 0xFF;
    spiState[spi].temp_memory_rx = 0xFF;

    /* Configure Tx DMA Stream */
    dmaInitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    if (tx_buffer) {
        dmaInitStructure.DMA_Memory0BaseAddr = (uint32_t)tx_buffer;
        dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    } else {
        dmaInitStructure.DMA_Memory0BaseAddr = (uint32_t)(&spiState[spi].temp_memory_tx);
        dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    DMA_Init(spiMap[spi].tx_dma_stream, &dmaInitStructure);

    /* Configure Rx DMA Stream */
    dmaInitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    if (rx_buffer) {
        dmaInitStructure.DMA_Memory0BaseAddr = (uint32_t)rx_buffer;
        dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    } else {
        dmaInitStructure.DMA_Memory0BaseAddr = (uint32_t)&spiState[spi].temp_memory_rx;
        dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    DMA_Init(spiMap[spi].rx_dma_stream, &dmaInitStructure);

    /* Enable SPI TX DMA Stream Interrupt */
    DMA_ITConfig(spiMap[spi].tx_dma_stream, DMA_IT_TC, ENABLE);
    /* Enable SPI RX DMA Stream Interrupt */
    DMA_ITConfig(spiMap[spi].rx_dma_stream, DMA_IT_TC, ENABLE);

    nvicInitStructure.NVIC_IRQChannel = spiMap[spi].tx_dma_stream_irqn;
    nvicInitStructure.NVIC_IRQChannelPreemptionPriority = spiState[spi].mode == SPI_MODE_MASTER ? 12 : 1;
    nvicInitStructure.NVIC_IRQChannelSubPriority = 0;
    nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicInitStructure);

    nvicInitStructure.NVIC_IRQChannel = spiMap[spi].rx_dma_stream_irqn;
    nvicInitStructure.NVIC_IRQChannelPreemptionPriority = spiState[spi].mode == SPI_MODE_MASTER ? 12 : 1;
    nvicInitStructure.NVIC_IRQChannelSubPriority = 0;
    nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicInitStructure);

    /* Enable the DMA Tx/Rx Stream */
    DMA_Cmd(spiMap[spi].tx_dma_stream, ENABLE);
    DMA_Cmd(spiMap[spi].rx_dma_stream, ENABLE);

    /* Enable the SPI Rx/Tx DMA request */
    SPI_I2S_DMACmd(spiMap[spi].peripheral, SPI_I2S_DMAReq_Rx, ENABLE);
    SPI_I2S_DMACmd(spiMap[spi].peripheral, SPI_I2S_DMAReq_Tx, ENABLE);

    spiState[spi].dma_current_transfer_length = length;
    spiState[spi].dma_last_transfer_length = 0;
    spiState[spi].dma_aborted = 0;
    spiState[spi].dma_configured = 1;
}

static void spiTxDmaStreamInterruptHandler(hal_spi_interface_t spi) {
    if (DMA_GetITStatus(spiMap[spi].tx_dma_stream, spiMap[spi].tx_dma_stream_tc_event) == SET) {
        DMA_ClearITPendingBit(spiMap[spi].tx_dma_stream, spiMap[spi].tx_dma_stream_tc_event);
        SPI_I2S_DMACmd(spiMap[spi].peripheral, SPI_I2S_DMAReq_Tx, DISABLE);
        DMA_Cmd(spiMap[spi].tx_dma_stream, DISABLE);
    }
}

static void spiRxDmaStreamInterruptHandler(hal_spi_interface_t spi) {
    if (DMA_GetITStatus(spiMap[spi].rx_dma_stream, spiMap[spi].rx_dma_stream_tc_event) == SET) {
        uint32_t remainingCount = DMA_GetCurrDataCounter(spiMap[spi].rx_dma_stream);

        if (!spiState[spi].dma_aborted) {
            while (SPI_I2S_GetFlagStatus(spiMap[spi].peripheral, SPI_I2S_FLAG_TXE) == RESET);
            while (SPI_I2S_GetFlagStatus(spiMap[spi].peripheral, SPI_I2S_FLAG_BSY) == SET);
        }

        if (spiState[spi].mode == SPI_MODE_SLAVE) {
            SPI_Cmd(spiMap[spi].peripheral, DISABLE);

            if (spiState[spi].dma_aborted) {
                /* Reset SPI peripheral completely, othewise we might end up with a case where
                 * SPI peripheral has already loaded some data from aborted transfer into its circular register,
                 * but haven't transferred it out yet. The next transfer then will start with this remainder data.
                 * The only way to clear it out is by completely disabling SPI peripheral (stopping its clock).
                 */
                spiState[spi].dma_aborted = 0;
                SPI_DeInit(spiMap[spi].peripheral);
                hal_spi_begin_ext(spi, SPI_MODE_SLAVE, spiState[spi].ss_pin, NULL);
            }
        }
        DMA_ClearITPendingBit(spiMap[spi].rx_dma_stream, spiMap[spi].rx_dma_stream_tc_event);
        SPI_I2S_DMACmd(spiMap[spi].peripheral, SPI_I2S_DMAReq_Rx, DISABLE);
        DMA_Cmd(spiMap[spi].rx_dma_stream, DISABLE);

        spiState[spi].dma_last_transfer_length = spiState[spi].dma_current_transfer_length - remainingCount;
        spiState[spi].dma_configured = 0;

        hal_spi_dma_user_callback callback = spiState[spi].dma_user_callback;
        if (callback) {
            // notify user program about transfer completion
            callback();
        }
    }
}

void hal_spi_init(hal_spi_interface_t spi) {
    spiState[spi].bit_order_set = false;
    spiState[spi].data_mode_set = false;
    spiState[spi].clock_divider_set = false;
    spiState[spi].enabled = false;
    spiState[spi].suspended = false;

    spiState[spi].ss_pin = spiMap[spi].ss_pin;
    spiState[spi].mode = SPI_MODE_MASTER;
    spiState[spi].ss_state = 0;
    spiState[spi].dma_current_transfer_length = 0;
    spiState[spi].dma_last_transfer_length = 0;
    spiState[spi].dma_configured = 0;
    spiState[spi].dma_aborted = 0;
}

void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin) {
    // Default to Master mode
    hal_spi_begin_ext(spi, SPI_MODE_MASTER, pin, NULL);
}

void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, void* reserved) {
    if (pin == SPI_DEFAULT_SS) {
        pin = spiMap[spi].ss_pin;
    }

    if (mode == SPI_MODE_SLAVE && !HAL_Pin_Is_Valid(pin)) {
        return;
    }

    spiState[spi].suspended = false;

    spiState[spi].ss_pin = pin;
    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();

    spiState[spi].mode = mode;

    /* Enable SPI Clock */
    *spiMap[spi].rcc_apb_register |= spiMap[spi].rcc_apb_clock_enable;
    *spiMap[spi].rcc_ahb_register |= spiMap[spi].rcc_ahb_clock_enable;

    /* Connect SPI pins to AF */
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi].sck_pin].gpio_peripheral, PIN_MAP[spiMap[spi].sck_pin].gpio_pin_source, spiMap[spi].af_mapping);
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi].miso_pin].gpio_peripheral, PIN_MAP[spiMap[spi].miso_pin].gpio_pin_source, spiMap[spi].af_mapping);
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi].mosi_pin].gpio_peripheral, PIN_MAP[spiMap[spi].mosi_pin].gpio_pin_source, spiMap[spi].af_mapping);

    HAL_Pin_Mode(spiMap[spi].sck_pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(spiMap[spi].miso_pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(spiMap[spi].mosi_pin, AF_OUTPUT_PUSHPULL);

    if (mode == SPI_MODE_MASTER && HAL_Pin_Is_Valid(pin)) {
        // Ensure that there is no glitch on SS pin
        PIN_MAP[pin].gpio_peripheral->BSRRL = PIN_MAP[pin].gpio_pin;
        HAL_Pin_Mode(pin, OUTPUT);
    } else if (mode == SPI_MODE_SLAVE) {
        HAL_Pin_Mode(pin, INPUT);
    }

    /* SPI configuration */
    spiState[spi].init_structure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiState[spi].init_structure.SPI_Mode = SPI_Mode_Master;
    spiState[spi].init_structure.SPI_DataSize = SPI_DataSize_8b;
    if(spiState[spi].data_mode_set != true) {
        //Default: SPI_MODE3
        spiState[spi].init_structure.SPI_CPOL = SPI_CPOL_High;
        spiState[spi].init_structure.SPI_CPHA = SPI_CPHA_2Edge;
    }
    spiState[spi].init_structure.SPI_NSS = SPI_NSS_Soft;

    if(spiState[spi].clock_divider_set != true) {
        /* Defaults to 15Mbit/s on SPI1, SPI2 and SPI3 */
        if(spi == HAL_SPI_INTERFACE1) {
            spiState[spi].init_structure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;//60/4=15
        } else {
            spiState[spi].init_structure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;//30/2=15
        }
    }
    if(spiState[spi].bit_order_set != true) {
        //Default: MSBFIRST
        spiState[spi].init_structure.SPI_FirstBit = SPI_FirstBit_MSB;
    }
    spiState[spi].init_structure.SPI_CRCPolynomial = 7;

    SPI_Init(spiMap[spi].peripheral, &spiState[spi].init_structure);

    if (mode == SPI_MODE_SLAVE) {
        /* Attach interrupt to slave select pin */
        HAL_InterruptExtraConfiguration irqConf = {0};
        irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
        irqConf.IRQChannelPreemptionPriority = 1;
        irqConf.IRQChannelSubPriority = 0;

        HAL_Interrupts_Attach(pin, &spiOnSelectedHandler, (void*)(spi), CHANGE, &irqConf);

        /* Switch to slave mode */
        spiState[spi].init_structure.SPI_Mode = SPI_Mode_Slave;
        SPI_Init(spiMap[spi].peripheral, &spiState[spi].init_structure);
    }

    if (mode == SPI_MODE_MASTER) {
        /* SPI peripheral should not be enabled in Slave mode until the device is selected */
        SPI_Cmd(spiMap[spi].peripheral, ENABLE);
    }

    spiState[spi].enabled = true;
}

void hal_spi_end(hal_spi_interface_t spi) {
    spiState[spi].suspended = false;
    if(spiState[spi].enabled != false) {
        if (spiState[spi].mode == SPI_MODE_SLAVE) {
            HAL_Interrupts_Detach(spiState[spi].ss_pin);
        }
        SPI_Cmd(spiMap[spi].peripheral, DISABLE);
        hal_spi_transfer_dma_cancel(spi);
        SPI_DeInit(spiMap[spi].peripheral);
        spiState[spi].enabled = false;
    }
}

static inline void spiSetBitOrderImpl(hal_spi_interface_t spi, uint8_t order) {
    if(order == LSBFIRST) {
        spiState[spi].init_structure.SPI_FirstBit = SPI_FirstBit_LSB;
    } else {
        spiState[spi].init_structure.SPI_FirstBit = SPI_FirstBit_MSB;
    }
}

void hal_spi_set_bit_order(hal_spi_interface_t spi, uint8_t order) {
    spiSetBitOrderImpl(spi, order);

    if(spiState[spi].enabled != false) {
        SPI_Cmd(spiMap[spi].peripheral, DISABLE);
        SPI_Init(spiMap[spi].peripheral, &spiState[spi].init_structure);
        SPI_Cmd(spiMap[spi].peripheral, ENABLE);
    }

    spiState[spi].bit_order_set = true;
}

static inline void spiSetDataModeImpl(hal_spi_interface_t spi, uint8_t mode) {
    switch(mode) {
        case SPI_MODE0: {
            spiState[spi].init_structure.SPI_CPOL = SPI_CPOL_Low;
            spiState[spi].init_structure.SPI_CPHA = SPI_CPHA_1Edge;
            break;
        }
        case SPI_MODE1: {
            spiState[spi].init_structure.SPI_CPOL = SPI_CPOL_Low;
            spiState[spi].init_structure.SPI_CPHA = SPI_CPHA_2Edge;
            break;
        }
        case SPI_MODE2: {
            spiState[spi].init_structure.SPI_CPOL = SPI_CPOL_High;
            spiState[spi].init_structure.SPI_CPHA = SPI_CPHA_1Edge;
            break;
        }
        case SPI_MODE3: {
            spiState[spi].init_structure.SPI_CPOL = SPI_CPOL_High;
            spiState[spi].init_structure.SPI_CPHA = SPI_CPHA_2Edge;
            break;
        }
        default: {
            break;
        }
    }
}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode) {
    spiSetDataModeImpl(spi, mode);

    if(spiState[spi].enabled != false) {
        SPI_Cmd(spiMap[spi].peripheral, DISABLE);
        SPI_Init(spiMap[spi].peripheral, &spiState[spi].init_structure);
        SPI_Cmd(spiMap[spi].peripheral, ENABLE);
    }

    spiState[spi].data_mode_set = true;
}

static inline void spiSetClockDividerImpl(hal_spi_interface_t spi, uint8_t rate) {
    spiState[spi].init_structure.SPI_BaudRatePrescaler = rate;
}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate) {
    spiSetClockDividerImpl(spi, rate);

    SPI_Init(spiMap[spi].peripheral, &spiState[spi].init_structure);

    spiState[spi].clock_divider_set = true;
}

int32_t hal_spi_set_settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved) {
    if (!set_default) {
        spiSetClockDividerImpl(spi, clockdiv);
        spiSetBitOrderImpl(spi, order);
        spiSetDataModeImpl(spi, mode);
    }

    spiState[spi].clock_divider_set = !set_default;
    spiState[spi].data_mode_set = !set_default;
    spiState[spi].bit_order_set = !set_default;

    if (set_default) {
        // hal_spi_end(spi);
        hal_spi_begin_ext(spi, spiState[spi].mode, spiState[spi].ss_pin, NULL);
    } else if (spiState[spi].enabled != false) {
        SPI_Cmd(spiMap[spi].peripheral, DISABLE);
        SPI_Init(spiMap[spi].peripheral, &spiState[spi].init_structure);
        SPI_Cmd(spiMap[spi].peripheral, ENABLE);
    }

    return 0;
}

uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data) {
    if (spiState[spi].mode == SPI_MODE_SLAVE) {
        return 0;
    }
    /* Wait for SPI Tx buffer empty */
    while (SPI_I2S_GetFlagStatus(spiMap[spi].peripheral, SPI_I2S_FLAG_TXE) == RESET);
    /* Send SPI1 data */
    SPI_I2S_SendData(spiMap[spi].peripheral, data);

    /* Wait for SPI data reception */
    while (SPI_I2S_GetFlagStatus(spiMap[spi].peripheral, SPI_I2S_FLAG_RXNE) == RESET);
    /* Read and return SPI received data */
    return SPI_I2S_ReceiveData(spiMap[spi].peripheral);
}

void hal_spi_transfer_dma(hal_spi_interface_t spi, void* tx_buffer, void* rx_buffer, uint32_t length, hal_spi_dma_user_callback userCallback) {
    int32_t state = HAL_disable_irq();
    spiState[spi].dma_user_callback = userCallback;
    /* Config and initiate DMA transfer */
    spiDmaConfig(spi, tx_buffer, rx_buffer, length);
    if (spiState[spi].mode == SPI_MODE_SLAVE && spiState[spi].ss_state == 1) {
        SPI_Cmd(spiMap[spi].peripheral, ENABLE);
    }
    HAL_enable_irq(state);
}

void hal_spi_transfer_dma_cancel(hal_spi_interface_t spi) {
    if (spiState[spi].dma_configured) {
        spiState[spi].dma_aborted = 1;
        DMA_Cmd(spiMap[spi].tx_dma_stream, DISABLE);
        DMA_Cmd(spiMap[spi].rx_dma_stream, DISABLE);
    }
}

int32_t hal_spi_transfer_dma_status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st) {
    int32_t transferLength = 0;
    if (spiState[spi].dma_configured == 0) {
        transferLength = spiState[spi].dma_last_transfer_length;
    } else {
        transferLength = spiState[spi].dma_current_transfer_length - DMA_GetCurrDataCounter(spiMap[spi].rx_dma_stream);
    }

    if (st != NULL) {
        st->configured_transfer_length = spiState[spi].dma_current_transfer_length;
        st->transfer_length = (uint32_t)transferLength;
        st->ss_state = spiState[spi].ss_state;
        st->transfer_ongoing = spiState[spi].dma_configured;
    }

    return transferLength;
}

void hal_spi_set_callback_on_selected(hal_spi_interface_t spi, hal_spi_select_user_callback cb, void* reserved) {
    spiState[spi].select_user_callback = cb;
}

bool hal_spi_is_enabled(hal_spi_interface_t spi) {
    return spiState[spi].enabled;
}

/**
 * Compatibility for the RC4 release of tinker, which used the no-arg version.
 */
bool hal_spi_is_enabled_deprecated() {
    return false;
}

/**
 * @brief  This function handles DMA1 Stream 7 interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream7_irq(void) {
    //HAL_SPI_INTERFACE2 and HAL_SPI_INTERFACE3 shares same DMA peripheral and stream
#if TOTAL_SPI == 3
    if (spiState[HAL_SPI_INTERFACE3].dma_configured) {
        spiTxDmaStreamInterruptHandler(HAL_SPI_INTERFACE3);
    } else
#endif
    {
        spiTxDmaStreamInterruptHandler(HAL_SPI_INTERFACE2);
    }
}

/**
 * @brief  This function handles DMA2 Stream 5 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream5_irq(void) {
    spiTxDmaStreamInterruptHandler(HAL_SPI_INTERFACE1);
}

/**
 * @brief  This function handles DMA1 Stream 2 interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream2_irq(void) {
    //HAL_SPI_INTERFACE2 and HAL_SPI_INTERFACE3 shares same DMA peripheral and stream
#if TOTAL_SPI == 3
    if (spiState[HAL_SPI_INTERFACE3].dma_configured) {
        spiRxDmaStreamInterruptHandler(HAL_SPI_INTERFACE3);
    } else
#endif
    {
        spiRxDmaStreamInterruptHandler(HAL_SPI_INTERFACE2);
    }
}

/**
 * @brief  This function handles DMA2 Stream 2 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream2_irq_override(void) {
    spiRxDmaStreamInterruptHandler(HAL_SPI_INTERFACE1);
}

void hal_spi_info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved) {
    info->system_clock = spi==HAL_SPI_INTERFACE1 ? 60000000 : 30000000;
    if (info->version >= HAL_SPI_INFO_VERSION_1) {
        int32_t state = HAL_disable_irq();
        if (spiState[spi].enabled) {
            uint32_t prescaler = (1 << ((spiState[spi].init_structure.SPI_BaudRatePrescaler / 8) + 1));
            info->clock = info->system_clock / prescaler;
        } else {
            info->clock = 0;
        }
        info->default_settings = !(spiState[spi].clock_divider_set || spiState[spi].data_mode_set || spiState[spi].bit_order_set);
        info->enabled = spiState[spi].enabled;
        info->mode = spiState[spi].mode;
        info->bit_order = spiState[spi].init_structure.SPI_FirstBit == SPI_FirstBit_MSB ? MSBFIRST : LSBFIRST;
        info->data_mode = spiState[spi].init_structure.SPI_CPOL | spiState[spi].init_structure.SPI_CPHA;
        if (info->version >= HAL_SPI_INFO_VERSION_2) {
            info->ss_pin = spiState[spi].ss_pin;
        }
        HAL_enable_irq(state);
    }
}

static void spiOnSelectedHandler(void *data) {
    hal_spi_interface_t spi = (hal_spi_interface_t)data;
    uint8_t state = !HAL_GPIO_Read(spiState[spi].ss_pin);
    spiState[spi].ss_state = state;
    if (state) {
        /* Selected */
        if (spiState[spi].dma_configured) {
            SPI_Cmd(spiMap[spi].peripheral, ENABLE);
        }
    } else {
        /* Deselected
         * If there is a pending DMA transfer, it needs to be canceled.
         */
        SPI_Cmd(spiMap[spi].peripheral, DISABLE);
        if (spiState[spi].dma_configured) {
            hal_spi_transfer_dma_cancel(spi);
        } else {
            /* Just in case clear DR */
            while (SPI_I2S_GetFlagStatus(spiMap[spi].peripheral, SPI_I2S_FLAG_RXNE) == SET) {
                (void)SPI_I2S_ReceiveData(spiMap[spi].peripheral);
            }
        }
    }

    if (spiState[spi].select_user_callback) {
        spiState[spi].select_user_callback(state);
    }
}

int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved) {
    if (sleep) {
        CHECK_TRUE(hal_spi_is_enabled(spi), SYSTEM_ERROR_NONE);
        CHECK_FALSE(spiState[spi].suspended, SYSTEM_ERROR_NONE);
        hal_spi_transfer_dma_cancel(spi);
        while (!spiState[spi].dma_aborted);
        hal_spi_end(spi); // It doesn't clear spi settings, so we can reuse the previous settings on woken up.
        spiState[spi].suspended = true;
    } else {
        CHECK_TRUE(spiState[spi].suspended, SYSTEM_ERROR_NONE);
        hal_spi_begin_ext(spi, spiState[spi].mode, spiState[spi].ss_pin, NULL);
        spiState[spi].suspended = false;
    }
    return SYSTEM_ERROR_NONE;
}

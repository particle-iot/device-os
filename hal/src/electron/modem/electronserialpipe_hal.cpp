/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "electronserialpipe_hal.h"
#include "stm32f2xx.h"
#include "usart_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "mdm_hal.h"

static USART_InitTypeDef USART_InitStructure;

ElectronSerialPipe::ElectronSerialPipe(int rxSize, int txSize) :
    _pipeRx( rxSize ),
    _pipeTx( txSize )
{
}

ElectronSerialPipe::~ElectronSerialPipe(void)
{
    // wait for transmission of outgoing data
    while (_pipeTx.readable())
    {
        char c = _pipeTx.getc();
        USART_SendData(USART3, c);
    }

    // Disable the USART
    USART_Cmd(USART3, DISABLE);

    // Deinitialise USART
    USART_DeInit(USART3);

    // Disable USART Receive and Transmit interrupts
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART3, USART_IT_TXE, DISABLE);

    NVIC_InitTypeDef NVIC_InitStructure;

    // Disable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

    NVIC_Init(&NVIC_InitStructure);

    // Disable USART Clock
    RCC->APB1ENR &= ~RCC_APB1Periph_USART3;

    // clear any received data
    // ... should be handled with ~Pipe deconstructor
}

void ElectronSerialPipe::begin(unsigned int baud)
{
    //HAL_USART_Begin(HAL_USART_SERIAL3, baud);
    USART_DeInit(USART3);

#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS
    // Configure USART RTS and CTS as alternate function push-pull
    HAL_Pin_Mode(RTS_UC, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(CTS_UC, AF_OUTPUT_PUSHPULL);
#endif
    // Configure USART Rx and Tx as alternate function push-pull, and enable GPIOA clock
    HAL_Pin_Mode(RXD_UC, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(TXD_UC, AF_OUTPUT_PUSHPULL);

    // Enable USART Clock
    RCC->APB1ENR |= RCC_APB1Periph_USART3;

    // Connect USART pins to AFx
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS
    GPIO_PinAFConfig(PIN_MAP[RTS_UC].gpio_peripheral, PIN_MAP[RTS_UC].gpio_pin_source, GPIO_AF_USART3);
    GPIO_PinAFConfig(PIN_MAP[CTS_UC].gpio_peripheral, PIN_MAP[CTS_UC].gpio_pin_source, GPIO_AF_USART3);
#endif
    GPIO_PinAFConfig(PIN_MAP[RXD_UC].gpio_peripheral, PIN_MAP[RXD_UC].gpio_pin_source, GPIO_AF_USART3);
    GPIO_PinAFConfig(PIN_MAP[TXD_UC].gpio_peripheral, PIN_MAP[TXD_UC].gpio_pin_source, GPIO_AF_USART3);

    // NVIC Configuration
    NVIC_InitTypeDef NVIC_InitStructure;
    // Enable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
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
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
#endif

    // Configure USART
    USART_Init(USART3, &USART_InitStructure);

    // Enable the USART
    USART_Cmd(USART3, ENABLE);

    // Enable USART Receive and Transmit interrupts
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}

// tx channel
int ElectronSerialPipe::writeable(void)
{
    return _pipeTx.free();
}

int ElectronSerialPipe::putc(int c)
{
    c = _pipeTx.putc(c);
    txStart();
    return c;
}

int ElectronSerialPipe::put(const void* buffer, int length, bool blocking)
{
    int count = length;
    const char* ptr = (const char*)buffer;
    if (count)
    {
        do
        {
            int written = _pipeTx.put(ptr, count, false);
            if (written) {
                ptr += written;
                count -= written;
                txStart();
            }
            else if (!blocking)
                break;
            /* nothing / just wait */;
        }
        while (count);
    }
    return (length - count);
}

void ElectronSerialPipe::txCopy(void)
{
    if (_pipeTx.readable()) {
        char c = _pipeTx.getc();
        USART_SendData(USART3, c);
    }
}

void ElectronSerialPipe::txIrqBuf(void)
{
    txCopy();
    // detach tx isr if we are done
    if (!_pipeTx.readable())
        USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
}

void ElectronSerialPipe::txStart(void)
{
    // disable the tx isr to avoid interruption
    USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
    txCopy();
    // attach the tx isr to handle the remaining data
    if (_pipeTx.readable())
        USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}

// rx channel
int ElectronSerialPipe::readable(void)
{
    return _pipeRx.readable();
}

int ElectronSerialPipe::getc(void)
{
    if (!_pipeRx.readable())
        return EOF;
    return _pipeRx.getc();
}

int ElectronSerialPipe::get(void* buffer, int length, bool blocking)
{
    return _pipeRx.get((char*)buffer,length,blocking);
}

void ElectronSerialPipe::rxIrqBuf(void)
{
    char c = USART_ReceiveData(USART3);
    if (_pipeRx.writeable())
        _pipeRx.putc(c);
    else
        /* overflow */;
}

/*******************************************************************************
 * Function Name  : HAL_USART3_Handler
 * Description    : This function handles USART3 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void HAL_USART3_Handler(void)
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        electronMDM.rxIrqBuf();
    }

    if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)
    {
        electronMDM.txIrqBuf();
    }
}

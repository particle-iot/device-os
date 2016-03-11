/**
 ******************************************************************************
 * @file    can_hal.c
 * @author  Brian Spranger, Julien Vanier
 * @version V1.0.0
 * @date    04-Jan-2016
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
#include "can_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "stm32f2xx.h"
#include "fixed_queue.h"

#include <cstring>

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* CAN Filter Master Register bits */
#define FMR_FINIT         ((uint32_t)0x00000001) /* Filter init mode */

/* Exported types ------------------------------------------------------------*/
struct STM32_CAN_Info
{
    CAN_TypeDef  *can_peripheral;

    uint32_t  can_clock_en;

    int32_t   can_tx_irqn;
    int32_t   can_rx0_irqn;
    int32_t   can_rx1_irqn;
    int32_t   can_sce_irqn;

    uint16_t   can_tx_pin;
    uint16_t   can_rx_pin;

    uint8_t   can_af_map;
    uint8_t   can_first_filter;
    uint8_t   can_last_filter;
};

class CANDriver
{
public:
    CANDriver(const STM32_CAN_Info &hw,
              uint16_t rxQueueSize,
              uint16_t txQueueSize)
        : hw(hw),
          enabled(false),
          nextFilter(hw.can_first_filter),
          rxQueue(rxQueueSize),
          txQueue(txQueueSize)
    {
    }
    
    void begin(uint32_t baud,
               uint32_t flags);
    void end();

    bool enqueueTx(const CANMessage &message);
    bool dequeueRx(CANMessage &message);
    uint8_t rxQueueSize();

    bool addFilter(uint32_t id, uint32_t mask, HAL_CAN_Filters type);
    void clearFilters();

    bool isEnabled();

    HAL_CAN_Errors errorStatus();

    void txInterruptHandler();
    void rx0InterruptHandler();

protected:
    CanTxMsg messageHALtoSTM(const CANMessage &in);
    CANMessage messageSTMtoHAL(const CanRxMsg &in);

    uint8_t pendingRxMessages();
    bool transmit(const CANMessage &message);
    bool receive(CANMessage &message);

protected:
    const STM32_CAN_Info &hw;
    bool enabled;
    uint8_t nextFilter;

    FixedQueue<CANMessage> rxQueue;
    FixedQueue<CANMessage> txQueue;
};

/* Private variables ---------------------------------------------------------*/

/*
 * CAN peripheral mapping
 */
static const STM32_CAN_Info CAN_MAP[TOTAL_CAN] = {
    /*
     * CAN_peripheral
     * clock control register (APBxENR)
     * clock enable bit value (RCC_APBxPeriph_CANx)
     * Tx interrupt number (CANx_TX_IRQn)
     * Rx0 interrupt number (CANx_RX0_IRQn)
     * Rx1 interrupt number (CANx_RX1_IRQn)
     * Sce interrupt number (CANx_SCE_IRQn)
     * TX pin
     * RX pin
     * GPIO AF map (GPIO_AF_CANx)
     * First filter bank for channel
     * Last filter bank for channel
     */
#ifdef HAL_HAS_CAN_D1_D2
    /* CAN_D1_D2 */ { CAN2, RCC_APB1Periph_CAN2, CAN2_TX_IRQn, CAN2_RX0_IRQn, CAN2_RX1_IRQn, CAN2_SCE_IRQn, D1, D2, GPIO_AF_CAN2, 14, 27 },
#endif
#ifdef HAL_HAS_CAN_C4_C5
    /* CAN_C4_C5 */ { CAN1, RCC_APB1Periph_CAN1, CAN1_TX_IRQn, CAN1_RX0_IRQn, CAN1_RX1_IRQn, CAN1_SCE_IRQn, C4, C5, GPIO_AF_CAN1, 0, 13 },
#endif
};

static CANDriver *drivers[TOTAL_CAN] = {};

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * HAL interface functions for CAN
 *******************************************************************************/

/*******************************************************************************
   Name:           HAL_CAN_Init
   Description:    Creates the CANDriver object

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @param rxQueueSize how many message to buffer on receive
       @param txQueueSize how many message to buffer before transmit

*******************************************************************************/
void HAL_CAN_Init(HAL_CAN_Channel channel,
                  uint16_t rxQueueSize,
                  uint16_t txQueueSize,
                  void *reserved)
{
    if(!drivers[channel])
    {
        drivers[channel] = new CANDriver(CAN_MAP[channel], rxQueueSize, txQueueSize);
    }
}

/*******************************************************************************
   Name:           HAL_CAN_Begin
   Description:    Initializes the CAN Hardware and interrupts

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @param baud The baud rate of the CAN bus 
       @param flags Configuration flags for the CAN channel

*******************************************************************************/
void HAL_CAN_Begin(HAL_CAN_Channel channel,
                   uint32_t        baud,
                   uint32_t        flags,
                   void *reserved)
{
    if(!drivers[channel])
    {
        return;
    }
    drivers[channel]->begin(baud, flags);
}

/*******************************************************************************
   Name:           HAL_CAN_End
   Description:    deinitializes the hardware

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)

*******************************************************************************/
void HAL_CAN_End(HAL_CAN_Channel channel,
                 void *reserved)
{
    if(!drivers[channel])
    {
        return;
    }
    drivers[channel]->end();
}

/*******************************************************************************
   Name:           HAL_CAN_Transmit
   Description:    Add a CAN message to the transmit queue

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @param message Pointer to the CAN message
       @return true if enqueued, false if queue is full

*******************************************************************************/
bool HAL_CAN_Transmit(HAL_CAN_Channel     channel,
                      const CANMessage *message,
                      void *reserved)
{
    if(!drivers[channel])
    {
        return false;
    }
    return drivers[channel]->enqueueTx(*message);
}

/*******************************************************************************
   Name:           HAL_CAN_Receive
   Description:    Pops a CAN message from the receive queue

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @param message Pointer to the CAN message
       @return true if message was received, false if queue is empty

*******************************************************************************/
bool HAL_CAN_Receive(HAL_CAN_Channel channel,
                     CANMessage *message,
                     void *reserved)
{
    if(!drivers[channel])
    {
        return false;
    }
    return drivers[channel]->dequeueRx(*message);
}

/*******************************************************************************
   Name:           HAL_CAN_Available_Messages
   Description:    How many message are in the receive queue

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @return count of messages

*******************************************************************************/
uint8_t HAL_CAN_Available_Messages(HAL_CAN_Channel channel,
                                   void *reserved)
{
    if(!drivers[channel])
    {
        return 0;
    }
    return drivers[channel]->rxQueueSize();
}

/*******************************************************************************
   Name:           HAL_CAN_Add_Filter
   Description:    Add an id/mask filter for received CAN messages

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @param id Desired ID for incoming CAN messages
       @param mask Bits that must match in ID of incoming CAN messages
       @param type Filter standard or extended IDs
       @return true if filter added, false if too many filters already

*******************************************************************************/
bool HAL_CAN_Add_Filter(HAL_CAN_Channel channel,
                        uint32_t id,
                        uint32_t mask,
                        HAL_CAN_Filters type,
                        void *reserved)
{
    if(!drivers[channel])
    {
        return false;
    }
    return drivers[channel]->addFilter(id, mask, type);
}

/*******************************************************************************
   Name:           HAL_CAN_Clear_Filters
   Description:    Go back to accepting all messages

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)

*******************************************************************************/
void HAL_CAN_Clear_Filters(HAL_CAN_Channel channel,
                           void *reserved)
{
    if(!drivers[channel])
    {
        return;
    }
    drivers[channel]->clearFilters();
}

/*******************************************************************************
   Name:           HAL_CAN_Is_Enabled
   Description:    returns if the CAN has been enabled

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
          @return true or false

*******************************************************************************/
bool HAL_CAN_Is_Enabled(HAL_CAN_Channel channel)
{
    if(!drivers[channel])
    {
        return false;
    }
    return drivers[channel]->isEnabled();
}

/*******************************************************************************
   Name:           HAL_CAN_Error_Status
   Description:    Is the bus in an error state?

   Parameters:
       @param channel CAN Channel (CAN1, CAN2, etc)
       @return Type of error on the bus

*******************************************************************************/
HAL_CAN_Errors HAL_CAN_Error_Status(HAL_CAN_Channel channel)
{
    if(!drivers[channel])
    {
        return CAN_NO_ERROR;
    }
    return drivers[channel]->errorStatus();
}

/*******************************************************************************
 * Global interrupt handlers
 *******************************************************************************/

#ifdef HAL_HAS_CAN_D1_D2

/*******************************************************************************
* Function Name  : HAL_CAN2_TX_Handler
* Description    : This function handles CAN2 Tx global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void HAL_CAN2_TX_Handler(void)
{
    if(!drivers[CAN_D1_D2])
    {
        return;
    }
    drivers[CAN_D1_D2]->txInterruptHandler();
}


/*******************************************************************************
* Function Name  : HAL_CAN2_RX0_Handler
* Description    : This function handles CAN2 Rx0 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void HAL_CAN2_RX0_Handler(void)
{
    if(!drivers[CAN_D1_D2])
    {
        return;
    }
    drivers[CAN_D1_D2]->rx0InterruptHandler();
}

#endif

#ifdef HAL_HAS_CAN_C4_C5

/*******************************************************************************
* Function Name  : HAL_CAN1_TX_Handler
* Description    : This function handles CAN1 Tx global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void HAL_CAN1_TX_Handler(void)
{
    if(!drivers[CAN_C4_C5])
    {
        return;
    }
    drivers[CAN_C4_C5]->txInterruptHandler();
}


/*******************************************************************************
* Function Name  : HAL_CAN1_RX0_Handler
* Description    : This function handles CAN1 Rx0 global interrupt request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void HAL_CAN1_RX0_Handler(void)
{
    if(!drivers[CAN_C4_C5])
    {
        return;
    }
    drivers[CAN_C4_C5]->rx0InterruptHandler();
}

#endif

#ifdef __cplusplus
}
#endif

/*******************************************************************************
 * Implementation of the CAN driver
 *******************************************************************************/

// RAII helpers to disable CAN interrupts
template <uint32_t Flag>
class DisableCANInterrupts
{
public:
    DisableCANInterrupts(CAN_TypeDef *peripheral)
        : peripheral(peripheral)
    {
        CAN_ITConfig(peripheral, Flag, DISABLE);
    }

    ~DisableCANInterrupts()
    {
        CAN_ITConfig(peripheral, Flag, ENABLE);
    }

private:
    CAN_TypeDef  *peripheral;
};

typedef DisableCANInterrupts<CAN_IT_TME> DisableTxInterrupts;
typedef DisableCANInterrupts<CAN_IT_FMP0> DisableRxInterrupts;

void CANDriver::begin(uint32_t baud,
        uint32_t flags)
{
    enabled = true;

    rxQueue.clear();
    txQueue.clear();

    // Configure CAN Rx and Tx as alternate function push-pull, and enable GPIOA clock
    HAL_Pin_Mode(hw.can_rx_pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(hw.can_tx_pin, AF_OUTPUT_PUSHPULL);

    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;

    // Enable CAN Clock. CAN1 clock must always be enabled
    RCC->APB1ENR |= hw.can_clock_en | RCC_APB1Periph_CAN1;

    // Connect CAN pins to alternate function
    STM32_Pin_Info  *PIN_MAP = HAL_Pin_Map();
    STM32_Pin_Info *rx_pin = &PIN_MAP[hw.can_rx_pin];
    STM32_Pin_Info *tx_pin = &PIN_MAP[hw.can_tx_pin];

    GPIO_PinAFConfig(rx_pin->gpio_peripheral, rx_pin->gpio_pin_source, hw.can_af_map);
    GPIO_PinAFConfig(tx_pin->gpio_peripheral, tx_pin->gpio_pin_source, hw.can_af_map);

    CAN_InitTypeDef   CAN_InitStructure;
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = ENABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = ENABLE;
    CAN_InitStructure.CAN_Mode = (flags & CAN_TEST_MODE) ? CAN_Mode_LoopBack : CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW  = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1  = CAN_BS1_11tq;
    CAN_InitStructure.CAN_BS2  = CAN_BS2_3tq;
    CAN_InitStructure.CAN_Prescaler = 2000000 / baud;

    // Configure CAN
    CAN_Init(hw.can_peripheral, &CAN_InitStructure);

    // Configure filters for the CAN2 (slave bank) have half the 28 filters
    if(hw.can_first_filter != 0)
    {
        CAN_SlaveStartBank(hw.can_first_filter);
    }

    // Accept all messages, if no filters were configured before
    // calling begin()
    if(nextFilter == hw.can_first_filter)
    {
        clearFilters();
    }

    // NVIC Configuration
    NVIC_InitTypeDef   NVIC_InitStructure;

    // Enable the CAN Tx Interrupt
    NVIC_InitStructure.NVIC_IRQChannel                   = hw.can_tx_irqn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable the CAN Rx FIFO 0 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel                   = hw.can_rx0_irqn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable CAN Receive and Transmit interrupts
    CAN_ClearITPendingBit(hw.can_peripheral, CAN_IT_TME);
    CAN_ITConfig(hw.can_peripheral, CAN_IT_TME, ENABLE);
    CAN_ClearITPendingBit(hw.can_peripheral, CAN_IT_FMP0);
    CAN_ITConfig(hw.can_peripheral, CAN_IT_FMP0, ENABLE);
}

void CANDriver::end() {
    enabled = false;

    // Deinitialise CAN
    CAN_DeInit(hw.can_peripheral);

    // Disable CAN Receive and Transmit interrupts
    CAN_ITConfig(hw.can_peripheral, CAN_IT_TME, DISABLE);
    CAN_ITConfig(hw.can_peripheral, CAN_IT_FMP0, DISABLE);

    NVIC_InitTypeDef   NVIC_InitStructure;

    //Disable the CAN Tx Interrupt
    NVIC_InitStructure.NVIC_IRQChannel    = hw.can_tx_irqn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);

    //Disable the CAN Rx FIFO 0 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel    = hw.can_rx0_irqn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Disable CAN Clock and CAN1 Clock
    RCC->APB1ENR &= ~(hw.can_clock_en | RCC_APB1Periph_CAN1);

    clearFilters();
}

/*******************************************************************************
   Name:           CANDriver::enqueueTx
   Description:    Attempts to transmit a CAN message, or enqueue to transmit
                   queue if all transmit mailboxes are full.

   Parameters:
       @param in The HAL CAN message to send
       @return true if message is transmitted or queue, false if transmit queue
               is full

*******************************************************************************/
bool CANDriver::enqueueTx(const CANMessage &in) {
    DisableTxInterrupts disTxInt(hw.can_peripheral);

    if(txQueue.empty() && transmit(in)) {
        return true;
    }

    return txQueue.push(in);
}

/*******************************************************************************
   Name:           CANDriver::dequeueRx
   Description:    Pops a message from the received queue

   Parameters:
       @param out Where the received HAL CAN message will go
       @return true if message is available, false if receive queue is empty

*******************************************************************************/
bool CANDriver::dequeueRx(CANMessage &out) {
    DisableRxInterrupts disRxInt(hw.can_peripheral);

    if(rxQueue.empty()) {
        return false;
    }

    out = rxQueue.pop();
    return true;
}

/*******************************************************************************
   Name:           CANDriver::messageHALtoSTM
   Description:    Map a HAL CAN message struct to an STM CAN message struct

   Parameters:
       @param in Source HAL CAN message
       @return Destination STM CAN message

*******************************************************************************/
CanTxMsg CANDriver::messageHALtoSTM(const CANMessage &in)
{
    CanTxMsg out = {};

    if(in.extended)
    {
        out.ExtId = in.id & 0x1FFFFFFFUL;
        out.IDE   = CAN_Id_Extended;
    }
    else
    {
        out.StdId = in.id & 0x7FFUL;
        out.IDE   = CAN_Id_Standard;
    }

    if(in.rtr)
    {
        out.RTR = CAN_RTR_REMOTE;
    }
    else
    {
        out.RTR = CAN_RTR_DATA;
    }

    out.DLC = in.len;
    std::memcpy(out.Data, in.data, 8);

    return out;
}

/*******************************************************************************
   Name:           STM_CAN_to_HAL_CAN
   Description:    Map an STM CAN message struct to a HAL CAN message struct

   Parameters:
       @param in Source STM CAN message
       @return Destination HAL CAN message

*******************************************************************************/
CANMessage CANDriver::messageSTMtoHAL(const CanRxMsg &in)
{
    CANMessage out = {};

    if(in.IDE == CAN_Id_Standard)
    {
        out.id = in.StdId;
    }
    else
    {
        out.id = in.ExtId;
        out.extended = true;
    }

    out.rtr = in.RTR == CAN_RTR_REMOTE;
    out.len = in.DLC;
    std::memcpy(out.data, in.Data, 8);

    return out;
}


/*******************************************************************************
   Name:           CANDriver::transmit
   Description:    Adds a CAN message in a transmit mailbox

   Parameters:
       @param message The CAN message to send
       @return true if added to TX mailbox, false if all mailboxes are full

*******************************************************************************/
bool CANDriver::transmit(const CANMessage &message)
{
    CanTxMsg  stm_message = messageHALtoSTM(message);
    uint8_t tx_mailbox = CAN_Transmit(hw.can_peripheral, &stm_message);

    return tx_mailbox != CAN_TxStatus_NoMailBox;
}

/*******************************************************************************
   Name:           CANDriver::receive
   Description:    Get a CAN message from the receive FIFO

   Parameters:
       @param message Where the CAN message will be written
       @return true if message received, false if no message are pending

*******************************************************************************/
bool CANDriver::receive(CANMessage &message)
{
    if(pendingRxMessages() == 0)
    {
        return false;
    }

    CanRxMsg stm_message;
    CAN_Receive(hw.can_peripheral, CAN_FIFO0, &stm_message);
    message = messageSTMtoHAL(stm_message);
    return true;
}

/*******************************************************************************
   Name:           CANDriver::pendingRxMessages
   Description:    returns the number of messages available in the RX FIFO

   Parameters:
       @return number of pending messages

*******************************************************************************/
uint8_t CANDriver::pendingRxMessages()
{
    return CAN_MessagePending(hw.can_peripheral, CAN_FIFO0);
}

/*******************************************************************************
   Name:           CANDriver::rxQueueSize
   Description:    Size of the receive queue

   Parameters:
       @return count of messages

*******************************************************************************/
uint8_t CANDriver::rxQueueSize()
{
   return rxQueue.size();
}

/*******************************************************************************
   Name:           CANDriver::addFilter
   Description:    Add an id/mask filter for received CAN messages

   Parameters:
       @param id Desired ID for incoming CAN messages
       @param mask Bits that must match in ID of incoming CAN messages
       @param type Filter standard or extended IDs
       @return true if filter added, false if too many filters already

*******************************************************************************/
bool CANDriver::addFilter(uint32_t id, uint32_t mask, HAL_CAN_Filters type)
{
    if(nextFilter > hw.can_last_filter)
    {
        return false;
    }

    // Filter configuration
    // See STM32F2 Reference Manual for register organization
    uint32_t filterId, filterMask;
    const uint32_t extendedBit = 0x4;
    const uint32_t rtrBit = 0x2;

    if(type == CAN_FILTER_STANDARD)
    {
        filterId = id << 21;
        filterMask = (mask << 21) | extendedBit | rtrBit;
    }
    else
    {
        filterId = (id << 3) | extendedBit;
        filterMask = (mask << 3) | extendedBit | rtrBit;
    }

    CAN_FilterInitTypeDef   CAN_FilterInitStructure = {};
    CAN_FilterInitStructure.CAN_FilterNumber         = nextFilter;
    CAN_FilterInitStructure.CAN_FilterMode           = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale          = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh         = filterId >> 16;
    CAN_FilterInitStructure.CAN_FilterIdLow          = filterId & 0xFFFF;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh     = filterMask >> 16;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow      = filterMask & 0xFFFF;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FilterFIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation     = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
    nextFilter++;

    return true;
}

/*******************************************************************************
   Name:           CANDriver::clearFilters
   Description:    Go back to accepting all messages

   Parameters:

*******************************************************************************/
void CANDriver::clearFilters()
{
    nextFilter = hw.can_first_filter;

    // Initialisation mode for the filter
    // Filters are shared for CAN1 and CAN2 and configured on CAN1
    CAN1->FMR |= FMR_FINIT;
    // Clear appropriate bits in filter activation register
    uint32_t mask = 0;
    for(uint8_t pos = hw.can_first_filter; pos <= hw.can_last_filter; pos++)
    {
        mask |= 1ul << pos;
    }
    CAN1->FA1R &= ~mask;

    // Allow all messages through
    CAN_FilterInitTypeDef   CAN_FilterInitStructure = {};
    CAN_FilterInitStructure.CAN_FilterNumber         = hw.can_first_filter;
    CAN_FilterInitStructure.CAN_FilterMode           = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale          = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh         = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow          = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh     = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow      = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FilterFIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation     = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
}


/*******************************************************************************
   Name:           CANDriver::isEnabled
   Description:    returns if the CAN has been enabled

   Parameters:
       @return true or false

*******************************************************************************/
bool CANDriver::isEnabled()
{
   return enabled;
}

/*******************************************************************************
   Name:           CANDriver::errorStatus
   Description:    returns if the CAN is in an error state

   Parameters:
       @return Type of error on the bus

*******************************************************************************/
HAL_CAN_Errors CANDriver::errorStatus()
{
    if(CAN_GetFlagStatus(hw.can_peripheral, CAN_FLAG_BOF))
    {
        return CAN_BUS_OFF;
    }
    else if(CAN_GetFlagStatus(hw.can_peripheral, CAN_FLAG_EPV))
    {
        return CAN_ERROR_PASSIVE;
    }
    else
    {
        return CAN_NO_ERROR;
    }
}



/*******************************************************************************
   Name:           CANDriver::txInterruptHandler
   Description:    CAN Transmit Interrupt
                   Transmits next message in queue

   Parameters: None

*******************************************************************************/
void CANDriver::txInterruptHandler()
{
    if(!isEnabled())
    {
        return;
    }

    CAN_ClearITPendingBit(hw.can_peripheral, CAN_IT_TME);

    if(txQueue.empty())
    {
        return;
    }

    CANMessage nextMessage = txQueue.pop();
    transmit(nextMessage);
}


/*******************************************************************************
   Name:           CANDriver::rx0InterruptHandler
   Description:    CAN FIFO0 RX interrupt
                   Enqueues received message

   Parameters: None

*******************************************************************************/
void CANDriver::rx0InterruptHandler()
{
    if(!isEnabled())
    {
        return;
    }

    // No need to clear CAN_IT_FMP0 flag since it can only be cleared by hardware

    CANMessage message;
    if(receive(message)) {
        rxQueue.push(message);
    }
}

/**
 ******************************************************************************
 * @file    can_hal.c
 * @author  Brian Spranger
 * @version V1.0.0
 * @date    30-Sep-2015
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
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum CAN_Num_Def {
  CAN_D1_D2 = 0
} CAN_Num_Def;
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
typedef struct STM32_CAN_Info {
	CAN_TypeDef* can_peripheral;

	__IO uint32_t* can_apbReg;
	uint32_t can_clock_en;

	int32_t can_tx_irqn;
        int32_t can_rx0_irqn;
        int32_t can_rx1_irqn;
        int32_t can_sce_irqn;

	uint16_t can_tx_pin;
	uint16_t can_rx_pin;

	uint8_t can_tx_pinsource;
	uint8_t can_rx_pinsource;

	uint8_t can_af_map;

	// Buffer pointers. These need to be global for IRQ handler access
	CAN_Ring_Buffer* can_tx_buffer;
	CAN_Ring_Buffer* can_rx_buffer;

	bool can_enabled;
	bool can_transmitting;
} STM32_CAN_Info;

STM32_CAN_Info CAN_MAP[TOTAL_CAN] =
{
		/*
		 * CAN_peripheral 
		 * clock control register (APBxENR)
		 * clock enable bit value (RCC_APBxPeriph_CANx/RCC_APBxPeriph_CANx)
		 * Tx interrupt number (CANx_TX_IRQn)
                 * Rx0 interrupt number (CANx_RX0_IRQn)
                 * Rx1 interrupt number (CANx_RX1_IRQn)
                 * Sce interrupt number (CANx_SCE_IRQn)
		 * TX pin
		 * RX pin
		 * TX pin source
		 * RX pin source
		 * GPIO AF map (GPIO_AF_CANx/GPIO_AF_CANx)
		 * <tx_buffer pointer> used internally and does not appear below
		 * <rx_buffer pointer> used internally and does not appear below
		 * <can enabled> used internally and does not appear below
		 * <can transmitting> used internally and does not appear below
		 */
#if PLATFORM_ID == 6
    { CAN2, &RCC->APB1ENR, RCC_APB1Periph_CAN2, CAN2_TX_IRQn, CAN2_RX0_IRQn, CAN2_RX1_IRQn, CAN2_SCE_IRQn, D1, D2, GPIO_PinSource6, GPIO_PinSource5, GPIO_AF_CAN2 } // CAN 2
#endif		
};

static STM32_CAN_Info *canMap[TOTAL_CAN]; // pointer to CAN_MAP[] containing CAN peripheral register locations (etc)

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

inline void store_message(CanRxMsg *pmessage, CAN_Ring_Buffer *buffer)
{
//	unsigned i = (unsigned int)(buffer->head + 1) % CAN_BUFFER_SIZE;
//
//	if (i != buffer->tail)
//	{
//            buffer->buffer[buffer->head].Ext = (pmessage->IDE & CAN_Id_Extended)?true:false;
//            if (buffer->buffer[buffer->head].Ext)
//            {
//                buffer->buffer[buffer->head].ID = pmessage->ExtId;
//            }
//            else
//            {
//                buffer->buffer[buffer->head].ID = pmessage->StdId; 
//            }
//            buffer->buffer[buffer->head].Len = pmessage->DLC; 
//            buffer->buffer[buffer->head].rtr = (pmessage->RTR & CAN_RTR_Remote)?true:false;
//            buffer->buffer[buffer->head].Data[0] = pmessage->Data[0];
//            buffer->buffer[buffer->head].Data[1] = pmessage->Data[1];
//            buffer->buffer[buffer->head].Data[2] = pmessage->Data[2];
//            buffer->buffer[buffer->head].Data[3] = pmessage->Data[3];
//            buffer->buffer[buffer->head].Data[4] = pmessage->Data[4];
//            buffer->buffer[buffer->head].Data[5] = pmessage->Data[5];
//            buffer->buffer[buffer->head].Data[6] = pmessage->Data[6];
//            buffer->buffer[buffer->head].Data[7] = pmessage->Data[7];
//            
//            buffer->head = i;
//	}
}

void HAL_CAN_Init(HAL_CAN_Channel channel, CAN_Ring_Buffer *rx_buffer, CAN_Ring_Buffer *tx_buffer)
{
	if(channel == HAL_CAN_Channel1)
	{
		canMap[channel] = &CAN_MAP[CAN_D1_D2];
	}

	canMap[channel]->can_rx_buffer = rx_buffer;
	canMap[channel]->can_tx_buffer = tx_buffer;

//	memset(canMap[channel]->can_rx_buffer, 0, sizeof(CAN_Ring_Buffer));
//	memset(canMap[channel]->can_tx_buffer, 0, sizeof(CAN_Ring_Buffer));

	canMap[channel]->can_enabled = false;
	canMap[channel]->can_transmitting = false;
}

void HAL_CAN_Begin(HAL_CAN_Channel channel, uint32_t baud)
{
	//CAN_DeInit(canMap[channel]->can_peripheral);

	// Configure CAN Rx and Tx as alternate function push-pull, and enable GPIOA clock
	//HAL_Pin_Mode(canMap[channel]->can_rx_pin, AF_OUTPUT_PUSHPULL);
	//HAL_Pin_Mode(canMap[channel]->can_tx_pin, AF_OUTPUT_PUSHPULL);

	// Enable CAN Clock
	//*(canMap[channel]->can_apbReg) |=  canMap[channel]->can_clock_en;
        
        RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;
        RCC->APB1ENR |= RCC_APB1Periph_CAN2;

	// Connect CAN pins to AFx
	//STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
	//GPIO_PinAFConfig(PIN_MAP[canMap[channel]->can_rx_pin].gpio_peripheral, canMap[channel]->can_rx_pinsource, canMap[channel]->can_af_map);
	//GPIO_PinAFConfig(PIN_MAP[canMap[channel]->can_tx_pin].gpio_peripheral, canMap[channel]->can_tx_pinsource, canMap[channel]->can_af_map);

        GPIO_InitTypeDef GPIO_InitStruct;
        
        // Setup the Rx Pin
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOB, &GPIO_InitStruct);
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2);
        
        // Setup the Tx pin
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOB, &GPIO_InitStruct);
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2);
        
	
        CAN_InitTypeDef CAN_InitStructure;
        CAN_InitStructure.CAN_TTCM = DISABLE;
        CAN_InitStructure.CAN_ABOM = ENABLE;
        CAN_InitStructure.CAN_AWUM = DISABLE;
        CAN_InitStructure.CAN_NART = DISABLE;
        CAN_InitStructure.CAN_RFLM = DISABLE;
        CAN_InitStructure.CAN_TXFP = ENABLE;
        CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
        CAN_InitStructure.CAN_SJW  = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1  = CAN_BS1_11tq;
        CAN_InitStructure.CAN_BS2  = CAN_BS2_3tq;
        
        switch (baud)
        {
            case 50000:
                CAN_InitStructure.CAN_Prescaler = 40;
                break;
            case 100000:
                CAN_InitStructure.CAN_Prescaler = 20;
                break;
            case 125000:
                CAN_InitStructure.CAN_Prescaler = 16;
                break;
            case 250000:
            default:
                CAN_InitStructure.CAN_Prescaler = 8;
                break;
            case 500000:
                CAN_InitStructure.CAN_Prescaler = 4;
                break;
            case 1000000:
                CAN_InitStructure.CAN_Prescaler = 2;
                break;
        }
        
	// Configure CAN
	CAN_Init(CAN2, &CAN_InitStructure);
        
        CAN_FilterInitTypeDef CAN_FilterInitStructure;
        //CAN_SlaveStartBank(1);
        CAN_FilterInitStructure.CAN_FilterNumber = 15;
        CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
        CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
        CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
        CAN_FilterInitStructure.CAN_FilterIdLow =  0x0000;
        CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
        CAN_FilterInitStructure.CAN_FilterMaskIdLow =  0x0000;
        CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FilterFIFO0;
        CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
        CAN_FilterInit(&CAN_FilterInitStructure);

	canMap[channel]->can_enabled = true;
	canMap[channel]->can_transmitting = false;

//	// NVIC Configuration
//	NVIC_InitTypeDef NVIC_InitStructure;
//	// Enable the CAN Tx Interrupt
//	NVIC_InitStructure.NVIC_IRQChannel = canMap[channel]->can_tx_irqn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//        
//        // Enable the CAN Rx FIFO 0 Interrupt
//        NVIC_InitStructure.NVIC_IRQChannel = canMap[channel]->can_rx0_irqn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//        NVIC_Init(&NVIC_InitStructure);
//        
//	// Enable CAN Receive and Transmit interrupts
//        CAN_ClearITPendingBit (canMap[channel]->can_peripheral, CAN_IT_TME);
//  	CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, ENABLE);
//        CAN_ClearITPendingBit (canMap[channel]->can_peripheral, CAN_IT_FMP0);
//	CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_FMP0, ENABLE);
}

void HAL_CAN_End(HAL_CAN_Channel channel)
{
    // wait for transmission of outgoing data
//    while (canMap[channel]->can_tx_buffer->head != canMap[channel]->can_tx_buffer->tail);

    // Deinitialise CAN
    CAN_DeInit(canMap[channel]->can_peripheral);

    // Disable CAN Receive and Transmit interrupts
    CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, DISABLE);
    CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_FMP0, DISABLE);

    NVIC_InitTypeDef NVIC_InitStructure;

    //Disable the CAN Tx Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = canMap[channel]->can_tx_irqn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    //Disable the CAN Rx FIFO 0 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = canMap[channel]->can_rx0_irqn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Disable CAN Clock
    *canMap[channel]->can_apbReg &= ~canMap[channel]->can_clock_en;

    // clear any received data
//    canMap[channel]->can_rx_buffer->head = canMap[channel]->can_rx_buffer->tail;

    // Undo any pin re-mapping done for this CAN
    // ...

//    memset(canMap[channel]->can_rx_buffer, 0, sizeof(CAN_Ring_Buffer));
//    memset(canMap[channel]->can_tx_buffer, 0, sizeof(CAN_Ring_Buffer));

    canMap[channel]->can_enabled = false;
    canMap[channel]->can_transmitting = false;
}

uint32_t HAL_CAN_Write_Data(HAL_CAN_Channel channel, CAN_Message_Struct *pmessage)
{
//    if(((canMap[channel]->can_tx_buffer->head + 1) % CAN_BUFFER_SIZE) != (canMap[channel]->can_tx_buffer->tail))
//    {
//        memcpy((void *)&(canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->head]),(void *)pmessage, sizeof(CAN_Message_Struct));
//        canMap[channel]->can_tx_buffer->head++;
//        canMap[channel]->can_tx_buffer->head %= CAN_BUFFER_SIZE;
//    }
	
//    // interrupts are off and data in queue;
//	if ((CAN_GetITStatus(canMap[channel]->can_peripheral, CAN_IT_TME) == RESET)
//			&& (canMap[channel]->can_tx_buffer->head != canMap[channel]->can_tx_buffer->tail)) 
//        {
//		// Get him busy
//		CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, ENABLE);
//	}
//
//	unsigned i = (canMap[channel]->can_tx_buffer->head + 1) % CAN_BUFFER_SIZE;
//
//	// If the output buffer is full, there's nothing for it other than to
//	// wait for the interrupt handler to empty it a bit
//	//         no space so       or  Called Off Panic with interrupt off get the message out!
//	//         make space                     Enter Polled IO mode
//	while (i == canMap[channel]->can_tx_buffer->tail || ((__get_PRIMASK() & 1) && canMap[channel]->can_tx_buffer->head != canMap[channel]->can_tx_buffer->tail) ) 
//        {
		// Interrupts are on but they are not being serviced because this was called from a higher
		// Priority interrupt

//		if (CAN_GetITStatus(canMap[channel]->can_peripheral, CAN_IT_TME) && CAN_GetFlagStatus(canMap[channel]->can_peripheral, CAN_IT_TME))
//		{
//                    // protect for good measure
//                    CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, DISABLE);
//                    // Write out a byte
//                if ((canMap[channel]->can_tx_buffer->head != canMap[channel]->can_tx_buffer->tail))
//                {
//                    CanTxMsg txmessage;
//                    if (true == canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Ext)
//                    {
//                      txmessage.ExtId = (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].ID & 0x1FFFFFFFUL);
//                      txmessage.StdId = 0U;
//                      txmessage.IDE = CAN_Id_Extended;
//                    }
//                    else
//                    {
//                      txmessage.ExtId = 0UL;
//                      txmessage.StdId = (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].ID & 0x7FFUL);  
//                      txmessage.IDE = CAN_Id_Standard;
//                    }
//                    if (true == canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].rtr)
//                    {
//                      txmessage.RTR = CAN_RTR_REMOTE;
//                    }
//                    else
//                    {
//                      txmessage.RTR = CAN_RTR_DATA;
//                    }
//                    txmessage.DLC = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Len;     
//                    txmessage.Data[0] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[0];
//                    txmessage.Data[1] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[1];
//                    txmessage.Data[2] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[2];
//                    txmessage.Data[3] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[3];
//                    txmessage.Data[4] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[4];
//                    txmessage.Data[5] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[5];
//                    txmessage.Data[6] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[6];
//                    txmessage.Data[7] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[7];
//
//                    if (CAN_TxStatus_NoMailBox != CAN_Transmit(canMap[channel]->can_peripheral, &txmessage))
//                    {
//                            canMap[channel]->can_tx_buffer->tail++;
//                            canMap[channel]->can_tx_buffer->tail %= CAN_BUFFER_SIZE;
//                    }
//                    canMap[channel]->can_transmitting = true;
//	            CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, ENABLE);
//                    // unprotect
////                    CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, ENABLE);
//		}
//	}


                    CanTxMsg txmessage;
                    txmessage.ExtId = 0x1234;
                    txmessage.IDE = CAN_Id_Extended;
                    txmessage.RTR = CAN_RTR_DATA;
                    txmessage.DLC = 8;     
                    txmessage.Data[0] = 1;
                    txmessage.Data[1] = 2;
                    txmessage.Data[2] = 3;
                    txmessage.Data[3] = 4;
                    txmessage.Data[4] = 5;
                    txmessage.Data[5] = 6;
                    txmessage.Data[6] = 7;
                    txmessage.Data[7] = 8;
//                    if (true == pmessage->Ext)
//                    {
//                      txmessage.ExtId = (pmessage->ID & 0x1FFFFFFFUL);
//                      txmessage.StdId = 0U;
//                      txmessage.IDE = CAN_Id_Extended;
//                    }
//                    else
//                    {
//                      txmessage.ExtId = 0UL;
//                      txmessage.StdId = (pmessage->ID & 0x7FFUL);  
//                      txmessage.IDE = CAN_Id_Standard;
//                    }
//                    if (true == pmessage->rtr)
//                    {
//                      txmessage.RTR = CAN_RTR_REMOTE;
//                    }
//                    else
//                    {
//                      txmessage.RTR = CAN_RTR_DATA;
//                    }
//                    txmessage.DLC = pmessage->Len;     
//                    txmessage.Data[0] = pmessage->Data[0];
//                    txmessage.Data[1] = pmessage->Data[1];
//                    txmessage.Data[2] = pmessage->Data[2];
//                    txmessage.Data[3] = pmessage->Data[3];
//                    txmessage.Data[4] = pmessage->Data[4];
//                    txmessage.Data[5] = pmessage->Data[5];
//                    txmessage.Data[6] = pmessage->Data[6];
//                    txmessage.Data[7] = pmessage->Data[7];

                    CAN_Transmit(canMap[channel]->can_peripheral, &txmessage);
                   
                    canMap[channel]->can_transmitting = true;
	            
                    // unprotect
//                    CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, ENABLE);
		

	return 1;
}

int32_t HAL_CAN_Available_Data(HAL_CAN_Channel channel)
{
	return (unsigned int)(CAN_BUFFER_SIZE + canMap[channel]->can_rx_buffer->head - canMap[channel]->can_rx_buffer->tail) % CAN_BUFFER_SIZE;
}

int32_t HAL_CAN_Read_Data(HAL_CAN_Channel channel, CAN_Message_Struct *pmessage)
{
    //uint8_t i, NumPendingMessages;
    CanRxMsg message;
    
    if (NULL != pmessage)
    {
    CAN_Receive(CAN2, CAN_FIFO0, &message);
    pmessage->Ext = (message.IDE & CAN_Id_Extended)?true:false;
    if (pmessage->Ext)
    {
        pmessage->ID = message.ExtId;
    }
    else
    {
        pmessage->ID = message.StdId; 
    }
    pmessage->Len = message.DLC; 
    pmessage->rtr = (message.RTR & CAN_RTR_Remote)?true:false;
    pmessage->Data[0] = message.Data[0];
    pmessage->Data[1] = message.Data[1];
    pmessage->Data[2] = message.Data[2];
    pmessage->Data[3] = message.Data[3];
    pmessage->Data[4] = message.Data[4];
    pmessage->Data[5] = message.Data[5];
    pmessage->Data[6] = message.Data[6];
    pmessage->Data[7] = message.Data[7];
    return 1;
    }
    return -1;
//    memset(&message, 0, sizeof(CanRxMsg));
//    //NumPendingMessages = CAN_MessagePending(canMap[channel]->can_peripheral, CAN_FIFO0);
//    
//    //for (i = 0; i < NumPendingMessages; i++)
//    {
//        // Read message from the receive data register
//        CAN_Receive(canMap[channel]->can_peripheral, CAN_FIFO0, &message);
//        store_message(&message, canMap[channel]->can_rx_buffer);
//    }
//    
//    
//        // if the head isn't ahead of the tail, we don't have any messages
//	if ((canMap[channel]->can_rx_buffer->head == canMap[channel]->can_rx_buffer->tail) ||
//                (NULL == pmessage))
//	{
//		return -1;
//	}
//	else
//	{
//		memcpy((void *)pmessage, (void *)&(canMap[channel]->can_rx_buffer->buffer[canMap[channel]->can_rx_buffer->tail]), sizeof(CAN_Message_Struct));
//		canMap[channel]->can_rx_buffer->tail = (unsigned int)(canMap[channel]->can_rx_buffer->tail + 1) % CAN_BUFFER_SIZE;
//		return 1;
//	}
}

int32_t HAL_CAN_Peek_Data(HAL_CAN_Channel channel, CAN_Message_Struct *pmessage)
{
	if (canMap[channel]->can_rx_buffer->head == canMap[channel]->can_rx_buffer->tail)
	{
            return -1;
	}
	else
	{
            memcpy((void *)pmessage, (void *)&(canMap[channel]->can_rx_buffer->buffer[canMap[channel]->can_rx_buffer->tail]), sizeof(CAN_Message_Struct));
            return 1;
	}
}

void HAL_CAN_Flush_Data(HAL_CAN_Channel channel)
{
//TODO // Loop until CAN DR register is empty
	//while ( canMap[channel]->can_tx_buffer->head != canMap[channel]->can_tx_buffer->tail );
	// Loop until last frame transmission complete
	//while (canMap[channel]->can_transmitting && (CAN_GetFlagStatus(canMap[channel]->can_peripheral, CAN_FLAG_TC) == RESET));
	//canMap[channel]->can_transmitting = false;
}

bool HAL_CAN_Is_Enabled(HAL_CAN_Channel channel)
{
	return canMap[channel]->can_enabled;
}


static void HAL_CAN_Tx_Handler(HAL_CAN_Channel channel)
{
//    uint8_t tx_mailbox = CAN_TxStatus_NoMailBox;
//    //if(CAN_GetITStatus(canMap[channel]->can_peripheral, CAN_IT_TME) != RESET)
//    {
//        // Loop to fill up the CAN Tx buffers
//        do
//        {
//            // Write byte to the transmit data register
//            if (canMap[channel]->can_tx_buffer->head == canMap[channel]->can_tx_buffer->tail)
//            {
//                    // Buffer empty, so disable the CAN Transmit interrupt
//                    CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, DISABLE);
//                    // drop out of the while loop
//                    tx_mailbox = CAN_TxStatus_NoMailBox;
//            }
//            else
//            {
//                CanTxMsg txmessage;
//                if (true == canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Ext)
//                {
//                  txmessage.ExtId = (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].ID & 0x1FFFFFFFUL);
//                  txmessage.StdId = 0U;
//                  txmessage.IDE = CAN_Id_Extended;
//                }
//                else
//                {
//                  txmessage.ExtId = 0UL;
//                  txmessage.StdId = (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].ID & 0x7FFUL);  
//                  txmessage.IDE = CAN_Id_Standard;
//                }
//                if (true == canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].rtr)
//                {
//                  txmessage.RTR = CAN_RTR_REMOTE;
//                }
//                else
//                {
//                  txmessage.RTR = CAN_RTR_DATA;
//                }
//                txmessage.DLC = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Len;     
//                txmessage.Data[0] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[0];
//                txmessage.Data[1] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[1];
//                txmessage.Data[2] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[2];
//                txmessage.Data[3] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[3];
//                txmessage.Data[4] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[4];
//                txmessage.Data[5] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[5];
//                txmessage.Data[6] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[6];
//                txmessage.Data[7] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[7];
//
//                tx_mailbox = CAN_Transmit(canMap[channel]->can_peripheral, &txmessage);
//                if (CAN_TxStatus_NoMailBox != tx_mailbox)
//                {
//                        canMap[channel]->can_tx_buffer->tail++;
//                        canMap[channel]->can_tx_buffer->tail %= CAN_BUFFER_SIZE;
//                }
//            }
//        }
//        while (CAN_TxStatus_NoMailBox != tx_mailbox);
//    }
    CAN_ClearITPendingBit (canMap[channel]->can_peripheral, CAN_IT_TME);
}

static void HAL_CAN_Rx0_Handler(HAL_CAN_Channel channel)
{
    //if(CAN_GetITStatus(canMap[channel]->can_peripheral, CAN_IT_FMP0) != RESET)
    //{
//    uint8_t i, NumPendingMessages;
//    CanRxMsg message;
//    
//    NumPendingMessages = CAN_MessagePending(canMap[channel]->can_peripheral, CAN_FIFO0);
//    
//    for (i = 0; i < NumPendingMessages; i++)
//    {
//        // Read message from the receive data register
//        CAN_Receive(canMap[channel]->can_peripheral, CAN_FIFO0, &message);
//        store_message(&message, canMap[channel]->can_rx_buffer);
//    }
}

static void HAL_CAN_Rx1_Handler(HAL_CAN_Channel channel)
{
//    uint8_t i, NumPendingMessages;
//    CanRxMsg message;
//    
//    NumPendingMessages = CAN_MessagePending(canMap[channel]->can_peripheral, CAN_FIFO1);
//    
//    for (i = 0; i < NumPendingMessages; i++)
//    {
//        // Read message from the receive data register
//        CAN_Receive(canMap[channel]->can_peripheral, CAN_FIFO1, &message);
//        store_message(&message, canMap[channel]->can_rx_buffer);
//    }
}
//static void HAL_CAN_Handler(HAL_CAN_Channel channel)
//{
//    CanRxMsg message;	
//        //if(CAN_GetITStatus(canMap[channel]->can_peripheral, CAN_IT_FMP0) != RESET)
//	{
//            // Read message from the receive data register
//            CAN_Receive(canMap[channel]->can_peripheral, CAN_FIFO0, &message);
//            store_message(&message, canMap[channel]->can_rx_buffer);
//	}
//
//	//if(CAN_GetITStatus(canMap[channel]->can_peripheral, CAN_IT_TME) != RESET)
//	{
//		// Write byte to the transmit data register
//		if (canMap[channel]->can_tx_buffer->head == canMap[channel]->can_tx_buffer->tail)
//		{
//			// Buffer empty, so disable the CAN Transmit interrupt
//			//CAN_ITConfig(canMap[channel]->can_peripheral, CAN_IT_TME, DISABLE);
//		}
//		else
//		{
//			CanTxMsg txmessage;
//                        if (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Ext)
//                        {
//                          txmessage.ExtId = (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].ID & 0x1FFFFFFFUL);
//                          txmessage.StdId = 0U;
//                          txmessage.IDE = CAN_Id_Extended;
//                        }
//                        else
//                        {
//                          txmessage.ExtId = 0UL;
//                          txmessage.StdId = (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].ID & 0x7FFUL);  
//                          txmessage.IDE = CAN_Id_Standard;
//                        }
//                        if (canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].rtr)
//                        {
//                          txmessage.RTR = CAN_RTR_REMOTE;
//                        }
//                        else
//                        {
//                            txmessage.RTR = CAN_RTR_DATA;
//                        }
//                        txmessage.DLC = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Len;     
//                        txmessage.Data[0] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[0];
//                        txmessage.Data[1] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[1];
//                        txmessage.Data[2] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[2];
//                        txmessage.Data[3] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[3];
//                        txmessage.Data[4] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[4];
//                        txmessage.Data[5] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[5];
//                        txmessage.Data[6] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[6];
//                        txmessage.Data[7] = canMap[channel]->can_tx_buffer->buffer[canMap[channel]->can_tx_buffer->tail].Data[7];
//                        // There is more data in the output buffer. Send the next byte
//                        if (CAN_TxStatus_NoMailBox != CAN_Transmit(canMap[channel]->can_peripheral, &txmessage))
//                        {
//                                canMap[channel]->can_tx_buffer->tail++;
//                                canMap[channel]->can_tx_buffer->tail %= CAN_BUFFER_SIZE;
//                        }
//		}
//	}
//
//	// // If Overrun occurs, clear the OVR condition
//	// if (CAN_GetFlagStatus(canMap[channel]->can_peripheral, CAN_FLAG_ORE) != RESET)
//	// {
//	// 	(void)CAN_ReceiveData(canMap[channel]->can_peripheral);
//	// 	CAN_ClearITPendingBit (canMap[channel]->can_peripheral, CAN_IT_ORE);
//	// }
//}

/*******************************************************************************
 * Function Name  : HAL_CAN2_Tx_Handler
 * Description    : This function handles CAN2 Tx global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void CAN2_TX_irq(void)
{
	HAL_CAN_Tx_Handler(HAL_CAN_Channel1);
}

/*******************************************************************************
 * Function Name  : HAL_CAN2_Rx0_Handler
 * Description    : This function handles CAN2 Rx0 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void CAN2_RX0_irq(void)
{
	HAL_CAN_Rx0_Handler(HAL_CAN_Channel1);
}

/*******************************************************************************
 * Function Name  : HAL_CAN2_Rx1_Handler
 * Description    : This function handles CAN2 Rx1 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void CAN2_RX1_irq(void)
{
	HAL_CAN_Rx1_Handler(HAL_CAN_Channel1);
}
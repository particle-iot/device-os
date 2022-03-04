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
    return false;
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
    return false;
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
    return 0;
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
    return false;
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
    return false;
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
    return CAN_NO_ERROR;
}

#ifdef __cplusplus
}
#endif


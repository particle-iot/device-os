/**
  ******************************************************************************
  * @file    cc3000_spi.c
  * @author  Satish Nair, Zachary Crockett, Mohit Bhoite and David Sidrane
  * @version V2.0.0
  * @date    29-March-2013
  * @brief   This file contains all the functions prototypes for the
  *          CC3000 SPI firmware driver.
  *
  *   feb-12-2014 V2.00  David Sidrane david_s5@usa.net see
  *   http://nscdg.com/spark/David_Sidrane's_rework_of_the_TI_CC3000_driver_for_Spark.pdf
  *   for details of rework
  ******************************************************************************


  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

#include <stdint.h>
#include "cc3000_spi.h"
#include "socket.h"
#include "spark_macros.h"
#include "debug.h"
#include "spi_bus.h"
#include "delay_hal.h"


#define READ_COMMAND           {READ, 0 , 0 , 0 , 0}
#define READ_OFFSET_TO_LENGTH   3 //cmd  dmy dmy lh  ll
#define SPI_HEADER_SIZE         (5)

typedef enum
{
  eSPI_STATE_POWERUP = 0,
  eSPI_STATE_INITIALIZED,
  eSPI_STATE_IDLE,
  eSPI_STATE_FIRST_WRITE,
  eSPI_STATE_WRITE_WAIT_IRQ,
  eSPI_STATE_WRITE_PROCEED,
  eSPI_STATE_WRITE_DONE,
  eSPI_STATE_READ_IRQ,
  eSPI_STATE_READ_PROCEED,
  eSPI_STATE_READ_READY,
  eSPI_STATE_READ_PREP_IRQ
} eCC3000States;


typedef enum
{
  eNone = 0,
  eAssert,
  eDeAssert,
} eCSActions;

typedef enum
{
  eRead = 0,
  eWrite
} eSPIOperation;


typedef struct
{
	gcSpiHandleRx SPIRxHandler;
	unsigned short usTxPacketLength;
	unsigned short usRxPacketLength;
	volatile eCC3000States ulSpiState;
	unsigned char *pTxPacket;
	unsigned char *pRxPacket;
	volatile int abort;
} tSpiInformation;

tSpiInformation sSpiInformation;

unsigned char wlan_rx_buffer[RX_SPI_BUFFER_SIZE];
unsigned char wlan_tx_buffer[TX_SPI_BUFFER_SIZE];

uint8_t spi_readCommand[] = READ_COMMAND;
CCASSERT(SPI_HEADER_SIZE == sizeof(spi_readCommand));


typedef uint32_t intState;
inline static intState DISABLE_INT()
{
  intState is = __get_PRIMASK();
  __disable_irq();
  return is;
}

inline static int ENABLE_INT(intState is)
{
    int rv = ((is & 1) == 0);
    if ((is & 1) == 0) {
        __enable_irq();
    }
    return rv;
}

static inline eCC3000States State()
{
  ;
  intState save = DISABLE_INT();
  eCC3000States rv = sSpiInformation.ulSpiState;
  ENABLE_INT(save);
  return rv;
}

static inline eCC3000States SetState(eCC3000States ns, eCSActions cs)
{
  intState save = DISABLE_INT();
  eCC3000States os = sSpiInformation.ulSpiState;
  sSpiInformation.ulSpiState = ns;
  switch (cs)
  {
    case eAssert:
      ASSERT_CS();
      break;
    case eDeAssert:
      DEASSERT_CS();
      break;
  }
  ENABLE_INT(save);
  return os;
}

static inline int WaitFor(eCC3000States s)
{
  int rv = 1;
  do
  {
      intState save = DISABLE_INT();
      rv = (s !=  sSpiInformation.ulSpiState);
      // The following handles the race or if the SPiResumeSpi was not called
      if (rv && s== eSPI_STATE_WRITE_PROCEED && 0==tSLInformation.ReadWlanInterruptPin())
      {
         sSpiInformation.ulSpiState = eSPI_STATE_WRITE_PROCEED;
         rv = 0;
      }
      ENABLE_INT(save);
  } while(rv && !sSpiInformation.abort);
  return !sSpiInformation.abort;
}

static inline int Still(eCC3000States s)
{
  int rv = 1;
  do
  {
      intState save = DISABLE_INT();
      rv = (s ==  sSpiInformation.ulSpiState);
      ENABLE_INT(save);
  } while(rv && !sSpiInformation.abort);
  return !sSpiInformation.abort;
}

static inline int Reserve(eCC3000States ns)
{
   int idle = 0;
   do
   {
       intState save = DISABLE_INT();
       idle = (eSPI_STATE_IDLE ==  sSpiInformation.ulSpiState);
       if (idle)
         {
           SetState(ns, eAssert);
         }
       ENABLE_INT(save);
   } while(!idle && !sSpiInformation.abort);
   return !sSpiInformation.abort;
 }
/****************************************************************************
 CC3000 SPI Protocol API
 ****************************************************************************/
void SpiOpen(gcSpiHandleRx pfRxHandler)
{

	sSpiInformation.ulSpiState = eSPI_STATE_POWERUP;

	sSpiInformation.SPIRxHandler = pfRxHandler;
	sSpiInformation.pRxPacket = wlan_rx_buffer;
	sSpiInformation.usRxPacketLength = 0;
	sSpiInformation.pTxPacket = NULL;
	sSpiInformation.usTxPacketLength = 0;

	/* Enable Interrupt */
        tSLInformation.WriteWlanPin( WLAN_DISABLE );
        HAL_Delay_Microseconds(MS2u(300));
 	tSLInformation.WlanInterruptEnable();
        HAL_Delay_Microseconds(MS2u(1));
        tSLInformation.WriteWlanPin( WLAN_ENABLE );
        WaitFor(eSPI_STATE_INITIALIZED);

}

void SpiClose(void)
{
     tSLInformation.WlanInterruptDisable();
     tSLInformation.WriteWlanPin( WLAN_DISABLE );

      sSpiInformation.pRxPacket = 0;
}

void SpiResumeSpi(void)
{
	//
	//Enable CC3000 SPI IRQ Line Interrupt
	//
	NVIC_EnableIRQ(CC3000_WIFI_INT_EXTI_IRQn);

}

void SpiPauseSpi(void)
{
	//
	//Disable CC3000 SPI IRQ Line Interrupt
	//
	NVIC_DisableIRQ(CC3000_WIFI_INT_EXTI_IRQn);
}


/**
 * @brief  This function TX and RX SPI data and configures interrupt generation
 * 		at the end of the TX interrupt
 * @param  ulTrueFlase True for a read or False for write
 * @param  ptrData Pointer to data to be written
 * @param  ulDataSize The size of the data to be written or read
 * @retval None
 */

int SpiIO(eSPIOperation op, const uint8_t *ptrData, uint32_t ulDataSize, int waitOnCompletion)
{

        eCC3000States current = State();
	/* Disable DMA Channels */
	CC3000_SPI_DMA_Channels(DISABLE);


	if (op == eRead)
	{
		CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) ptrData, ulDataSize);
		CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) spi_readCommand, ulDataSize);
	}
	else
	{
		CC3000_DMA_Config(CC3000_DMA_RX, (uint8_t*) sSpiInformation.pRxPacket, ulDataSize);
		CC3000_DMA_Config(CC3000_DMA_TX, (uint8_t*) ptrData, ulDataSize);
	}

	/* Enable DMA SPI Interrupt */
	DMA_ITConfig(CC3000_SPI_TX_DMA_CHANNEL, DMA_IT_TC, ENABLE);

	/* Enable DMA Channels */
	CC3000_SPI_DMA_Channels(ENABLE);
	return waitOnCompletion ? Still(current) : 0;

}

long SpiSetUp(unsigned char *pUserBuffer, unsigned short usLength)
{
        size_t tx_len = (usLength & 1) ? usLength : usLength +1;

        pUserBuffer[0] = WRITE;
        pUserBuffer[1] = HI(tx_len);
        pUserBuffer[2] = LO(tx_len);
        pUserBuffer[3] = 0;
        pUserBuffer[4] = 0;

        tSLInformation.solicitedResponse = 1; // We are doing a write
        sSpiInformation.pTxPacket = pUserBuffer;
        sSpiInformation.usTxPacketLength = usLength;
        tx_len += SPI_HEADER_SIZE;
        return  tx_len;
}


/**
 * @brief  Sends header information to CC3000
 * @param  None
 * @retval None
 */
long SpiWrite(unsigned char *ucBuf, unsigned short usLength)
{

        int NotAborted = 0;
        switch (State())
        {
        case eSPI_STATE_POWERUP:

          NotAborted = WaitFor(eSPI_STATE_INITIALIZED);
          if (!NotAborted) {
              break;
          }
          // fall through - No break at the end of case

        case eSPI_STATE_INITIALIZED:

          NotAborted = SetState(eSPI_STATE_FIRST_WRITE, eAssert);
          usLength = SpiSetUp(ucBuf,usLength);
          if (NotAborted)
          {
              //Delay for at least 50 us
              HAL_Delay_Microseconds(50);

              //SPI writes first 4 bytes of data
              NotAborted = SpiIO(eWrite, ucBuf, 4, TRUE);
              if (NotAborted)
              {
                //Delay for at least 50 us
                HAL_Delay_Microseconds(50);

                //SPI writes next 4 bytes of data
                NotAborted = SpiIO(eWrite, &ucBuf[4], usLength - 4, TRUE);
              }
          }
          break;

        default:
            NotAborted = WaitFor(eSPI_STATE_IDLE);
            if(NotAborted)
            {
              NotAborted = Reserve(eSPI_STATE_WRITE_WAIT_IRQ);
              if(NotAborted)
              {
                  usLength = SpiSetUp(ucBuf, usLength);
                  NotAborted = WaitFor(eSPI_STATE_WRITE_PROCEED);
                  if(NotAborted)
                  {
                      NotAborted = SpiIO(eWrite, ucBuf, usLength, TRUE);
                  }
              }
            }
          break;
        }
        return NotAborted ? usLength : -1;
}

/**
 * @brief  The handler for Interrupt that is generated on SPI at the end of DMA
 transfer.
 * @param  None
 * @retval None
 */
void SPI_DMA_IntHandler(void)
{
	unsigned long ucTxFinished, ucRxFinished;
	unsigned short data_to_recv = 0;

	ucTxFinished = DMA_GetFlagStatus(CC3000_SPI_TX_DMA_TCFLAG );
	ucRxFinished = DMA_GetFlagStatus(CC3000_SPI_RX_DMA_TCFLAG );
	switch(sSpiInformation.ulSpiState)
	{
	case eSPI_STATE_READ_IRQ:
	  // Both Done
          if (ucTxFinished && ucRxFinished)
          {
                  /* Clear SPI_DMA Interrupt Pending Flags */
                  DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

                  sSpiInformation.ulSpiState = eSPI_STATE_READ_PROCEED;

                  uint16_t *pnetlen = (uint16_t *) &sSpiInformation.pRxPacket[READ_OFFSET_TO_LENGTH];
                  data_to_recv = ntohs(*pnetlen);
                  if (data_to_recv)
                     {
                       /* We will read ARRAY_SIZE(spi_readCommand) + data_to_recv. is it odd? */

                       if ((data_to_recv +  arraySize(spi_readCommand)) & 1)
                         {
                           /* Odd so make it even */

                           data_to_recv++;
                         }

                       /* Read the whole payload in at the beginning of the buffer
                        * Will it fit?
                        */
                       SPARK_ASSERT(data_to_recv <= arraySize(wlan_rx_buffer));
                       SpiIO(eRead,sSpiInformation.pRxPacket,data_to_recv, FALSE);
                     }
          }
	  break;

	case eSPI_STATE_READ_PROCEED:
          //
          // All the data was read - finalize handling by switching to the task
          // and calling from task Event Handler
          //
          if (ucRxFinished)
          {
                  /* Clear SPI_DMA Interrupt Pending Flags */
                  DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

                  SpiPauseSpi();
                  SetState(eSPI_STATE_IDLE, eDeAssert);
                  // Call out to the Unsolicited handler
                  // It will handle the event or leave it there for an outstanding opcode
                  // It it handles it the it Will resume the SPI ISR
                  // It it dose not handles it and there are not outstanding Opcodes the it Will resume the SPI ISR
                  sSpiInformation.SPIRxHandler(sSpiInformation.pRxPacket);

          }
          break;

	case eSPI_STATE_FIRST_WRITE:
	case eSPI_STATE_WRITE_PROCEED:
          if (ucTxFinished)
          {
                  /* Loop until SPI busy */
                  while (SPI_I2S_GetFlagStatus(CC3000_SPI, SPI_I2S_FLAG_BSY ) != RESET)
                  {
                  }

                  /* Clear SPI_DMA Interrupt Pending Flags */
                  DMA_ClearFlag(CC3000_SPI_TX_DMA_TCFLAG | CC3000_SPI_RX_DMA_TCFLAG);

                  if ( sSpiInformation.ulSpiState == eSPI_STATE_FIRST_WRITE)
                  {
                      sSpiInformation.ulSpiState = eSPI_STATE_WRITE_PROCEED;
                  }
                  else
                  {
                      SetState(eSPI_STATE_IDLE, eDeAssert);
                  }
          }
          break;

	default:
	  INVALID_CASE(sSpiInformation.ulSpiState);
	  break;

	}
}

void handle_spi_request()
{
    if (State() == eSPI_STATE_READ_PREP_IRQ && try_acquire_spi_bus(BUS_OWNER_CC3000))
    {
        DEBUG("Handling CC3000 request on main thread");
        SetState(eSPI_STATE_READ_IRQ, eAssert);
        SpiIO(eRead, sSpiInformation.pRxPacket, arraySize(spi_readCommand), FALSE);
        DEBUG("...done");
    }
}

/**
 * @brief  The handler for Interrupt that is generated when CC3000 brings the
 IRQ line low.
 * @param  None
 * @retval None
 */
void SPI_EXTI_IntHandler(void)
{
	//Pending is cleared in first level of ISR handler
	if (!tSLInformation.ReadWlanInterruptPin())
	{
	    switch(sSpiInformation.ulSpiState)
	    {
	    case eSPI_STATE_POWERUP:
	        sSpiInformation.ulSpiState = eSPI_STATE_INITIALIZED;
	      break;
            case eSPI_STATE_IDLE:
                if (try_acquire_spi_bus(BUS_OWNER_CC3000)) {
                    SetState(eSPI_STATE_READ_IRQ, eAssert);
                    SpiIO(eRead, sSpiInformation.pRxPacket, arraySize(spi_readCommand), FALSE);
                } else {
                    SetState(eSPI_STATE_READ_PREP_IRQ, eNone);
                    WARN("CC3000 cannot lock bus - scheduling later (owner=%d)", current_bus_owner());
                }
		break;
            case eSPI_STATE_WRITE_WAIT_IRQ:
              sSpiInformation.ulSpiState = eSPI_STATE_WRITE_PROCEED;
              break;
            default:
              break;
	    }
	}
}


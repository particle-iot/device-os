/*
 * @brief LPC17xx/40xx General Purpose DMA driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights. NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __GPDMA_17XX_40XX_H_
#define __GPDMA_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup GPDMA_17XX_40XX CHIP: LPC17xx/40xx General Purpose DMA driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/**
 * @brief Number of channels on GPDMA
 */
#define GPDMA_NUMBER_CHANNELS 8
#if defined(CHIP_LPC177X_8X) || defined(CHIP_LPC407X_8X)
/**
 * @brief GPDMA request connections
 */
#define GPDMA_CONN_MEMORY           ((0UL))
#define GPDMA_CONN_SDC              ((1UL))			/*!< SD card */
#define GPDMA_CONN_SSP0_Tx          ((2UL))			/*!< SSP0 Tx */
#define GPDMA_CONN_SSP0_Rx          ((3UL))			/*!< SSP0 Rx */
#define GPDMA_CONN_SSP1_Tx          ((4UL))			/*!< SSP1 Tx */
#define GPDMA_CONN_SSP1_Rx          ((5UL))			/*!< SSP1 Rx */
#define GPDMA_CONN_SSP2_Tx          ((6UL))			/*!< SSP2 Tx */
#define GPDMA_CONN_SSP2_Rx          ((7UL))			/*!< SSP2 Rx */
#define GPDMA_CONN_ADC              ((8UL))			/*!< ADC */
#define GPDMA_CONN_DAC              ((9UL))			/*!< DAC */
#define GPDMA_CONN_UART0_Tx         ((10UL))		/*!< UART0 Tx */
#define GPDMA_CONN_UART0_Rx         ((11UL))		/*!< UART0 Rx */
#define GPDMA_CONN_UART1_Tx         ((12UL))		/*!< UART1 Tx */
#define GPDMA_CONN_UART1_Rx         ((13UL))		/*!< UART1 Rx */
#define GPDMA_CONN_UART2_Tx         ((14UL))		/*!< UART2 Tx */
#define GPDMA_CONN_UART2_Rx         ((15UL))		/*!< UART2 Rx */
#define GPDMA_CONN_MAT0_0           ((16UL))		/*!< MAT0.0 */
#define GPDMA_CONN_MAT0_1           ((17UL))		/*!< MAT0.1 */
#define GPDMA_CONN_MAT1_0           ((18UL))		/*!< MAT1.0 */
#define GPDMA_CONN_MAT1_1           ((19UL))		/*!< MAT1.1 */
#define GPDMA_CONN_MAT2_0           ((20UL))		/*!< MAT2.0 */
#define GPDMA_CONN_MAT2_1           ((21UL))		/*!< MAT2.1 */
#define GPDMA_CONN_I2S_Channel_0    ((22UL))		/*!< I2S channel 0 */
#define GPDMA_CONN_I2S_Channel_1    ((23UL))		/*!< I2S channel 1 */
#define GPDMA_CONN_UART3_Tx         ((26UL))		/*!< UART3 Tx */
#define GPDMA_CONN_UART3_Rx         ((27UL))		/*!< UART3 Rx */
#define GPDMA_CONN_UART4_Tx         ((28UL))		/*!< UART3 Tx */
#define GPDMA_CONN_UART4_Rx         ((29UL))		/*!< UART3 Rx */
#define GPDMA_CONN_MAT3_0           ((30UL))		/*!< MAT3.0 */
#define GPDMA_CONN_MAT3_1           ((31UL))		/*!< MAT3.1 */

#elif defined(CHIP_LPC175X_6X)
#define GPDMA_CONN_SSP0_Tx 			((0UL)) 		/**< SSP0 Tx */
#define GPDMA_CONN_SSP0_Rx 			((1UL)) 		/**< SSP0 Rx */
#define GPDMA_CONN_SSP1_Tx 			((2UL)) 		/**< SSP1 Tx */
#define GPDMA_CONN_SSP1_Rx 			((3UL)) 		/**< SSP1 Rx */
#define GPDMA_CONN_ADC 				((4UL)) 		/**< ADC */
#define GPDMA_CONN_I2S_Channel_0 	((5UL)) 		/**< I2S channel 0 */
#define GPDMA_CONN_I2S_Channel_1 	((6UL)) 		/**< I2S channel 1 */
#define GPDMA_CONN_DAC 				((7UL)) 		/**< DAC */
#define GPDMA_CONN_UART0_Tx			((8UL)) 		/**< UART0 Tx */
#define GPDMA_CONN_UART0_Rx			((9UL)) 		/**< UART0 Rx */
#define GPDMA_CONN_UART1_Tx			((10UL)) 		/**< UART1 Tx */
#define GPDMA_CONN_UART1_Rx			((11UL)) 		/**< UART1 Rx */
#define GPDMA_CONN_UART2_Tx			((12UL)) 		/**< UART2 Tx */
#define GPDMA_CONN_UART2_Rx			((13UL)) 		/**< UART2 Rx */
#define GPDMA_CONN_UART3_Tx			((14UL)) 		/**< UART3 Tx */
#define GPDMA_CONN_UART3_Rx			((15UL)) 		/**< UART3 Rx */
#define GPDMA_CONN_MAT0_0 			((16UL)) 		/**< MAT0.0 */
#define GPDMA_CONN_MAT0_1 			((17UL)) 		/**< MAT0.1 */
#define GPDMA_CONN_MAT1_0 			((18UL)) 		/**< MAT1.0 */
#define GPDMA_CONN_MAT1_1   		((19UL)) 		/**< MAT1.1 */
#define GPDMA_CONN_MAT2_0   		((20UL)) 		/**< MAT2.0 */
#define GPDMA_CONN_MAT2_1   		((21UL)) 		/**< MAT2.1 */
#define GPDMA_CONN_MAT3_0 			((22UL)) 		/**< MAT3.0 */
#define GPDMA_CONN_MAT3_1   		((23UL)) 		/**< MAT3.1 */
#define GPDMA_CONN_MEMORY 			((24UL))
#endif
/**
 * @brief GPDMA Burst size in Source and Destination definitions
 */
#define GPDMA_BSIZE_1   ((0UL))	/*!< Burst size = 1 */
#define GPDMA_BSIZE_4   ((1UL))	/*!< Burst size = 4 */
#define GPDMA_BSIZE_8   ((2UL))	/*!< Burst size = 8 */
#define GPDMA_BSIZE_16  ((3UL))	/*!< Burst size = 16 */
#define GPDMA_BSIZE_32  ((4UL))	/*!< Burst size = 32 */
#define GPDMA_BSIZE_64  ((5UL))	/*!< Burst size = 64 */
#define GPDMA_BSIZE_128 ((6UL))	/*!< Burst size = 128 */
#define GPDMA_BSIZE_256 ((7UL))	/*!< Burst size = 256 */

/**
 * @brief Width in Source transfer width and Destination transfer width definitions
 */
#define GPDMA_WIDTH_BYTE        ((0UL))	/*!< Width = 1 byte */
#define GPDMA_WIDTH_HALFWORD    ((1UL))	/*!< Width = 2 bytes */
#define GPDMA_WIDTH_WORD        ((2UL))	/*!< Width = 4 bytes */

/**
 * @brief Flow control definitions
 */
#define DMA_CONTROLLER 0		/*!< Flow control is DMA controller*/
#define SRC_PER_CONTROLLER 1	/*!< Flow control is Source peripheral controller*/
#define DST_PER_CONTROLLER 2	/*!< Flow control is Destination peripheral controller*/

/**
 * @brief DMA channel handle structure
 */
typedef struct {
	FunctionalState ChannelStatus;	/*!< DMA channel status */
} DMA_ChannelHandle_t;

/**
 * @brief Transfer Descriptor structure typedef
 */
typedef struct DMA_TransferDescriptor {
	uint32_t src;	/*!< Source address */
	uint32_t dst;	/*!< Destination address */
	uint32_t lli;	/*!< Pointer to next descriptor structure */
	uint32_t ctrl;	/*!< Control word that has transfer size, type etc. */
} DMA_TransferDescriptor_t;

/**
 * @brief	Read the status from different registers according to the type
 * @param	pGPDMA	: The base of GPDMA on the chip
 * @param	type	: Status mode, should be:
 *						- GPDMA_STAT_INT		: GPDMA Interrupt Status
 *						- GPDMA_STAT_INTTC		: GPDMA Interrupt Terminal Count Request Status
 *						- GPDMA_STAT_INTERR		: GPDMA Interrupt Error Status
 *						- GPDMA_STAT_RAWINTTC	: GPDMA Raw Interrupt Terminal Count Status
 *						- GPDMA_STAT_RAWINTERR	: GPDMA Raw Error Interrupt Status
 *						- GPDMA_STAT_ENABLED_CH	: GPDMA Enabled Channel Status
 * @param	channel	: The GPDMA channel : 0 - 7
 * @return	SET is interrupt is pending or RESET if not pending
 */
STATIC INLINE IntStatus Chip_GPDMA_IntGetStatus(LPC_GPDMA_T *pGPDMA, IP_GPDMA_STATUS_T type, uint8_t channel)
{
	return IP_GPDMA_IntGetStatus(pGPDMA, type, channel);
}

/**
 * @brief	Clear the Interrupt Flag from different registers according to the type
 * @param	pGPDMA	: The base of GPDMA on the chip
 * @param	type	: Flag mode, should be:
 *						- GPDMA_STATCLR_INTTC	: GPDMA Interrupt Terminal Count Request
 *						- GPDMA_STATCLR_INTERR	: GPDMA Interrupt Error
 * @param	channel	: The GPDMA channel : 0 - 7
 * @return	Nothing
 */
STATIC INLINE void Chip_GPDMA_ClearIntPending(LPC_GPDMA_T *pGPDMA, IP_GPDMA_STATECLEAR_T type, uint8_t channel)
{
	IP_GPDMA_ClearIntPending(pGPDMA, type, channel);
}

/**
 * @brief	Enable or Disable the GPDMA Channel
 * @param	pGPDMA		: The base of GPDMA on the chip
 * @param	channelNum	: The GPDMA channel : 0 - 7
 * @param	NewState	: ENABLE to enable GPDMA or DISABLE to disable GPDMA
 * @return	Nothing
 */
STATIC INLINE void Chip_GPDMA_ChannelCmd(LPC_GPDMA_T *pGPDMA, uint8_t channelNum, FunctionalState NewState)
{
	IP_GPDMA_ChannelCmd(pGPDMA, channelNum, NewState);
}

/**
 * @brief	Initialize the GPDMA
 * @param	pGPDMA	: The base of GPDMA on the chip
 * @return	Nothing
 */
void Chip_GPDMA_Init(LPC_GPDMA_T *pGPDMA);

/**
 * @brief	Shutdown the GPDMA
 * @param	pGPDMA	: The base of GPDMA on the chip
 * @return	Nothing
 */
void Chip_GPDMA_DeInit(LPC_GPDMA_T *pGPDMA);

/**
 * @brief	Stop a stream DMA transfer
 * @param	pGPDMA		: The base of GPDMA on the chip
 * @param	ChannelNum	: Channel Number to be closed
 * @return	Nothing
 */
void Chip_DMA_Stop(LPC_GPDMA_T *pGPDMA, uint8_t ChannelNum);

/**
 * @brief	The GPDMA stream interrupt status checking
 * @param	pGPDMA		: The base of GPDMA on the chip
 * @param	ChannelNum	: Channel Number to be checked on interruption
 * @return	Status:
 *              - SUCCESS	: DMA transfer success
 *              - ERROR		: DMA transfer failed
 */
Status Chip_DMA_Interrupt(LPC_GPDMA_T *pGPDMA, uint8_t ChannelNum);

/**
 * @brief	Get a free GPDMA channel for one DMA connection
 * @param	pGPDMA					: The base of GPDMA on the chip
 * @param	PeripheralConnection_ID	: Some chip fix each peripheral DMA connection on a specified channel ( have not used in 17xx/40xx )
 * @return	The channel number which is selected
 */
uint8_t Chip_DMA_GetFreeChannel(LPC_GPDMA_T *pGPDMA,
								uint32_t PeripheralConnection_ID);

/**
 * @brief	Do a DMA transfer M2M, M2P,P2M or P2P
 * @param	pGPDMA		: The base of GPDMA on the chip
 * @param	ChannelNum	: Channel used for transfer
 * @param	src			: Address of Memory or PeripheralConnection_ID which is the source
 * @param	dst			: Address of Memory or PeripheralConnection_ID which is the destination
 * @param	TransferType: Select the transfer controller and the type of transfer. Should be:
 *                               - GPDMA_TRANSFERTYPE_M2M_CONTROLLER_DMA
 *                               - GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA
 *                               - GPDMA_TRANSFERTYPE_P2M_CONTROLLER_DMA
 *                               - GPDMA_TRANSFERTYPE_P2P_CONTROLLER_DMA
 *                               - GPDMA_TRANSFERTYPE_P2P_CONTROLLER_DestPERIPHERAL
 *                               - GPDMA_TRANSFERTYPE_M2P_CONTROLLER_PERIPHERAL
 *                               - GPDMA_TRANSFERTYPE_P2M_CONTROLLER_PERIPHERAL
 *                               - GPDMA_TRANSFERTYPE_P2P_CONTROLLER_SrcPERIPHERAL
 * @param	Size		: The number of DMA transfers
 * @return	ERROR on error, SUCCESS on success
 */
Status Chip_DMA_Transfer(LPC_GPDMA_T *pGPDMA,
						 uint8_t ChannelNum,
						 uint32_t src,
						 uint32_t dst,
						 IP_GPDMA_FLOW_CONTROL_T TransferType,
						 uint32_t Size);

/**
 * @brief	Do a DMA transfer using linked list of descriptors
 * @param	pGPDMA			: The base of GPDMA on the chip
 * @param	ChannelNum		: Channel used for transfer *must be obtained using Chip_DMA_GetFreeChannel()*
 * @param	DMADescriptor	: First node in the linked list of descriptors
 * @param	TransferType	: Select the transfer controller and the type of transfer. (See, #IP_GPDMA_FLOW_CONTROL_T)
 * @return	ERROR on error, SUCCESS on success
 */
Status Chip_DMA_SGTransfer(LPC_GPDMA_T *pGPDMA,
						   uint8_t ChannelNum,
						   const DMA_TransferDescriptor_t *DMADescriptor,
						   IP_GPDMA_FLOW_CONTROL_T TransferType);

/**
 * @brief	Prepare a single DMA descriptor
 * @param	pGPDMA			: The base of GPDMA on the chip
 * @param	DMADescriptor	: DMA Descriptor to be initialized
 * @param	src				: Address of Memory or one of @link #GPDMA_CONN_MEMORY
 *                              PeripheralConnection_ID @endlink, which is the source
 * @param	dst				: Address of Memory or one of @link #GPDMA_CONN_MEMORY
 *                              PeripheralConnection_ID @endlink, which is the destination
 * @param	Size			: The number of DMA transfers
 * @param	TransferType	: Select the transfer controller and the type of transfer. (See, #IP_GPDMA_FLOW_CONTROL_T)
 * @param	NextDescriptor	: Pointer to next descriptor (0 if no more descriptors available)
 * @return	ERROR on error, SUCCESS on success
 */
Status Chip_DMA_PrepareDescriptor(LPC_GPDMA_T *pGPDMA,
								  DMA_TransferDescriptor_t *DMADescriptor,
								  uint32_t src,
								  uint32_t dst,
								  uint32_t Size,
								  IP_GPDMA_FLOW_CONTROL_T TransferType,
								  const DMA_TransferDescriptor_t *NextDescriptor);

/**
 * @brief	Initialize channel configuration strucutre
 * @param	pGPDMA			: The base of GPDMA on the chip
 * @param	GPDMACfg		: Pointer to configuration structure to be initialized
 * @param	ChannelNum		: Channel used for transfer *must be obtained using Chip_DMA_GetFreeChannel()*
 * @param	src				: Address of Memory or one of @link #GPDMA_CONN_MEMORY
 *                              PeripheralConnection_ID @endlink, which is the source
 * @param	dst				: Address of Memory or one of @link #GPDMA_CONN_MEMORY
 *                              PeripheralConnection_ID @endlink, which is the destination
 * @param	Size			: The number of DMA transfers
 * @param	TransferType	: Select the transfer controller and the type of transfer. (See, #IP_GPDMA_FLOW_CONTROL_T)
 * @return	ERROR on error, SUCCESS on success
 */
int Chip_DMA_InitChannelCfg(LPC_GPDMA_T *pGPDMA,
							GPDMA_Channel_CFG_T *GPDMACfg,
							uint8_t  ChannelNum,
							uint32_t src,
							uint32_t dst,
							uint32_t Size,
							IP_GPDMA_FLOW_CONTROL_T TransferType);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __GPDMA_17XX_40XX_H_ */

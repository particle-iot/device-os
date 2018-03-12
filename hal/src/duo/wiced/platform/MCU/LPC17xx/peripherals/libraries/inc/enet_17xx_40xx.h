/*
 * @brief LPC17xx/40xx ethernet driver
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
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
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

#ifndef __ENET_17XX_40XX_H_
#define __ENET_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ENET_17XX_40XX CHIP: LPC17xx/40xx Ethernet driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

/** @defgroup ENET_17XX_40XX_OPTIONS CHIP: LPC17xx/40xx Ethernet driver build options
 * @ingroup CHIP_17XX_40XX_DRIVER_OPTIONS ENET_17XX_40XX
 * The ethernet driver has options that configure it's operation at build-time.  A
 * build-time option is configured by the use of a definition passed to the compiler
 * during the build process or by adding the definition to the sys_config.h file.<br>
 *
 * <b>USE_RMII</b><br>
 * When defined, the driver will be built for RMII operation.<br>
 * When not defined, the driver will be built for MII operation.<br>
 * @{
 */

/**
 * @}
 */

typedef IP_ENET_002_TXDESC_T     ENET_TXDESC_T;
typedef IP_ENET_002_TXSTAT_T     ENET_TXSTAT_T;
typedef IP_ENET_002_RXDESC_T     ENET_RXDESC_T;
typedef IP_ENET_002_RXSTAT_T     ENET_RXSTAT_T;
typedef IP_ENET_002_BUFF_STATUS_T     ENET_BUFF_STATUS;

/**
 * @brief	Resets the ethernet interface
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 * @note	Resets the ethernet interface. This should be called prior to
 * Chip_ENET_Init with a small delay after this call.
 */
STATIC INLINE void Chip_ENET_Reset(LPC_ENET_T *pENET)
{
	IP_ENET_Reset(pENET);
}

/**
 * @brief	Sets the address of the interface
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	macAddr	: Pointer to the 6 bytes used for the MAC address
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_SetADDR(LPC_ENET_T *pENET, const uint8_t *macAddr)
{
	IP_ENET_SetADDR(pENET, macAddr);
}

/**
 * @brief	Sets up the PHY link clock divider and PHY address
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	div		: Divider index, not a divider value, see user manual
 * @param	addr	: PHY address, used with MII read and write
 * @return	Nothing
 * @note	The MII clock divider rate is divided from the peripheral clock returned
 * from the Chip_Clock_GetSystemClockRate() function. Use Chip_ENET_FindMIIDiv()
 * with a desired clock rate to find the correct divider index value.
 */
STATIC INLINE void Chip_ENET_SetupMII(LPC_ENET_T *pENET, uint32_t div, uint8_t addr)
{
	IP_ENET_SetupMII(pENET, (uint8_t)div, addr);
}

/**
 * @brief	Starts a PHY write via the MII
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	reg		: PHY register to write
 * @param	data	: Data to write to PHY register
 * @return	Nothing
 * @note	Start a PHY write operation. Does not block, requires calling
 * IP_ENET_IsMIIBusy to determine when write is complete.
 */
STATIC INLINE void Chip_ENET_StartMIIWrite(LPC_ENET_T *pENET, uint8_t reg, uint16_t data)
{
	IP_ENET_StartMIIWrite(pENET, reg, data);
}

/**
 * @brief	Starts a PHY read via the MII
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	reg		: PHY register to read
 * @return	Nothing
 * @note	Start a PHY read operation. Does not block, requires calling
 * IP_ENET_IsMIIBusy to determine when read is complete and calling
 * IP_ENET_ReadMIIData to get the data.
 */
STATIC INLINE void Chip_ENET_StartMIIRead(LPC_ENET_T *pENET, uint8_t reg)
{
	IP_ENET_StartMIIRead(pENET, reg);
}

/**
 * @brief	Returns MII link (PHY) busy status
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Returns true if busy, otherwise false
 */
STATIC INLINE bool Chip_ENET_IsMIIBusy(LPC_ENET_T *pENET)
{
	return IP_ENET_IsMIIBusy(pENET);
}

/**
 * @brief	Returns the value read from the PHY
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Read value from PHY
 */
STATIC INLINE uint16_t Chip_ENET_ReadMIIData(LPC_ENET_T *pENET)
{
	return IP_ENET_ReadMIIData(pENET);
}

/**
 * @brief	Enables ethernet transmit
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_TXEnable(LPC_ENET_T *pENET)
{
	IP_ENET_TXEnable(pENET);
}

/**
 * @brief Disables ethernet transmit
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_TXDisable(LPC_ENET_T *pENET)
{
	IP_ENET_TXDisable(pENET);
}

/**
 * @brief	Enables ethernet packet reception
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_RXEnable(LPC_ENET_T *pENET)
{
	IP_ENET_RXEnable(pENET);
}

/**
 * @brief	Disables ethernet packet reception
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_RXDisable(LPC_ENET_T *pENET)
{
	IP_ENET_RXDisable(pENET);
}

/**
 * @brief	Reset Tx Logic
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_ResetTXLogic(LPC_ENET_T *pENET)
{
	IP_ENET_ResetTXLogic(pENET);
}

/**
 * @brief	Reset Rx Logic
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_ResetRXLogic(LPC_ENET_T *pENET)
{
	IP_ENET_ResetRXLogic(pENET);
}

/**
 * @brief	Enable Rx Filter
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	mask	: Filter mask (Or-ed bit values of ENET_RXFILTERCTRL_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_EnableRXFilter(LPC_ENET_T *pENET, uint32_t mask)
{
	IP_ENET_EnableRXFilter(pENET, mask);
}

/**
 * @brief	Disable Rx Filter
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	mask	: Filter mask (Or-ed bit values of ENET_RXFILTERCTRL_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_DisableRXFilter(LPC_ENET_T *pENET, uint32_t mask)
{
	IP_ENET_DisableRXFilter(pENET, mask);
}

/**
 * @brief	Sets full duplex operation for the interface
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_SetFullDuplex(LPC_ENET_T *pENET)
{
	IP_ENET_SetFullDuplex(pENET);
}

/**
 * @brief	Sets half duplex operation for the interface
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_SetHalfDuplex(LPC_ENET_T *pENET)
{
	IP_ENET_SetHalfDuplex(pENET);
}

/**
 * @brief	Selects 100Mbps for the current speed
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_Set100Mbps(LPC_ENET_T *pENET)
{
	IP_ENET_Set100Mbps(pENET);
}

/**
 * @brief	Selects 10Mbps for the current speed
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_Set10Mbps(LPC_ENET_T *pENET)
{
	IP_ENET_Set10Mbps(pENET);
}

/**
 * @brief	Configures the initial ethernet transmit descriptors
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	pDescs	: Pointer to TX descriptor list
 * @param	pStatus	: Pointer to TX status list
 * @param	descNum	: the number of desciptors
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_InitTxDescriptors(LPC_ENET_T *pENET, ENET_TXDESC_T *pDescs,
											   ENET_TXSTAT_T *pStatus,
											   uint32_t descNum)
{
	IP_ENET_InitTxDescriptors(pENET, pDescs, pStatus, descNum);
}

/**
 * @brief	Configures the initial ethernet receive descriptors
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	pDescs	: Pointer to TX descriptor list
 * @param	pStatus	: Pointer to TX status list
 * @param	descNum	: the number of desciptors
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_InitRxDescriptors(LPC_ENET_T *pENET, ENET_RXDESC_T *pDescs,
											   ENET_RXSTAT_T *pStatus,
											   uint32_t descNum)
{
	IP_ENET_InitRxDescriptors(pENET, pDescs, pStatus, descNum);
}

/**
 * @brief	Get the current Tx Produce Descriptor Index
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Tx Produce Index
 */
STATIC INLINE uint16_t Chip_ENET_GetTXProduceIndex(LPC_ENET_T *pENET)
{
	return IP_ENET_GetTXProduceIndex(pENET);
}

/**
 * @brief	Get the current Tx Consume Descriptor Index
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Tx Consume Index
 */
STATIC INLINE uint16_t Chip_ENET_GetTXConsumeIndex(LPC_ENET_T *pENET)
{
	return IP_ENET_GetTXConsumeIndex(pENET);
}

/**
 * @brief	Get the current Rx Produce Descriptor Index
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Rx Produce Index
 */
STATIC INLINE uint16_t Chip_ENET_GetRXProduceIndex(LPC_ENET_T *pENET)
{
	return IP_ENET_GetRXProduceIndex(pENET);
}

/**
 * @brief	Get the current Rx Consume Descriptor Index
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Rx Consume Index
 */
STATIC INLINE uint16_t Chip_ENET_GetRXConsumeIndex(LPC_ENET_T *pENET)
{
	return IP_ENET_GetRXConsumeIndex(pENET);
}

/**
 * @brief	Get the buffer status with the current Produce Index and Consume Index
 * @param	pENET			: The base of ENET peripheral on the chip
 * @param	produceIndex	: Produce Index
 * @param	consumeIndex	: Consume Index
 * @param	buffSize		: Buffer size
 * @return	Status (One of status value: ENET_BUFF_EMPTY/ENET_BUFF_FULL/ENET_BUFF_PARTIAL_FULL)
 */
STATIC INLINE ENET_BUFF_STATUS Chip_ENET_GetBufferStatus(LPC_ENET_T *pENET, uint16_t produceIndex,
														 uint16_t consumeIndex,
														 uint16_t buffSize)
{
    ( (void)(pENET) );
	return IP_ENET_GetBufferStatus(produceIndex, consumeIndex, buffSize);
}

/**
 * @brief	Get the number of descriptors filled
 * @param	pENET			: The base of ENET peripheral on the chip
 * @param	produceIndex	: Produce Index
 * @param	consumeIndex	: Consume Index
 * @param	buffSize		: Buffer size
 * @return	the number of descriptors
 */
STATIC INLINE uint32_t Chip_ENET_GetFillDescNum(LPC_ENET_T *pENET, uint16_t produceIndex, uint16_t consumeIndex, uint16_t buffSize)
{
    ( (void)(pENET) );
	return IP_ENET_GetFillDescNum(produceIndex, consumeIndex, buffSize);
}

/**
 * @brief	Get the number of free descriptors
 * @param	pENET			: The base of ENET peripheral on the chip
 * @param	produceIndex	: Produce Index
 * @param	consumeIndex	: Consume Index
 * @param	buffSize		: Buffer size
 * @return	the number of descriptors
 */
STATIC INLINE uint32_t Chip_ENET_GetFreeDescNum(LPC_ENET_T *pENET, uint16_t produceIndex, uint16_t consumeIndex, uint16_t buffSize)
{
    ( (void)(pENET) );
	return IP_ENET_GetFreeDescNum(produceIndex, consumeIndex, buffSize);
}

/**
 * @brief	Check if Tx buffer is full
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	true/false
 */
STATIC INLINE bool Chip_ENET_IsTxFull(LPC_ENET_T *pENET)
{
	return IP_ENET_IsTxFull(pENET);
}

/**
 * @brief	Check if Rx buffer is empty
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	true/false
 */
STATIC INLINE bool Chip_ENET_IsRxEmpty(LPC_ENET_T *pENET)
{
	return IP_ENET_IsRxEmpty(pENET);
}

/**
 * @brief	Increase the current Tx Produce Descriptor Index
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	The new index value
 */
STATIC INLINE uint16_t Chip_ENET_IncTXProduceIndex(LPC_ENET_T *pENET)
{
	return IP_ENET_IncTXProduceIndex(pENET);
}

/**
 * @brief	Increase the current Rx Consume Descriptor Index
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	The new index value
 */
STATIC INLINE uint16_t Chip_ENET_IncRXConsumeIndex(LPC_ENET_T *pENET)
{
	return IP_ENET_IncRXConsumeIndex(pENET);
}

/**
 * @brief	Enable ENET interrupts
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	mask	: Interrupt mask  (Or-ed bit values of ENET_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_EnableInt(LPC_ENET_T *pENET, uint32_t mask)
{
	IP_ENET_EnableInt(pENET, mask);
}

/**
 * @brief	Disable ENET interrupts
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	mask	: Interrupt mask  (Or-ed bit values of ENET_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_DisableInt(LPC_ENET_T *pENET, uint32_t mask)
{
	IP_ENET_DisableInt(pENET, mask);
}

/**
 * @brief	Get the interrupt status
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	interrupt status (Or-ed bit values of ENET_INT_*)
 */
STATIC INLINE uint32_t Chip_ENET_GetIntStatus(LPC_ENET_T *pENET)
{
	return IP_ENET_GetIntStatus(pENET);
}

/**
 * @brief	Clear the interrupt status
 * @param	pENET	: The base of ENET peripheral on the chip
 * @param	mask	: Interrupt mask  (Or-ed bit values of ENET_INT_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_ENET_ClearIntStatus(LPC_ENET_T *pENET, uint32_t mask)
{
	IP_ENET_ClearIntStatus(pENET, mask);
}

/**
 * @brief	Initialize ethernet interface
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 * @note	Performs basic initialization of the ethernet interface in a default
 * state. This is enough to place the interface in a usable state, but
 * may require more setup outside this function.
 */
void Chip_ENET_Init(LPC_ENET_T *pENET);

/**
 * @brief	De-initialize the ethernet interface
 * @param	pENET	: The base of ENET peripheral on the chip
 * @return	Nothing
 */
void Chip_ENET_DeInit(LPC_ENET_T *pENET);

/**
 * @brief	Find the divider index for a desired MII clock rate
 * @param	pENET		: The base of ENET peripheral on the chip
 * @param	clockRate	: Clock rate to get divider index for
 * @return	MII divider index to get the closest clock rate for clockRate
 * @note	Use this function to get a divider index for the Chip_ENET_SetupMII()
 * function determined from the desired MII clock rate.
 */
uint32_t Chip_ENET_FindMIIDiv(LPC_ENET_T *pENET, uint32_t clockRate);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ENET_17XX_40XX_H_ */

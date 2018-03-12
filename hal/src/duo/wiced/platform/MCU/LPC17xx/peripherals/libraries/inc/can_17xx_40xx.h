/*
 * @brief LPC17xx/40xx CAN driver
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

#ifndef __CAN_17XX_40XX_H_
#define __CAN_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup CAN_17XX_40XX CHIP: LPC17xx/40xx CAN driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

#define CAN_SEG1_DEFAULT_VAL 5
#define CAN_SEG2_DEFAULT_VAL 4
#define CAN_SJW_DEFAULT_VAL 0

/**
 * @brief CAN structures and definitions
 */
/** AF Look-up Tables structure */
typedef IP_CAN_AF_LUT_T      CANAF_LUT_T;
/** Standard Identifier Entry Structure*/
typedef IP_CAN_STD_ID_Entry_T    CAN_STD_ID_ENTRY_T;
/** Standard Identifier Range Entry Structure*/
typedef IP_CAN_STD_ID_RANGE_Entry_T  CAN_STD_ID_RANGE_ENTRY_T;
/** Extended Identifier Entry Structure*/
typedef IP_CAN_EXT_ID_Entry_T    CAN_EXT_ID_ENTRY_T;
/** Extended Identifier Range Entry Structure*/
typedef IP_CAN_EXT_ID_RANGE_Entry_T  CAN_EXT_ID_RANGE_ENTRY_T;
/** CAN Message Structure*/
typedef IP_CAN_MSG_T             CAN_MSG_T;
/** CAN Buffer ID definition*/
typedef IP_CAN_BUFFER_ID_T            CAN_BUFFER_ID;
/** CAN Look-up Table Definition*/
typedef IP_CAN_AF_RAM_SECTION_T        CANAF_RAM_SECTION;

/**
 * @brief CAN Mode definition
 */
typedef enum CHIP_CAN_MODE {
	CAN_RESET_MODE = CAN_MOD_RM,				/*!< CAN Reset Mode */
	CAN_SELFTEST_MODE = CAN_MOD_STM,			/*!< CAN Selftest Mode */
	CAN_TEST_MODE = CAN_MOD_TM,					/*!< CAN Test Mode */
	CAN_LISTEN_ONLY_MODE = CAN_MOD_LOM,			/*!< CAN Listen Only Mode */
	CAN_SLEEP_MODE = CAN_MOD_SM,				/*!< CAN Sleep Mode */
	CAN_OPERATION_MODE = CAN_MOD_OPERATION,		/*!< CAN Operation Mode */
	CAN_TRANSMIT_PRIORITY_MODE = CAN_MOD_TPM,	/*!< CAN Transmit Priority Mode */
	CAN_RECEIVE_PRIORITY_MODE = CAN_MOD_RPM,	/*!< CAN Receive Priority Mode */
} CHIP_CAN_MODE_T;

/**
 * @brief CAN AF Mode definition
 */
typedef enum CHIP_CAN_AF_MODE {
	CAN_AF_NORMAL_MODE = 0,					/*!< Acceptance Filter Normal Mode */
	CAN_AF_OFF_MODE = CANAF_AFMR_ACCOFF,	/*!< Acceptance Filter Off Mode */
	CAN_AF_BYBASS_MODE = CANAF_AFMR_ACCBP,	/*!< Acceptance Fileter Bypass Mode */
	CAN_AF_FULL_MODE = CANAF_AFMR_EFCAN,	/*!< FullCAN Mode Enhancement */
} CHIP_CAN_AF_MODE_T;

/**
 * @brief	Get the status of the CAN Controller
 * @param	pCAN	: Pointer to CAN controller register block
 * @return	Status (Or'ed bit values of CAN_SR_*(n) with n = CAN_BUFFER_1/2/3).
 */
STATIC INLINE uint32_t Chip_CAN_GetStatus(LPC_CAN_T *pCAN)
{
	return IP_CAN_GetStatus(pCAN);
}

/**
 * @brief	Enable the CAN Interrupts
 * @param	pCAN	: Pointer to CAN controller register block
 * @param	IntMask	: Interrupt Mask (Or-ed bits value of CAN_IER_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_CAN_IntEnable(LPC_CAN_T *pCAN, uint32_t IntMask)
{
	IP_CAN_IntEnable(pCAN, IntMask);
}

/**
 * @brief	Disable the CAN Interrupts
 * @param	pCAN	: Pointer to CAN controller register block
 * @param	IntMask	: Interrupt Mask (Or-ed bits value of CAN_IER_*)
 * @return	Nothing
 */
STATIC INLINE void Chip_CAN_IntDisable(LPC_CAN_T *pCAN, uint32_t IntMask)
{
	IP_CAN_IntDisable(pCAN, IntMask);
}

/**
 * @brief	Get interrupt status of the given CAN Controller
 * @param	pCAN	: Pointer to CAN controller register block
 * @return	Status (Or'ed bit values of CAN_ICR_* )
 */
STATIC INLINE uint32_t Chip_CAN_GetIntStatus(LPC_CAN_T *pCAN)
{
	return IP_CAN_GetIntStatus(pCAN);
}

/**
 * @brief	Enable/Disable CAN controller FullCAN Interrupts
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	NewState	: Enable/Disable
 * @return	Nothing
 */
STATIC INLINE void Chip_CAN_FullCANIntConfig(LPC_CANAF_T *pCANAF, FunctionalState NewState)
{
	IP_CAN_FullCANIntConfig(pCANAF, NewState);
}

/**
 * @brief	Get FullCAN interrupt status of the given object
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	ObjID	: Object ID
 * @return  Status
 */
STATIC INLINE uint32_t Chip_CAN_GetFullCANIntStatus(LPC_CANAF_T *pCANAF, uint8_t ObjID)
{
	return IP_CAN_GetFullCANIntStatus(pCANAF, ObjID);
}

/**
 * @brief	Set CAN controller enter/exit to a given mode
 * @param	pCAN		: Pointer to CAN controller register block
 * @param	Mode		: Mode selected
 * @param	NewState	: ENABLE: enter, DISABLE: exit
 * @return	None
 */
STATIC INLINE void Chip_CAN_SetMode(LPC_CAN_T *pCAN, CHIP_CAN_MODE_T Mode, FunctionalState NewState)
{
	IP_CAN_SetMode(pCAN, Mode, NewState);
}

/**
 * @brief	Set CAN AF Mode
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	AfMode	: Mode selected
 * @return	None
 */
STATIC INLINE void Chip_CAN_SetAFMode(LPC_CANAF_T *pCANAF, CHIP_CAN_AF_MODE_T AfMode)
{
	IP_CAN_AF_SetMode(pCANAF, AfMode);
}

/**
 * @brief	Set CAN AF LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	pAFSections	: Pointer to buffer storing AF Section Data
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_SetAFLUT(LPC_CANAF_T *pCANAF, CANAF_LUT_T *pAFSections)
{
	return IP_CAN_SetAFLUT(pCANAF, LPC_CANAF_RAM, pAFSections);
}

/**
 * @brief	Insert a FullCAN Entry into the current LUT
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	pEntry	: Pointer to the entry which will be inserted
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_InsertFullCANEntry(LPC_CANAF_T *pCANAF, CAN_STD_ID_ENTRY_T *pEntry)
{
	return IP_CAN_InsertFullCANEntry(pCANAF, LPC_CANAF_RAM, pEntry);
}

/**
 * @brief	Insert an individual Standard Entry into the current LUT
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	pEntry	: Pointer to the entry which will be inserted
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_InsertIndividualSTDEntry(LPC_CANAF_T *pCANAF, CAN_STD_ID_ENTRY_T *pEntry)
{
	return IP_CAN_InsertIndividualSTDEntry(pCANAF, LPC_CANAF_RAM, pEntry);
}

/**
 * @brief	Insert an Group Standard Entry into the current LUT
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	pEntry	: Pointer to the entry which will be inserted
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_InsertGroupSTDEntry(LPC_CANAF_T *pCANAF, CAN_STD_ID_RANGE_ENTRY_T *pEntry)
{
	return IP_CAN_InsertGroupSTDEntry(pCANAF, LPC_CANAF_RAM, pEntry);
}

/**
 * @brief	Insert an individual Extended Entry into the current LUT
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	pEntry	: Pointer to the entry which will be inserted
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_InsertIndividualEXTEntry(LPC_CANAF_T *pCANAF, CAN_EXT_ID_ENTRY_T *pEntry)
{
	return IP_CAN_InsertIndividualEXTEntry(pCANAF, LPC_CANAF_RAM, pEntry);
}

/**
 * @brief	Insert an Group Extended Entry into the current LUT
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	pEntry	: Pointer to the entry which will be inserted
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_InsertGroupEXTEntry(LPC_CANAF_T *pCANAF, CAN_EXT_ID_RANGE_ENTRY_T *pEntry)
{
	return IP_CAN_InsertGroupEXTEntry(pCANAF, LPC_CANAF_RAM, pEntry);
}

/**
 * @brief	Remove a FullCAN Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry removed
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_RemoveFullCANEntry(LPC_CANAF_T *pCANAF, int16_t Position)
{
	return IP_CAN_RemoveFullCANEntry(pCANAF, LPC_CANAF_RAM, Position);
}

/**
 * @brief	Remove an individual Standard Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry removed
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_RemoveIndividualSTDEntry(LPC_CANAF_T *pCANAF, int16_t Position)
{
	return IP_CAN_RemoveIndividualSTDEntry(pCANAF, LPC_CANAF_RAM, Position);
}

/**
 * @brief	Remove an Group Standard Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry removed
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_RemoveGroupSTDEntry(LPC_CANAF_T *pCANAF, int16_t Position)
{
	return IP_CAN_RemoveGroupSTDEntry(pCANAF, LPC_CANAF_RAM, Position);
}

/**
 * @brief	Remove an individual Extended  Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry removed
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_RemoveIndividualEXTEntry(LPC_CANAF_T *pCANAF, int16_t Position)
{
	return IP_CAN_RemoveIndividualEXTEntry(pCANAF, LPC_CANAF_RAM, Position);
}

/**
 * @brief	Remove an Group Extended  Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry removed
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_RemoveGroupEXTEntry(LPC_CANAF_T *pCANAF, int16_t Position)
{
	return IP_CAN_RemoveGroupEXTEntry(pCANAF, LPC_CANAF_RAM, Position);
}

/**
 * @brief	Get the number of entries in the given section
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	SectionID	: Section ID
 * @return	Number of entries
 */
STATIC INLINE uint16_t Chip_CAN_GetEntriesNum(LPC_CANAF_T *pCANAF, CANAF_RAM_SECTION SectionID)
{
	return IP_CAN_GetEntriesNum(pCANAF, LPC_CANAF_RAM, SectionID);
}

/**
 * @brief	Read a FullCAN Entry into from current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	:  Position of the entry in the given section (started from 0)
 * @param	pEntry		:  Pointer to the entry which will be read
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_ReadFullCANEntry(LPC_CANAF_T *pCANAF, uint16_t Position,
											   CAN_STD_ID_ENTRY_T *pEntry)
{
	return IP_CAN_ReadFullCANEntry(pCANAF, LPC_CANAF_RAM, Position, pEntry);
}

/**
 * @brief	Read an individual Standard Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	:  Position of the entry in the given section (started from 0)
 * @param	pEntry		:  Pointer to the entry which will be read
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_ReadIndividualSTDEntry(LPC_CANAF_T *pCANAF, uint16_t Position,
													 CAN_STD_ID_ENTRY_T *pEntry)
{
	return IP_CAN_ReadIndividualSTDEntry(pCANAF, LPC_CANAF_RAM, Position, pEntry);
}

/**
 * @brief	Read an Group Standard Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry in the given section (started from 0)
 * @param	pEntry		: Pointer to the entry which will be read
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_ReadGroupSTDEntry(LPC_CANAF_T *pCANAF, uint16_t Position,
												CAN_STD_ID_RANGE_ENTRY_T *pEntry)
{
	return IP_CAN_ReadGroupSTDEntry(pCANAF, LPC_CANAF_RAM, Position, pEntry);
}

/**
 * @brief	Read an individual Extended Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry in the given section (started from 0)
 * @param	pEntry		: Pointer to the entry which will be read
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_ReadIndividualEXTEntry(LPC_CANAF_T *pCANAF, uint16_t Position,
													 CAN_EXT_ID_ENTRY_T *pEntry)
{
	return IP_CAN_ReadIndividualEXTEntry(pCANAF, LPC_CANAF_RAM, Position, pEntry);
}

/**
 * @brief	Read an Group Extended Entry from the current LUT
 * @param	pCANAF		: Pointer to CAN AF Register block
 * @param	Position	: Position of the entry in the given section (started from 0)
 * @param	pEntry		: Pointer to the entry which will be read
 * @return	SUCCESS/ERROR
 */
STATIC INLINE Status Chip_CAN_ReadGroupEXTEntry(LPC_CANAF_T *pCANAF, uint16_t Position,
												CAN_EXT_ID_RANGE_ENTRY_T *pEntry)
{
	return IP_CAN_ReadGroupEXTEntry(pCANAF, LPC_CANAF_RAM, Position, pEntry);
}

/**
 * @brief	Get message received by the CAN Controller
 * @param	pCAN	: Pointer to CAN controller register block
 * @param	pMsg	: Pointer to the buffer storing the information of the received message
 * @return	SUCCESS (message information saved) or ERROR (no message received)
 */
STATIC INLINE Status Chip_CAN_Receive(LPC_CAN_T *pCAN, CAN_MSG_T *pMsg)
{
	return IP_CAN_Receive(pCAN, pMsg);
}

/**
 * @brief	Get message received automatically by the AF
 * @param	pCANAF	: Pointer to CAN AF Register block
 * @param	ObjID	: Object ID
 * @param	pMsg	: Pointer to the buffer storing the information of the received message
 * @param	pSCC	: Pointer to the buffer storing the controller ID of the received message
 * @return	SUCCESS (message information saved) or ERROR (no message received)
 */
STATIC INLINE Status Chip_CAN_FullCANReceive(LPC_CANAF_T *pCANAF, uint8_t ObjID, CAN_MSG_T *pMsg, uint8_t *pSCC)
{
	return IP_CAN_FullCANReceive(pCANAF, LPC_CANAF_RAM, ObjID, pMsg, pSCC);
}

/**
 * @brief	Request the given CAN Controller to send message
 * @param	pCAN	: Pointer to CAN controller register block
 * @param	TxBufID	: ID of the buffer which will be used for transmission
 * @param	pMsg	: Pointer to the buffer of message which will be sent
 * @return	SUCCESS (message information saved) or ERROR (no message received)
 */
STATIC INLINE Status Chip_CAN_Send(LPC_CAN_T *pCAN, CAN_BUFFER_ID TxBufID, CAN_MSG_T *pMsg)
{
	return IP_CAN_Send(pCAN, TxBufID, pMsg);
}

/**
 * @brief	Initialize CAN Interface
 * @param	pCAN	: Pointer to CAN controller register block
 * @return	Nothing
 */
void Chip_CAN_Init(LPC_CAN_T *pCAN);

/**
 * @brief	De-Initialize CAN Interface
 * @param	pCAN	: Pointer to CAN controller register block
 * @return	Nothing
 */
void Chip_CAN_DeInit(LPC_CAN_T *pCAN);

/**
 * @brief	Set CAN bitrate
 * @param	pCAN	: Pointer to CAN controller register block
 * @param	BitRate	: Expected bitrate
 * @return	SUCCESS/ERROR
 */
Status Chip_CAN_SetBitRate(LPC_CAN_T *pCAN, uint32_t BitRate);

/**
 * @brief	Get Free TxBuffer
 * @param	pCAN	: Pointer to CAN controller register block
 * @return	Buffer ID
 */
CAN_BUFFER_ID Chip_CAN_GetFreeTxBuf(LPC_CAN_T *pCAN);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_17XX_40XX_H_ */

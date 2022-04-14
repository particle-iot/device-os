/*
 * @brief LPC17xx/40xx ADC conversion driver
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

#ifndef __ADC_17XX_40XX_H_
#define __ADC_17XX_40XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ADC_17XX_40XX CHIP: LPC17xx/40xx ADC conversion driver
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */
#if defined(CHIP_LPC175X_6X)
#define ADC_MAX_SAMPLE_RATE 200000
#else
#define ADC_MAX_SAMPLE_RATE 400000
#endif

/**
 * @brief The channels on one ADC peripheral
 */
typedef enum CHIP_ADC_CHANNEL {
	ADC_CH0 = 0,	/*!< ADC channel 0 */
	ADC_CH1,		/*!< ADC channel 1 */
	ADC_CH2,		/*!< ADC channel 2 */
	ADC_CH3,		/*!< ADC channel 3 */
	ADC_CH4,		/*!< ADC channel 4 */
	ADC_CH5,		/*!< ADC channel 5 */
	ADC_CH6,		/*!< ADC channel 6 */
	ADC_CH7,		/*!< ADC channel 7 */
} CHIP_ADC_CHANNEL_T;

/**
 * @brief Edge configuration, which controls rising or falling edge on the selected signal for the start of a conversion
 */
typedef enum CHIP_ADC_EDGE_CFG {
	ADC_TRIGGERMODE_RISING = 0,		/*!< Trigger event: rising edge */
	ADC_TRIGGERMODE_FALLING,		/*!< Trigger event: falling edge */
} CHIP_ADC_EDGE_CFG_T;

/**
 * @brief Start mode, which controls the start of an A/D conversion when the BURST bit is 0
 */
typedef enum CHIP_ADC_START_MODE {
	ADC_NO_START = 0,
	ADC_START_NOW,			/*!< Start conversion now */
	ADC_START_ON_CTOUT15,	/*!< Start conversion when the edge selected by bit 27 occurs on CTOUT_15 */
	ADC_START_ON_CTOUT8,	/*!< Start conversion when the edge selected by bit 27 occurs on CTOUT_8 */
	ADC_START_ON_ADCTRIG0,	/*!< Start conversion when the edge selected by bit 27 occurs on ADCTRIG0 */
	ADC_START_ON_ADCTRIG1,	/*!< Start conversion when the edge selected by bit 27 occurs on ADCTRIG1 */
	ADC_START_ON_MCOA2		/*!< Start conversion when the edge selected by bit 27 occurs on Motocon PWM output MCOA2 */
} CHIP_ADC_START_MODE_T;

/**
 * @brief Clock setup structure for ADC controller passed to the initialize function
 */
typedef struct {
	uint32_t adcRate;		/*!< ADC rate */
	uint8_t  bitsAccuracy;	/*!< ADC bit accuracy */
	bool	 burstMode;		/*!< ADC Burt Mode */
} ADC_Clock_Setup_T;

/**
 * @brief	Read the ADC value from a channel
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	channel		: ADC channel to read
 * @param	data		: Pointer to where to put data
 * @return	SUCCESS or ERROR if no conversion is ready
 */
STATIC INLINE Status Chip_ADC_Read_Value(LPC_ADC_T *pADC, uint8_t channel, uint16_t *data)
{
	return IP_ADC_Get_Val(pADC, channel, data);
}

/**
 * @brief	Read the ADC channel status
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	channel		: ADC channel to read
 * @param	StatusType	: Status type of ADC_DR_*
 * @return	SET or RESET
 */
STATIC INLINE FlagStatus Chip_ADC_Read_Status(LPC_ADC_T *pADC, uint8_t channel, uint32_t StatusType)
{
	return IP_ADC_GetStatus(pADC, channel, StatusType);
}

/**
 * @brief	Enable/Disable interrupt for ADC channel
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	channel		: ADC channel to read
 * @param	NewState	: New state, ENABLE or DISABLE
 * @return	SET or RESET
 */
STATIC INLINE void Chip_ADC_Channel_Int_Cmd(LPC_ADC_T *pADC, uint8_t channel, FunctionalState NewState)
{
	IP_ADC_Int_Enable(pADC, channel, NewState);
}

/**
 * @brief	Enable/Disable global interrupt for ADC channel
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	NewState	: New state, ENABLE or DISABLE
 * @return	Nothing
 */
STATIC INLINE void Chip_ADC_Global_Int_Cmd(LPC_ADC_T *pADC, FunctionalState NewState)
{
	IP_ADC_Int_Enable(pADC, 8, NewState);
}

/**
 * @brief	Shutdown ADC
 * @param	pADC	: The base of ADC peripheral on the chip
 * @return	Nothing
 */
void Chip_ADC_DeInit(LPC_ADC_T *pADC);

/**
 * @brief	Initialize the ADC peripheral and the ADC setup structure to default value
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	ADCSetup	: ADC setup structure to be set
 * @return	Nothing
 * Default setting for ADC is 400kHz - 10bits
 */
void Chip_ADC_Init(LPC_ADC_T *pADC, ADC_Clock_Setup_T *ADCSetup);

/**
 * @brief	Select the mode starting the AD conversion
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	mode		: Stating mode, should be :
 *							- ADC_NO_START				: Must be set for Burst mode
 *							- ADC_START_NOW				: Start conversion now
 *							- ADC_START_ON_CTOUT15		: Start conversion when the edge selected by bit 27 occurs on CTOUT_15
 *							- ADC_START_ON_CTOUT8		: Start conversion when the edge selected by bit 27 occurs on CTOUT_8
 *							- ADC_START_ON_ADCTRIG0		: Start conversion when the edge selected by bit 27 occurs on ADCTRIG0
 *							- ADC_START_ON_ADCTRIG1		: Start conversion when the edge selected by bit 27 occurs on ADCTRIG1
 *							- ADC_START_ON_MCOA2		: Start conversion when the edge selected by bit 27 occurs on Motocon PWM output MCOA2
 * @param	EdgeOption	: Stating Edge Condition, should be :
 *							- ADC_TRIGGERMODE_RISING	: Trigger event on rising edge
 *							- ADC_TRIGGERMODE_FALLING	: Trigger event on falling edge
 * @return	Nothing
 */
void Chip_ADC_Set_StartMode(LPC_ADC_T *pADC, CHIP_ADC_START_MODE_T mode, CHIP_ADC_EDGE_CFG_T EdgeOption);

/**
 * @brief	Set the ADC Sample rate
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	ADCSetup	: ADC setup structure to be modified
 * @param	rate		: Sample rate, should be set so the clock for A/D converter is less than or equal to 4.5MHz.
 * @return	Nothing
 */
void Chip_ADC_Set_SampleRate(LPC_ADC_T *pADC, ADC_Clock_Setup_T *ADCSetup, uint32_t rate);

/**
 * @brief	Enable or disable the ADC channel on ADC peripheral
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	channel		: Channel to be enable or disable
 * @param	NewState	: New state, should be:
 *								- ENABLE
 *								- DISABLE
 * @return	Nothing
 */
void Chip_ADC_Channel_Enable_Cmd(LPC_ADC_T *pADC, CHIP_ADC_CHANNEL_T channel, FunctionalState NewState);

/**
 * @brief	Enable burst mode
 * @param	pADC		: The base of ADC peripheral on the chip
 * @param	NewState	: New state, should be:
 *							- ENABLE
 *							- DISABLE
 * @return	Nothing
 */
void Chip_ADC_Burst_Cmd(LPC_ADC_T *pADC, FunctionalState NewState);

/**
 * @brief	Read the ADC value and convert it to 8bits value
 * @param	pADC	: The base of ADC peripheral on the chip
 * @param	channel:	selected channel
 * @param	data	: Storage for data
 * @return	Status	: ERROR or SUCCESS
 */
Status Chip_ADC_Read_Byte(LPC_ADC_T *pADC, CHIP_ADC_CHANNEL_T channel, uint8_t *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_17XX_40XX_H_ */

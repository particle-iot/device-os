/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
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
#include <stdint.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "hw_config.h"
#include "syshealth_hal.h"
#include "rtc_hal.h"

extern void linkme();

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t IWDG_SYSTEM_RESET;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_Core_Init(void)
{
}

/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
{
	// this ensures the stm32_it.c functions aren't dropped by the linker, thinking
	// they are unused. Without this none of the interrupts handlers are linked.
	linkme();
	DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);
#ifdef DFU_BUILD_ENABLE
	/* Set the Vector Table(VT) base location at 0x5000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x5000);

	USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
	/* Disable the Serial Wire JTAG Debug Port SWJ-DP */
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#else
#ifdef SWD_ENABLE_JTAG_DISABLE
  /* Disable JTAG, but enable SWJ-DP */
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
#endif
#endif

	Set_System();

	SysTick_Configuration();

	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

	HAL_RTC_Configuration();

	/* Execute Stop mode if STOP mode flag is set via System.sleep(pin, mode) */
	HAL_Core_Execute_Stop_Mode();

	LED_SetRGBColor(RGB_COLOR_WHITE);
	LED_On(LED_RGB);

#ifdef IWDG_RESET_ENABLE
	// ToDo this needs rework for new bootloader
	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		/* IWDGRST flag set */
		IWDG_SYSTEM_RESET = 1;

		/* Clear reset flags */
		RCC_ClearFlag();
	}

	/* We are duplicating the IWDG call here for compatibility with old bootloader */
	/* Set IWDG Timeout to 3 secs */
	IWDG_Reset_Enable(3 * TIMING_IWDG_RELOAD);
#endif

#ifdef DFU_BUILD_ENABLE
	Load_SystemFlags();
#endif

	sFLASH_Init();

        module_user_init_hook();
}

uint16_t HAL_Core_Mode_Button_Pressed_Time()
{
    return BUTTON_GetDebouncedTime(BUTTON1);
}


bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
	bool pressedState = false;

	if(BUTTON_GetDebouncedTime(BUTTON1) >= pressedMillisDuration)
	{
		pressedState = true;
	}

	return pressedState;
}

void HAL_Core_Mode_Button_Reset(void)
{

    /* Disable TIM1 CC4 Interrupt */
    TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);

    BUTTON_ResetDebouncedState(BUTTON1);

    HAL_Notify_Button_State(BUTTON1, false);

    /* Enable BUTTON1 Interrupt */
    BUTTON_EXTI_Config(BUTTON1, ENABLE);

}

void HAL_Core_System_Reset(void)
{
	NVIC_SystemReset();
}

void HAL_Core_Factory_Reset(void)
{
	Factory_Reset_SysFlag = 0xAAAA;
	Save_SystemFlags();
	HAL_Core_System_Reset();
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
    BKP_WriteBackupRegister(BKP_DR1, ENTER_SAFE_MODE_APP_REQUEST);
    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Bootloader(bool persist)
{
    if (persist)
    {
        BKP_WriteBackupRegister(BKP_DR10, 0xFFFF);
        FLASH_OTA_Update_SysFlag = 0xFFFF;
        Save_SystemFlags();
    }
    else
    {
        BKP_WriteBackupRegister(BKP_DR1, ENTER_DFU_APP_REQUEST);
    }

    HAL_Core_System_Reset();
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
	if ((wakeUpPin < TOTAL_PINS) && (edgeTriggerMode <= FALLING))
	{
		uint16_t BKP_DR9_Data = wakeUpPin;//set wakeup pin mumber
		BKP_DR9_Data |= (edgeTriggerMode << 8);//set edge trigger mode
		BKP_DR9_Data |= (0xA << 12);//set stop mode flag

		/*************************************************/
		//BKP_DR9_Data: 0xAXXX
		//                ||||
		//                ||----- octet wakeUpPin number
		//                |------ nibble edgeTriggerMode
		//                ------- nibble stop mode flag
		/*************************************************/

		/* Execute Stop mode on next system reset */
		BKP_WriteBackupRegister(BKP_DR9, BKP_DR9_Data);

		/* Reset System */
		NVIC_SystemReset();
	}
}

void HAL_Core_Execute_Stop_Mode(void)
{
	if((BKP_ReadBackupRegister(BKP_DR9) >> 12) == 0xA)
	{
		uint16_t wakeUpPin = BKP_ReadBackupRegister(BKP_DR9) & 0xFF;
		InterruptMode edgeTriggerMode = (InterruptMode)((BKP_ReadBackupRegister(BKP_DR9) >> 8) & 0x0F);

		/* Clear Stop mode system flag */
		BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

		if ((wakeUpPin < TOTAL_PINS) && (edgeTriggerMode <= FALLING))
		{
			PinMode wakeUpPinMode = INPUT;

			/* Set required pinMode based on edgeTriggerMode */
			switch(edgeTriggerMode)
			{
			case CHANGE:
				wakeUpPinMode = INPUT;
				break;

			case RISING:
				wakeUpPinMode = INPUT_PULLDOWN;
				break;

			case FALLING:
				wakeUpPinMode = INPUT_PULLUP;
				break;
			}
			HAL_Pin_Mode(wakeUpPin, wakeUpPinMode);

			/* Configure EXTI Interrupt : wake-up from stop mode using pin interrupt */
			HAL_Interrupts_Attach(wakeUpPin, NULL, NULL, edgeTriggerMode, NULL);

			/* Request to enter STOP mode with regulator in low power mode */
			PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

			/* Detach the Interrupt pin */
	        HAL_Interrupts_Detach(wakeUpPin);

			/* At this stage the system has resumed from STOP mode */
			/* Enable HSE, PLL and select PLL as system clock source after wake-up from STOP */

			/* Enable HSE */
			RCC_HSEConfig(RCC_HSE_ON);

			/* Wait till HSE is ready */
			if(RCC_WaitForHSEStartUp() != SUCCESS)
			{
				/* If HSE startup fails try to recover by system reset */
				NVIC_SystemReset();
			}

			/* Enable PLL */
			RCC_PLLCmd(ENABLE);

			/* Wait till PLL is ready */
			while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

			/* Select PLL as system clock source */
			RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

			/* Wait till PLL is used as system clock source */
			while(RCC_GetSYSCLKSource() != 0x08);
		}
	}
}

void HAL_Core_Enter_Standby_Mode(void)
{
	/* Execute Standby mode on next system reset */
	BKP_WriteBackupRegister(BKP_DR9, 0xA5A5);

	/* Reset System */
	NVIC_SystemReset();
}

void HAL_Core_Execute_Standby_Mode(void)
{
	/* Should we execute System Standby mode */
	if(BKP_ReadBackupRegister(BKP_DR9) == 0xA5A5)
	{
		/* Clear Standby mode system flag */
		BKP_WriteBackupRegister(BKP_DR9, 0xFFFF);

                /* Enable WKUP pin */
                PWR_WakeUpPinCmd(ENABLE);

		/* Request to enter STANDBY mode */
		PWR_EnterSTANDBYMode();

		/* Following code will not be reached */
		while(1);
	}
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
	/* Hardware CRC32 calculation */
	uint32_t i, j;
	uint32_t Data;

	CRC_ResetDR();

	i = bufferSize >> 2;

	while (i--)
	{
		Data = *((uint32_t *)pBuffer);
		pBuffer += 4;

		Data = __RBIT(Data);//reverse the bit order of input Data
		CRC->DR = Data;
	}

	Data = CRC->DR;
	Data = __RBIT(Data);//reverse the bit order of output Data

	i = bufferSize & 3;

	while (i--)
	{
		Data ^= (uint32_t)*pBuffer++;

		for (j = 0 ; j < 8 ; j++)
		{
			if (Data & 1)
				Data = (Data >> 1) ^ 0xEDB88320;
			else
				Data >>= 1;
		}
	}

	Data ^= 0xFFFFFFFF;

	return Data;
}

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag)
{
    switch (flag) {
        case BOOTLOADER_FLAG_VERSION:
            return SYSTEM_FLAG(Bootloader_Version_SysFlag);
        case BOOTLOADER_FLAG_STARTUP_MODE:
            return SYSTEM_FLAG(StartupMode_SysFlag);
    }
    return 0;
}



// todo find a technique that allows accessor functions to be inlined while still keeping
// hardware independence.
bool HAL_watchdog_reset_flagged()
{
	return IWDG_SYSTEM_RESET;
}

void HAL_Notify_WDT()
{
	KICK_WDT();
}


void HAL_SysTick_Hook(void) __attribute__((weak));

void HAL_SysTick_Hook(void)
{

}

volatile bool systick_hook_enabled = false;

/*******************************************************************************
 * Function Name  : SysTick_Handler
 * Description    : This function handles SysTick Handler.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void SysTick_Handler(void)
{
    System1MsTick();

    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }

    // another hook for an rtos
    if (systick_hook_enabled)
        HAL_SysTick_Hook();

    HAL_SysTick_Handler();

}


void HAL_Hook_Main() __attribute__((weak));

void HAL_Hook_Main()
{
    // nada
}

int main() {
    // the rtos systick can only be enabled after the system has been initialized
    systick_hook_enabled = true;
    HAL_Hook_Main();
    app_setup_and_loop();
    return 0;
}


void HAL_Bootloader_Lock(bool lock)
{
    if (lock)
        FLASH_WriteProtection_Enable(BOOTLOADER_FLASH_PAGES);
    else
        FLASH_WriteProtection_Disable(BOOTLOADER_FLASH_PAGES);
}

uint32_t freeheap();

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    info->freeheap = freeheap();
    return 0;
}

unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved)
{
    return SystemCoreClock;
}

int HAL_Feature_Set(HAL_Feature feature, bool enabled)
{
    return -1;
}

bool HAL_Feature_Get(HAL_Feature feature)
{
    return false;
}

int HAL_Set_System_Config(hal_system_config_t config_item, const void* data, unsigned length)
{
    return -1;
}


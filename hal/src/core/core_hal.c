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

typedef struct Last_Reset_Info {
    int reason;
    uint32_t data;
} Last_Reset_Info;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t IWDG_SYSTEM_RESET;

static Last_Reset_Info last_reset_info = { RESET_REASON_NONE, 0 };

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static void Init_Last_Reset_Info()
{
    if (HAL_Core_System_Reset_FlagSet(SOFTWARE_RESET))
    {
        // Load reset info from backup registers
        last_reset_info.reason = BKP_ReadBackupRegister(BKP_DR2);
        const uint16_t hi = BKP_ReadBackupRegister(BKP_DR3);
        const uint16_t lo = BKP_ReadBackupRegister(BKP_DR4);
        last_reset_info.data = ((uint32_t)hi << 16) | (uint32_t)lo;
        // Clear backup registers
        BKP_WriteBackupRegister(BKP_DR2, 0);
        BKP_WriteBackupRegister(BKP_DR3, 0);
        BKP_WriteBackupRegister(BKP_DR4, 0);
    }
    else // Hardware reset
    {
        if (HAL_Core_System_Reset_FlagSet(WATCHDOG_RESET))
        {
            last_reset_info.reason = RESET_REASON_WATCHDOG;
        }
        else if (HAL_Core_System_Reset_FlagSet(POWER_MANAGEMENT_RESET))
        {
            last_reset_info.reason = RESET_REASON_POWER_MANAGEMENT; // Reset generated when entering standby mode (nRST_STDBY: 0)
        }
        else if (HAL_Core_System_Reset_FlagSet(POWER_DOWN_RESET))
        {
            last_reset_info.reason = RESET_REASON_POWER_DOWN;
        }
        else if (HAL_Core_System_Reset_FlagSet(PIN_RESET)) // Pin reset flag should be checked in the last place
        {
            last_reset_info.reason = RESET_REASON_PIN_RESET;
        }
        else if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) // Check if MCU was in standby mode
        {
            last_reset_info.reason = RESET_REASON_POWER_MANAGEMENT; // Reset generated when exiting standby mode (nRST_STDBY: 1)
        }
        else
        {
            last_reset_info.reason = RESET_REASON_UNKNOWN;
        }
        last_reset_info.data = 0; // Not used
    }
    // Note: RCC reset flags should be cleared, see HAL_Core_Init()
}

void HAL_Core_Init(void)
{
    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Clear RCC reset flags
        RCC_ClearFlag();
    }
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

void HAL_Core_Mode_Button_Reset(uint16_t button)
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

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved)
{
    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Save reset info to backup registers
        BKP_WriteBackupRegister(BKP_DR2, reason);
        BKP_WriteBackupRegister(BKP_DR3, data >> 16);
        BKP_WriteBackupRegister(BKP_DR4, data & 0xffff);
    }
    HAL_Core_System_Reset();
}

int HAL_Core_Get_Last_Reset_Info(int *reason, uint32_t *data, void *reserved)
{
    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        if (reason)
        {
            *reason = last_reset_info.reason;
        }
        if (data)
        {
            *data = last_reset_info.data;
        }
        return 0;
    }
    return -1;
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

int HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved)
{
    HAL_Core_Enter_Stop_Mode(pins != NULL && pins_count > 0 ? *pins : TOTAL_PINS, mode != NULL && mode_count > 0 ? (uint16_t)(*mode) : 0xffff, seconds);
    return -1;
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
    if (seconds > 0) {
        HAL_RTC_Cancel_UnixAlarm();
        HAL_RTC_Set_UnixAlarm((time_t) seconds);
    }

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
    int32_t state = HAL_disable_irq();
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

            /* Disable RTC Alarm */
            HAL_RTC_Cancel_UnixAlarm();

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
    HAL_enable_irq(state);
}

int HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags)
{
    // Configure RTC wake-up
    if (seconds > 0) {
        HAL_RTC_Cancel_UnixAlarm();
        HAL_RTC_Set_UnixAlarm((time_t) seconds);
    }

	/* Execute Standby mode on next system reset */
	BKP_WriteBackupRegister(BKP_DR9, 0xA5A5);

	/* Reset System */
	NVIC_SystemReset();

    // This will never get reached
    return 0;
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

    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Load last reset info from RCC / backup registers
        Init_Last_Reset_Info();
    }

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

static inline bool Is_System_Reset_Flag_Set(uint8_t flag)
{
    return SYSTEM_FLAG(RCC_CSR_SysFlag) & ((uint32_t)1 << (flag & 0x1f));
}

bool HAL_Core_System_Reset_FlagSet(RESET_TypeDef resetType)
{
    switch(resetType)
    {
    case PIN_RESET:
        return Is_System_Reset_Flag_Set(RCC_FLAG_PINRST);
    case SOFTWARE_RESET:
        return Is_System_Reset_Flag_Set(RCC_FLAG_SFTRST);
    case WATCHDOG_RESET:
        return Is_System_Reset_Flag_Set(RCC_FLAG_IWDGRST) || Is_System_Reset_Flag_Set(RCC_FLAG_WWDGRST);
    case POWER_MANAGEMENT_RESET:
        return Is_System_Reset_Flag_Set(RCC_FLAG_LPWRRST);
    case POWER_DOWN_RESET:
        return Is_System_Reset_Flag_Set(RCC_FLAG_PORRST);
    default:
        return false;
    }
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
    if (feature == FEATURE_RESET_INFO)
    {
        return true;
    }
    return false;
}

int HAL_Set_System_Config(hal_system_config_t config_item, const void* data, unsigned length)
{
    return -1;
}

int HAL_System_Backup_Save(size_t offset, const void* buffer, size_t length, void* reserved)
{
	return -1;
}

int HAL_Ssystem_Backup_Restore(size_t offset, void* buffer, size_t max_length, size_t* length, void* reserved)
{
	return -1;
}

void HAL_Core_Button_Mirror_Pin_Disable(uint8_t bootloader, uint8_t button, void* reserved)
{
}

void HAL_Core_Button_Mirror_Pin(uint16_t pin, InterruptMode mode, uint8_t bootloader, uint8_t button, void *reserved)
{
}

void HAL_Core_Led_Mirror_Pin_Disable(uint8_t led, uint8_t bootloader, void* reserved)
{
}

void HAL_Core_Led_Mirror_Pin(uint8_t led, pin_t pin, uint32_t flags, uint8_t bootloader, void* reserved)
{
}

/**
 ******************************************************************************
 * @file    main.c
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    30-April-2013
 * @brief   main file
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

/* Includes ------------------------------------------------------------------*/
#include "syshealth_hal.h"
#include "core_hal.h"
#include "dfu_hal.h"
#include "hw_config.h"
#include "rgbled.h"
#include "button_hal.h"

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
#define LOAD_DCT_FUNCTIONS
#include "bootloader_dct.h"
#endif

#if PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10
#define USE_LED_THEME
#include "led_signal.h"

#define CHECK_FIRMWARE
#include "module.h"
#endif

void platform_startup();


/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint8_t REFLASH_FROM_BACKUP = 0;	//0, 1
uint8_t OTA_FLASH_AVAILABLE = 0;	//0, 1
volatile uint8_t USB_DFU_MODE = 0;		//0, 1
volatile uint8_t FACTORY_RESET_MODE = 0;		//0, 1
volatile uint8_t SAFE_MODE = 0;
volatile uint8_t RESET_SETTINGS = 0;

pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t ApplicationAddress;

volatile uint32_t TimingBUTTON;
volatile uint32_t TimingLED;
volatile uint32_t TimingIWDGReload;

// Customizable LED colors
static uint32_t FirmwareUpdateColor = RGB_COLOR_MAGENTA;
static uint32_t SafeModeColor = RGB_COLOR_MAGENTA;
static uint32_t DFUModeColor = RGB_COLOR_YELLOW;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void flashModulesCallback(bool isUpdating)
{
    if(isUpdating)
    {
        OTA_FLASH_AVAILABLE = 1;
        LED_SetRGBColor(FirmwareUpdateColor);
    }
    else
    {
        OTA_FLASH_AVAILABLE = 0;
        LED_Off(LED_RGB);
    }
}

/**
 * Don't use the backup register. Both options are given here so we
 * can restore for any given platform if needed later.
 */
#if PLATFORM_ID < 0
#define BACKUP_REGISTER(reg) HAL_Core_Read_Backup_Register(reg)
#else
#define BACKUP_REGISTER(reg) 0xFFFF
#endif

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void)
{
    /*
        At this stage the microcontroller clock setting is already configured,
        this is done through SystemInit() function which is called from startup
        file (startup_stm32f10x_md.s) before to branch to application main.
        To reconfigure the default setting of SystemInit() function, refer to
        system_stm32f10x.c file
     */

    //--------------------------------------------------------------------------
    //    Initialize the system
    //--------------------------------------------------------------------------
    //    System Clocks
    //    System Interrupts
    //    Configure the I/Os
    //    Configure the Timer
    //    Configure the LEDs
    //    Configure the MODE button
    //--------------------------------------------------------------------------
    Set_System();
    BUTTON_Init_Ext();

    //--------------------------------------------------------------------------

    // Setup SysTick Timer for 1 msec interrupts to call Timing_Decrement()
    SysTick_Configuration();

    platform_startup();

    USE_SYSTEM_FLAGS = 1;

    //--------------------------------------------------------------------------
    //  Load the system flags saved at SYSTEM_FLAGS_ADDRESS = 0x08004C00
    //  Bootloader_Version_SysFlag
    //  NVMEM_SPARK_Reset_SysFlag
    //  FLASH_OTA_Update_SysFlag
    //  Factory_Reset_SysFlag
    //--------------------------------------------------------------------------
    Load_SystemFlags();

    Save_Reset_Syndrome();

    //BOOTLOADER_VERSION defined in bootloader/import.mk
    //This can also be overridden via make command line arguments
    if (SYSTEM_FLAG(Bootloader_Version_SysFlag) != BOOTLOADER_VERSION)
    {
        Bootloader_Update_Version(BOOTLOADER_VERSION);
    }

    if (SYSTEM_FLAG(StartupMode_SysFlag) != 0) {
        SYSTEM_FLAG(StartupMode_SysFlag) = 0;
        Save_SystemFlags();
    }

    uint8_t features = SYSTEM_FLAG(FeaturesEnabled_SysFlag);
    // disabling this until we can be sure DCT corruption won't bite.
    if (true || (features!=0xFF && (((~(features>>4)&0xF)) != (features & 0xF))) || (features&8)) {     // bit 3 must be reset for this to be enabled
        features = 0xFF;        // ignore - corrupt. Top 4 bits should be the inverse of the bottom 4
    }

#ifdef USE_LED_THEME
    // Load LED theme colors
    get_led_theme_colors(&FirmwareUpdateColor, &SafeModeColor, &DFUModeColor);
#endif

    //--------------------------------------------------------------------------

    /*
     * Check that firmware is valid at this address.
     */
    ApplicationAddress = CORE_FW_ADDRESS;

    // 0x0005 is written to the backup register at the end of firmware update.
    // if the register reads 0x0005, it signifies that the firmware update
    // was successful
    if((BACKUP_REGISTER(BKP_DR_10) == 0x0005) ||
            (SYSTEM_FLAG(FLASH_OTA_Update_SysFlag) == 0x0005))
    {
        // OTA was complete and the firmware is now available to be transfered to
        // the internal flash memory
        OTA_FLASH_AVAILABLE = 1;
    }

    // 0x5555 is written to the backup register at the beginning of firmware update
    // if the register still reads 0x5555, it signifies that the firmware update
    // was never completed => FAIL
    else if((BACKUP_REGISTER(BKP_DR_10) == 0x5555) ||
            (SYSTEM_FLAG(FLASH_OTA_Update_SysFlag) == 0x5555))
    {
        // OTA transfer failed, hence, load firmware from the backup address
        OTA_FLASH_AVAILABLE = 0;
        REFLASH_FROM_BACKUP = 1;
    }

    // 0xAAAA is written to the Factory_Reset_SysFlag in order to trigger a factory reset
    if (0xAAAA == SYSTEM_FLAG(Factory_Reset_SysFlag))
    {
        FACTORY_RESET_MODE = 1;
    }

    // Get the Bootloader Mode that will be used when IWDG reset occurs due to invalid firmware
    volatile uint16_t BKP_DR1_Value = HAL_Core_Read_Backup_Register(BKP_DR_01);

    if(BKP_DR1_Value != 0xFFFF)
    {
        // Check if application requested to enter DFU mode
        if (BKP_DR1_Value == ENTER_DFU_APP_REQUEST)
        {
            USB_DFU_MODE = 1;
            //Subsequent system reset or power on-off should execute normal firmware
            HAL_Core_Write_Backup_Register(BKP_DR_01, 0xFFFF);
        }
        else if (BKP_DR1_Value == ENTER_SAFE_MODE_APP_REQUEST)
        {
            SAFE_MODE = 1;
            HAL_Core_Write_Backup_Register(BKP_DR_01, 0xFFFF);
        }
        // Else check if the system has resumed from IWDG reset
        else if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
        {
            // These are still initialized to zero, no need to do it again.
            // REFLASH_FROM_BACKUP = 0;
            // OTA_FLASH_AVAILABLE = 0;
            // USB_DFU_MODE = 0;
            // FACTORY_RESET_MODE = 0;

            switch(BKP_DR1_Value)
            {
                case FIRST_RETRY:	// On 1st retry attempt, try to recover using sFlash - Backup Area
                    REFLASH_FROM_BACKUP = 1;
                    BKP_DR1_Value += 1;
                    break;

                case SECOND_RETRY:	// On 2nd retry attempt, try to recover using sFlash - Factory Reset
                    FACTORY_RESET_MODE = 1;
                    SYSTEM_FLAG(NVMEM_SPARK_Reset_SysFlag) = 0x0000;
                    BKP_DR1_Value += 1;
                    break;

                case THIRD_RETRY:	// On 3rd retry attempt, try to recover using USB DFU Mode (Final attempt)
                    USB_DFU_MODE = 1;
                    // fall through - No break at the end of case

                    // toDO create a location in vector table for bootloadr->app - app->bootloader API.
                    // add version number to build, and mode (debug,release etc) in vector table
                    // Then make informed decisions on what to do on WDT timeouts
                    // for now ran something
                case ENTERED_SparkCoreConfig:
                case ENTERED_Main:
                case ENTERED_Setup:
                case ENTERED_Loop:
                case RAN_Loop:
                case PRESERVE_APP:

                default:
                    BKP_DR1_Value = 0xFFFF;
                    break;
            }

            HAL_Core_Write_Backup_Register(BKP_DR_01, BKP_DR1_Value);

            OTA_Flashed_ResetStatus();

            // Clear reset flags
            RCC_ClearFlag();
        }
    }
    else
    {
        // On successful firmware transition, BKP_DR1_Value is reset to default 0xFFFF
        BKP_DR1_Value = 1;	//Assume we have an invalid firmware loaded in internal flash
        HAL_Core_Write_Backup_Register(BKP_DR_01, BKP_DR1_Value);
    }

    //--------------------------------------------------------------------------
    //    Check if BUTTON1 is pressed and determine the status
    //--------------------------------------------------------------------------
    if (BUTTON_Is_Pressed(BUTTON1) && (features & BL_BUTTON_FEATURES))
    {
#define TIMING_SAFE_MODE 1000
#define TIMING_DFU_MODE 3000
#define TIMING_RESTORE_MODE 6500
#define TIMING_RESET_MODE 10000
#define TIMING_ALL 12000            // add a couple of seconds for visual feedback

        bool factory_reset_available = (features & BL_FEATURE_FACTORY_RESET) && FLASH_IsFactoryResetAvailable();

        TimingBUTTON = TIMING_ALL;
        // uint8_t factory_reset = 0;
        while (BUTTON_Is_Pressed(BUTTON1) && TimingBUTTON)
        {
            if(!RESET_SETTINGS && BUTTON_Pressed_Time(BUTTON1) > TIMING_RESET_MODE)
            {
                // if pressed for 10 sec, enter Factory Reset Mode
                // This tells the WLAN setup to clear the WiFi user profiles on bootup
                LED_SetRGBColor(RGB_COLOR_WHITE);
                RESET_SETTINGS = 1;
            }
            else if(!FACTORY_RESET_MODE && BUTTON_Pressed_Time(BUTTON1) > TIMING_RESTORE_MODE)
            {
                if (factory_reset_available) {
                    LED_SetRGBColor(RGB_COLOR_GREEN);
                    SYSTEM_FLAG(NVMEM_SPARK_Reset_SysFlag) = 0x0000;
                    FACTORY_RESET_MODE = 1;
                }
            }
            else if(!USB_DFU_MODE && BUTTON_Pressed_Time(BUTTON1) >= TIMING_DFU_MODE)
            {
                // if pressed for > 3 sec, enter USB DFU Mode
                if (features&BL_FEATURE_DFU_MODE) {
                    LED_SetRGBColor(DFUModeColor);
                    USB_DFU_MODE = 1;           // stay in DFU mode until the button is released so we have slow-led blinking
                }
            }
            else if(!SAFE_MODE && BUTTON_Pressed_Time(BUTTON1) >= TIMING_SAFE_MODE)
            {
                OTA_FLASH_AVAILABLE = 0;
                REFLASH_FROM_BACKUP = 0;
                FACTORY_RESET_MODE = 0;

                if (features&BL_FEATURE_SAFE_MODE) {
                    // if pressed for > 1 sec, enter Safe Mode
                    LED_SetRGBColor(SafeModeColor);
                    SAFE_MODE = 1;
                }
            }
        }

        // if (factory_reset || USB_DFU_MODE || SAFE_MODE) {
        //     // now set the factory reset mode (to change the LED to rapid blinking.))
        //     FACTORY_RESET_MODE = factory_reset;
        //     USB_DFU_MODE &= !factory_reset;
        //     SAFE_MODE &= !USB_DFU_MODE;
        // }
    }

    if (RESET_SETTINGS) {
        SYSTEM_FLAG(NVMEM_SPARK_Reset_SysFlag) = 0x0001;
        Save_SystemFlags();
        USB_DFU_MODE = 0;
        SAFE_MODE = 0;
    }

    if (SAFE_MODE) {
        SYSTEM_FLAG(StartupMode_SysFlag) = 0x0001;
        Save_SystemFlags();
    }

    //--------------------------------------------------------------------------

    if (OTA_FLASH_AVAILABLE == 1)
    {
        LED_SetRGBColor(FirmwareUpdateColor);
        // Load the OTA Firmware from external flash
        OTA_Flash_Reset();
    }
    else if (FACTORY_RESET_MODE)
    {
        if (FACTORY_RESET_MODE == 1)
        {
            if (SYSTEM_FLAG(NVMEM_SPARK_Reset_SysFlag) == 0x0001)
                LED_SetRGBColor(RGB_COLOR_WHITE);
            else
                LED_SetRGBColor(RGB_COLOR_GREEN);
            // Restore the Factory Firmware
            // On success the device will reset)
            if (!FACTORY_Flash_Reset()) {
                if (is_application_valid(ApplicationAddress)) {
                    // we have a valid image to fall back to, so just reset
                    NVIC_SystemReset();
                }
                // else fall through to DFU
            }
        } else {
            // This else clause is only for JTAG debugging
            // Break and set FACTORY_RESET_MODE to 2
            // to run the current code at 0x08005000
            FACTORY_RESET_MODE = 0;
            Finish_Update();
        }
    }
    else if (USB_DFU_MODE == 0)
    {
#ifdef FLASH_UPDATE_MODULES
        /*
         * Update Internal/Serial Flash based on application_dct=>flash_modules settings
         * BM-14 bootloader with FLASH_UPDATE_MODULES enabled DOES NOT fit in < 16KB
         * BM-09 bootloader with FLASH_UPDATE_MODULES enabled fits in < 16KB
         * Currently FLASH_UPDATE_MODULES support is enabled only on BM-09 bootloader
         */
        FLASH_UpdateModules(flashModulesCallback);
#ifdef LOAD_DCT_FUNCTIONS
        // DCT functions may need to be reloaded after updating a system module
        load_dct_functions();
#endif
#else
        if (REFLASH_FROM_BACKUP == 1)
        {
            LED_SetRGBColor(RGB_COLOR_RED);
            // Restore the Backup Firmware from external flash
            BACKUP_Flash_Reset();
        }
#endif

        // ToDo add CRC check
        // Test if user code is programmed starting from ApplicationAddress
        if (is_application_valid(ApplicationAddress))
        {
            // Jump to user application
            JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
            Jump_To_Application = (pFunction) JumpAddress;

            uint8_t disable_iwdg = 0;
#ifdef CHECK_FIRMWARE
            // Pre-0.7.0 firmwares were expecting IWDG flag to be set in the DCT, now it's stored in
            // the backup registers. As a workaround, we disable IWDG if an older firmware is detected
            const int module_ver = get_main_module_version();
            if (module_ver >= 0 && module_ver < SYSTEM_MODULE_VERSION_0_7_0_RC1) {
                disable_iwdg = 1;
            }
#endif
            if (!disable_iwdg) {
                // Set IWDG Timeout to 5 secs based on platform specific system flags
                IWDG_Reset_Enable(5 * TIMING_IWDG_RELOAD);
            }

            Reset_System();

            // Initialize user application's Stack Pointer
            __set_MSP(*(__IO uint32_t*) ApplicationAddress);
            Jump_To_Application();
        }
#if !HAL_PLATFORM_MESH
        else
        {
            LED_SetRGBColor(RGB_COLOR_RED);
            FACTORY_Flash_Reset();
            // if we get here, the factory reset wasn't successful
        }
#endif
        // else drop through to DFU mode

    }
    // Otherwise enters DFU mode to allow user to program his application

    FACTORY_RESET_MODE = 0;  // ensure the LED is slow flashing (100)
    OTA_FLASH_AVAILABLE = 0; //   |
    REFLASH_FROM_BACKUP = 0; //   |
    RESET_SETTINGS = 0;

    LED_SetRGBColor(DFUModeColor);

    USB_DFU_MODE = 1;

    HAL_DFU_USB_Init();

    // Main loop
    while (1)
    {
        //Do nothing
    }
}

extern void DFU_Check_Reset();

/*******************************************************************************
 * Function Name  : Timing_Decrement
 * Description    : Decrements the various Timing variables related to SysTick.
                   This function is called every 1mS.
 * Input          : None
 * Output         : Timing
 * Return         : None
 *******************************************************************************/
void Timing_Decrement(void)
{
    if (TimingBUTTON != 0x00)
    {
        TimingBUTTON--;
    }

    if (TimingLED != 0x00)
    {
        TimingLED--;
    }
    else if(FACTORY_RESET_MODE || REFLASH_FROM_BACKUP || OTA_FLASH_AVAILABLE || RESET_SETTINGS)
    {
        LED_Toggle(LED_RGB);
        TimingLED = 50;
    }
    else if(SAFE_MODE || USB_DFU_MODE)
    {
        LED_Toggle(LED_RGB);
        TimingLED = 100;
    }

    DFU_Check_Reset();
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif


extern unsigned long link_constructors_location;
extern unsigned long link_constructors_end;

static void call_constructors(unsigned long *start, unsigned long *end) __attribute__((noinline));

static void call_constructors(unsigned long *start, unsigned long *end)
{
	unsigned long *i;
	void (*funcptr)();
	for (i = start; i < end; i++)
	{
		funcptr=(void (*)())(*i);
		funcptr();
	}
}

void CallConstructors(void)
{
	call_constructors(&link_constructors_location, &link_constructors_end);
}

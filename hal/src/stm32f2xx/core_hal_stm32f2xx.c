/**
 ******************************************************************************
 * @file    core_hal.c
 * @author  Satish Nair, Matthew McGowan
 * @version V1.0.0
 * @date    31-Oct-2014
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

#include <stdint.h>
#include <stdlib.h>
#include "core_hal.h"
#include "watchdog_hal.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "interrupts_impl.h"
#include "ota_flash_hal.h"
#include "rtc_hal.h"
#include "rng_hal.h"
#include "syshealth_hal.h"
#include "rgbled.h"
#include "delay_hal.h"
#include "hw_config.h"
#include "service_debug.h"
#include "flash_mal.h"
#include <stdatomic.h>
#include "stm32f2xx.h"
#include "core_cm3.h"
#include "bootloader.h"
#include "core_hal_stm32f2xx.h"
#include "stm32f2xx.h"
#include "timer_hal.h"
#include "dct.h"
#include "hal_platform.h"
#include "malloc.h"
#include "usb_hal.h"
#include "usart_hal.h"
#include "deviceid_hal.h"
#include "pinmap_impl.h"
#include "ota_module.h"
#include "hal_event.h"
#include "system_error.h"

#if PLATFORM_ID==PLATFORM_P1
#include "wwd_management.h"
#include "wlan_hal.h"
#endif

extern char link_heap_location, link_heap_location_end;

#define STOP_MODE_EXIT_CONDITION_PIN 0x01
#define STOP_MODE_EXIT_CONDITION_RTC 0x02

void HardFault_Handler( void ) __attribute__( ( naked ) );

__attribute__((externally_visible)) void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    /* These are volatile to try and prevent the compiler/linker optimising them
    away as the variables never actually get used.  If the debugger won't show the
    values of the variables, make them global my moving their declaration outside
    of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr; /* Link register. */
    volatile uint32_t pc; /* Program counter. */
    volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* Silence "variable set but not used" error */
    if (false) {
      (void)r0; (void)r1; (void)r2; (void)r3; (void)r12; (void)lr; (void)pc; (void)psr;
    }

    if (SCB->CFSR & (1<<25) /* DIVBYZERO */) {
        // stay consistent with the core and cause 5 flashes
        UsageFault_Handler();
    }
    else {
    PANIC(HardFault,"HardFault");

    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}
}


void HardFault_Handler(void)
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

/*******************************************************************************
 * Function Name  : MemManage_Handler
 * Description    : This function handles Memory Manage exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
        PANIC(MemManage,"MemManage");
	while (1)
	{
	}
}

/*******************************************************************************
 * Function Name  : BusFault_Handler
 * Description    : This function handles Bus Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
        PANIC(BusFault,"BusFault");
        while (1)
	{
	}
}


/*******************************************************************************
 * Function Name  : UsageFault_Handler
 * Description    : This function handles Usage Fault exception.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
        PANIC(UsageFault,"UsageFault");
	while (1)
	{
	}
}



/* Private typedef -----------------------------------------------------------*/

typedef struct Last_Reset_Info {
    int reason;
    uint32_t data;
} Last_Reset_Info;

typedef enum Feature_Flag {
    FEATURE_FLAG_RESET_INFO = 0x01 // HAL_Feature::FEATURE_RESET_INFO
} Feature_Flag;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static Last_Reset_Info last_reset_info = { RESET_REASON_NONE, 0 };

static volatile uint32_t feature_flags = 0;
static volatile bool feature_flags_loaded = false;

/* Private function prototypes -----------------------------------------------*/
extern uint32_t HAL_Interrupts_Pin_IRQn(pin_t pin);

void (*HAL_TIM1_Handler)(void);
void (*HAL_TIM3_Handler)(void);
void (*HAL_TIM4_Handler)(void);
void (*HAL_TIM5_Handler)(void);
void (*HAL_TIM8_Handler)(void);

//HAL Interrupt Handlers defined in xxx_hal.c files
void HAL_CAN1_TX_Handler(void) __attribute__ ((weak));
void HAL_CAN1_RX0_Handler(void) __attribute__ ((weak));
void HAL_CAN1_RX1_Handler(void) __attribute__ ((weak));
void HAL_CAN1_SCE_Handler(void) __attribute__ ((weak));
void HAL_CAN2_TX_Handler(void) __attribute__ ((weak));
void HAL_CAN2_RX0_Handler(void) __attribute__ ((weak));
void HAL_CAN2_RX1_Handler(void) __attribute__ ((weak));
void HAL_CAN2_SCE_Handler(void) __attribute__ ((weak));

/* Private functions ---------------------------------------------------------*/

static void Init_Last_Reset_Info()
{
    if (HAL_Core_System_Reset_FlagSet(SOFTWARE_RESET))
    {
        // Load reset info from backup registers
        last_reset_info.reason = RTC_ReadBackupRegister(RTC_BKP_DR2);
        last_reset_info.data = RTC_ReadBackupRegister(RTC_BKP_DR3);
        // Clear backup registers
        RTC_WriteBackupRegister(RTC_BKP_DR2, 0);
        RTC_WriteBackupRegister(RTC_BKP_DR3, 0);
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
        else if (HAL_Core_System_Reset_FlagSet(POWER_BROWNOUT_RESET))
        {
            last_reset_info.reason = RESET_REASON_POWER_BROWNOUT;
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

static int Write_Feature_Flag(uint32_t flag, bool value, bool *prev_value)
{
    if (HAL_IsISR()) {
        return -1; // DCT cannot be accessed from an ISR
    }
    uint32_t flags = 0;
    int result = dct_read_app_data_copy(DCT_FEATURE_FLAGS_OFFSET, &flags, sizeof(flags));
    if (result != 0) {
        return result;
    }
    const bool cur_value = flags & flag;
    if (prev_value) {
        *prev_value = cur_value;
    }
    if (cur_value != value) {
        if (value) {
            flags |= flag;
        } else {
            flags &= ~flag;
        }
        result = dct_write_app_data(&flags, DCT_FEATURE_FLAGS_OFFSET, 4);
        if (result != 0) {
            return result;
        }
    }
    feature_flags = flags;
    feature_flags_loaded = true;
    return 0;
}

static int Read_Feature_Flag(uint32_t flag, bool* value)
{
    uint32_t flags = 0;
    if (!feature_flags_loaded) {
        if (HAL_IsISR()) {
            return -1; // DCT cannot be accessed from an ISR
        }
        const int result = dct_read_app_data_copy(DCT_FEATURE_FLAGS_OFFSET, &flags, sizeof(flags));
        if (result != 0) {
            return result;
        }
        feature_flags = flags;
        feature_flags_loaded = true;
    } else {
        flags = feature_flags;
    }
    *value = flags & flag;
    return 0;
}

/* Extern variables ----------------------------------------------------------*/

volatile uint8_t rtos_started = 0;

/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
{
    DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);

#ifdef DFU_BUILD_ENABLE
    //Currently this is done through WICED library API so commented.
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x20000);
    USE_SYSTEM_FLAGS = 1;
#endif

    Set_System();

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);

    /* Disable WKP pin */
    PWR_WakeUpPinCmd(DISABLE);

    //Wiring pins default to inputs
#if !defined(USE_SWD_JTAG) && !defined(USE_SWD)
    for (pin_t pin=0; pin<=19; pin++)
        HAL_Pin_Mode(pin, INPUT);
#if PLATFORM_ID==8 // Additional pins for P1
    for (pin_t pin=24; pin<=29; pin++)
        HAL_Pin_Mode(pin, INPUT);
    if (isWiFiPowersaveClockDisabled()) {
        HAL_Pin_Mode(30, INPUT); // Wi-Fi Powersave clock is disabled, default to INPUT
    }
#endif
#if PLATFORM_ID==10 // Additional pins for Electron
    for (pin_t pin=24; pin<=35; pin++)
        HAL_Pin_Mode(pin, INPUT);
#endif
#endif

    HAL_Core_Config_systick_configuration();

    HAL_RTC_Configuration();

    HAL_RNG_Configuration();

    // Initialize system-part2 stdlib PRNG with a seed from hardware PRNG
    // in case some system code happens to use rand()
    srand(HAL_RNG_GetRandomNumber());

#ifdef DFU_BUILD_ENABLE
    Load_SystemFlags();
#endif

    // TODO: Use current LED theme
    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);

    // override the WICED interrupts, specifically SysTick - there is a bug
    // where WICED isn't ready for a SysTick until after main() has been called to
    // fully intialize the RTOS.
    HAL_Core_Setup_override_interrupts();

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
    // write protect system module parts if not already protected
    FLASH_WriteProtectMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION - CORE_FW_ADDRESS, true);
#endif /* defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE */

#ifdef HAS_SERIAL_FLASH
    //Initialize Serial Flash
    sFLASH_Init();
#else
    FLASH_AddToFactoryResetModuleSlot(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS,
                                      FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE,
                                      FACTORY_RESET_MODULE_FUNCTION, MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS); //true to verify the CRC during copy also
#endif
}

#if !defined(MODULAR_FIRMWARE) || !MODULAR_FIRMWARE
__attribute__((externally_visible, section(".early_startup.HAL_Core_Config"))) uint32_t startup = (uint32_t)&HAL_Core_Config;
#endif

void HAL_Core_Setup(void) {

    /* Reset system to disable IWDG if enabled in bootloader */
    IWDG_Reset_Enable(0);

    HAL_Core_Setup_finalize();

    if (bootloader_update_if_needed()) {
        HAL_Core_System_Reset();
    }

    HAL_save_device_id(DCT_DEVICE_ID_OFFSET);

#if !defined(MODULAR_FIRMWARE) || !MODULAR_FIRMWARE
    module_user_init_hook();
#endif
}

void HAL_Core_Init(void) {
    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Clear RCC reset flags
        RCC_ClearFlag();
    }

    // Perform platform-specific initialization
    HAL_Core_Init_finalize();
}

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE

bool HAL_Core_Validate_User_Module(void)
{
    bool valid = false;
    Load_SystemFlags();

    const uint8_t flags = SYSTEM_FLAG(StartupMode_SysFlag);
    if (flags == 0xff /* Old bootloader */ || !(flags & 1)) // Safe mode flag
    {
        //CRC verification Enabled by default
        if (FLASH_isUserModuleInfoValid(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, USER_FIRMWARE_IMAGE_LOCATION))
        {
            //CRC check the user module and set to module_user_part_validated
            valid = FLASH_VerifyCRC32(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION,
                                         FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION))
                    && HAL_Verify_User_Dependencies();
        }
        else if(FLASH_isUserModuleInfoValid(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION))
        {
            //Reset and let bootloader perform the user module factory reset
            //Doing this instead of calling FLASH_RestoreFromFactoryResetModuleSlot()
            //saves precious system_part2 flash size i.e. fits in < 128KB
            HAL_Core_Factory_Reset();

            while(1);//Device should reset before reaching this line
        }
    }
    return valid;
}

bool HAL_Core_Validate_Modules(uint32_t flags, void* reserved)
{
    const module_bounds_t* bounds = NULL;
    hal_module_t mod;
    bool module_fetched = false;
    bool valid = false;

    // First verify bootloader module
    bounds = find_module_bounds(MODULE_FUNCTION_BOOTLOADER, 0);
    module_fetched = fetch_module(&mod, bounds, false, MODULE_VALIDATION_INTEGRITY);

    valid = module_fetched && (mod.validity_checked == mod.validity_result);

    if (!valid) {
        return valid;
    }

    // Now check system-parts
    int i = 0;
    if (flags & 1) {
        // Validate only that system-part that depends on bootloader passes dependency check
        i = 2;
    }
    do {
        bounds = find_module_bounds(MODULE_FUNCTION_SYSTEM_PART, i++);
        if (bounds) {
            module_fetched = fetch_module(&mod, bounds, false, MODULE_VALIDATION_INTEGRITY);
            valid = module_fetched && (mod.validity_checked == mod.validity_result);
        }
        if (flags & 1) {
            bounds = NULL;
        }
    } while(bounds != NULL && valid);

    return valid;
}
#endif


bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    bool pressedState = false;

    if(BUTTON_GetDebouncedTime(BUTTON1) >= pressedMillisDuration)
    {
        pressedState = true;
    }
    if(BUTTON_GetDebouncedTime(BUTTON1_MIRROR) >= pressedMillisDuration)
    {
        pressedState = true;
    }

    return pressedState;
}

/**
 * Force the button in the unpressed state.
 */
void HAL_Core_Mode_Button_Reset(uint16_t button)
{
    HAL_Buttons[button].debounce_time = 0x00;

    if (HAL_Buttons[BUTTON1].active + HAL_Buttons[BUTTON1_MIRROR].active == 0) {
        /* Disable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);
    }

    HAL_Notify_Button_State((Button_TypeDef)button, false);

    /* Enable Button Interrupt */
    if (button != BUTTON1_MIRROR) {
        BUTTON_EXTI_Config((Button_TypeDef)button, ENABLE);
    } else {
        HAL_InterruptExtraConfiguration irqConf = {0};
        irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_2;
        irqConf.IRQChannelPreemptionPriority = 0;
        irqConf.IRQChannelSubPriority = 0;
        irqConf.keepHandler = 1;
        irqConf.keepPriority = 1;
        HAL_Interrupts_Attach(HAL_Buttons[button].hal_pin, NULL, NULL, HAL_Buttons[button].interrupt_mode, &irqConf);
    }

}

void HAL_Core_System_Reset(void)
{
    NVIC_SystemReset();
}

void HAL_Core_Factory_Reset(void)
{
    system_flags.Factory_Reset_SysFlag = 0xAAAA;
    Save_SystemFlags();
    HAL_Core_System_Reset_Ex(RESET_REASON_FACTORY_RESET, 0, NULL);
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved)
{
    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Save reset info to backup registers
        RTC_WriteBackupRegister(RTC_BKP_DR2, reason);
        RTC_WriteBackupRegister(RTC_BKP_DR3, data);
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

void HAL_Core_Enter_Bootloader(bool persist)
{
    if (persist)
    {
        RTC_WriteBackupRegister(RTC_BKP_DR10, 0xFFFF);
        system_flags.FLASH_OTA_Update_SysFlag = 0xFFFF;
        Save_SystemFlags();
    }
    else
    {
        RTC_WriteBackupRegister(RTC_BKP_DR1, ENTER_DFU_APP_REQUEST);
    }

    HAL_Core_System_Reset_Ex(RESET_REASON_DFU_MODE, 0, NULL);
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
    RTC_WriteBackupRegister(RTC_BKP_DR1, ENTER_SAFE_MODE_APP_REQUEST);
    HAL_Core_System_Reset_Ex(RESET_REASON_SAFE_MODE, 0, NULL);
}

int HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved)
{
    // Initial sanity check
    if ((pins_count == 0 || mode_count == 0 || pins == NULL || mode == NULL) && seconds <= 0) {
        return SYSTEM_ERROR_NOT_ALLOWED;
    }

    // Validate pins and modes
    if ((pins_count > 0 && pins == NULL) || (pins_count > 0 && mode_count == 0) || (mode_count > 0 && mode == NULL)) {
        return SYSTEM_ERROR_NOT_ALLOWED;
    }

    for (unsigned i = 0; i < pins_count; i++) {
        if (pins[i] >= TOTAL_PINS) {
            return SYSTEM_ERROR_NOT_ALLOWED;
        }
    }

    for (unsigned i = 0; i < mode_count; i++) {
        switch(mode[i]) {
            case RISING:
            case FALLING:
            case CHANGE:
                break;
            default:
                return SYSTEM_ERROR_NOT_ALLOWED;
        }
    }

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Detach USB
    HAL_USB_Detach();

    // Flush all USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++)
    {
        if (HAL_USART_Is_Enabled(usart))
        {
            HAL_USART_Flush_Data(usart);
        }
    }

    int32_t state = HAL_disable_irq();

    uint32_t exit_conditions = 0x00;

    // Suspend all EXTI interrupts
    HAL_Interrupts_Suspend();

    for (unsigned i = 0; i < pins_count; i++) {
        pin_t wakeUpPin = pins[i];
        InterruptMode edgeTriggerMode = (i < mode_count) ? mode[i] : mode[mode_count - 1];

        PinMode wakeUpPinMode = INPUT;
        /* Set required pinMode based on edgeTriggerMode */
        switch(edgeTriggerMode)
        {
        case RISING:
            wakeUpPinMode = INPUT_PULLDOWN;
            break;
        case FALLING:
            wakeUpPinMode = INPUT_PULLUP;
            break;
        case CHANGE:
        default:
            wakeUpPinMode = INPUT;
            break;
        }

        HAL_Pin_Mode(wakeUpPin, wakeUpPinMode);
        HAL_InterruptExtraConfiguration irqConf = {0};
        irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_2;
        irqConf.IRQChannelPreemptionPriority = 0;
        irqConf.IRQChannelSubPriority = 0;
        irqConf.keepHandler = 1;
        irqConf.keepPriority = 1;
        HAL_Interrupts_Attach(wakeUpPin, NULL, NULL, edgeTriggerMode, &irqConf);

        exit_conditions |= STOP_MODE_EXIT_CONDITION_PIN;
    }

    // Configure RTC wake-up
    if (seconds > 0) {
        /*
         * - To wake up from the Stop mode with an RTC alarm event, it is necessary to:
         * - Configure the EXTI Line 17 to be sensitive to rising edges (Interrupt
         * or Event modes) using the EXTI_Init() function.
         *
         */
        HAL_RTC_Cancel_UnixAlarm();
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

        // Connect RTC to EXTI line
        EXTI_InitTypeDef EXTI_InitStructure = {0};
        EXTI_InitStructure.EXTI_Line = EXTI_Line17;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        exit_conditions |= STOP_MODE_EXIT_CONDITION_RTC;
    }

    HAL_Core_Execute_Stop_Mode();

    int32_t reason = SYSTEM_ERROR_UNKNOWN;

    if (exit_conditions & STOP_MODE_EXIT_CONDITION_PIN) {
        STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
        for (unsigned i = 0; i < pins_count; i++) {
            pin_t wakeUpPin = pins[i];
            if (EXTI_GetITStatus(PIN_MAP[wakeUpPin].gpio_pin) != RESET) {
                reason = i + 1;
            }
            /* Detach the Interrupt pin */
            HAL_Interrupts_Detach_Ext(wakeUpPin, 1, NULL);
        }
    }

    if (exit_conditions & STOP_MODE_EXIT_CONDITION_RTC) {
        if (NVIC_GetPendingIRQ(RTC_Alarm_IRQn)) {
            reason = 0;
        }
        // No need to detach RTC Alarm from EXTI, since it will be detached in HAL_Interrupts_Restore()

        // RTC Alarm should be canceled to avoid entering HAL_RTCAlarm_Handler or if we were woken up by pin
        HAL_RTC_Cancel_UnixAlarm();
    }

    // Restore
    HAL_Interrupts_Restore();

    // Successfully exited STOP mode
    HAL_enable_irq(state);

    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    HAL_USB_Attach();

    return reason;
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
    InterruptMode m = (InterruptMode)edgeTriggerMode;
    HAL_Core_Enter_Stop_Mode_Ext(&wakeUpPin, 1, &m, 1, seconds, NULL);
}

void HAL_Core_Execute_Stop_Mode(void)
{
    /* Request to enter STOP mode with regulator in low power mode */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

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

int HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags)
{
    // Configure RTC wake-up
    if (seconds > 0) {
        HAL_RTC_Cancel_UnixAlarm();
        HAL_RTC_Set_UnixAlarm((time_t) seconds);
    }

    return HAL_Core_Execute_Standby_Mode_Ext(flags, NULL);
}

int HAL_Core_Execute_Standby_Mode_Ext(uint32_t flags, void* reserved)
{
    if ((flags & HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN) == 0) {
    /* Enable WKUP pin */
    PWR_WakeUpPinCmd(ENABLE);
    } else {
        /* Disable WKUP pin */
        PWR_WakeUpPinCmd(DISABLE);
    }

    /* Request to enter STANDBY mode */
    PWR_EnterSTANDBYMode();

    /* Following code will not be reached */
    while(1);

    return 0;
}

void HAL_Core_Execute_Standby_Mode(void)
{
    HAL_Core_Execute_Standby_Mode_Ext(0, NULL);
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
    return Compute_CRC32(pBuffer, bufferSize);
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

void generate_key()
{
    // normallly allocating such a large buffer on the stack would be a bad idea, however, we are quite near the start of execution, with few levels of recursion.
    char buf[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];
    // ensure the private key is provisioned

    // Reset the system after generating the key - reports of Serial not being available in listening mode
    // after generating the key.
    private_key_generation_t genspec;
    genspec.size = sizeof(genspec);
    genspec.gen = PRIVATE_KEY_GENERATE_MISSING;
    HAL_FLASH_Read_CorePrivateKey(buf, &genspec);
    if (genspec.generated_key)
        HAL_Core_System_Reset();
}

/**
 * The entrypoint to our application.
 * This should be called from the RTOS main thread once initialization has been
 * completed, constructors invoked and and HAL_Core_Config() has been called.
 */
void application_start()
{
    rtos_started = 1;

    // one the key is sent to the cloud, this can be removed, since the key is fetched in
    // Spark_Protocol_init(). This is just a temporary measure while the key still needs
    // to be fetched via DFU.

    HAL_Core_Setup();

    generate_key();

    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Load last reset info from RCC / backup registers
        Init_Last_Reset_Info();
    }

    app_setup_and_loop();
}


#ifdef UNUSED
#undef UNUSED
#endif
#define UNUSED(x) (void)(x)

uint8_t handle_timer(TIM_TypeDef* TIMx, uint16_t TIM_IT, hal_irq_t irq)
{
    uint8_t result = (TIM_GetITStatus(TIMx, TIM_IT)!=RESET);
    if (result) {
        HAL_System_Interrupt_Trigger(irq, NULL);
        TIM_ClearITPendingBit(TIMx, TIM_IT);
    }
    return result;
}


/**
 * The following tick hook will only get called if configUSE_TICK_HOOK
 * is set to 1 within FreeRTOSConfig.h
 */
void SysTickOverride(void)
{
    System1MsTick();

    /* Handle short and generic tasks for the device HAL on 1ms ticks */
    HAL_1Ms_Tick();

    HAL_SysTick_Handler();

    HAL_System_Interrupt_Trigger(SysInterrupt_SysTick, NULL);
}

/**
 * @brief  This function handles EXTI2_IRQ or EXTI_9_5_IRQ Handler.
 * @param  None
 * @retval None
 */
void Handle_Mode_Button_EXTI_irq(Button_TypeDef button)
{
    if (button == BUTTON1_MIRROR || EXTI_GetITStatus(HAL_Buttons[button].exti_line) != RESET)
    {
        /* Clear the EXTI line pending bit (cleared in WICED GPIO IRQ handler) */
        EXTI_ClearITPendingBit(HAL_Buttons[button].exti_line);

        HAL_Buttons[button].debounce_time = 0x00;
        HAL_Buttons[button].active = 1;

        /* Disable BUTTON1 Interrupt */
        if (button != BUTTON1_MIRROR)
            BUTTON_EXTI_Config(button, DISABLE);
        else
            HAL_Interrupts_Detach_Ext(HAL_Buttons[button].hal_pin, 1, NULL);

        /* Enable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    }

}


void ADC_irq()
{
    HAL_System_Interrupt_Trigger(SysInterrupt_ADC_IRQ, NULL);
}

/**
 * @brief  This function handles TIM1_CC_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM1_CC_irq(void)
{
    if(NULL != HAL_TIM1_Handler)
    {
        HAL_TIM1_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_CC_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM1, TIM_IT_CC1, SysInterrupt_TIM1_Compare1) ||
    handle_timer(TIM1, TIM_IT_CC2, SysInterrupt_TIM1_Compare2) ||
    handle_timer(TIM1, TIM_IT_CC3, SysInterrupt_TIM1_Compare3) ||
    handle_timer(TIM1, TIM_IT_CC4, SysInterrupt_TIM1_Compare4);
    UNUSED(result);
}

/**
 * @brief  This function handles TIM2_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM2_irq(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        if (HAL_Buttons[BUTTON1].active && BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
        {
            if (!HAL_Buttons[BUTTON1].debounce_time)
            {
                HAL_Buttons[BUTTON1].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
                HAL_Notify_Button_State(BUTTON1, true);
            }
            HAL_Buttons[BUTTON1].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
        }
        else if (HAL_Buttons[BUTTON1].active)
        {
            HAL_Buttons[BUTTON1].active = 0;
            HAL_Core_Mode_Button_Reset(BUTTON1);
        }

        if (HAL_Buttons[BUTTON1_MIRROR].port && HAL_Buttons[BUTTON1_MIRROR].active &&
            BUTTON_GetState(BUTTON1_MIRROR) == (HAL_Buttons[BUTTON1_MIRROR].interrupt_mode == RISING ? 1 : 0)) {
            if (!HAL_Buttons[BUTTON1_MIRROR].debounce_time)
            {
                HAL_Buttons[BUTTON1_MIRROR].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
                HAL_Notify_Button_State(BUTTON1_MIRROR, true);
            }
            HAL_Buttons[BUTTON1_MIRROR].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
        }
        else if (HAL_Buttons[BUTTON1_MIRROR].port && HAL_Buttons[BUTTON1_MIRROR].active)
        {
            HAL_Buttons[BUTTON1_MIRROR].active = 0;
            HAL_Core_Mode_Button_Reset(BUTTON1_MIRROR);
        }
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM2_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM2, TIM_IT_CC1, SysInterrupt_TIM2_Compare1) ||
    handle_timer(TIM2, TIM_IT_CC2, SysInterrupt_TIM2_Compare2) ||
    handle_timer(TIM2, TIM_IT_CC3, SysInterrupt_TIM2_Compare3) ||
    handle_timer(TIM2, TIM_IT_CC4, SysInterrupt_TIM2_Compare4) ||
    handle_timer(TIM2, TIM_IT_Update, SysInterrupt_TIM2_Update) ||
    handle_timer(TIM2, TIM_IT_Trigger, SysInterrupt_TIM2_Trigger);
    UNUSED(result);
}

/**
 * @brief  This function handles TIM3_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM3_irq(void)
{
    if(NULL != HAL_TIM3_Handler)
    {
        HAL_TIM3_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM3_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM3, TIM_IT_CC1, SysInterrupt_TIM3_Compare1) ||
    handle_timer(TIM3, TIM_IT_CC2, SysInterrupt_TIM3_Compare2) ||
    handle_timer(TIM3, TIM_IT_CC3, SysInterrupt_TIM3_Compare3) ||
    handle_timer(TIM3, TIM_IT_CC4, SysInterrupt_TIM3_Compare4) ||
    handle_timer(TIM3, TIM_IT_Update, SysInterrupt_TIM3_Update) ||
    handle_timer(TIM3, TIM_IT_Trigger, SysInterrupt_TIM3_Trigger);
    UNUSED(result);
}

/**
 * @brief  This function handles TIM4_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM4_irq(void)
{
    if(NULL != HAL_TIM4_Handler)
    {
        HAL_TIM4_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM4_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM4, TIM_IT_CC1, SysInterrupt_TIM4_Compare1) ||
    handle_timer(TIM4, TIM_IT_CC2, SysInterrupt_TIM4_Compare2) ||
    handle_timer(TIM4, TIM_IT_CC3, SysInterrupt_TIM4_Compare3) ||
    handle_timer(TIM4, TIM_IT_CC4, SysInterrupt_TIM4_Compare4) ||
    handle_timer(TIM4, TIM_IT_Update, SysInterrupt_TIM4_Update) ||
    handle_timer(TIM4, TIM_IT_Trigger, SysInterrupt_TIM4_Trigger);
    UNUSED(result);

}

/**
 * @brief  This function handles TIM5_IRQ Handler.
 * @param  None
 * @retval None
 */
void TIM5_irq(void)
{
    if(NULL != HAL_TIM5_Handler)
    {
        HAL_TIM5_Handler();
    }

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM5_IRQ, NULL);
    uint8_t result =
    handle_timer(TIM5, TIM_IT_CC1, SysInterrupt_TIM5_Compare1) ||
    handle_timer(TIM5, TIM_IT_CC2, SysInterrupt_TIM5_Compare2) ||
    handle_timer(TIM5, TIM_IT_CC3, SysInterrupt_TIM5_Compare3) ||
    handle_timer(TIM5, TIM_IT_CC4, SysInterrupt_TIM5_Compare4) ||
    handle_timer(TIM5, TIM_IT_Update, SysInterrupt_TIM5_Update) ||
    handle_timer(TIM5, TIM_IT_Trigger, SysInterrupt_TIM5_Trigger);
    UNUSED(result);
}

void TIM6_DAC_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM6_DAC_IRQ, NULL);
    handle_timer(TIM6, TIM_IT_Update, SysInterrupt_TIM6_Update);
}

void TIM7_override(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM7_IRQ, NULL);
    handle_timer(TIM7, TIM_IT_Update, SysInterrupt_TIM7_Update);
}

void TIM8_BRK_TIM12_irq(void)
{

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_BRK_TIM12_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_Break, SysInterrupt_TIM8_Break) ||
    handle_timer(TIM12, TIM_IT_CC1, SysInterrupt_TIM12_Compare1) ||
    handle_timer(TIM12, TIM_IT_CC2, SysInterrupt_TIM12_Compare2) ||
    handle_timer(TIM12, TIM_IT_Update, SysInterrupt_TIM12_Update) ||
    handle_timer(TIM12, TIM_IT_Trigger, SysInterrupt_TIM12_Trigger);
    UNUSED(result);
}

void TIM8_UP_TIM13_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_UP_TIM13_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_Update, SysInterrupt_TIM8_Update) ||
    handle_timer(TIM13, TIM_IT_CC1, SysInterrupt_TIM13_Compare) ||
    handle_timer(TIM13, TIM_IT_Update, SysInterrupt_TIM13_Update);
    UNUSED(result);
}

void TIM8_TRG_COM_TIM14_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_TRG_COM_TIM14_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_Trigger, SysInterrupt_TIM8_Trigger) ||
    handle_timer(TIM14, TIM_IT_COM, SysInterrupt_TIM14_COM) ||
    handle_timer(TIM14, TIM_IT_CC1, SysInterrupt_TIM14_Compare);
    handle_timer(TIM14, TIM_IT_Update, SysInterrupt_TIM14_Update);
    UNUSED(result);
}

void TIM8_CC_irq(void)
{
#if PLATFORM_ID == 10 // Electron
    if(NULL != HAL_TIM8_Handler)
    {
        HAL_TIM8_Handler();
    }
#endif

    HAL_System_Interrupt_Trigger(SysInterrupt_TIM8_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM8, TIM_IT_CC1, SysInterrupt_TIM8_Compare1) ||
    handle_timer(TIM8, TIM_IT_CC2, SysInterrupt_TIM8_Compare2) ||
    handle_timer(TIM8, TIM_IT_CC3, SysInterrupt_TIM8_Compare3) ||
    handle_timer(TIM8, TIM_IT_CC4, SysInterrupt_TIM8_Compare4);
    UNUSED(result);
}

void TIM1_BRK_TIM9_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_BRK_TIM9_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM1, TIM_IT_Break, SysInterrupt_TIM1_Break) ||
    handle_timer(TIM9, TIM_IT_CC1, SysInterrupt_TIM9_Compare1) ||
    handle_timer(TIM9, TIM_IT_CC2, SysInterrupt_TIM9_Compare2) ||
    handle_timer(TIM9, TIM_IT_Update, SysInterrupt_TIM9_Update) ||
    handle_timer(TIM9, TIM_IT_Trigger, SysInterrupt_TIM9_Trigger);
    UNUSED(result);
}

void TIM1_UP_TIM10_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_UP_TIM10_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM1, TIM_IT_Update, SysInterrupt_TIM1_Update) ||
    handle_timer(TIM10, TIM_IT_CC1, SysInterrupt_TIM10_Compare) ||
    handle_timer(TIM10, TIM_IT_Update, SysInterrupt_TIM10_Update);
    UNUSED(result);
}

void TIM1_TRG_COM_TIM11_irq(void)
{
    HAL_System_Interrupt_Trigger(SysInterrupt_TIM1_TRG_COM_TIM11_IRQ, NULL);

    uint8_t result =
    handle_timer(TIM1, TIM_IT_Trigger, SysInterrupt_TIM1_Trigger) ||
    handle_timer(TIM1, TIM_IT_COM, SysInterrupt_TIM1_COM) ||
    handle_timer(TIM11, TIM_IT_CC1, SysInterrupt_TIM11_Compare) ||
    handle_timer(TIM11, TIM_IT_Update, SysInterrupt_TIM11_Update);
    UNUSED(result);
}

void CAN1_TX_irq()
{
    if(NULL != HAL_CAN1_TX_Handler)
    {
        HAL_CAN1_TX_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN1_TX_IRQ, NULL);
}

void CAN1_RX0_irq()
{
    if(NULL != HAL_CAN1_RX0_Handler)
    {
        HAL_CAN1_RX0_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN1_RX0_IRQ, NULL);
}

void CAN1_RX1_irq()
{
    if(NULL != HAL_CAN1_RX1_Handler)
    {
        HAL_CAN1_RX1_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN1_RX1_IRQ, NULL);
}

void CAN1_SCE_irq()
{
    if(NULL != HAL_CAN1_SCE_Handler)
    {
        HAL_CAN1_SCE_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN1_SCE_IRQ, NULL);
}

void CAN2_TX_irq()
{
    if(NULL != HAL_CAN2_TX_Handler)
    {
        HAL_CAN2_TX_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_TX_IRQ, NULL);
}

void CAN2_RX0_irq()
{
    if(NULL != HAL_CAN2_RX0_Handler)
    {
        HAL_CAN2_RX0_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_RX0_IRQ, NULL);
}

void CAN2_RX1_irq()
{
    if(NULL != HAL_CAN2_RX1_Handler)
    {
        HAL_CAN2_RX1_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_RX1_IRQ, NULL);
}

void CAN2_SCE_irq()
{
    if(NULL != HAL_CAN2_SCE_Handler)
    {
        HAL_CAN2_SCE_Handler();
    }
    HAL_System_Interrupt_Trigger(SysInterrupt_CAN2_SCE_IRQ, NULL);
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
    case POWER_BROWNOUT_RESET:
        return Is_System_Reset_Flag_Set(RCC_FLAG_BORRST);
    default:
        return false;
    }
}

unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved)
{
    return SystemCoreClock;
}

extern size_t pvPortLargestFreeBlock();

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    struct mallinfo heapinfo = mallinfo();
    // fordblks  The total number of bytes in free blocks.
    info->freeheap = heapinfo.fordblks;
    if (offsetof(runtime_info_t, total_init_heap) + sizeof(info->total_init_heap) <= info->size) {
        info->total_init_heap = (uint32_t)(&link_heap_location_end - &link_heap_location);
    }

    if (offsetof(runtime_info_t, total_heap) + sizeof(info->total_heap) <= info->size) {
        info->total_heap = heapinfo.arena;
    }

    if (offsetof(runtime_info_t, max_used_heap) + sizeof(info->max_used_heap) <= info->size) {
        info->max_used_heap = heapinfo.usmblks;
    }

    if (offsetof(runtime_info_t, user_static_ram) + sizeof(info->user_static_ram) <= info->size) {
        info->user_static_ram = info->total_init_heap - info->total_heap;
    }

    if (offsetof(runtime_info_t, largest_free_block_heap) + sizeof(info->largest_free_block_heap) <= info->size) {
    		info->largest_free_block_heap = pvPortLargestFreeBlock();
    }

    return 0;
}

void vApplicationMallocFailedHook(size_t xWantedSize)
{
	hal_notify_event(HAL_EVENT_OUT_OF_MEMORY, xWantedSize, 0);
}

int HAL_Feature_Set(HAL_Feature feature, bool enabled)
{
    switch (feature)
    {
        case FEATURE_RETAINED_MEMORY:
        {
            FunctionalState state = enabled ? ENABLE : DISABLE;
            // Switch on backup SRAM clock
            // Switch on backup power regulator, so that it survives the deep sleep mode,
            // software and hardware reset. Power must be supplied to VIN or VBAT to retain SRAM values.
            PWR_BackupRegulatorCmd(state);
            // Wait until backup power regulator is ready, should be fairly instantaneous... but timeout in 10ms.
            if (state == ENABLE) {
                system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                while (PWR_GetFlagStatus(PWR_FLAG_BRR) == RESET) {
                    if (HAL_Timer_Get_Milli_Seconds() - start > 10UL) {
                        return -2;
                    }
                };
            }
            return 0;
        }
        case FEATURE_RESET_INFO:
        {
            return Write_Feature_Flag(FEATURE_FLAG_RESET_INFO, enabled, NULL);
        }

#if PLATFORM_ID==PLATFORM_P1
        case FEATURE_WIFI_POWERSAVE_CLOCK:
        {
            wwd_set_wlan_sleep_clock_enabled(enabled);
            uint8_t current = 0;
            dct_read_app_data_copy(DCT_RADIO_FLAGS_OFFSET, &current, sizeof(current));
            current &= 0xFC;
            if (!enabled) {
                current |= 2;   // 0bxxxxxx10 to disable the clock, any other value to enable it.
            }
            dct_write_app_data(&current, DCT_RADIO_FLAGS_OFFSET, 1);
            return 0;
        }
#endif
#if HAL_PLATFORM_CLOUD_UDP
        case FEATURE_CLOUD_UDP: {
            const uint8_t data = (enabled ? 0xff : 0x00);
            return dct_write_app_data(&data, DCT_CLOUD_TRANSPORT_OFFSET, sizeof(data));
        }
#endif // HAL_PLATFORM_CLOUD_UDP
    }
    return -1;
}

bool HAL_Feature_Get(HAL_Feature feature)
{
    switch (feature)
    {
        // Warm Start: active when resuming from Standby mode (deep sleep)
        case FEATURE_WARM_START:
        {
            return (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET);
        }
        // Retained Memory: active when backup regulator is enabled
        case FEATURE_RETAINED_MEMORY:
        {
            return (PWR_GetFlagStatus(PWR_FLAG_BRR) != RESET);
        }

        case FEATURE_CLOUD_UDP:
        {
        		uint8_t value = false;
#if HAL_PLATFORM_CLOUD_UDP
        		dct_read_app_data_copy(DCT_CLOUD_TRANSPORT_OFFSET, &value, sizeof(value));
        		value = value==0xFF;		// default is to use UDP
#endif
        		return value;
        }
        case FEATURE_RESET_INFO:
        {
            bool value = false;
            return (Read_Feature_Flag(FEATURE_FLAG_RESET_INFO, &value) == 0) ? value : false;
        }
    }
    return false;
}

void HAL_Core_Mode_Button_Mirror_Pressed(void* param) {
    Handle_Mode_Button_EXTI_irq(BUTTON1_MIRROR);
}

static void BUTTON_Mirror_Init() {
    if (HAL_Buttons[BUTTON1_MIRROR].port) {
        PinMode pinMode = INPUT_PULLUP;
        switch(HAL_Buttons[BUTTON1_MIRROR].interrupt_mode)
        {
        case RISING:
            pinMode = INPUT_PULLDOWN;
            break;
        case FALLING:
            pinMode = INPUT_PULLUP;
            break;
        }

        int32_t state = HAL_disable_irq();
        HAL_Pin_Mode(HAL_Buttons[BUTTON1_MIRROR].hal_pin, pinMode);
        HAL_Interrupts_Attach(HAL_Buttons[BUTTON1_MIRROR].hal_pin, HAL_Core_Mode_Button_Mirror_Pressed, NULL, HAL_Buttons[BUTTON1_MIRROR].interrupt_mode, NULL);
        if (HAL_Buttons[BUTTON1_MIRROR].exti_line == HAL_Buttons[BUTTON1].exti_line) {
            HAL_Buttons[BUTTON1].exti_port_source = HAL_Buttons[BUTTON1_MIRROR].exti_port_source;
            HAL_Buttons[BUTTON1].exti_pin_source = HAL_Buttons[BUTTON1_MIRROR].exti_pin_source;
            HAL_Buttons[BUTTON1].port = HAL_Buttons[BUTTON1_MIRROR].port;
        }
        HAL_enable_irq(state);
    }
}

static void BUTTON_Mirror_Persist(button_config_t* conf) {
    button_config_t saved_config;
    dct_read_app_data_copy(DCT_MODE_BUTTON_MIRROR_OFFSET, &saved_config, sizeof(saved_config));

    if (conf) {
        if (saved_config.active == 0xFF || memcmp((void*)conf, (void*)&saved_config, sizeof(button_config_t)))
        {
            dct_write_app_data((void*)conf, DCT_MODE_BUTTON_MIRROR_OFFSET, sizeof(button_config_t));
        }
    } else {
        if (saved_config.active != 0xFF) {
            memset((void*)&saved_config, 0xff, sizeof(button_config_t));
            dct_write_app_data((void*)&saved_config, DCT_MODE_BUTTON_MIRROR_OFFSET, sizeof(button_config_t));
        }
    }
}

void HAL_Core_Button_Mirror_Pin_Disable(uint8_t bootloader, uint8_t button, void* reserved) {
    (void)button; // unused
    int32_t state = HAL_disable_irq();
    if (HAL_Buttons[BUTTON1_MIRROR].port) {
        HAL_Interrupts_Detach_Ext(HAL_Buttons[BUTTON1_MIRROR].hal_pin, 1, NULL);
        HAL_Buttons[BUTTON1_MIRROR].active = 0;
        HAL_Buttons[BUTTON1_MIRROR].port = 0;
    }
    HAL_enable_irq(state);

    if (bootloader) {
        BUTTON_Mirror_Persist(NULL);
    }
}

static inline uint32_t Gpio_Peripheral_To_Port_Source(GPIO_TypeDef* gpio)
{
    switch((uint32_t)gpio) {
        case (uint32_t)GPIOA:
            return 0;
        case (uint32_t)GPIOB:
            return 1;
        case (uint32_t)GPIOC:
            return 2;
        case (uint32_t)GPIOD:
            return 3;
    }

    return 0;
}

static inline uint32_t Gpio_Peripheral_To_Clk(GPIO_TypeDef* gpio) {
    switch((uint32_t)gpio) {
        case (uint32_t)GPIOA:
            return RCC_AHB1Periph_GPIOA;
        case (uint32_t)GPIOB:
            return RCC_AHB1Periph_GPIOB;
        case (uint32_t)GPIOC:
            return RCC_AHB1Periph_GPIOC;
        case (uint32_t)GPIOD:
            return RCC_AHB1Periph_GPIOD;
    }

    return 0;
}

static inline uint32_t Tim_Peripheral_To_Af(TIM_TypeDef* tim) {
    switch((uint32_t)tim) {
        case (uint32_t)TIM1:
            return GPIO_AF_TIM1;
        case (uint32_t)TIM2:
            return GPIO_AF_TIM2;
        case (uint32_t)TIM3:
            return GPIO_AF_TIM3;
        case (uint32_t)TIM4:
            return GPIO_AF_TIM4;
        case (uint32_t)TIM5:
            return GPIO_AF_TIM5;
#if PLATFORM_ID == 10
        case (uint32_t)TIM8:
            return GPIO_AF_TIM8;
#endif // PLATFORM_ID == 10
    }

    return 0;
}

void HAL_Core_Button_Mirror_Pin(uint16_t pin, InterruptMode mode, uint8_t bootloader, uint8_t button, void *reserved) {
    (void)button; // unused
    STM32_Pin_Info* pinmap = HAL_Pin_Map();
    if (pin > TOTAL_PINS)
        return;

    if (mode != RISING && mode != FALLING)
        return;

    uint8_t gpio_port_source = Gpio_Peripheral_To_Port_Source(pinmap[pin].gpio_peripheral);
    uint32_t gpio_clk = Gpio_Peripheral_To_Clk(pinmap[pin].gpio_peripheral);

    button_config_t conf = {
        .port = pinmap[pin].gpio_peripheral,
        .pin = pinmap[pin].gpio_pin,
        .hal_pin = pin,
        .debounce_time = 0,
        .interrupt_mode = mode,

        .exti_line = pinmap[pin].gpio_pin,
        .exti_pin_source = pinmap[pin].gpio_pin_source,
        .exti_port_source = gpio_port_source,
        .exti_trigger = mode == RISING ? EXTI_Trigger_Rising : EXTI_Trigger_Falling
    };

    HAL_Buttons[BUTTON1_MIRROR] = conf;

    BUTTON_Mirror_Init();

    if (pinmap[pin].gpio_pin == HAL_Buttons[BUTTON1].pin) {
        LOG(WARN, "Pin %d shares the same EXTI as SETUP/MODE button", pin);
        BUTTON_Mirror_Persist(NULL);
        return;
    }

    if (!bootloader) {
        BUTTON_Mirror_Persist(NULL);
        return;
    }

    // Construct button_config_t for bootloader
    button_config_t bootloader_conf = {
        .active = 0xAA,
        .port = pinmap[pin].gpio_peripheral,
        .pin = pinmap[pin].gpio_pin,
        .clk = gpio_clk,
        .mode = GPIO_Mode_IN,
        .pupd = mode == RISING ? GPIO_PuPd_DOWN : GPIO_PuPd_UP,
        .debounce_time = 0xBBCC,

        .exti_line = pinmap[pin].gpio_pin,
        .exti_port_source = gpio_port_source,
        .exti_pin_source = pinmap[pin].gpio_pin_source,
        .exti_irqn = HAL_Interrupts_Pin_IRQn(pin),
        .exti_irq_prio = HAL_Buttons[BUTTON1].exti_irq_prio,
        .exti_trigger = mode == RISING ? EXTI_Trigger_Rising : EXTI_Trigger_Falling
    };

    BUTTON_Mirror_Persist(&bootloader_conf);
}

static void LED_Mirror_Persist(uint8_t led, led_config_t* conf) {
    const size_t offset = DCT_LED_MIRROR_OFFSET + ((led - LED_MIRROR_OFFSET) * sizeof(led_config_t));
    led_config_t saved_config;
    dct_read_app_data_copy(offset, &saved_config, sizeof(saved_config));

    if (conf) {
        if (saved_config.version == 0xFF || memcmp((void*)conf, (void*)&saved_config, sizeof(led_config_t)))
        {
            dct_write_app_data((void*)conf, offset, sizeof(led_config_t));
        }
    } else {
        if (saved_config.version != 0xFF) {
            memset((void*)&saved_config, 0xff, sizeof(led_config_t));
            dct_write_app_data((void*)&saved_config, offset, sizeof(led_config_t));
        }
    }
}

void HAL_Core_Led_Mirror_Pin_Disable(uint8_t led, uint8_t bootloader, void* reserved)
{
    int32_t state = HAL_disable_irq();
    led_config_t* ledc = HAL_Led_Get_Configuration(led, NULL);
    if (ledc->is_active) {
        ledc->is_active = 0;
        HAL_Pin_Mode(ledc->hal_pin, INPUT);
    }
    HAL_enable_irq(state);

    if (bootloader) {
        LED_Mirror_Persist(led, NULL);
    }
}

void HAL_Core_Led_Mirror_Pin(uint8_t led, pin_t pin, uint32_t flags, uint8_t bootloader, void* reserved)
{
    if (pin > TOTAL_PINS)
        return;

    STM32_Pin_Info* pinmap = HAL_Pin_Map();

    if (!pinmap[pin].timer_peripheral)
        return;

    // NOTE: `flags` currently only control whether the LED state should be inverted
    // NOTE: All mirrored LEDs are currently PWM

    led_config_t conf = {
        .version = 0x01,
        .port = pinmap[pin].gpio_peripheral,
        .pin = pinmap[pin].gpio_pin,
        .hal_pin = pin,
        .hal_mode = AF_OUTPUT_PUSHPULL,
        .pin_source = pinmap[pin].gpio_pin_source,
        .af = 0,
        .tim_peripheral = 0,
        .tim_channel = 0,
        .is_active = 1,
        .is_hal_pin = 1,
        .is_inverted = flags & 1,
        .is_pwm = 1
    };

    int32_t state = HAL_disable_irq();
    HAL_Led_Init(led, &conf, NULL);
    HAL_enable_irq(state);

    if (!bootloader) {
        LED_Mirror_Persist(led, NULL);
        return;
    }

    led_config_t bootloader_conf = {
        .version = 0x01,
        .port = pinmap[pin].gpio_peripheral,
        .pin = pinmap[pin].gpio_pin,
        .clk = Gpio_Peripheral_To_Clk(pinmap[pin].gpio_peripheral),
        .mode = GPIO_Mode_AF,
        .pin_source = pinmap[pin].gpio_pin_source,
        .af = Tim_Peripheral_To_Af(pinmap[pin].timer_peripheral),
        .tim_peripheral = pinmap[pin].timer_peripheral,
        .tim_channel = pinmap[pin].timer_ch,
        .is_active = 1,
        .is_hal_pin = 0,
        .is_inverted = flags & 1,
        .is_pwm = 1
    };

    LED_Mirror_Persist(led, &bootloader_conf);
}

int HAL_Core_Enter_Panic_Mode(void* reserved)
{
    __disable_irq();
    return 0;
}

#if HAL_PLATFORM_CLOUD_UDP

#include "dtls_session_persist.h"
#include "deepsleep_hal_impl.h"
#include <string.h>

retained_system SessionPersistDataOpaque session;

int HAL_System_Backup_Save(size_t offset, const void* buffer, size_t length, void* reserved)
{
	if (offset==0 && length==sizeof(SessionPersistDataOpaque))
	{
		memcpy(&session, buffer, length);
		return 0;
	}
	return -1;
}

int HAL_System_Backup_Restore(size_t offset, void* buffer, size_t max_length, size_t* length, void* reserved)
{
	if (offset==0 && max_length>=sizeof(SessionPersistDataOpaque) && session.size==sizeof(SessionPersistDataOpaque))
	{
		*length = sizeof(SessionPersistDataOpaque);
		memcpy(buffer, &session, sizeof(session));
		return 0;
	}
	return -1;
}


#else

int HAL_System_Backup_Save(size_t offset, const void* buffer, size_t length, void* reserved)
{
	return -1;
}

int HAL_System_Backup_Restore(size_t offset, void* buffer, size_t max_length, size_t* length, void* reserved)
{
	return -1;
}

#endif

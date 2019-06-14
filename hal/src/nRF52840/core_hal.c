/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "core_hal.h"
#include "service_debug.h"
#include <nrf52840.h>
#include "hal_event.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "hw_config.h"
#include "syshealth_hal.h"
#include <nrfx_types.h>
#include <nrf_mbr.h>
#include <nrf_sdm.h>
#include <nrf_sdh.h>
#include <nrf_rtc.h>
#include "button_hal.h"
#include "hal_platform.h"
#include "user.h"
#include "dct.h"
#include "rng_hal.h"
#include "interrupts_hal.h"
#include <nrf_power.h>
#include "ota_module.h"
#include "bootloader.h"
#include <stdlib.h>
#include <malloc.h>
#include "rtc_hal.h"
#include "timer_hal.h"
#include "pinmap_impl.h"
#include <nrf_pwm.h>
#include "system_error.h"
#include <nrf_lpcomp.h>
#include <nrfx_gpiote.h>
#include <nrf_drv_clock.h>
#include "usb_hal.h"
#include "usart_hal.h"
#include "system_error.h"
#include <nrf_rtc.h>
#include "gpio_hal.h"
#include "exflash_hal.h"
#include "flash_common.h"
#include <nrf_pwm.h>
#include "concurrent_hal.h"

#define BACKUP_REGISTER_NUM        10
static int32_t backup_register[BACKUP_REGISTER_NUM] __attribute__((section(".backup_registers")));
static volatile uint8_t rtos_started = 0;

static struct Last_Reset_Info {
    int reason;
    uint32_t data;
} last_reset_info = { RESET_REASON_NONE, 0 };

typedef enum Feature_Flag {
    FEATURE_FLAG_RESET_INFO = 0x01,
    FEATURE_FLAG_ETHERNET_DETECTION = 0x02
} Feature_Flag;

typedef enum {
    STOP_MODE_EXIT_CONDITION_NONE = 0x00,
    STOP_MODE_EXIT_CONDITION_PIN  = 0x01,
    STOP_MODE_EXIT_CONDITION_RTC  = 0x02
} stop_mode_exit_condition_t;

void HardFault_Handler( void ) __attribute__( ( naked ) );

void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);

void SysTick_Handler(void);
void SysTickOverride(void);

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info);
void app_error_handler_bare(uint32_t err);

extern char link_heap_location, link_heap_location_end;
extern char link_interrupt_vectors_location;
extern char link_ram_interrupt_vectors_location;
extern char link_ram_interrupt_vectors_location_end;
extern char _Stack_Init;

static void* new_heap_end = &link_heap_location_end;

extern void malloc_enable(uint8_t);
extern void malloc_set_heap_end(void*);
extern void* malloc_heap_end();
#if defined(MODULAR_FIRMWARE)
void* module_user_pre_init();
#endif

__attribute__((externally_visible)) void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress ) {
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
    } else {
        PANIC(HardFault,"HardFault");

        /* Go to infinite loop when Hard Fault exception occurs */
        while (1) {
            ;
        }
    }
}


void HardFault_Handler(void) {
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

void app_error_fault_handler(uint32_t _id, uint32_t _pc, uint32_t _info) {
    volatile uint32_t id = _id;
    volatile uint32_t pc = _pc;
    volatile uint32_t info = _info;
    (void)id; (void)pc; (void)info;
    PANIC(HardFault,"HardFault");
    while(1) {
        ;
    }
}

void app_error_handler_bare(uint32_t error_code) {
    PANIC(HardFault,"HardFault");
    while(1) {
        ;
    }
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    PANIC(HardFault,"HardFault");
    while(1) {
    }
}

void MemManage_Handler(void) {
    /* Go to infinite loop when Memory Manage exception occurs */
    PANIC(MemManage,"MemManage");
    while (1) {
        ;
    }
}

void BusFault_Handler(void) {
    /* Go to infinite loop when Bus Fault exception occurs */
    PANIC(BusFault,"BusFault");
    while (1) {
        ;
    }
}

void UsageFault_Handler(void) {
    /* Go to infinite loop when Usage Fault exception occurs */
    PANIC(UsageFault,"UsageFault");
    while (1) {
        ;
    }
}

void SysTickOverride(void) {
    HAL_SysTick_Handler();
}

void SysTickChain() {
    SysTick_Handler();
    SysTickOverride();
}

/**
 * Called by HAL_Core_Init() to pre-initialize any low level hardware before
 * the main loop runs.
 */
void HAL_Core_Init_finalize(void) {
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    isrs[IRQN_TO_IDX(SysTick_IRQn)] = (uint32_t)SysTickChain;
}

void HAL_Core_Init(void) {
    HAL_Core_Init_finalize();
}

void HAL_Core_Config_systick_configuration(void) {
    // SysTick_Configuration();
    // sd_nvic_EnableIRQ(SysTick_IRQn);
    // dcd_migrate_data();
}

/**
 * Called by HAL_Core_Config() to allow the HAL implementation to override
 * the interrupt table if required.
 */
void HAL_Core_Setup_override_interrupts(void) {
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    /* Set MBR to forward interrupts to application */
    *((volatile uint32_t*)0x20000000) = (uint32_t)isrs;
    /* Reset SoftDevice vector address */
    *((volatile uint32_t*)0x20000004) = 0xFFFFFFFF;

    SCB->VTOR = 0x0;

    /* Init softdevice */
    sd_mbr_command_t com = {SD_MBR_COMMAND_INIT_SD, };
    uint32_t ret = sd_mbr_command(&com);
    SPARK_ASSERT(ret == NRF_SUCCESS);
    /* Forward unhandled interrupts to the application */
    sd_softdevice_vector_table_base_set((uint32_t)isrs);
    /* Enable softdevice */
    nrf_sdh_enable_request();
    /* Wait until softdevice enabled*/
    while (!nrf_sdh_is_enabled()) {
        ;
    }
}

void HAL_Core_Restore_Interrupt(IRQn_Type irqn) {
    uint32_t handler = ((const uint32_t*)&link_interrupt_vectors_location)[IRQN_TO_IDX(irqn)];

    // Special chain handler
    if (irqn == SysTick_IRQn) {
        handler = (uint32_t)SysTickChain;
    }

    volatile uint32_t* isrs = (volatile uint32_t*)&link_ram_interrupt_vectors_location;
    isrs[IRQN_TO_IDX(irqn)] = handler;
}

/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void) {
    DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);

#ifdef DFU_BUILD_ENABLE
    USE_SYSTEM_FLAGS = 1;
#endif

    /* Forward interrupts */
    memcpy(&link_ram_interrupt_vectors_location, &link_interrupt_vectors_location, &link_ram_interrupt_vectors_location_end-&link_ram_interrupt_vectors_location);
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    SCB->VTOR = (uint32_t)isrs;

    Set_System();

    hal_timer_init(NULL);

    HAL_Core_Setup_override_interrupts();

    HAL_RNG_Configuration();

    HAL_RTC_Configuration();

#if defined(MODULAR_FIRMWARE)
    if (HAL_Core_Validate_User_Module()) {
        new_heap_end = module_user_pre_init();
        if (new_heap_end > malloc_heap_end()) {
            malloc_set_heap_end(new_heap_end);
        }
    } else {
        // Update the user module if needed
        user_update_if_needed();
    }

    // Enable malloc before littlefs initialization.
    malloc_enable(1);
#endif

#ifdef DFU_BUILD_ENABLE
    Load_SystemFlags();
#endif

    // TODO: Use current LED theme
    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);

    FLASH_AddToFactoryResetModuleSlot(
      FLASH_INTERNAL, EXTERNAL_FLASH_FAC_XIP_ADDRESS,
      FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE,
      FACTORY_RESET_MODULE_FUNCTION, MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS); //true to verify the CRC during copy also
}

void HAL_Core_Setup(void) {
    /* DOES NOT DO ANYTHING
     * SysTick is enabled within FreeRTOS
     */
    HAL_Core_Config_systick_configuration();

    if (bootloader_update_if_needed()) {
        HAL_Core_System_Reset();
    }
}

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
bool HAL_Core_Validate_User_Module(void)
{
    bool valid = false;

    //CRC verification Enabled by default
    if (FLASH_isUserModuleInfoValid(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, USER_FIRMWARE_IMAGE_LOCATION))
    {
        //CRC check the user module and set to module_user_part_validated
        valid = FLASH_VerifyCRC32(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION,
                                  FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION))
                && HAL_Verify_User_Dependencies();
    }
    else if(FLASH_isUserModuleInfoValid(FLASH_INTERNAL, EXTERNAL_FLASH_FAC_XIP_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION))
    {
        // If user application is invalid, we should at least enable
        // the heap allocation for littlelf to set system flags.
        malloc_enable(1);

        //Reset and let bootloader perform the user module factory reset
        //Doing this instead of calling FLASH_RestoreFromFactoryResetModuleSlot()
        //saves precious system_part2 flash size i.e. fits in < 128KB
        HAL_Core_Factory_Reset();

        while(1);//Device should reset before reaching this line
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
    bounds = find_module_bounds(MODULE_FUNCTION_BOOTLOADER, 0, HAL_PLATFORM_MCU_DEFAULT);
    module_fetched = fetch_module(&mod, bounds, false, MODULE_VALIDATION_INTEGRITY);

    valid = module_fetched && (mod.validity_checked == mod.validity_result);

    if (!valid) {
        return valid;
    }

    // Now check system-parts
    int i = 0;
    if (flags & 1) {
        // Validate only that system-part that depends on bootloader passes dependency check
        i = 1;
    }
    do {
        bounds = find_module_bounds(MODULE_FUNCTION_SYSTEM_PART, i++, HAL_PLATFORM_MCU_DEFAULT);
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

bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration) {
    bool pressedState = false;

    if(BUTTON_GetDebouncedTime(BUTTON1) >= pressedMillisDuration) {
        pressedState = true;
    }
    if(BUTTON_GetDebouncedTime(BUTTON1_MIRROR) >= pressedMillisDuration) {
        pressedState = true;
    }

    return pressedState;
}

void HAL_Core_Mode_Button_Reset(uint16_t button)
{
}

void HAL_Core_System_Reset(void) {
    NVIC_SystemReset();
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved) {
    if (HAL_Feature_Get(FEATURE_RESET_INFO)) {
        // Save reset info to backup registers
        HAL_Core_Write_Backup_Register(BKP_DR_02, reason);
        HAL_Core_Write_Backup_Register(BKP_DR_03, data);
    }

    HAL_Core_System_Reset();
}

void HAL_Core_Factory_Reset(void) {
    system_flags.Factory_Reset_SysFlag = 0xAAAA;
    Save_SystemFlags();
    HAL_Core_System_Reset_Ex(RESET_REASON_FACTORY_RESET, 0, NULL);
}

void HAL_Core_Enter_Safe_Mode(void* reserved) {
    HAL_Core_Write_Backup_Register(BKP_DR_01, ENTER_SAFE_MODE_APP_REQUEST);
    HAL_Core_System_Reset_Ex(RESET_REASON_SAFE_MODE, 0, NULL);
}

bool HAL_Core_Enter_Safe_Mode_Requested(void)
{
    Load_SystemFlags();
    uint8_t flags = SYSTEM_FLAG(StartupMode_SysFlag);

    return (flags & 1);
}

void HAL_Core_Enter_Bootloader(bool persist) {
    if (persist) {
        HAL_Core_Write_Backup_Register(BKP_DR_10, 0xFFFF);
        system_flags.FLASH_OTA_Update_SysFlag = 0xFFFF;
        Save_SystemFlags();
    } else {
        HAL_Core_Write_Backup_Register(BKP_DR_01, ENTER_DFU_APP_REQUEST);
    }

    HAL_Core_System_Reset_Ex(RESET_REASON_DFU_MODE, 0, NULL);
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds) {
    InterruptMode m = (InterruptMode)edgeTriggerMode;
    HAL_Core_Enter_Stop_Mode_Ext(&wakeUpPin, 1, &m, 1, seconds, NULL);
}

static void fpu_sleep_prepare(void) {
    uint32_t fpscr;
    fpscr = __get_FPSCR();
    /*
     * Clear FPU exceptions.
     * Without this step, the FPU interrupt is marked as pending,
     * preventing system from sleeping. Exceptions cleared:
     * - IOC - Invalid Operation cumulative exception bit.
     * - DZC - Division by Zero cumulative exception bit.
     * - OFC - Overflow cumulative exception bit.
     * - UFC - Underflow cumulative exception bit.
     * - IXC - Inexact cumulative exception bit.
     * - IDC - Input Denormal cumulative exception bit.
     */
    __set_FPSCR(fpscr & ~0x9Fu);
    __DMB();
    NVIC_ClearPendingIRQ(FPU_IRQn);

    /*__
     * Assert no critical FPU exception is signaled:
     * - IOC - Invalid Operation cumulative exception bit.
     * - DZC - Division by Zero cumulative exception bit.
     * - OFC - Overflow cumulative exception bit.
     */
    SPARK_ASSERT((fpscr & 0x07) == 0);
}

int HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved) {
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

    for (int i = 0; i < mode_count; i++) {
        switch(mode[i]) {
            case RISING:
            case FALLING:
            case CHANGE:
                break;
            default:
                return SYSTEM_ERROR_NOT_ALLOWED;
        }
    }

    // Detach USB
    HAL_USB_Detach();

    // Flush all USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++) {
        if (HAL_USART_Is_Enabled(usart)) {
            HAL_USART_Flush_Data(usart);
        }
    }

    // Make sure we acquire exflash lock BEFORE going into a critical section
    hal_exflash_lock();

    // Disable thread scheduling
    os_thread_scheduling(false, NULL);

    // This will disable all but SoftDevice interrupts (by modifying NVIC->ICER)
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Disable SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Put external flash into sleep and disable QSPI peripheral
    hal_exflash_special_command(HAL_EXFLASH_COMMAND_NONE, HAL_EXFLASH_COMMAND_SLEEP, NULL, NULL, 0);
    hal_exflash_uninit();

    uint32_t exit_conditions = STOP_MODE_EXIT_CONDITION_NONE;

    // Reducing power consumption
    // Suspend all PWM instance remembering their state
    bool pwm_state[4] = {
        NRF_PWM0->ENABLE & PWM_ENABLE_ENABLE_Msk,
        NRF_PWM1->ENABLE & PWM_ENABLE_ENABLE_Msk,
        NRF_PWM2->ENABLE & PWM_ENABLE_ENABLE_Msk,
        NRF_PWM3->ENABLE & PWM_ENABLE_ENABLE_Msk,
    };
    nrf_pwm_disable(NRF_PWM0);
    nrf_pwm_disable(NRF_PWM1);
    nrf_pwm_disable(NRF_PWM2);
    nrf_pwm_disable(NRF_PWM3);

    // _Attempt_ to disable HFCLK. This may not succeed, resulting in a higher current consumption
    // FIXME
    bool hfclk_resume = false;
    if (nrf_drv_clock_hfclk_is_running()) {
        hfclk_resume = true;

        // Temporarily enable SoftDevice API interrupts just in case
        uint32_t base_pri = __get_BASEPRI();
        // We are also allowing IRQs with priority = 5, because that's what
        // OpenThread uses for its SWI3
        __set_BASEPRI(_PRIO_APP_LOW << (8 - __NVIC_PRIO_BITS));
        sd_nvic_critical_region_exit(st);
        {
            nrf_drv_clock_hfclk_release();
            // while (nrf_drv_clock_hfclk_is_running()) {
            //     ;
            // }
        }
        // And disable again
        sd_nvic_critical_region_enter(&st);
        __set_BASEPRI(base_pri);
    }

    // Remember current microsecond counter
    uint64_t micros_before_sleep = hal_timer_micros(NULL);
    // Disable hal_timer (RTC2)
    hal_timer_deinit(NULL);

    // Make sure LFCLK is running
    nrf_drv_clock_lfclk_request(NULL);
    while (!nrf_drv_clock_lfclk_is_running()) {
        ;
    }

    // Configure RTC2 to count at 125ms interval. This should allow us to sleep
    // (2^24 - 1) * 125ms = 24 days
    NVIC_SetPriority(RTC2_IRQn, 4);
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_STOP);
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_CLEAR);
    nrf_rtc_prescaler_set(NRF_RTC2, 4095);
    nrf_rtc_int_disable(NRF_RTC2, NRF_RTC_INT_TICK_MASK);
    nrf_rtc_int_disable(NRF_RTC2, NRF_RTC_INT_OVERFLOW_MASK);
    nrf_rtc_int_disable(NRF_RTC2, NRF_RTC_INT_COMPARE0_MASK);
    nrf_rtc_int_disable(NRF_RTC2, NRF_RTC_INT_COMPARE1_MASK);
    nrf_rtc_int_disable(NRF_RTC2, NRF_RTC_INT_COMPARE2_MASK);
    nrf_rtc_int_disable(NRF_RTC2, NRF_RTC_INT_COMPARE3_MASK);
    // Start RTC2
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_START);
    __DSB();
    __ISB();

    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();

    // Suspend all GPIOTE interrupts
    HAL_Interrupts_Suspend();
    {
        uint32_t intenset = 0;
        // Configure GPIOTE for wakeup
        for (int i = 0; i < pins_count; i++) {
            pin_t wake_up_pin = pins[i];
            InterruptMode edge_trigger_mode = (i < mode_count) ? mode[i] : mode[mode_count - 1];

            nrf_gpio_pin_pull_t wake_up_pin_mode;
            nrf_gpio_pin_sense_t wake_up_pin_sense;
            nrf_gpiote_polarity_t wake_up_pin_polarity;
            // Set required pin_mode based on edge_trigger_mode
            switch(edge_trigger_mode) {
                case RISING: {
                    wake_up_pin_mode = NRF_GPIO_PIN_PULLDOWN;
                    wake_up_pin_sense = NRF_GPIO_PIN_SENSE_HIGH;
                    wake_up_pin_polarity = NRF_GPIOTE_POLARITY_LOTOHI;
                    break;
                }
                case FALLING: {
                    wake_up_pin_mode = NRF_GPIO_PIN_PULLUP;
                    wake_up_pin_sense = NRF_GPIO_PIN_SENSE_LOW;
                    wake_up_pin_polarity = NRF_GPIOTE_POLARITY_HITOLO;
                    break;
                }
                case CHANGE:
                default:
                    wake_up_pin_mode = NRF_GPIO_PIN_NOPULL;
                    wake_up_pin_polarity = NRF_GPIOTE_POLARITY_TOGGLE;
                    break;
            }

            // For any pin that is not currently configured in GPIOTE with IN event
            // we are going to use low power PORT events
            uint32_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[wake_up_pin].gpio_port, PIN_MAP[wake_up_pin].gpio_pin);
            // Set pin mode
            nrf_gpio_cfg_input(nrf_pin, wake_up_pin_mode);
            bool use_in = false;
            for (unsigned i = 0; i < GPIOTE_CH_NUM; ++i) {
                if (nrf_gpiote_event_pin_get(i) == nrf_pin &&
                        nrf_gpiote_int_is_enabled(NRF_GPIOTE_INT_IN0_MASK << i)) {
                    // We have to use IN event for this pin in order to successfully execute interrupt handler
                    use_in = true;
                    nrf_gpiote_event_configure(i, nrf_pin, wake_up_pin_polarity);
                    intenset |= NRF_GPIOTE_INT_IN0_MASK << i;
                    break;
                }
            }

            if (!use_in) {
                // Use PORT for this pin
                if (wake_up_pin_mode == NRF_GPIO_PIN_NOPULL) {
                    // Read current state, choose sense accordingly
                    // Dummy read just in case
                    (void)nrf_gpio_pin_read(nrf_pin);
                    uint32_t cur_state = nrf_gpio_pin_read(nrf_pin);
                    if (cur_state) {
                        nrf_gpio_cfg_sense_set(nrf_pin, NRF_GPIO_PIN_SENSE_LOW);
                    } else {
                        nrf_gpio_cfg_sense_set(nrf_pin, NRF_GPIO_PIN_SENSE_HIGH);
                    }
                } else {
                    nrf_gpio_cfg_sense_input(nrf_pin, wake_up_pin_mode, wake_up_pin_sense);
                }
                intenset |= NRF_GPIOTE_INT_PORT_MASK;
            }
        }

        // Disable unnecessary GPIOTE interrupts
        uint32_t cur_intenset = NRF_GPIOTE->INTENSET;
        nrf_gpiote_int_disable(cur_intenset ^ intenset);
        nrf_gpiote_int_enable(intenset);

        // Clear events and interrupts
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_0);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_1);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_2);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_3);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_4);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_5);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_6);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_7);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
        NVIC_ClearPendingIRQ(GPIOTE_IRQn);

        if (pins_count > 0) {
            exit_conditions |= STOP_MODE_EXIT_CONDITION_PIN;
        }
    }

    int reason = SYSTEM_ERROR_UNKNOWN;

    // Workaround for FPU anomaly
    fpu_sleep_prepare();

    // Masks all interrupts lower than softdevice. This allows us to be woken ONLY by softdevice
    // or GPIOTE and RTC.
    // IMPORTANT: No SoftDevice API calls are allowed until HAL_enable_irq()
    int hst = HAL_disable_irq();

    // Remember original priorities
    uint32_t gpiote_priority = NVIC_GetPriority(GPIOTE_IRQn);
    uint32_t rtc2_priority = NVIC_GetPriority(RTC2_IRQn);

    // Enable RTC2 and GPIOTE interrupts if needed
    if (exit_conditions & STOP_MODE_EXIT_CONDITION_PIN) {
        NVIC_EnableIRQ(GPIOTE_IRQn);
    }
    if (seconds > 0) {
        // Reconfigure RTC2 for wake-up
        exit_conditions |= STOP_MODE_EXIT_CONDITION_RTC;
        NVIC_ClearPendingIRQ(RTC2_IRQn);

        nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_TICK);
        nrf_rtc_event_enable(NRF_RTC2, RTC_EVTEN_TICK_Msk);
        // Make sure that RTC is ticking
        // See 'TASK and EVENT jitter/delay'
        // http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52840.ps%2Frtc.html
        while (!nrf_rtc_event_pending(NRF_RTC2, NRF_RTC_EVENT_TICK)) {
            ;
        }
        nrf_rtc_event_disable(NRF_RTC2, RTC_EVTEN_TICK_Msk);
        nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_TICK);

        nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_COMPARE_0);
        nrf_rtc_event_disable(NRF_RTC2, RTC_EVTEN_COMPARE0_Msk);

        // Configure CC0
        uint32_t counter = nrf_rtc_counter_get(NRF_RTC2);
        uint32_t cc = counter + seconds * 8;
        NVIC_EnableIRQ(RTC2_IRQn);
        nrf_rtc_cc_set(NRF_RTC2, 0, cc);
        nrf_rtc_int_enable(NRF_RTC2, NRF_RTC_INT_COMPARE0_MASK);
        nrf_rtc_event_enable(NRF_RTC2, RTC_EVTEN_COMPARE0_Msk);
    }

    __DSB();
    __ISB();

    while (true) {
        if ((exit_conditions & STOP_MODE_EXIT_CONDITION_RTC) && NVIC_GetPendingIRQ(RTC2_IRQn)) {
            // Woken up by RTC
            reason = 0;
            break;
        } else if (exit_conditions & STOP_MODE_EXIT_CONDITION_PIN) {
            if (NVIC_GetPendingIRQ(GPIOTE_IRQn)) {
                // Woken up by a pin, figure out which one
                if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT)) {
                    // PORT event
                    for (int i = 0; i < pins_count; i++) {
                        pin_t wake_up_pin = pins[i];
                        uint32_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[wake_up_pin].gpio_port, PIN_MAP[wake_up_pin].gpio_pin);
                        nrf_gpio_pin_sense_t sense = nrf_gpio_pin_sense_get(nrf_pin);
                        if (sense != NRF_GPIO_PIN_NOSENSE) {
                            uint32_t state = nrf_gpio_pin_read(nrf_pin);
                            if ((state && sense == NRF_GPIO_PIN_SENSE_HIGH) ||
                                    (!state && sense == NRF_GPIO_PIN_SENSE_LOW)) {
                                reason = i + 1;
                                break;
                            }
                        }
                    }
                } else {
                    // Check IN events
                    for (unsigned i = 0; i < GPIOTE_CH_NUM && reason < 0; ++i) {
                        if (NRF_GPIOTE->EVENTS_IN[i] && nrf_gpiote_int_is_enabled(NRF_GPIOTE_INT_IN0_MASK << i)) {
                            pin_t pin = NRF_PIN_LOOKUP_TABLE[nrf_gpiote_event_pin_get(i)];
                            if (pin != PIN_INVALID) {
                                for (unsigned p = 0; p < pins_count; ++p) {
                                    pin_t wake_up_pin = pins[p];
                                    if (wake_up_pin == pin) {
                                        reason = p + 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
        }

        {
            // Mask interrupts completely
            __disable_irq();
            // Bump RTC2 and GPIOTE priorities if needed
            if (exit_conditions & STOP_MODE_EXIT_CONDITION_PIN) {
                NVIC_SetPriority(GPIOTE_IRQn, 0);
            }
            if (exit_conditions & STOP_MODE_EXIT_CONDITION_RTC) {
                NVIC_SetPriority(RTC2_IRQn, 0);
            }

            __DSB();
            __ISB();

            // Go to sleep
            __WFI();

            // Unbump RTC2 and GPIOTE priorities
            NVIC_SetPriority(GPIOTE_IRQn, gpiote_priority);
            NVIC_SetPriority(RTC2_IRQn, rtc2_priority);

            // Unmask interrupts, we are still under the effect of HAL_disable_irq
            // that masked all but SoftDevice interrupts using BASEPRI
            __enable_irq();
        }
    }

    // Restore HFCLK
    if (hfclk_resume) {
        // Temporarily enable SoftDevice API interrupts
        uint32_t base_pri = __get_BASEPRI();
        __set_BASEPRI(_PRIO_SD_LOWEST << (8 - __NVIC_PRIO_BITS));
        {
            nrf_drv_clock_hfclk_request(NULL);
            while (!nrf_drv_clock_hfclk_is_running()) {
                ;
            }
        }
        // And disable again
        __set_BASEPRI(base_pri);
    }

    // Count the number of microseconds we've slept and reconfigure hal_timer (RTC2)
    uint64_t slept_for = (uint64_t)nrf_rtc_counter_get(NRF_RTC2) * 125000;
    // Stop RTC2
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_STOP);
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_CLEAR);
    // Reconfigure it hal_timer_init() and apply the offset
    hal_timer_init_config_t hal_timer_conf = {
        .size = sizeof(hal_timer_init_config_t),
        .version = 0,
        .base_clock_offset = micros_before_sleep + slept_for
    };
    hal_timer_init(&hal_timer_conf);

    // Restore GPIOTE cionfiguration
    HAL_Interrupts_Restore();

    // Re-initialize external flash
    hal_exflash_init();

    // Re-enable PWM
    // FIXME: this is not ideal but will do for now
    {
        NRF_PWM_Type* const pwms[] = {NRF_PWM0, NRF_PWM1, NRF_PWM2, NRF_PWM3};
        for (unsigned i = 0; i < sizeof(pwms) / sizeof(pwms[0]); i++) {
            NRF_PWM_Type* pwm = pwms[i];
            if (pwm_state[i]) {
                nrf_pwm_enable(pwm);
                if (nrf_pwm_event_check(pwm, NRF_PWM_EVENT_SEQEND0) ||
                        nrf_pwm_event_check(pwm, NRF_PWM_EVENT_SEQEND1)) {
                    nrf_pwm_event_clear(pwm, NRF_PWM_EVENT_SEQEND0);
                    nrf_pwm_event_clear(pwm, NRF_PWM_EVENT_SEQEND1);
                    if (pwm->SEQ[0].PTR && pwm->SEQ[0].CNT) {
                        nrf_pwm_task_trigger(pwm, NRF_PWM_TASK_SEQSTART0);
                    } else if (pwm->SEQ[1].PTR && pwm->SEQ[1].CNT) {
                        nrf_pwm_task_trigger(pwm, NRF_PWM_TASK_SEQSTART1);
                    }
                }
            }
        }
    }

    // Re-enable SysTick
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    // This will reenable all non-SoftDevice interrupts previously disabled
    // by sd_nvic_critical_region_enter()
    sd_nvic_critical_region_exit(st);

    // Unmasks all non-softdevice interrupts
    HAL_enable_irq(hst);

    // Release LFCLK
    nrf_drv_clock_lfclk_release();

    // Re-enable USB
    HAL_USB_Attach();

    // Unlock external flash
    hal_exflash_unlock();

    // Enable thread scheduling
    os_thread_scheduling(true, NULL);

    return reason;
}

void HAL_Core_Execute_Stop_Mode(void) {
    // Kept for compatibility only. Despite the fact that it's exported, it's never used directly
}

int HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags) {
    // RTC cannot be kept running in System OFF mode, so wake up by RTC
    // is not supported in deep sleep
    if (seconds > 0) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    return HAL_Core_Execute_Standby_Mode_Ext(flags, NULL);
}

int HAL_Core_Execute_Standby_Mode_Ext(uint32_t flags, void* reserved) {
    // Force to use external wakeup pin on Gen 3 Device
    if (flags & HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    // Make sure we acquire exflash lock BEFORE going into a critical section
    hal_exflash_lock();

    // Disable thread scheduling
    os_thread_scheduling(false, NULL);

    // This will disable all but SoftDevice interrupts (by modifying NVIC->ICER)
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Disable SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Put external flash into sleep mode and disable QSPI peripheral
    hal_exflash_special_command(HAL_EXFLASH_COMMAND_NONE, HAL_EXFLASH_COMMAND_SLEEP, NULL, NULL, 0);
    hal_exflash_uninit();

    // Uninit GPIOTE
    nrfx_gpiote_uninit();

    // Disable low power comparator
    nrf_lpcomp_disable();

    // Deconfigure any possible SENSE configuration
    HAL_Interrupts_Suspend();

    // Disable GPIOTE PORT interrupts
    nrf_gpiote_int_disable(GPIOTE_INTENSET_PORT_Msk);

    // Clear any GPIOTE events
    nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);

    // Configure wakeup pin
    NRF5x_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[WKP].gpio_port, PIN_MAP[WKP].gpio_pin);
    nrf_gpio_cfg_sense_input(nrf_pin, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);

    // Disable PWM
    nrf_pwm_disable(NRF_PWM0);
    nrf_pwm_disable(NRF_PWM1);
    nrf_pwm_disable(NRF_PWM2);
    nrf_pwm_disable(NRF_PWM3);

    // RAM retention is configured on early boot in Set_System()

    SPARK_ASSERT(sd_power_system_off() == NRF_SUCCESS);
    while (1);
    return 0;
}

void HAL_Core_Execute_Standby_Mode(void) {
}

bool HAL_Core_System_Reset_FlagSet(RESET_TypeDef resetType) {
    uint32_t reset_reason = SYSTEM_FLAG(RCC_CSR_SysFlag);
    if (reset_reason == 0xffffffff) {
        sd_power_reset_reason_get(&reset_reason);
    }
    switch(resetType) {
        case PIN_RESET: {
            return reset_reason == NRF_POWER_RESETREAS_RESETPIN_MASK;
        }
        case SOFTWARE_RESET: {
            return reset_reason == NRF_POWER_RESETREAS_SREQ_MASK;
        }
        case WATCHDOG_RESET: {
            return reset_reason == NRF_POWER_RESETREAS_DOG_MASK;
        }
        case POWER_MANAGEMENT_RESET: {
            // SYSTEM OFF Mode
            return reset_reason == NRF_POWER_RESETREAS_OFF_MASK;
        }
        // If none of the reset sources are flagged, this indicates that
        // the chip was reset from the on-chip reset generator,
        // which will indicate a power-on-reset or a brownout reset.
        case POWER_DOWN_RESET:
        case POWER_BROWNOUT_RESET: {
            return reset_reason == 0;
        }
        default:
            return false;
    }
}

static void Init_Last_Reset_Info()
{
    if (HAL_Core_System_Reset_FlagSet(SOFTWARE_RESET))
    {
        // Load reset info from backup registers
        last_reset_info.reason = HAL_Core_Read_Backup_Register(BKP_DR_02);
        last_reset_info.data = HAL_Core_Read_Backup_Register(BKP_DR_03);
        // Clear backup registers
        HAL_Core_Write_Backup_Register(BKP_DR_02, 0);
        HAL_Core_Write_Backup_Register(BKP_DR_03, 0);
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
        // TODO: Reset from USB, NFC, LPCOMP...
        else
        {
            last_reset_info.reason = RESET_REASON_UNKNOWN;
        }
        last_reset_info.data = 0; // Not used
    }

    // Clear Reset info register
    sd_power_reset_reason_clr(0xFFFFFFFF);
}

int HAL_Core_Get_Last_Reset_Info(int *reason, uint32_t *data, void *reserved) {
    if (HAL_Feature_Get(FEATURE_RESET_INFO)) {
        if (reason) {
            *reason = last_reset_info.reason;
        }
        if (data) {
            *data = last_reset_info.data;
        }
        return 0;
    }
    return -1;
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize) {
    // TODO: Use the peripheral lock?
    return Compute_CRC32(pBuffer, bufferSize, NULL);
}

uint16_t HAL_Core_Mode_Button_Pressed_Time() {
    return 0;
}

void HAL_Bootloader_Lock(bool lock) {

}

unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved) {
    return SystemCoreClock;
}


static TaskHandle_t  app_thread_handle;
#define APPLICATION_STACK_SIZE 6144
#define MALLOC_LOCK_TIMEOUT_MS (60000) // This is defined in "ticks" and each tick is 1ms

/**
 * The mutex to ensure only one thread manipulates the heap at a given time.
 */
xSemaphoreHandle malloc_mutex = 0;

static void init_malloc_mutex(void) {
    malloc_mutex = xSemaphoreCreateRecursiveMutex();
}

void __malloc_lock(void* ptr) {
    if (malloc_mutex) {
        if (!xSemaphoreTakeRecursive(malloc_mutex, MALLOC_LOCK_TIMEOUT_MS)) {
            PANIC(HeapError, "Semaphore Lock Timeout");
            while (1);
        }
    }
}

void __malloc_unlock(void* ptr) {
    if (malloc_mutex) {
        xSemaphoreGiveRecursive(malloc_mutex);
    }
}

/**
 * The entrypoint to our application.
 * This should be called from the RTOS main thread once initialization has been
 * completed, constructors invoked and and HAL_Core_Config() has been called.
 */
void application_start() {
    rtos_started = 1;

    // one the key is sent to the cloud, this can be removed, since the key is fetched in
    // Spark_Protocol_init(). This is just a temporary measure while the key still needs
    // to be fetched via DFU.

    HAL_Core_Setup();

    // TODO:
    // generate_key();

    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Load last reset info from RCC / backup registers
        Init_Last_Reset_Info();
    }

    app_setup_and_loop();
}

void application_task_start(void* arg) {
    application_start();
}

/**
 * Called from startup_stm32f2xx.s at boot, main entry point.
 */
int main(void) {
    init_malloc_mutex();
    xTaskCreate( application_task_start, "app_thread", APPLICATION_STACK_SIZE/sizeof( portSTACK_TYPE ), NULL, 2, &app_thread_handle);

    vTaskStartScheduler();

    /* we should never get here */
    while (1);

    return 0;
}

static int Write_Feature_Flag(uint32_t flag, bool value, bool *prev_value) {
    if (HAL_IsISR()) {
        return -1; // DCT cannot be accessed from an ISR
    }
    uint32_t flags = 0;
    int result = dct_read_app_data_copy(DCT_FEATURE_FLAGS_OFFSET, &flags, sizeof(flags));
    if (result != 0) {
        return result;
    }
    // NOTE: inverted logic!
    const bool cur_value = !(flags & flag);
    if (prev_value) {
        *prev_value = cur_value;
    }
    if (cur_value != value) {
        if (value) {
            flags &= ~flag;
        } else {
            flags |= flag;
        }
        result = dct_write_app_data(&flags, DCT_FEATURE_FLAGS_OFFSET, 4);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

static int Read_Feature_Flag(uint32_t flag, bool* value) {
    if (HAL_IsISR()) {
        return -1; // DCT cannot be accessed from an ISR
    }
    uint32_t flags = 0;
    const int result = dct_read_app_data_copy(DCT_FEATURE_FLAGS_OFFSET, &flags, sizeof(flags));
    if (result != 0) {
        return result;
    }
    *value = !(flags & flag);
    return 0;
}

int HAL_Feature_Set(HAL_Feature feature, bool enabled) {
   switch (feature) {
        case FEATURE_RETAINED_MEMORY: {
            // TODO: Switch on backup SRAM clock
            // Switch on backup power regulator, so that it survives the deep sleep mode,
            // software and hardware reset. Power must be supplied to VIN or VBAT to retain SRAM values.
            return -1;
        }
        case FEATURE_RESET_INFO: {
            return Write_Feature_Flag(FEATURE_FLAG_RESET_INFO, enabled, NULL);
        }
#if HAL_PLATFORM_CLOUD_UDP
        case FEATURE_CLOUD_UDP: {
            const uint8_t data = (enabled ? 0xff : 0x00);
            return dct_write_app_data(&data, DCT_CLOUD_TRANSPORT_OFFSET, sizeof(data));
        }
#endif // HAL_PLATFORM_CLOUD_UDP
        case FEATURE_ETHERNET_DETECTION: {
            return Write_Feature_Flag(FEATURE_FLAG_ETHERNET_DETECTION, enabled, NULL);
        }
    }

    return -1;
}

bool HAL_Feature_Get(HAL_Feature feature) {
    switch (feature) {
        case FEATURE_CLOUD_UDP: {
            return true; // Mesh platforms are UDP-only
        }
        case FEATURE_RESET_INFO: {
            return true;
        }
        case FEATURE_ETHERNET_DETECTION: {
            bool value = false;
            return (Read_Feature_Flag(FEATURE_FLAG_ETHERNET_DETECTION, &value) == 0) ? value : false;
        }
    }
    return false;
}

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR) {
    if ((BKP_DR == 0) || (BKP_DR > BACKUP_REGISTER_NUM)) {
        return -1;
    }

    return BKP_DR - 1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data) {
    int32_t BKP_DR_Index = HAL_Core_Backup_Register(BKP_DR);
    if (BKP_DR_Index != -1) {
        backup_register[BKP_DR_Index] = Data;
    }
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR) {
    int32_t BKP_DR_Index = HAL_Core_Backup_Register(BKP_DR);
    if (BKP_DR_Index != -1) {
        return backup_register[BKP_DR_Index];
    }
    return 0xFFFFFFFF;
}

extern size_t pvPortLargestFreeBlock();

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    struct mallinfo heapinfo = mallinfo();
    // fordblks  The total number of bytes in free blocks.
    info->freeheap = heapinfo.fordblks;
    if (offsetof(runtime_info_t, total_init_heap) + sizeof(info->total_init_heap) <= info->size) {
        info->total_init_heap = (uintptr_t)new_heap_end - (uintptr_t)&link_heap_location;
    }

    if (offsetof(runtime_info_t, total_heap) + sizeof(info->total_heap) <= info->size) {
        info->total_heap = heapinfo.arena;
    }

    if (offsetof(runtime_info_t, max_used_heap) + sizeof(info->max_used_heap) <= info->size) {
        info->max_used_heap = heapinfo.usmblks;
    }

    if (offsetof(runtime_info_t, user_static_ram) + sizeof(info->user_static_ram) <= info->size) {
        info->user_static_ram = (uintptr_t)&_Stack_Init - (uintptr_t)new_heap_end;
    }

    if (offsetof(runtime_info_t, largest_free_block_heap) + sizeof(info->largest_free_block_heap) <= info->size) {
    		info->largest_free_block_heap = pvPortLargestFreeBlock();
    }

    return 0;
}

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag)
{
    switch (flag)
    {
        case BOOTLOADER_FLAG_VERSION:
            return SYSTEM_FLAG(Bootloader_Version_SysFlag);
        case BOOTLOADER_FLAG_STARTUP_MODE:
            return SYSTEM_FLAG(StartupMode_SysFlag);
    }
    return 0;
}

int HAL_Core_Enter_Panic_Mode(void* reserved)
{
    __disable_irq();
    return 0;
}

#if HAL_PLATFORM_CLOUD_UDP

#include "dtls_session_persist.h"
#include <string.h>

SessionPersistDataOpaque session __attribute__((section(".backup_system")));

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

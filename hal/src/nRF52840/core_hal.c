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
#include "user_hal.h"
#include "nrfx_wdt.h"

#define BACKUP_REGISTER_NUM        10
static int32_t backup_register[BACKUP_REGISTER_NUM] __attribute__((section(".backup_registers")));
static volatile uint8_t rtos_started = 0;

static struct Last_Reset_Info {
    int reason;
    uint32_t data;
} last_reset_info = { RESET_REASON_NONE, 0 };

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

extern uintptr_t link_interrupt_vectors_location[];
extern uintptr_t link_ram_interrupt_vectors_location[];
extern uintptr_t link_ram_interrupt_vectors_location_end;
extern char link_stack_location;


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
        " .balign 4                                                 \n"
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
    sd_mbr_command_t com = {};
    com.command = SD_MBR_COMMAND_INIT_SD;
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

#if HAL_PLATFORM_PROHIBIT_XIP
static void prohibit_xip(void) {
    const uint8_t regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    // Disable MPU
    MPU->CTRL = 0;
    // Clear all regions
    for (uint8_t i = 0; i < regions; i++) {
        MPU->RNR    = i;
        MPU->RASR   = 0;
        MPU->RBAR   = 0;
    }

    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Forbid access to XIP
    MPU->RNR = 0;
    MPU->RBAR = EXTERNAL_FLASH_XIP_BASE;
    // XN: 1, instruction fetches disabled
    // AP: 0b000, all accesses generate a permission fault
    // TEX: 0b000, S: 0, C: 1, B: 0, as suggested by core manual for flash memory
    const uint32_t attr = 0x10050000;
    MPU->RASR = attr | (1 << MPU_RASR_ENABLE_Pos) | ((31 - __CLZ(EXTERNAL_FLASH_XIP_LENGTH) - 1) << MPU_RASR_SIZE_Pos);
    // Enable MPU
    MPU->CTRL = 0x00000005;

    sd_nvic_critical_region_exit(st);

    // Use a DSB followed by an ISB instruction to ensure that the new MPU configuration is used by subsequent instructions. 
    __DSB();
    __ISB();

    // Uncomment the code bellow will trigger hardfault
    // uint8_t data = *((uint8_t*)(EXTERNAL_FLASH_XIP_BASE + EXTERNAL_FLASH_OTA_ADDRESS));
    // LOG(ERROR, "%d", data);
}
#endif // HAL_PLATFORM_PROHIBIT_XIP

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
    memcpy(&link_ram_interrupt_vectors_location, &link_interrupt_vectors_location, (uintptr_t)&link_ram_interrupt_vectors_location_end-(uintptr_t)&link_ram_interrupt_vectors_location);
    uint32_t* isrs = (uint32_t*)&link_ram_interrupt_vectors_location;
    SCB->VTOR = (uint32_t)isrs;

#if HAL_PLATFORM_PROHIBIT_XIP
    prohibit_xip();
#endif

    Set_System();

    hal_timer_init(NULL);

    HAL_Core_Setup_override_interrupts();

    HAL_RNG_Configuration();

#if defined(MODULAR_FIRMWARE)
    hal_user_module_descriptor user_desc = {};
    if (!hal_user_module_get_descriptor(&user_desc)) {
        new_heap_end = user_desc.pre_init();
        if (new_heap_end > malloc_heap_end()) {
            malloc_set_heap_end(new_heap_end);
        }
    } else {
        // Expand heap to the maximum size
        // NOTE: for monolithic builds this should already be at max size
        extern uintptr_t platform_heap_modular_max_location_end;
        malloc_set_heap_end((void*)&platform_heap_modular_max_location_end);
    }

    // Enable malloc before littlefs initialization.
    malloc_enable(1);
#endif

#ifdef DFU_BUILD_ENABLE
    Load_SystemFlags();
#endif

    // TODO: Use current LED theme
    if (HAL_Feature_Get(FEATURE_LED_OVERRIDDEN)) {
        // Just in case
        LED_Off(PARTICLE_LED_RGB);
    } else {
        LED_SetRGBColor(RGB_COLOR_WHITE);
        LED_On(PARTICLE_LED_RGB);
    }

    FLASH_AddToFactoryResetModuleSlot(
      FLASH_SERIAL, EXTERNAL_FLASH_FAC_ADDRESS,
      FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE,
      FACTORY_RESET_MODULE_FUNCTION, MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS); //true to verify the CRC during copy also

      HAL_Core_Write_Backup_Register(BKP_DR_01, 0xFFFF);
}

void HAL_Core_Setup(void) {
    /* DOES NOT DO ANYTHING
     * SysTick is enabled within FreeRTOS
     */
    HAL_Core_Config_systick_configuration();

    if (bootloader_update_if_needed()) {
        HAL_Core_System_Reset();
    }

    // Initialize stdlib PRNG with a seed from hardware RNG
    srand(HAL_RNG_GetRandomNumber());

    hal_rtc_init();

#if !defined(MODULAR_FIRMWARE) || !MODULAR_FIRMWARE
    module_user_init_hook();
#endif
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
    else if(FLASH_isUserModuleInfoValid(FLASH_SERIAL, EXTERNAL_FLASH_FAC_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION))
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

    if(hal_button_get_debounce_time(HAL_BUTTON1) >= pressedMillisDuration) {
        pressedState = true;
    }
    if(hal_button_get_debounce_time(HAL_BUTTON1_MIRROR) >= pressedMillisDuration) {
        pressedState = true;
    }

    return pressedState;
}

void HAL_Core_Mode_Button_Reset(uint16_t button)
{
}

void HAL_Core_System_Reset(void) {
    if (nrf_wdt_started()) {
        // Dirty-hack
        HAL_Core_Write_Backup_Register(BKP_DR_04, 0xDEADBEEF);
        nrf_gpiote_int_disable(GPIOTE_INTENSET_PORT_Msk);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
        nrf_gpio_cfg_sense_input(QSPI_FLASH_CSN_PIN, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
        sd_power_system_off();
    }
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
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
}

int HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved) {
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
    return SYSTEM_ERROR_DEPRECATED;
}

void HAL_Core_Execute_Stop_Mode(void) {
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
}

int HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags) {
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
    return SYSTEM_ERROR_DEPRECATED;
}

int HAL_Core_Execute_Standby_Mode_Ext(uint32_t flags, void* reserved) {
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
    return SYSTEM_ERROR_DEPRECATED;
}

void HAL_Core_Execute_Standby_Mode(void) {
    // Deprecated. See: hal/src/nRF52840/sleep_hal.cpp
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
            if (HAL_Core_Read_Backup_Register(BKP_DR_04) == 0xDEADBEEF) {
                return true;
            }
            return reset_reason == NRF_POWER_RESETREAS_SREQ_MASK;
        }
        case WATCHDOG_RESET: {
            return reset_reason == NRF_POWER_RESETREAS_DOG_MASK;
        }
        case POWER_MANAGEMENT_RESET: {
            // SYSTEM OFF Mode
            return (reset_reason == NRF_POWER_RESETREAS_OFF_MASK) || (reset_reason == NRF_POWER_RESETREAS_LPCOMP_MASK);
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

    HAL_Core_Write_Backup_Register(BKP_DR_04, 0);

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

void __malloc_lock(struct _reent *ptr) {
    if (malloc_mutex) {
        if (!xSemaphoreTakeRecursive(malloc_mutex, MALLOC_LOCK_TIMEOUT_MS)) {
            PANIC(HeapError, "Semaphore Lock Timeout");
            while (1);
        }
    }
}

void __malloc_unlock(struct _reent *ptr) {
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
 * Called from startup_${MCU_DEVICE_LC}.s at boot, main entry point.
 */
int main(void) {
    init_malloc_mutex();
    xTaskCreate( application_task_start, "app_thread", APPLICATION_STACK_SIZE/sizeof( portSTACK_TYPE ), NULL, 2, &app_thread_handle);

    if (HAL_Feature_Get(FEATURE_LED_OVERRIDDEN)) {
        LED_Signaling_Start();
    }

    vTaskStartScheduler();

    /* we should never get here */
    while (1);

    return 0;
}

static int Write_Feature_Flag(uint32_t flag, bool value, bool *prev_value) {
    if (hal_interrupt_is_isr()) {
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
    if (hal_interrupt_is_isr()) {
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
        case FEATURE_LED_OVERRIDDEN: {
            return Write_Feature_Flag(FEATURE_FLAG_LED_OVERRIDDEN, enabled, NULL);
        }
        case FEATURE_DISABLE_LISTENING_MODE: {
            return Write_Feature_Flag(FEATURE_FLAG_DISBLE_LISTENING_MODE, enabled, NULL);
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
        case FEATURE_LED_OVERRIDDEN: {
            bool value = false;
            return (Read_Feature_Flag(FEATURE_FLAG_LED_OVERRIDDEN, &value) == 0) ? value : false;
        }
        case FEATURE_DISABLE_LISTENING_MODE: {
            bool value = false;
            return (Read_Feature_Flag(FEATURE_FLAG_DISBLE_LISTENING_MODE, &value) == 0) ? value : false;
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
        info->user_static_ram = (uintptr_t)&link_stack_location - (uintptr_t)new_heap_end;
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


#include "dynalib.h"
#include "module_system_part1_init.h"
#include "system_mode.h"
#include "module_user_init.h"
#include "hw_config.h"
#include "core_hal.h"
#include <stdint.h>
#include <stddef.h>

DYNALIB_TABLE_EXTERN(hal);
DYNALIB_TABLE_EXTERN(rt);
DYNALIB_TABLE_EXTERN(system);
DYNALIB_TABLE_EXTERN(system_net);
DYNALIB_TABLE_EXTERN(system_cloud);
DYNALIB_TABLE_EXTERN(hal_peripherals);
DYNALIB_TABLE_EXTERN(hal_i2c);
DYNALIB_TABLE_EXTERN(hal_gpio);
DYNALIB_TABLE_EXTERN(hal_spi);
DYNALIB_TABLE_EXTERN(hal_core);
DYNALIB_TABLE_EXTERN(hal_socket);
DYNALIB_TABLE_EXTERN(hal_wlan);
DYNALIB_TABLE_EXTERN(hal_usart);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
const void* const system_part2_module[] = {
    DYNALIB_TABLE_NAME(hal),
    DYNALIB_TABLE_NAME(rt),
    DYNALIB_TABLE_NAME(system),
    DYNALIB_TABLE_NAME(hal_peripherals),
    DYNALIB_TABLE_NAME(hal_i2c),
    DYNALIB_TABLE_NAME(hal_gpio),
    DYNALIB_TABLE_NAME(hal_spi),
    DYNALIB_TABLE_NAME(hal_core),
    DYNALIB_TABLE_NAME(hal_socket),
    DYNALIB_TABLE_NAME(hal_wlan),
    DYNALIB_TABLE_NAME(hal_usart),
    DYNALIB_TABLE_NAME(system_net),
    DYNALIB_TABLE_NAME(system_cloud),
};

extern void** dynalib_location_user;

uint8_t is_user_function_valid(uint8_t index) {
    size_t fn = (size_t)dynalib_location_user[index];
    return fn > (size_t)&dynalib_location_user && fn <= (size_t)0x80A00000;
}

static bool module_user_part_validated = false;

void module_user_part_restore_and_validation_check(void)
{
    //CRC verification Enabled by default
    FLASH_AddToFactoryResetModuleSlot(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS,
                                      FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE,
                                      FACTORY_RESET_MODULE_FUNCTION, MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS); //true to verify the CRC during copy also

    if (FLASH_isModuleInfoValid(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, USER_FIRMWARE_IMAGE_LOCATION))
    {
        //CRC check the user module and set to module_user_part_validated
        module_user_part_validated = FLASH_VerifyCRC32(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION,
                                     FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION));
    }
    else if(FLASH_isModuleInfoValid(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS, USER_FIRMWARE_IMAGE_LOCATION))
    {
        //Reset and let bootloader perform the user module factory reset
        //Doing this instead of calling FLASH_RestoreFromFactoryResetModuleSlot()
        //saves precious system_part2 flash size i.e. fits in < 128KB
        HAL_Core_Factory_Reset();

        while(1);//Device should reset before reaching this line
    }
}

/**
 * Determines if the user module is present and valid.
 * @return 
 */
bool is_user_module_valid()
{
    return module_user_part_validated;
}

/**
 * The current start of heap.
 */
extern void* sbrk_heap_top;

/**
 * Global initialization function. Called after memory has been initialized in this module
 * but before C++ constructors are executed and before any dynamic memory has been allocated.
 */
void system_part2_pre_init() {
    // initialize dependent modules
    module_system_part1_pre_init();

    module_user_part_restore_and_validation_check();

    if (is_user_module_valid()) {
        void* new_heap_top = module_user_pre_init();
        if (new_heap_top>sbrk_heap_top)
            sbrk_heap_top = new_heap_top;
    }
    else {
        // indicate to the system that it shouldn't run user code
        set_system_mode(SAFE_MODE);
    }

    // now call any C++ constructors in this module's dependencies
    module_system_part1_init();
}

/*
 * Invoked after all module-scope instances have been constructed.
 */
void system_part2_init() {
    if (is_user_module_valid()) {
        module_user_init();
    }
}

void setup() {
    if (is_user_module_valid()) {
        module_user_setup();
    }
}

void loop() {
    if (is_user_module_valid()) {
        module_user_loop();
    }    
}

__attribute__((section(".module_pre_init"))) const void* system_part2_pre_init_fn = system_part2_pre_init;
__attribute__((section(".module_init"))) const void* system_part2_init_fn = system_part2_init;

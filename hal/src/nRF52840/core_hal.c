#include "core_hal.h"
#include "service_debug.h"
#include "nrf52840.h"
#include "hal_event.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

volatile uint8_t rtos_started = 0;

void HardFault_Handler( void ) __attribute__( ( naked ) );

void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);

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

void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    PANIC(MemManage,"MemManage");
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    PANIC(BusFault,"BusFault");
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    PANIC(UsageFault,"UsageFault");
    while (1)
    {
    }
}

void vApplicationMallocFailedHook(size_t xWantedSize)
{
    hal_notify_event(HAL_EVENT_OUT_OF_MEMORY, xWantedSize, 0);
}

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
}

void HAL_Core_Setup(void)
{
}

bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    return false;
}

void HAL_Core_Mode_Button_Reset(uint16_t button)
{
}

void HAL_Core_System_Reset(void)
{
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved)
{
}

void HAL_Core_Factory_Reset(void)
{
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
}

void HAL_Core_Enter_Bootloader(bool persist)
{
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
}

int32_t HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved)
{
    return -1;
}

void HAL_Core_Execute_Stop_Mode(void)
{
}

void HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags)
{
}

void HAL_Core_Execute_Standby_Mode(void)
{
}

int HAL_Core_Get_Last_Reset_Info(int *reason, uint32_t *data, void *reserved)
{
    return -1;
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
    return 0;
}

// todo find a technique that allows accessor functions to be inlined while still keeping
// hardware independence.
bool HAL_watchdog_reset_flagged()
{
    return false;
}

void HAL_Notify_WDT()
{
}

uint16_t HAL_Core_Mode_Button_Pressed_Time()
{
    return 0;
}

void HAL_Bootloader_Lock(bool lock)
{
}


unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved)
{
    return 1;
}


static TaskHandle_t  app_thread_handle;
#define APPLICATION_STACK_SIZE 6144

/**
 * The mutex to ensure only one thread manipulates the heap at a given time.
 */
xSemaphoreHandle malloc_mutex = 0;

static void init_malloc_mutex(void)
{
    malloc_mutex = xSemaphoreCreateRecursiveMutex();
}

void __malloc_lock(void* ptr)
{
    if (malloc_mutex)
        while (!xSemaphoreTakeRecursive(malloc_mutex, 0xFFFFFFFF)) {}
}

void __malloc_unlock(void* ptr)
{
    if (malloc_mutex)
        xSemaphoreGiveRecursive(malloc_mutex);
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

    /*
    generate_key();

    if (HAL_Feature_Get(FEATURE_RESET_INFO))
    {
        // Load last reset info from RCC / backup registers
        Init_Last_Reset_Info();
    }
    */

    app_setup_and_loop();
}

void application_task_start(void* arg)
{
    application_start();
}

/**
 * Called from startup_stm32f2xx.s at boot, main entry point.
 */
int main(void)
{
    init_malloc_mutex();
    xTaskCreate( application_task_start, "app_thread", APPLICATION_STACK_SIZE/sizeof( portSTACK_TYPE ), NULL, 2, &app_thread_handle);

    vTaskStartScheduler();

    /* we should never get here */
    while (1);

    return 0;
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

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    return 0xFFFFFFFF;
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

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    return -1;
}

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag)
{
    if (flag==BOOTLOADER_FLAG_STARTUP_MODE)
        return 0xFF;
    return 0xFFFF;
}

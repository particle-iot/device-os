
#include "wiced.h"

#include "core_hal.h"

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_Core_Init(void)
{
    wiced_init();
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

bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    return false;
}

void HAL_Core_Mode_Button_Reset(void)
{    
}

void HAL_Core_System_Reset(void)
{  
}

void HAL_Core_Factory_Reset(void)
{
}

void HAL_Core_Enter_Bootloader(void)
{
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
}

void HAL_Core_Execute_Stop_Mode(void)
{
}

void HAL_Core_Enter_Standby_Mode(void)
{
}

void HAL_Core_Execute_Standby_Mode(void)
{
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(uint8_t *pBuffer, uint32_t bufferSize)
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

void application_start() {
    app_setup_and_loop();
}
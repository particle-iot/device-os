#include "delay_hal.h"

/**
 * Updated by interrupt in stm32_it.c
 */
volatile uint32_t TimingDelay;


/*******************************************************************************
* Function Name  : Delay
* Description    : Inserts a delay time.
* Input          : nTime: specifies the delay time length, in milliseconds.
* Output         : None
* Return         : None
*******************************************************************************/
void HAL_Delay_Milliseconds(uint32_t nTime)
{
    TimingDelay = nTime;

    while (TimingDelay != 0x00);
}

/*******************************************************************************
 * Function Name  : Delay_Microsecond
 * Description    : Inserts a delay time in microseconds using 32-bit DWT->CYCCNT
 * Input          : uSec: specifies the delay time length, in microseconds.
 * Output         : None
 * Return         : None
 *******************************************************************************/
void HAL_Delay_Microseconds(uint32_t uSec)
{
  volatile uint32_t DWT_START = DWT->CYCCNT;

  // keep DWT_TOTAL from overflowing (max 59.652323s w/72MHz SystemCoreClock)
  if (uSec > (UINT_MAX / SYSTEM_US_TICKS))
  {
    uSec = (UINT_MAX / SYSTEM_US_TICKS);
  }

  volatile uint32_t DWT_TOTAL = (SYSTEM_US_TICKS * uSec);

  while((DWT->CYCCNT - DWT_START) < DWT_TOTAL)
  {
    KICK_WDT();
  }
}



#include "hw_system_flags.h"
#include "hw_config.h"
#include "nrf.h"

void Save_Reset_Syndrome()
{
    //Save RCC clock control & status register
    uint32_t flags = NRF_POWER->RESETREAS;
    if (SYSTEM_FLAG(RCC_CSR_SysFlag) != flags)
    {
        SYSTEM_FLAG(RCC_CSR_SysFlag) = flags;
        Save_SystemFlags();
    }
}

uint8_t RCC_GetFlagStatus(uint8_t flag)
{
    if (flag == RCC_FLAG_IWDGRST)
    {
        if (NRF_POWER->RESETREAS & POWER_RESETREAS_DOG_Msk)
        {
            return SET;
        }
        else
        {
            return RESET;
        }
    }
    else
    {
        return RESET;
    }
}

void RCC_ClearFlag(void)
{
    NRF_POWER->RESETREAS = 0xFFFFFFFF;
}

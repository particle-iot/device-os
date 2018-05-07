
#include "hw_system_flags.h"

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

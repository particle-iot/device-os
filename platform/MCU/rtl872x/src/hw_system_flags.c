
#include "hw_system_flags.h"
#include "hw_config.h"

void Save_Reset_Syndrome()
{
}

uint8_t RCC_GetFlagStatus(uint8_t flag)
{
    return RESET;
}

void RCC_ClearFlag(void)
{
}


#include "core_hal.h"

#include "spark_wiring_system.h"

SystemClass System;

void SystemClass::factoryReset(void)
{
  //This method will work only if the Core is supplied
  //with the latest version of Bootloader
  HAL_Core_Factory_Reset();
}

void SystemClass::bootloader(void)
{
  //The drawback here being it will enter bootloader mode until firmware
  //is loaded again. Require bootloader changes for proper working.
  HAL_Core_Enter_Bootloader();
}

void SystemClass::reset(void)
{
  HAL_Core_System_Reset();
}


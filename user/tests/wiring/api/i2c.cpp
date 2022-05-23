
#include "testapi.h"

test(i2c_hal_backwards_compatibility)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    I2C_Mode mode = I2C_MODE_MASTER;
    HAL_I2C_Interface i2c = HAL_I2C_INTERFACE1;
    HAL_I2C_Config config;
    HAL_I2C_Transmission_Config transConfig;

    // These APIs are exposed to user application.
    API_COMPILE(HAL_I2C_Init(i2c, &config));
    API_COMPILE(HAL_I2C_Set_Speed(i2c, 0, NULL));
    API_COMPILE(HAL_I2C_Enable_DMA_Mode(i2c, 0, NULL));
    API_COMPILE(HAL_I2C_Stretch_Clock(i2c, 0, NULL));
    API_COMPILE(HAL_I2C_Begin(i2c, mode, 0, NULL));
    API_COMPILE(HAL_I2C_End(i2c, NULL));
    API_COMPILE(HAL_I2C_Request_Data(i2c, 0, 0, 0, NULL));
    API_COMPILE(HAL_I2C_Request_Data_Ex(i2c, &transConfig, NULL));
    API_COMPILE(HAL_I2C_Begin_Transmission(i2c, 0, &transConfig));
    API_COMPILE(HAL_I2C_End_Transmission(i2c, 0, NULL));
    API_COMPILE(HAL_I2C_Write_Data(i2c, 0, NULL));
    API_COMPILE(HAL_I2C_Available_Data(i2c, NULL));
    API_COMPILE(HAL_I2C_Read_Data(i2c, NULL));
    API_COMPILE(HAL_I2C_Peek_Data(i2c, NULL));
    API_COMPILE(HAL_I2C_Flush_Data(i2c, NULL));
    API_COMPILE(HAL_I2C_Is_Enabled(i2c, NULL));
    API_COMPILE(HAL_I2C_Set_Callback_On_Receive(i2c, NULL, NULL));
    API_COMPILE(HAL_I2C_Set_Callback_On_Request(i2c, NULL, NULL));
    API_COMPILE(HAL_I2C_Reset(i2c, 0, NULL));
    API_COMPILE(HAL_I2C_Acquire(i2c, NULL));
    API_COMPILE(HAL_I2C_Release(i2c, NULL));
#pragma GCC diagnostic pop
}

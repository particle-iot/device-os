
#include "testapi.h"

test(usart_hal_backwards_compatibility)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    Ring_Buffer rxBuf, txBuf;
    HAL_USART_Serial serial = HAL_USART_SERIAL1;
    HAL_USART_Buffer_Config config;
    (void) config;

    // These APIs are exposed to user application.
    API_COMPILE(HAL_USART_Init(serial, &rxBuf, &txBuf));
    API_COMPILE(HAL_USART_Begin(serial, 0));
    API_COMPILE(HAL_USART_End(serial));
    API_COMPILE(HAL_USART_Write_Data(serial, 0));
    API_COMPILE(HAL_USART_Available_Data_For_Write(serial));
    API_COMPILE(HAL_USART_Available_Data(serial));
    API_COMPILE(HAL_USART_Read_Data(serial));
    API_COMPILE(HAL_USART_Peek_Data(serial));
    API_COMPILE(HAL_USART_Flush_Data(serial));
    API_COMPILE(HAL_USART_Is_Enabled(serial));
    API_COMPILE(HAL_USART_Half_Duplex(serial, 0));
    API_COMPILE(HAL_USART_BeginConfig(serial, 0, 0, NULL));
    API_COMPILE(HAL_USART_Write_NineBitData(serial, 0));
    API_COMPILE(HAL_USART_Send_Break(serial, NULL));
    API_COMPILE(HAL_USART_Break_Detected(serial));
#pragma GCC diagnostic pop
}

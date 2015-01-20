
#include <stddef.h>
#include "ota_flash_hal.h"
#include "delay_hal.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_rgb.h"
#include "system_update.h"
#include "system_cloud.h"
#include "rgbled.h"


ymodem_serial_flash_update_handler Ymodem_Serial_Flash_Update = NULL;

volatile uint8_t SPARK_CLOUD_CONNECT = 1; //default is AUTOMATIC mode
volatile uint8_t SPARK_CLOUD_SOCKETED;
volatile uint8_t SPARK_CLOUD_CONNECTED;
volatile uint8_t SPARK_FLASH_UPDATE;
volatile uint32_t TimingFlashUpdateTimeout;

void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler)
{
    Ymodem_Serial_Flash_Update = handler;
}

bool system_serialSaveFile(Stream *serialObj, uint32_t sFlashAddress)
{
    bool status = false;

    if (NULL != Ymodem_Serial_Flash_Update)
    {
        status = Ymodem_Serial_Flash_Update(serialObj, sFlashAddress);
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;
    }

    return status;
}

bool system_serialFirmwareUpdate(Stream *serialObj)
{
    bool status = false;

    if (NULL != Ymodem_Serial_Flash_Update)
    {
        status = Ymodem_Serial_Flash_Update(serialObj, HAL_OTA_FlashAddress());
        if (status == true)
        {
            serialObj->println("Restarting system to apply firmware update...");
            HAL_Delay_Milliseconds(100);
            HAL_FLASH_End();
        }
        else
        {
            SPARK_FLASH_UPDATE = 0;
            TimingFlashUpdateTimeout = 0;
        }
    }
    else
    {
        serialObj->println("Firmware update using this terminal is not supported!");
        serialObj->println("Add #include \"YmodemUtil\\YmodemUtil.h\" to your sketch and try again.");
    }

    return status;
}

void begin_flash_file(int flashType, uint32_t sFlashAddress, uint32_t fileSize)
{
    RGB.control(true);
    RGB.color(RGB_COLOR_MAGENTA);
    SPARK_FLASH_UPDATE = flashType;
    TimingFlashUpdateTimeout = 0;
    HAL_FLASH_Begin(sFlashAddress, fileSize);
}

void Spark_Prepare_To_Save_File(uint32_t sFlashAddress, uint32_t fileSize)
{
    begin_flash_file(2, sFlashAddress, fileSize);
}

void Spark_Prepare_For_Firmware_Update(void)
{
    begin_flash_file(1, HAL_OTA_FlashAddress(), HAL_OTA_FlashLength());
}

void Spark_Finish_Firmware_Update(void)
{
    if (SPARK_FLASH_UPDATE == 2)
    {
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;
    }
    else
    {
        //Reset the system to complete the OTA update
        HAL_FLASH_End();
    }
    RGB.control(false);
}

uint16_t Spark_Save_Firmware_Chunk(unsigned char *buf, uint32_t buflen)
{
    uint16_t chunkUpdatedIndex;
    TimingFlashUpdateTimeout = 0;
    chunkUpdatedIndex = HAL_FLASH_Update(buf, buflen);
    LED_Toggle(LED_RGB);
    return chunkUpdatedIndex;
}


#include <stddef.h>
#include "ota_flash_hal.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_rgb.h"
#include "system_update.h"
#include "system_cloud.h"
#include "rgbled.h"
#include "module_info.h"
#include "spark_wiring_usbserial.h"
#include "system_ymodem.h"
#include "system_task.h"
#include "spark_wiring_system.h"

#ifdef START_DFU_FLASHER_SERIAL_SPEED
static uint32_t start_dfu_flasher_serial_speed = START_DFU_FLASHER_SERIAL_SPEED;
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
static uint32_t start_ymodem_flasher_serial_speed = START_YMODEM_FLASHER_SERIAL_SPEED;
#endif

ymodem_serial_flash_update_handler Ymodem_Serial_Flash_Update_Handler = NULL;

volatile uint8_t SPARK_CLOUD_CONNECT = 1; //default is AUTOMATIC mode
volatile uint8_t SPARK_CLOUD_SOCKETED;
volatile uint8_t SPARK_CLOUD_CONNECTED;
volatile uint8_t SPARK_FLASH_UPDATE;
volatile uint32_t TimingFlashUpdateTimeout;

void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler)
{
    Ymodem_Serial_Flash_Update_Handler = handler;
}

void set_start_dfu_flasher_serial_speed(uint32_t speed)
{
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    start_dfu_flasher_serial_speed = speed;
#endif
}

void set_start_ymodem_flasher_serial_speed(uint32_t speed)
{
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    start_ymodem_flasher_serial_speed = speed;
#endif
}

bool system_serialSaveFile(Stream *serialObj, uint32_t sFlashAddress)
{
    bool status = false;

    if (NULL != Ymodem_Serial_Flash_Update_Handler)
    {
        status = Ymodem_Serial_Flash_Update_Handler(serialObj, sFlashAddress);
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;
    }

    return status;
}

bool system_serialFirmwareUpdate(Stream *serialObj)
{
    bool status = false;

    if (NULL != Ymodem_Serial_Flash_Update_Handler)
    {
        status = Ymodem_Serial_Flash_Update_Handler(serialObj, HAL_OTA_FlashAddress());
        if (status == true)
        {
            serialObj->println("Restarting system to apply firmware update...");
            HAL_Delay_Milliseconds(100);
            Spark_Finish_Firmware_Update(true);
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
        serialObj->println("Add #include \"Ymodem/Ymodem.h\" to your sketch and try again.");
    }

    return status;
}

void system_lineCodingBitRateHandler(uint32_t bitrate)
{
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    if (bitrate == start_dfu_flasher_serial_speed)
    {
        //Reset device and briefly enter DFU bootloader mode
        System.dfu(false);
    }
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    else if (!WLAN_SMART_CONFIG_START && bitrate == start_ymodem_flasher_serial_speed)
    {
        //Set the Ymodem flasher flag to execute system_serialFirmwareUpdate()
        set_ymodem_serial_flash_update_handler(Ymodem_Serial_Flash_Update);
        RGB.control(true);
        RGB.color(RGB_COLOR_MAGENTA);
        SPARK_FLASH_UPDATE = 3;
        TimingFlashUpdateTimeout = 0;
    }
#endif
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

#ifdef MODULAR_FIRMWARE
#define USER_OTA_MODULE_FUNCTION    MODULE_FUNCTION_USER_PART
#else
#define USER_OTA_MODULE_FUNCTION    MODULE_FUNCTION_MONO_FIRMWARE
#endif

void Spark_Finish_Firmware_Update(bool reset)
{
    if (SPARK_FLASH_UPDATE == 2)
    {
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;
    }
    else
    {
        //Monolithic firmware - will only work on photon
        uint32_t moduleAddress = HAL_FLASH_ModuleAddress(HAL_OTA_FlashAddress());
        uint32_t moduleLength = HAL_FLASH_ModuleLength(HAL_OTA_FlashAddress());

        if (HAL_FLASH_VerifyCRC32(HAL_OTA_FlashAddress(), moduleLength) != false)
        {
            HAL_FLASH_AddToNextAvailableModulesSlot(FLASH_INTERNAL, HAL_OTA_FlashAddress(),
                                                    FLASH_INTERNAL, moduleAddress,
                                                    (moduleLength + 4),//+4 to copy the CRC too                
                                                    USER_OTA_MODULE_FUNCTION,
                                                    MODULE_VERIFY_CRC|MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_FUNCTION);//true to verify the CRC during copy also
        }

        HAL_FLASH_End();
        if (reset)
            HAL_Core_System_Reset();
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

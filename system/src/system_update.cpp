
#include <stddef.h>
#include "spark_wiring_system.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_rgb.h"
#include "spark_wiring_usbserial.h"
#include "ota_flash_hal.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "system_update.h"
#include "system_cloud.h"
#include "rgbled.h"
#include "module_info.h"
#include "system_network.h"
#include "system_ymodem.h"
#include "system_task.h"
#include "spark_protocol_functions.h"
#include "hw_config.h"
#include "string_convert.h"
#include "appender.h"

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

bool system_firmwareUpdate(Stream* stream, void* reserved) 
{
#if PLATFORM_ID>2    
    set_ymodem_serial_flash_update_handler(Ymodem_Serial_Flash_Update);
#endif    
    system_file_transfer_t tx;            
    tx.descriptor.store = FileTransfer::Store::FIRMWARE;    
    return system_fileTransfer(&tx);
}

bool system_fileTransfer(system_file_transfer_t* tx, void* reserved)
{
    bool status = false;
    Stream* serialObj = tx->stream;
    
    if (NULL != Ymodem_Serial_Flash_Update_Handler)
    {        
        status = Ymodem_Serial_Flash_Update_Handler(serialObj, tx->descriptor, NULL);
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;

        if (status == true)
        {
            if (tx->descriptor.store==FileTransfer::Store::FIRMWARE) {
                serialObj->println("Restarting system to apply firmware update...");
                HAL_Delay_Milliseconds(100);
                HAL_Core_System_Reset();
            }
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
    if (!network_listening(0, 0, NULL) && bitrate == start_ymodem_flasher_serial_speed)
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

int Spark_Prepare_For_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{    
    if (file.store==FileTransfer::Store::FIRMWARE) 
    {
        // address is relative to the OTA region. Normally will be 0.
        file.file_address = HAL_OTA_FlashAddress() + file.chunk_address;
        
        // chunk_size 0 indicates defaults.
        if (file.chunk_size==0) {
            file.chunk_size = HAL_OTA_ChunkSize();
            file.file_length = HAL_OTA_FlashLength();
        }        
    }
    int result = 0;
    if (flags & 1) {
        // only check address
    }
    else {
        RGB.control(true);
        RGB.color(RGB_COLOR_MAGENTA);
        SPARK_FLASH_UPDATE = 1;
        TimingFlashUpdateTimeout = 0;
        HAL_FLASH_Begin(file.file_address, file.file_length, NULL);
    }
    return result;
}

#ifdef MODULAR_FIRMWARE
#define USER_OTA_MODULE_FUNCTION    MODULE_FUNCTION_USER_PART
#else
#define USER_OTA_MODULE_FUNCTION    MODULE_FUNCTION_MONO_FIRMWARE
#endif

void serial_dump(const char* msg, ...);

int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{
    SPARK_FLASH_UPDATE = 0;
    TimingFlashUpdateTimeout = 0;
    //serial_dump("update finished flags=%d store=%d", flags, file.store);
    
    if (flags & 1) {    // update successful
        if (file.store==FileTransfer::Store::FIRMWARE)
        {
            /*hal_update_complete_t result = */HAL_FLASH_End(NULL);
                     
            // todo - talk with application and see if now is a good time to reset
            // if update not applied, do we need to reset?
            HAL_Core_System_Reset();        
        }
    }
    RGB.control(false);
    return 0;
}

int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved)
{    
    TimingFlashUpdateTimeout = 0;
    int result = -1;
    if (file.store==FileTransfer::Store::FIRMWARE)
    {
        result = HAL_FLASH_Update(chunk, file.chunk_address, file.chunk_size, NULL);
        LED_Toggle(LED_RGB);
    }
    return result;
}

class AppendJson
{
    appender_fn fn;
    void* data;
    
public:

    AppendJson(appender_fn fn, void* data) {
        this->fn = fn; this->data = data;
    }
    
    bool write_quoted(const char* value) {
        return write('"') &&
               write(value) &&
               write('"');
    }
    
    bool write_attribute(const char* name) {
        return 
                write_quoted(name) &&
                write(':');
    }
    
    bool write_string(const char* name, const char* value) {
        return write_attribute(name) &&
               write_quoted(value) &&
               next();
    }
    
    bool newline() { return true; /*return write("\r\n");*/ }
    
    bool write_value(const char* name, int value) {
        char buf[10];
        itoa(value, buf, 10);
        return write_attribute(name) &&
               write(buf) &&
               next();
    }
    
    bool end_list() {
        return write_attribute("_") &&
               write_quoted("");
    }
    
    bool write(char c) {
        return fn(data, (const uint8_t*)&c, 1);
    }
    
    bool write(const char* string) {
        return fn(data, (const uint8_t*)string, strlen(string));
    }
    
    bool next() { return write(',') && newline(); }
};

const char* module_function_string(module_function_t func) {
    switch (func) {
        case MODULE_FUNCTION_NONE: return "n";
        case MODULE_FUNCTION_RESOURCE: return "r";
        case MODULE_FUNCTION_BOOTLOADER: return "b";
        case MODULE_FUNCTION_MONO_FIRMWARE: return "m";
        case MODULE_FUNCTION_SYSTEM_PART: return "s";
        case MODULE_FUNCTION_USER_PART: return "u";
        default: return "_";
    }    
}

const char* module_store_string(module_store_t store) {
    switch (store) {
        case MODULE_STORE_MAIN: return "m";
        case MODULE_STORE_BACKUP: return "b";
        case MODULE_STORE_FACTORY: return "f";
        case MODULE_STORE_SCRATCHPAD: return "t";
        default: return "_";
    }
}

const char* module_name(uint8_t index, char* buf)
{
    return itoa(index, buf, 10);
}

bool system_info_to_json(appender_fn append, void* append_data, hal_system_info_t& system)
{
    AppendJson json(append, append_data);
    bool result = true;
    result &= json.write_value("p", system.platform_id)  
        && json.write_attribute("m")
        && json.write('[');
    char buf[65];
    for (unsigned i=0; i<system.module_count; i++) {        
        if (i) result &= json.write(',');
        const hal_module_t& module = system.modules[i];
        const module_info_t* info = module.info;
        buf[64] = 0;
        bool output_uuid = module.suffix && module_function(info)==MODULE_FUNCTION_USER_PART;
        result &= json.write('{') && json.write_value("s", module.bounds.maximum_size) && json.write_string("l", module_store_string(module.bounds.store))
                && json.write_value("vc",module.validity_checked) && json.write_value("vv", module.validity_result)
          && (!output_uuid || json.write_string("u", bytes2hexbuf(module.suffix->sha, 32, buf)))
          && (!info || (json.write_string("f", module_function_string(module_function(info)))
                        && json.write_string("n", module_name(module_index(info), buf))
                        && json.write_value("v", info->module_version)))
        // on the photon we have just one dependency, this will need generalizing for other platforms
          && json.write_attribute("d") && json.write('[');
          
        for (unsigned int d=0; d<1 && info; d++) {
            const module_dependency_t& dependency = info->dependency;
            module_function_t function = module_function_t(dependency.module_function);
            if (function==MODULE_FUNCTION_NONE) // skip empty dependents
                continue;
            if (d) result &= json.write(',');
            result &= json.write('{') 
              && json.write_string("f", module_function_string(function))
              && json.write_string("n", module_name(dependency.module_index, buf))
              && json.write_value("v", dependency.module_version)
               && json.end_list() && json.write('}');
        }          
        result &= json.write("]}");
    }
    
    result &= json.write(']');
    return result;
}


bool system_module_info(appender_fn append, void* append_data, void* reserved)
{
    hal_system_info_t info;    
    HAL_System_Info(&info, true, NULL);
    bool result = system_info_to_json(append, append_data, info);
    HAL_System_Info(&info, false, NULL);
    return result;
}


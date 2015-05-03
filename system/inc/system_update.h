
#ifndef SYSTEM_UPDATE_H
#define	SYSTEM_UPDATE_H

#include "file_transfer.h"
#include "static_assert.h"
#include "appender.h"
#ifdef __cplusplus
extern "C" {
#endif    

typedef struct Stream Stream;
#include <stdint.h>

typedef bool (*ymodem_serial_flash_update_handler)(Stream *serialObj, FileTransfer::Descriptor& file, void*);
void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler);

void set_start_dfu_flasher_serial_speed(uint32_t speed);
void set_start_ymodem_flasher_serial_speed(uint32_t speed);

/**
 * Updates firmware via ymodem from a given stream.
 * @param stream
 * @return true on successful update.
 */
bool system_firmwareUpdate(Stream* stream, void* reserved=NULL);


struct system_file_transfer_t {
    system_file_transfer_t() {
        memset(this, 0, sizeof(*this));
        size = sizeof(*this);
    }
    
    uint16_t size;
    uint16_t padding;   // reuse if possible
    Stream* stream;
    FileTransfer::Descriptor descriptor;
};

STATIC_ASSERT(system_file_transfer_size, sizeof(system_file_transfer_t)==sizeof(FileTransfer::Descriptor)+8);
        
bool system_fileTransfer(system_file_transfer_t* transfer, void* reserved=NULL);

void system_lineCodingBitRateHandler(uint32_t bitrate);

bool system_module_info(appender_fn appender, void* append_data, void* reserved=NULL);

/**
 * 
 * @param file
 * @param flags bit 0 set (1) means it's a dry run to check parameters. bit 0 cleared means it's the real thing.
 * @param reserved NULL
 * @return 0 on success. 
 */
int Spark_Prepare_For_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved);

/**
 * 
 * @param file
 * @param flags 1 if the image was successfully received. 0 if there was interruption or other error.
 * @param reserved NULL
 * @return 0 on success.
 */
int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved);

/**
 * Provides a chunk of the file data.
 * @param file
 * @param chunk     The chunk data
 * @param reserved
 * @return 
 */
int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved);

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_UPDATE_H */


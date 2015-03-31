
#ifndef SYSTEM_UPDATE_H
#define	SYSTEM_UPDATE_H

#include "spark_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif    

typedef struct Stream Stream;
#include <stdint.h>

typedef bool (*ymodem_serial_flash_update_handler)(Stream *serialObj, FileTransfer::Descriptor& file);
void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler);

void set_start_dfu_flasher_serial_speed(uint32_t speed);
void set_start_ymodem_flasher_serial_speed(uint32_t speed);

bool system_serialFirmwareUpdate(Stream* stream);
bool system_serialFileTransfer(Stream* stream, FileTransfer::Descriptor& file);

void system_lineCodingBitRateHandler(uint32_t bitrate);

int Spark_Prepare_For_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags);
int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file);
int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk);

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_UPDATE_H */


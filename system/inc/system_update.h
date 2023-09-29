
#ifndef SYSTEM_UPDATE_H
#define	SYSTEM_UPDATE_H

#include "file_transfer.h"
#include "static_assert.h"
#include "appender.h"
#include "system_defs.h"
#include "system_info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef class Stream Stream;
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
    system_file_transfer_t()
            : size{sizeof(*this)},
              padding{0},
              stream{nullptr},
              descriptor{} {
    }

    uint16_t size;
    uint16_t padding;   // reuse if possible
    Stream* stream;
    FileTransfer::Descriptor descriptor;
};

PARTICLE_STATIC_ASSERT(system_file_transfer_size, sizeof(system_file_transfer_t)==sizeof(FileTransfer::Descriptor)+8 || sizeof(void*)!=4);

bool system_fileTransfer(system_file_transfer_t* transfer, void* reserved=NULL);

void system_lineCodingBitRateHandler(uint32_t bitrate);

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

/**
 * Firmware update status.
 */
typedef enum {
    /**
     * The system will check for firmware updates when the device connects to the Cloud.
     */
    SYSTEM_UPDATE_STATUS_UNKNOWN = 0,
    /**
     * No firmware update available.
     */
    SYSTEM_UPDATE_STATUS_NOT_AVAILABLE = 1,
    /**
     * A firmware update is available.
     */
    SYSTEM_UPDATE_STATUS_PENDING = 2,
    /**
     * A firmware update is in progress.
     */
    SYSTEM_UPDATE_STATUS_IN_PROGRESS = 3
} system_update_status;

void system_shutdown_if_needed();
void system_pending_shutdown(System_Reset_Reason reason);

int system_set_flag(system_flag_t flag, uint8_t value, void* reserved);
int system_get_flag(system_flag_t flag, uint8_t* value,void* reserved);
int system_refresh_flag(system_flag_t flag);

/**
 * Get the firmware update status.
 *
 * @return A value defined by the `system_update_status` enum or a negative result code in case of
 *         an error.
 */
int system_get_update_status(void* reserved);

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_UPDATE_H */


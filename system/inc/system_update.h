
#ifndef SYSTEM_UPDATE_H
#define	SYSTEM_UPDATE_H

#include "file_transfer.h"
#include "static_assert.h"
#include "appender.h"
#include "system_defs.h"
#include "system_info.h"
#include "security_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef class Stream Stream;
#include <stdint.h>

void set_start_dfu_flasher_serial_speed(uint32_t speed);

/**
 * Updates firmware via ymodem from a given stream.
 * @param stream
 * @return true on successful update.
 */
bool system_firmwareUpdate_deprecated(Stream* stream, void* reserved=NULL);

void set_ymodem_serial_flash_update_handler_deprecated(void*);


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

bool system_fileTransfer_deprecated(system_file_transfer_t* transfer, void* reserved=NULL);

void system_lineCodingBitRateHandler(uint32_t bitrate);

/**
 *
 * @param file
 * @param flags bit 0 set (1) means it's a dry run to check parameters. bit 0 cleared means it's the real thing.
 * @param reserved NULL
 * @return 0 on success.
 */
SECURITY_MODE_PROTECTED_FN(int, Spark_Prepare_For_Firmware_Update, (FileTransfer::Descriptor& file, uint32_t flags, void* reserved));

/**
 *
 * @param file
 * @param flags 1 if the image was successfully received. 0 if there was interruption or other error.
 * @param reserved NULL
 * @return 0 on success.
 */
SECURITY_MODE_PROTECTED_FN(int, Spark_Finish_Firmware_Update, (FileTransfer::Descriptor& file, uint32_t flags, void* reserved));

/**
 * Provides a chunk of the file data.
 * @param file
 * @param chunk     The chunk data
 * @param reserved
 * @return
 */
SECURITY_MODE_PROTECTED_FN(int, Spark_Save_Firmware_Chunk, (FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved));

typedef enum
{
    /**
     * When 0, no OTA update is pending.
     * When 1, an OTA update is pending, and will start when the SYSTEM_FLAG_OTA_UPDATES_FLAG
     * is set.
     */
    SYSTEM_FLAG_OTA_UPDATE_PENDING,

    /**
     * When 0, OTA updates are not started.
     * When 1, OTA updates are started. Default.
     */
    SYSTEM_FLAG_OTA_UPDATE_ENABLED,

    /*
     * When 0, no reset is pending.
     * When 1, a reset is pending. The system will perform the reset
     * when SYSTEM_FLAG_RESET_ENABLED is set to 1.
     */
    SYSTEM_FLAG_RESET_PENDING,

    /**
     * When 0, the system is not able to perform a system reset.
     * When 1, thee system will reset the device when a reset is pending.
     */
    SYSTEM_FLAG_RESET_ENABLED,

    /**
     * A persistent flag that when set will cause the system to startup
     * in listening mode. The flag is automatically cleared on reboot.
     */
    SYSTEM_FLAG_STARTUP_LISTEN_MODE,

	/**
	 * Enable/Disable use of serial1 during setup.
	 */
	SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1,

    /**
     * Enable/disable publishing of last reset info to the cloud.
     */
    SYSTEM_FLAG_PUBLISH_RESET_INFO,

    /**
     * When 0, the system doesn't reset network connection on cloud connection errors.
     * When 1 (default), the system resets network connection after a number of failed attempts to
     * connect to the cloud.
     */
    SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS,

    /**
     * Enable/Disable runtime power management peripheral detection
     */
    SYSTEM_FLAG_PM_DETECTION,

	/**
	 * When 0, OTA updates are only applied when SYSTEM_FLAG_OTA_UPDATE_ENABLED is set.
	 * When 1, OTA updates are applied irrespective of the value of SYSTEM_FLAG_OTA_UPDATE_ENABLED.
	 */
    SYSTEM_FLAG_OTA_UPDATE_FORCED,

    SYSTEM_FLAG_MAX

} system_flag_t;

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


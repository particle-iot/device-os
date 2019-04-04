
#ifndef SYSTEM_UPDATE_H
#define	SYSTEM_UPDATE_H

#include "file_transfer.h"
#include "static_assert.h"
#include "appender.h"

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
    system_file_transfer_t() {
        memset(this, 0, sizeof(*this));
        size = sizeof(*this);
    }

    uint16_t size;
    uint16_t padding;   // reuse if possible
    Stream* stream;
    FileTransfer::Descriptor descriptor;
};

PARTICLE_STATIC_ASSERT(system_file_transfer_size, sizeof(system_file_transfer_t)==sizeof(FileTransfer::Descriptor)+8 || sizeof(void*)!=4);

bool system_fileTransfer(system_file_transfer_t* transfer, void* reserved=NULL);

void system_lineCodingBitRateHandler(uint32_t bitrate);

bool system_module_info(appender_fn appender, void* append_data, void* reserved=NULL);
bool system_metrics(appender_fn appender, void* append_data, uint32_t flags, uint32_t page, void* reserved=NULL);
bool append_system_version_info(Appender* appender);

bool ota_update_info(appender_fn append, void* append_data, void* mod, bool full, void* reserved);

typedef enum {
    MODULE_INFO_JSON_INCLUDE_PLATFORM_ID = 0x0001
} module_info_json_flags_t;

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
int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* module);

/**
 * Provides a chunk of the file data.
 * @param file
 * @param chunk     The chunk data
 * @param reserved
 * @return
 */
int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved);

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


void system_shutdown_if_needed();
void system_pending_shutdown();

int system_set_flag(system_flag_t flag, uint8_t value, void* reserved);
int system_get_flag(system_flag_t flag, uint8_t* value,void* reserved);
int system_refresh_flag(system_flag_t flag);

/**
 * Formats the diagnostic data using an appender function.
 *
 * @param id Array of data source IDs. This argument can be set to NULL to format all registered data sources.
 * @param count Number of data source IDs in the array.
 * @param flags Formatting flags.
 * @param append Appender function.
 * @param append_data Opaque data passed to the appender function.
 * @param reserved Reserved argument (should be set to NULL).
 */
int system_format_diag_data(const uint16_t* id, size_t count, unsigned flags, appender_fn append, void* append_data,
        void* reserved);

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_UPDATE_H */


/**
 ******************************************************************************
  Copyright (c) 2014-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */


#pragma once

#include <stdint.h>
#include <time.h>
#include "protocol_defs.h"
#include "system_tick_hal.h"
#include "spark_descriptor.h"
#include "events.h"
#include "dsakeygen.h"
#include "eckeygen.h"
#include "file_transfer.h"
#include "protocol_selector.h"
#include "protocol_defs.h"
#include "system_defs.h"
#include "completion_handler.h"
#include "hal_platform.h"
#include "time_compat.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct SparkKeys
{
  uint16_t size;
  unsigned char *core_private;
  unsigned char *server_public;
  unsigned char *core_public;
};

PARTICLE_STATIC_ASSERT(SparkKeys_size, sizeof(SparkKeys)==16 || sizeof(void*)!=4);

enum ProtocolFactory
{
	PROTOCOL_NONE,
	PROTOCOL_LIGHTSSL,
	PROTOCOL_DTLS,
};

struct SparkCallbacks
{
    uint16_t size;
    /**
     * The type of protocol to instantiate.
     */
    uint8_t protocolFactory;

    uint8_t reserved;

    int (*send)(const unsigned char *buf, uint32_t buflen, void* handle);
    int (*receive)(unsigned char *buf, uint32_t buflen, void* handle);

#if HAL_PLATFORM_OTA_PROTOCOL_V3
    int (*start_firmware_update)(size_t file_size, const char* file_hash, size_t* partial_size, unsigned flags);
    int (*finish_firmware_update)(unsigned flags);
    int (*save_firmware_chunk)(const char* chunk_data, size_t chunk_size, size_t chunk_offset, size_t partial_size);
#else
    /**
     * @param flags 1 dry run only.
     * Return 0 on success.
     */
    int (*prepare_for_firmware_update)(FileTransfer::Descriptor& data, uint32_t flags, void*);

    /**
     *
     * @return 0 on success
     */
    int (*save_firmware_chunk)(FileTransfer::Descriptor& descriptor, const unsigned char* chunk, void*);

    /**
     * Finalize the data storage.
     * #param reset - if the device should be reset to apply the changes.
     * #return 0 on success. Other values indicate an issue with the file.
     */
    int (*finish_firmware_update)(FileTransfer::Descriptor& data, uint32_t flags, void*);
#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3

    uint32_t (*calculate_crc)(const unsigned char *buf, uint32_t buflen);

    void (*signal)(bool on, unsigned int param, void* reserved);
    system_tick_t (*millis)();

    /**
     * Sets the time. Time is given in milliseconds since the epoch, UCT.
     */
    void (*set_time)(uint32_t t, unsigned int param, void* reserved);

    // size == 40

    /**
     * A pointer that is passed back to the send/receive functions.
     */
    void* transport_context;

    // size == 44

    enum PersistType
    {
        PERSIST_SESSION = 0
    };
    int (*save)(const void* data, size_t length, uint8_t type, void* reserved);
    /**
     * Restore to the given buffer. Returns the number of bytes restored.
     */
    int (*restore)(void* data, size_t max_length, uint8_t type, void* reserved);

    // size == 52

    /**
     * Notify the client that all messages sent to the server have been processed.
     */
    void (*notify_client_messages_processed)(void* reserved);

    // size == 56
};

PARTICLE_STATIC_ASSERT(SparkCallbacks_size, sizeof(SparkCallbacks)==(sizeof(void*)*14));

/**
 * Application-supplied callbacks. (Deliberately distinct from the system-supplied
 * callbacks.)
 */
typedef struct CommunicationsHandlers {
    uint16_t size;

    /**
     * Handle the cryptographically secure random seed from the cloud.
     * @param seed  A random value. This is typically used to seed a pseudo-random generator.
     */
    void (*random_seed_from_cloud)(unsigned int seed);

} CommunicationsHandlers;


PARTICLE_STATIC_ASSERT(CommunicationHandlers_size, sizeof(CommunicationsHandlers)==8 || sizeof(void*)!=4);

typedef struct {
    uint16_t size;
    product_id_t product_id;
    product_firmware_version_t product_version;
    uint16_t reserved;  // make the padding explicit
} product_details_t;

PARTICLE_STATIC_ASSERT(product_details_size, sizeof(product_details_t)==8);


void spark_protocol_communications_handlers(ProtocolFacade* protocol, CommunicationsHandlers* handlers);

void spark_protocol_init(ProtocolFacade* protocol, const char *id,
          const SparkKeys &keys,
          const SparkCallbacks &callbacks,
          const SparkDescriptor &descriptor, void* reserved=NULL);
int spark_protocol_handshake(ProtocolFacade* protocol, void* reserved=NULL);
bool spark_protocol_event_loop(ProtocolFacade* protocol, void* reserved=NULL);
bool spark_protocol_is_initialized(ProtocolFacade* protocol);
int spark_protocol_presence_announcement(ProtocolFacade* protocol, unsigned char *buf, const unsigned char *id, void* reserved=NULL);

/**
 * Content type.
 */
enum spark_protocol_content_type {
    PROTOCOL_CONTENT_TYPE_PLAIN_TEXT = 1 ///< Plain text.
};

/**
 * Flags used by `spark_protocol_event_data_callback` and `spark_protocol_write_event_data()`.
 */
enum spark_protocol_event_data_flag {
    PROTOCOL_EVENT_DATA_READABLE = 0x01, ///< Event data is available for reading.
    PROTOCOL_EVENT_DATA_WRITABLE = 0x02, ///< Event data is available for writing.
    PROTOCOL_EVENT_DATA_MORE = 0x04 ///< Sender has more data to send.
};

/**
 * Callback invoked when an event's data becomes available for reading or writing.
 *
 * @param handle Event handle.
 * @param flags Flags defined by the `spark_protocol_event_data_flag` enum.
 * @param user_data User data.
 * @return 0 on success or a negative result code in case of an error.
 */
typedef int(*spark_protocol_event_data_callback)(int handle, unsigned flags, void* user_data);

/**
 * Additional parameters for `spark_protocol_send_event()`.
 */
typedef struct {
    /**
     * Size of this structure.
     */
    size_t size;
    /**
     * Completion callback.
     */
    completion_callback complete_fn;
    /**
     * User data passed to the `complete_fn` and `data_fn` callbacks.
     */
    void* user_data;
    /**
     * Content type of the event data (see the `spark_protocol_content_type` enum).
     */
    int content_type;
    /**
     * Size of the event data.
     *
     * This parameter can be set to `SIZE_MAX` if the size of the complete event data is unknown
     * at the moment when sending of the event is initiated.
     */
    size_t data_size;
    /**
     * Handle assigned to the event by the protocol layer.
     */
    int handle;
    /**
     * Callback invoked when the event data becomes available for writing.
     */
    spark_protocol_event_data_callback data_fn;
} spark_protocol_send_event_param;

/**
 * Send an event.
 *
 * @param protocol Protocol instance.
 * @param event_name Event name.
 * @param data Event data (a string).
 * @param ttl Event's TTL (time to live) in seconds.
 * @param flags Event flags (see the `EventType::Flags` enum).
 * @param param Additional parameters.
 * @return `true` if the event has been sent or `false` otherwise.
 */
bool spark_protocol_send_event(ProtocolFacade* protocol, const char *event_name, const char *data, int ttl,
        uint32_t flags, spark_protocol_send_event_param* param);

/**
 * Read the data of an incoming event.
 *
 * @param protocol Protocol instance.
 * @param handle Event handle.
 * @param data Destination buffer.
 * @param size Number of bytes to read.
 * @param reserved Reserved for future use.
 * @return Number of bytes read or a negative result code in case of an error.
 */
int spark_protocol_read_event_data(ProtocolFacade* protocol, int handle, char* data, size_t size, void* reserved);

/**
 * Write the data of an outgoing event.
 *
 * @param protocol Protocol instance.
 * @param handle Event handle.
 * @param data Event data.
 * @param size Number of bytes to write.
 * @param flags Flags defined by the `spark_protocol_event_data_flag` enum.
 * @param reserved Reserved for future use.
 * @return Number of bytes written or a negative result code in case of an error.
 */
int spark_protocol_write_event_data(ProtocolFacade* protocol, int handle, const char* data, size_t size, unsigned flags,
        void* reserved);

/**
 * Get the number of bytes of an event's data available for reading or writing.
 *
 * @param protocol Protocol instance.
 * @param handle Event handle.
 * @param reserved Reserved for future use.
 * @return Number of bytes available or a negative result code in case of an error.
 */
int spark_protocol_event_data_available(ProtocolFacade* protocol, int handle, void* reserved);

/**
 * Cancel sending or receiving of an event.
 *
 * @param protocol Protocol instance.
 * @param handle Event handle.
 * @param reserved Reserved for future use.
 * @return 0 on success or a negative result code in case of an error.
 */
int spark_protocol_cancel_event(ProtocolFacade* protocol, int handle, void* reserved);

bool spark_protocol_send_subscription_device(ProtocolFacade* protocol, const char *event_name, const char *device_id, void* reserved=NULL);
bool spark_protocol_send_subscription_scope(ProtocolFacade* protocol, const char *event_name, SubscriptionScope::Enum scope, void* reserved=NULL);
bool spark_protocol_add_event_handler(ProtocolFacade* protocol, const char *event_name, EventHandler handler, SubscriptionScope::Enum scope, const char* id, void* handler_data=NULL);
bool spark_protocol_send_time_request(ProtocolFacade* protocol, void* reserved=NULL);
void spark_protocol_send_subscriptions(ProtocolFacade* protocol, void* reserved=NULL);
void spark_protocol_remove_event_handlers(ProtocolFacade* protocol, const char *event_name, void* reserved=NULL);
void spark_protocol_set_product_id(ProtocolFacade* protocol, product_id_t product_id, unsigned int param = 0, void* reserved = NULL);
void spark_protocol_set_product_firmware_version(ProtocolFacade* protocol, product_firmware_version_t product_firmware_version, unsigned int param=0, void* reserved = NULL);
void spark_protocol_get_product_details(ProtocolFacade* protocol, product_details_t* product_details, void* reserved=NULL);

int spark_protocol_set_connection_property(ProtocolFacade* protocol, unsigned property, int value, const void* data, void* reserved);
int spark_protocol_get_connection_property(ProtocolFacade* protocol, unsigned property, void* data, size_t* size, void* reserved);
bool spark_protocol_time_request_pending(ProtocolFacade* protocol, void* reserved=NULL);
system_tick_t spark_protocol_time_last_synced(ProtocolFacade* protocol, time32_t* tm32, time_t* tm);

int spark_protocol_to_system_error (int error);

typedef struct {
	size_t size;				// size of this structure
	uint32_t flags;				// for now 0, may be used to influence the details retrieved
	uint16_t current_size;
	uint16_t maximum_size;
} spark_protocol_describe_data;

int spark_protocol_get_describe_data(ProtocolFacade* protocol, spark_protocol_describe_data* limits, void* reserved);

/**
 * @brief Send a describe message.
 *
 * @param[in] protocol The protocol used to send cloud messages
 * @param desc_flags The information description flags (default value: \p DESCRIBE_METRICS)
 * @arg \p DESCRIBE_APPLICATION
 * @arg \p DESCRIBE_METRICS
 * @arg \p DESCRIBE_SYSTEM
 * @param[in,out] reserved Reserved for future use (default value: \p NULL).
 *
 * @returns \p ProtocolError result code
 * @retval \p ProtocolError::NO_ERROR
 * @retval \p ProtocolError::IO_ERROR_GENERIC_SEND
 */
int spark_protocol_post_description(ProtocolFacade* protocol, int desc_flags, void* reserved);

namespace ProtocolCommands {
  enum Enum {
    SLEEP = 0, // Deprecated, use DISCONNECT instead
    WAKE = 1, // Deprecated, use PING instead
    DISCONNECT = 2,
    TERMINATE = 3,
    PING = 4
  };
};

/**
 * Parameters of a DISCONNECT command.
 */
typedef struct spark_disconnect_command {
    uint16_t size; ///< Size of this structure.
    uint16_t cloud_reason; ///< Cloud disconnection reason (a value defined by the `cloud_disconnect_reason` enum).
    uint16_t network_reason; ///< Network disconnection reason (a value defined by the `network_disconnect_reason` enum).
    uint16_t reset_reason; ///< System reset reason (a value defined by the `System_Reset_Reason` enum).
    uint32_t sleep_duration; ///< Sleep duration in seconds.
    uint32_t timeout; ///< Maximum time in milliseconds to spend waiting for acknowledgements.
} spark_disconnect_command;

int spark_protocol_command(ProtocolFacade* protocol, ProtocolCommands::Enum cmd, uint32_t value = 0, const void* data = NULL);

/**
 * Protocol status flags.
 *
 * @see `protocol_status`
 */
typedef enum protocol_status_flag {
    /**
     * This flag is set if there are client messages waiting for an acknowledgement.
     *
     * @see `SparkCallbacks::notify_client_messages_processed`
     */
    PROTOCOL_STATUS_HAS_PENDING_CLIENT_MESSAGES = 0x01
} protocol_status_flag;

/**
 * Protocol status.
 */
typedef struct protocol_status {
    uint16_t size; ///< Size of this structure.
    uint32_t flags; ///< Status flags (see `protocol_status_flag`).
} protocol_status;

/**
 * Get protocol status.
 *
 * @param protocol Protocol instance.
 * @param status Status info.
 * @param reserved This argument should be set to NULL.
 * @param 0 on success.
 */
int spark_protocol_get_status(ProtocolFacade* protocol, protocol_status* status, void* reserved);

/**
 * Decrypt a buffer using the given public key.
 * @param ciphertext        The ciphertext to decrypt
 * @param private_key       The private key (in DER format).
 * @param plaintext         buffer to hold the resulting plaintext
 * @param max_plaintext_len The size of the plaintext buffer
 * @return The number of plaintext bytes in the plain text buffer, or <0 on error.
 */
extern int decrypt_rsa(const uint8_t* ciphertext, const uint8_t* private_key,
        uint8_t* plaintext, int32_t max_plaintext_len);

void extract_public_rsa_key(uint8_t* device_pubkey, const uint8_t* device_privkey);

/**
 * Retrieves a pointer to a statically allocated instance.
 * @return A statically allocated instance of ProtocolFacade.
 */
extern ProtocolFacade* spark_protocol_instance();

#ifdef	__cplusplus
}
#endif


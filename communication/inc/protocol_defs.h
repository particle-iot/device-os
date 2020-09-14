#pragma once

#include <functional>
#include "system_tick_hal.h"
#include "hal_platform.h"

#include "system_error.h"
#include "platforms.h"

#ifndef UNIT_TEST
#include "mbedtls_config.h"
#endif

typedef uint16_t product_id_t;
typedef uint16_t product_firmware_version_t;

namespace particle { namespace protocol {

#ifndef PRODUCT_ID
#define PRODUCT_ID (0xffff)
#endif

#ifndef PRODUCT_FIRMWARE_VERSION
#define PRODUCT_FIRMWARE_VERSION (0xffff)
#endif

#define MAX_SUBSCRIPTIONS (6)       // 2 system and 4 application

#if HAL_PLATFORM_OTA_PROTOCOL_V3

/**
 * Minimum supported chunk size.
 *
 * This parameter affects the size of the chunk bitmap maintained by the protocol implementation.
 */
const size_t MIN_OTA_CHUNK_SIZE = 512;

static_assert(MIN_OTA_CHUNK_SIZE % 4 == 0, "Invalid MIN_OTA_CHUNK_SIZE");

/**
 * CoAP overhead per chunk message:
 *
 * Message header (4 bytes)
 * Uri-Path option (2 bytes)
 * Chunk-Index option (4-7 bytes)
 * Payload marker (1 byte)
 */
const size_t OTA_CHUNK_COAP_OVERHEAD = 14;

static_assert(MBEDTLS_SSL_MAX_CONTENT_LEN >= MIN_OTA_CHUNK_SIZE + OTA_CHUNK_COAP_OVERHEAD,
        "MBEDTLS_SSL_MAX_CONTENT_LEN is too small");

/**
 * Maximum chunk size.
 */
const size_t MAX_OTA_CHUNK_SIZE = (MBEDTLS_SSL_MAX_CONTENT_LEN - OTA_CHUNK_COAP_OVERHEAD) / 4 * 4;

static_assert(MAX_OTA_CHUNK_SIZE >= MIN_OTA_CHUNK_SIZE, "Invalid MAX_OTA_CHUNK_SIZE");

#endif // HAL_PLATFORM_OTA_PROTOCOL_V3

enum ProtocolError
{
    /* 00 */ NO_ERROR,
    /* 01 */ PING_TIMEOUT,
    /* 02 */ IO_ERROR,  // too generic, discontinue using this.  Perfer/add a specific one below
    /* 03 */ INVALID_STATE,
    /* 04 */ INSUFFICIENT_STORAGE,
    /* 05 */ MALFORMED_MESSAGE,
    /* 06 */ DECRYPTION_ERROR,
    /* 07 */ ENCRYPTION_ERROR,
    /* 08 */ AUTHENTICATION_ERROR,
    /* 09 */ BANDWIDTH_EXCEEDED,
    /* 10 */ MESSAGE_TIMEOUT,
    /* 11 */ MISSING_MESSAGE_ID,
    /* 12 */ MESSAGE_RESET,
    /* 13 */ SESSION_RESUMED,
    /* 14 */ IO_ERROR_FORWARD_MESSAGE_CHANNEL,
    /* 15 */ IO_ERROR_SET_DATA_MAX_EXCEEDED,
    /* 16 */ IO_ERROR_PARSING_SERVER_PUBLIC_KEY,
    /* 17 */ IO_ERROR_GENERIC_ESTABLISH,
    /* 18 */ IO_ERROR_GENERIC_RECEIVE,
    /* 19 */ IO_ERROR_GENERIC_SEND,
    /* 20 */ IO_ERROR_GENERIC_MBEDTLS_SSL_WRITE,
    /* 21 */ IO_ERROR_DISCARD_SESSION,
    /* 21 */ IO_ERROR_LIGHTSSL_BLOCKING_SEND,
    /* 22 */ IO_ERROR_LIGHTSSL_BLOCKING_RECEIVE,
    /* 23 */ IO_ERROR_LIGHTSSL_RECEIVE,
    /* 24 */ IO_ERROR_LIGHTSSL_HANDSHAKE_NONCE,
    /* 25 */ IO_ERROR_LIGHTSSL_HANDSHAKE_RECV_KEY,
    /* 26 */ NOT_IMPLEMENTED,
    /* 27 */ MISSING_REQUEST_TOKEN,
    /* 28 */ NOT_FOUND,
    /* 29 */ NO_MEMORY,
    /* 30 */ INTERNAL,

    /*
     * NOTE: when adding more ProtocolError codes, be sure to update toSystemError() in protocol_defs.cpp
     */
    UNKNOWN = 0x7FFFF
};

// Converts protocol error to system error code
system_error_t toSystemError(ProtocolError error);

typedef int32_t message_handle_t;

const message_handle_t INVALID_MESSAGE_HANDLE = (message_handle_t)-1;

typedef uint16_t chunk_index_t;

const chunk_index_t NO_CHUNKS_MISSING = 65535;
const chunk_index_t MAX_CHUNKS        = 65535;

#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_ASOM
// FIXME: Argon doesn't seem to be able to process more than 25 chunks per flight
const size_t MISSED_CHUNKS_TO_SEND = 20;
#else
const size_t MISSED_CHUNKS_TO_SEND = 40;
#endif

const size_t MINIMUM_CHUNK_INCREASE   = 2u;
const size_t MAX_EVENT_TTL_SECONDS    = 16777215;
const size_t MAX_OPTION_DELTA_LENGTH  = 12;
const size_t MAX_FUNCTION_ARG_LENGTH = 622;
const size_t MAX_FUNCTION_KEY_LENGTH = 64;
const size_t MAX_VARIABLE_KEY_LENGTH = 64;
const size_t MAX_EVENT_NAME_LENGTH   = 64;
const size_t MAX_EVENT_DATA_LENGTH   = 622;

// Timeout in milliseconds given to receive an acknowledgement for a published event
const unsigned SEND_EVENT_ACK_TIMEOUT = 20000;

#ifndef PROTOCOL_BUFFER_SIZE
#define PROTOCOL_BUFFER_SIZE MBEDTLS_SSL_MAX_CONTENT_LEN
#endif


namespace ChunkReceivedCode {
  enum Enum {
    OK = 0x44,
    BAD = 0x80
  };
}

enum DescriptionType {
    DESCRIBE_SYSTEM = 1<<0,            	// modules
    DESCRIBE_APPLICATION = 1<<1,       	// functions and variables
	DESCRIBE_METRICS = 1<<2,				// metrics/diagnostics
    DESCRIBE_DEFAULT = DESCRIBE_SYSTEM | DESCRIBE_APPLICATION,
	DESCRIBE_MAX = (1<<3)-1
};

namespace Connection
{

/**
 * Connection properties.
 */
enum Enum
{
    PING = 0, ///< Set keepalive interval.
    FAST_OTA = 1, ///< Enable/disable fast OTA.
    DEVICE_INITIATED_DESCRIBE = 2, ///< Enable device-initiated describe messages.
    COMPRESSED_OTA = 3 ///< Enable support for compressed/combined OTA updates.
};

}

typedef std::function<system_tick_t()> millis_callback;
typedef std::function<int()> callback;

const product_id_t UNDEFINED_PRODUCT_ID = product_id_t(-1);
const product_firmware_version_t UNDEFINED_PRODUCT_VERSION = product_firmware_version_t(-1);

namespace UpdateFlag {
enum Enum {
    ERROR         = 0x00,
    SUCCESS       = 0x01,
    VALIDATE_ONLY = 0x02
};
}

typedef uint32_t keepalive_source_t;

typedef struct
{
    uint16_t size;
    keepalive_source_t keepalive_source;
} connection_properties_t;

namespace KeepAliveSource {
enum Enum {
    USER   = 1<<0,   // set by user in wiring
    SYSTEM = 1<<1    // set by system
};
}

/**
 * Application state descriptor.
 */
class AppStateDescriptor {
public:
    /**
     * State flags.
     *
     * These flags determine which fields are set for this application state.
     */
    enum StateFlag {
        SYSTEM_DESCRIBE_CRC = 0x01, ///< Checksum of the system description.
        APP_DESCRIBE_CRC = 0x02, ///< Checksum of the application description.
        SUBSCRIPTIONS_CRC = 0x04, ///< Checksum of the event subscriptions.
        PROTOCOL_FLAGS = 0x08, ///< Protocol flags.
        ALL = SYSTEM_DESCRIBE_CRC | APP_DESCRIBE_CRC | SUBSCRIPTIONS_CRC | PROTOCOL_FLAGS ///< All the defined fields.
    };

    /**
     * Construct a descriptor.
     *
     * @param stateFlags State flags.
     * @param systemDescrCrc Checksum of the system description.
     * @param appDescrCrc Checksum of the application description.
     * @param subscrCrc Checksum of the event subscriptions.
     * @param protocolFlags Protocol flags.
     */
    explicit AppStateDescriptor(uint32_t stateFlags = 0, uint32_t systemDescrCrc = 0, uint32_t appDescrCrc = 0,
                uint32_t subscrCrc = 0, uint32_t protocolFlags = 0) :
            stateFlags_(stateFlags),
            systemDescrCrc_(systemDescrCrc),
            appDescrCrc_(appDescrCrc),
            subscrCrc_(subscrCrc),
            protocolFlags_(protocolFlags) {
    }

    /**
     * Set the checksum of the system description.
     */
    AppStateDescriptor& systemDescribeCrc(uint32_t crc) {
        systemDescrCrc_ = crc;
        stateFlags_ |= StateFlag::SYSTEM_DESCRIBE_CRC;
        return *this;
    }

    /**
     * Set the checksum of the application description.
     */
    AppStateDescriptor& appDescribeCrc(uint32_t crc) {
        appDescrCrc_ = crc;
        stateFlags_ |= StateFlag::APP_DESCRIBE_CRC;
        return *this;
    }

    /**
     * Set the checksum of the event subscriptions.
     */
    AppStateDescriptor& subscriptionsCrc(uint32_t crc) {
        subscrCrc_ = crc;
        stateFlags_ |= StateFlag::SUBSCRIPTIONS_CRC;
        return *this;
    }

    /**
     * Set the protocol flags.
     */
    AppStateDescriptor& protocolFlags(uint32_t flags) {
        protocolFlags_ = flags;
        stateFlags_ |= StateFlag::PROTOCOL_FLAGS;
        return *this;
    }

    /**
     * Returns `true` if `other` is equal to this descriptor, otherwise returns `false`.
     *
     * The `flags` argument determines which fields of the application state are compared (see the `StateFlag` enum).
     */
    bool equalsTo(const AppStateDescriptor& other, uint32_t flags = StateFlag::ALL) const {
        if ((stateFlags_ & flags) != (other.stateFlags_ & flags)) {
            return false;
        }
        if ((flags & StateFlag::SUBSCRIPTIONS_CRC) && (stateFlags_ & StateFlag::SUBSCRIPTIONS_CRC) &&
                (subscrCrc_ != other.subscrCrc_)) {
            return false;
        }
        if ((flags & StateFlag::APP_DESCRIBE_CRC) && (stateFlags_ & StateFlag::APP_DESCRIBE_CRC) &&
                (appDescrCrc_ != other.appDescrCrc_)) {
            return false;
        }
        if ((flags & StateFlag::SYSTEM_DESCRIBE_CRC) && (stateFlags_ & StateFlag::SYSTEM_DESCRIBE_CRC) &&
                (systemDescrCrc_ != other.systemDescrCrc_)) {
            return false;
        }
        if ((flags & StateFlag::PROTOCOL_FLAGS) && (stateFlags_ & StateFlag::PROTOCOL_FLAGS) &&
                (protocolFlags_ != other.protocolFlags_)) {
            return false;
        }
        return true;
    }

    /**
     * Returns the state flags.
     */
    uint32_t stateFlags() const {
        return stateFlags_;
    }

    /**
     * Returns `true` if this descriptor is empty, otherwise returns `false`.
     *
     * An empty descriptor has all state flags cleared.
     */
    bool isEmpty() const {
        return !stateFlags_;
    }

private:
    uint32_t stateFlags_;
    uint32_t systemDescrCrc_;
    uint32_t appDescrCrc_;
    uint32_t subscrCrc_;
    uint32_t protocolFlags_;
};

}} // namespace particle::protocol

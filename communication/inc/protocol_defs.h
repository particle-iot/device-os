#pragma once

#include <functional>
#include "system_tick_hal.h"
#include "hal_platform.h"

#include "system_error.h"
#include "platforms.h"

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

enum ProtocolError
{
    NO_ERROR = 0,
    PING_TIMEOUT = 1,
    IO_ERROR = 2, // too generic, discontinue using this. Perfer/add a specific one below
    INVALID_STATE = 3,
    INSUFFICIENT_STORAGE = 4,
    MALFORMED_MESSAGE = 5,
    DECRYPTION_ERROR = 6,
    ENCRYPTION_ERROR = 7,
    AUTHENTICATION_ERROR = 8,
    BANDWIDTH_EXCEEDED = 9,
    MESSAGE_TIMEOUT = 10,
    MISSING_MESSAGE_ID = 11,
    MESSAGE_RESET = 12,
    SESSION_RESUMED = 13,
    IO_ERROR_FORWARD_MESSAGE_CHANNEL = 14,
    IO_ERROR_SET_DATA_MAX_EXCEEDED = 15,
    IO_ERROR_PARSING_SERVER_PUBLIC_KEY = 16,
    IO_ERROR_GENERIC_ESTABLISH = 17,
    IO_ERROR_GENERIC_RECEIVE = 18,
    IO_ERROR_GENERIC_SEND = 19,
    IO_ERROR_GENERIC_MBEDTLS_SSL_WRITE = 20,
    IO_ERROR_DISCARD_SESSION = 21,
    IO_ERROR_LIGHTSSL_BLOCKING_SEND = 22,
    IO_ERROR_LIGHTSSL_BLOCKING_RECEIVE = 23,
    IO_ERROR_LIGHTSSL_RECEIVE = 24,
    IO_ERROR_LIGHTSSL_HANDSHAKE_NONCE = 25,
    IO_ERROR_LIGHTSSL_HANDSHAKE_RECV_KEY = 26,
    NOT_IMPLEMENTED = 27,
    MISSING_REQUEST_TOKEN = 28,
    NOT_FOUND = 29,
    NO_MEMORY = 30,
    INTERNAL = 31,
    OTA_UPDATE_ERROR = 32, // Generic OTA update error
    IO_ERROR_SOCKET_SEND_FAILED = 33,
    IO_ERROR_SOCKET_RECV_FAILED = 34,
    IO_ERROR_REMOTE_END_CLOSED = 35,
    // NOTE: when adding more ProtocolError codes, be sure to update toSystemError() in protocol_defs.cpp
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

const size_t MINIMUM_CHUNK_INCREASE = 2u;
const size_t MAX_EVENT_TTL_SECONDS = 16777215;
const size_t MAX_OPTION_DELTA_LENGTH = 12;
const size_t MAX_FUNCTION_KEY_LENGTH = 64;
const size_t MAX_VARIABLE_KEY_LENGTH = 64;
const size_t MAX_EVENT_NAME_LENGTH = 64;

const size_t MAX_EVENT_DATA_LENGTH = 1024;
const size_t MAX_FUNCTION_ARG_LENGTH = 1024;
const size_t MAX_VARIABLE_VALUE_LENGTH = 1024;

// Timeout in milliseconds given to receive an acknowledgement for a published event
const unsigned SEND_EVENT_ACK_TIMEOUT = 20000;

/**
 * Maximum possible size of a CoAP message carrying a cloud event.
 */
const size_t MAX_EVENT_MESSAGE_SIZE = 4 /* Header */ + 1 /* Token */ + (MAX_EVENT_NAME_LENGTH + 5) /* Uri-Path options */
        + 5 /* Max-Age option */ + 1 /* Payload marker */ + MAX_EVENT_DATA_LENGTH /* Payload data */;
/**
 * Maximum possible size of a CoAP message carrying a function call.
 */
const size_t MAX_FUNCTION_CALL_MESSAGE_SIZE = 4 /* Header */ + 1 /* Token */ + (MAX_FUNCTION_KEY_LENGTH + 5) /* Uri-Path options */
        + (MAX_FUNCTION_ARG_LENGTH + 3) /* Uri-Query option */;
/**
 * Maximum possible size of a CoAP message carrying a variable value.
 */
const size_t MAX_VARIABLE_VALUE_MESSAGE_SIZE = 4 /* Header */ + 1 /* Token */ + 1 /* Payload marker */ +
        MAX_VARIABLE_VALUE_LENGTH /* Payload data */;

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
    DESCRIBE_SYSTEM = 1<<0, // modules
    DESCRIBE_APPLICATION = 1<<1, // functions and variables
    DESCRIBE_METRICS = 1<<2, // metrics/diagnostics
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
    PING = 0, ///< Keepalive interval (set).
    FAST_OTA = 1, ///< Enable/disable fast OTA (set).
    ENABLE_DEVICE_INITIATED_DESCRIBE = 2, ///< Enable device-initiated describe messages (set).
    COMPRESSED_OTA = 3, ///< Enable/disable support for compressed/combined OTA updates (set).
    SYSTEM_MODULE_VERSION = 4, ///< Module version of the system firmware (set).
    MAX_BINARY_SIZE = 5, ///< Maximum size of a firmware binary (set).
    OTA_CHUNK_SIZE = 6, ///< Size of an OTA update chunk (set).
    MAX_TRANSMIT_MESSAGE_SIZE = 7, ///< Maximum size of of outgoing CoAP message (set).
    MAX_EVENT_DATA_SIZE = 8, ///< Maximum size of event data (get).
    MAX_VARIABLE_VALUE_SIZE = 9, ///< Maximum size of a variable value (get).
    MAX_FUNCTION_ARGUMENT_SIZE = 10 ///< Maximum size of a function call argument (get).
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
        SYSTEM_MODULE_VERSION = 0x10, ///< Module version of the system firmware.
        MAX_MESSAGE_SIZE = 0x20, ///< Maximum size of a CoAP message.
        MAX_BINARY_SIZE = 0x40, ///< Maximum size of a firmware binary.
        OTA_CHUNK_SIZE = 0x80, ///< Size of an OTA update chunk.
        ALL = SYSTEM_DESCRIBE_CRC | APP_DESCRIBE_CRC | SUBSCRIPTIONS_CRC | PROTOCOL_FLAGS | SYSTEM_MODULE_VERSION |
                MAX_MESSAGE_SIZE | MAX_BINARY_SIZE | OTA_CHUNK_SIZE ///< All the defined fields.
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
    explicit AppStateDescriptor(uint32_t stateFlags = 0, uint16_t systemVersion = 0, uint32_t systemDescrCrc = 0,
                uint32_t appDescrCrc = 0, uint32_t subscrCrc = 0, uint32_t protocolFlags = 0, uint16_t maxMessageSize = 0,
                uint32_t maxBinarySize = 0, uint16_t otaChunkSize = 0) :
            stateFlags_(stateFlags),
            systemDescrCrc_(systemDescrCrc),
            appDescrCrc_(appDescrCrc),
            subscrCrc_(subscrCrc),
            protocolFlags_(protocolFlags),
            maxBinarySize_(maxBinarySize),
            systemVersion_(systemVersion),
            maxMessageSize_(maxMessageSize),
            otaChunkSize_(otaChunkSize) {
    }

    /**
     * Set the module version of the system firmware.
     */
    AppStateDescriptor& systemVersion(uint16_t version) {
        systemVersion_ = version;
        stateFlags_ |= StateFlag::SYSTEM_MODULE_VERSION;
        return *this;
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
     * Set the maximum size of a CoAP message.
     */
    AppStateDescriptor& maxMessageSize(uint16_t size) {
        maxMessageSize_ = size;
        stateFlags_ |= StateFlag::MAX_MESSAGE_SIZE;
        return *this;
    }

    /**
     * Set the maximum size of a firmware binary.
     */
    AppStateDescriptor& maxBinarySize(uint32_t size) {
        maxBinarySize_ = size;
        stateFlags_ |= StateFlag::MAX_BINARY_SIZE;
        return *this;
    }

    /**
     * Set the size of an OTA update chunk.
     */
    AppStateDescriptor& otaChunkSize(uint16_t size) {
        otaChunkSize_ = size;
        stateFlags_ |= StateFlag::OTA_CHUNK_SIZE;
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
        if ((flags & StateFlag::SYSTEM_MODULE_VERSION) && (stateFlags_ & StateFlag::SYSTEM_MODULE_VERSION) &&
                (systemVersion_ != other.systemVersion_)) {
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
        if ((flags & StateFlag::MAX_MESSAGE_SIZE) && (stateFlags_ & StateFlag::MAX_MESSAGE_SIZE) &&
                (maxMessageSize_ != other.maxMessageSize_)) {
            return false;
        }
        if ((flags & StateFlag::MAX_BINARY_SIZE) && (stateFlags_ & StateFlag::MAX_BINARY_SIZE) &&
                (maxBinarySize_ != other.maxBinarySize_)) {
            return false;
        }
        if ((flags & StateFlag::OTA_CHUNK_SIZE) && (stateFlags_ & StateFlag::OTA_CHUNK_SIZE) &&
                (otaChunkSize_ != other.otaChunkSize_)) {
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
    uint32_t maxBinarySize_;
    uint16_t systemVersion_;
    uint16_t maxMessageSize_;
    uint16_t otaChunkSize_;
};

}} // namespace particle::protocol

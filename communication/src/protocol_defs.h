#pragma once

#include <functional>
#include "system_tick_hal.h"

namespace particle { namespace protocol {

#ifndef PRODUCT_ID
#define PRODUCT_ID (0xffff)
#endif

#ifndef PRODUCT_FIRMWARE_VERSION
#define PRODUCT_FIRMWARE_VERSION (0xffff)
#endif

enum ProtocolError
{
	NO_ERROR,
	PING_TIMEOUT,
	TRANSPORT_FAILURE,
	INVALID_STATE,
	INSUFFICIENT_STORAGE,
	MALFORMED_MESSAGE,
};

typedef uint16_t chunk_index_t;

const chunk_index_t NO_CHUNKS_MISSING = 65535;
const chunk_index_t MAX_CHUNKS = 65535;
const size_t MISSED_CHUNKS_TO_SEND = 50;
const size_t MAX_FUNCTION_ARG_LENGTH = 64;


namespace ChunkReceivedCode {
  enum Enum {
    OK = 0x44,
    BAD = 0x80
  };
}


typedef std::function<system_tick_t()> millis_callback;
typedef std::function<int()> callback;


}}

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
};

typedef std::function<system_tick_t()> millis_callback;
typedef std::function<int()> callback;


}}

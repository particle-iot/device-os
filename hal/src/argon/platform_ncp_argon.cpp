#include "platform_ncp.h"
#include "network/ncp.h"
#include "wifi_manager.h"
#include "wifi_ncp_client.h"
#include "led_service.h"
#include "check.h"
#include "scope_guard.h"
#include "system_led_signal.h"
#include "xmodem_sender.h"
#include "stream.h"

MeshNCPIdentifier platform_current_ncp_identifier() {
	return MESH_NCP_ESP32;
}

class BufferStream : public particle::InputStream {
public:
	BufferStream(const uint8_t* buffer, size_t length) : buffer(buffer), remaining(length) {}

    int read(char* data, size_t size) override {
        CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        if (!remaining) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining);
        memcpy(data, buffer, size);
        return size;
    }

    int skip(size_t size) override {
        if (!remaining) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, remaining);
        buffer += size;
        remaining -= size;
        return size;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!remaining) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
	const uint8_t* buffer;
	size_t remaining;
};

using particle::XmodemSender;

// FIXME: This function accesses the module info via XIP and may fail to parse it correctly under
// some not entirely clear circumstances. Disabling compiler optimizations helps to work around
// the problem
__attribute__((optimize("O0"))) hal_update_complete_t platform_ncp_update_module(const hal_module_t* module) {
    return HAL_UPDATE_ERROR; // FIXME
/*
	// not so happy about mixing the layers like this. Seems strange that HAL should be dependent on wiring.
	auto& atclient = *particle::wifiManager()->ncpClient()->atParser();

	// we pass only the actual binary after the module info and up to the suffix
	const uint8_t* start = (const uint8_t*)module->info;
	static_assert(sizeof(module_info_t)==24, "expected module info size to be 24");
	start += sizeof(module_info_t);		// skip the module info
	const uint8_t* end = start + (uint32_t(module->info->module_end_address) - uint32_t(module->info->module_start_address));
	const unsigned length = end-start;
	XmodemSender sender;
	BufferStream moduleStream(start, length);
	CHECK_RETURN(sender.init(atclient.getStream(), &moduleStream, length), HAL_UPDATE_ERROR);
	uint16_t previous_version = 0;
	atclient.getModuleVersion(&previous_version);
	LOG(INFO, "Updating ESP32 firmware from version %d to version %d", previous_version, module->info->module_version);
	CHECK_RETURN(atclient.startUpdate(length), HAL_UPDATE_ERROR);

	bool success = false;
	for (;;) {
		const int ret = sender.run();
		if (ret != XmodemSender::RUNNING) {
			if (ret == XmodemSender::DONE) {
				LOG(INFO, "XMODEM transfer finished");
				success = true;
			} else {
				LOG(ERROR, "XMODEM transfer failed: %d", ret);
			}
			break;
		}
		LED_Toggle(LED_RGB);
	}
	LED_On(LED_RGB);
	CHECK_RETURN(atclient.finishUpdate(), HAL_UPDATE_ERROR);
	atclient.reset();

	hal_update_complete_t result = success ? HAL_UPDATE_APPLIED : HAL_UPDATE_ERROR;
	// won't fail the update if we for some reason cannot get the module version
	uint16_t version;
	CHECK_RETURN(atclient.getModuleVersion(&version), result);
	LOG(INFO, "ESP32 firmware version updated to version %d", version);
	return result;
*/
}

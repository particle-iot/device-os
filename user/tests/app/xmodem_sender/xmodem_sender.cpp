#include "application.h"

#include "exflash_hal.h"

#include "xmodem_sender.h"
#include "check.h"
#include "network/ncp.h"

#include "../../../../services/inc/stream.h"

#include <algorithm>
#include <cstdarg>

SYSTEM_MODE(SEMI_AUTOMATIC);

namespace {

// Input stream reading data from the external flash
class ExtFlashStream: public InputStream {
public:
    explicit ExtFlashStream(size_t offs, size_t size) :
            size_(size),
            offs_(offs) {
    }

    int read(char* data, size_t size) override {
        CHECK(peek(data, size));
        return skip(size);
    }

    int peek(char* data, size_t size) override {
        if (!size_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        const size_t n = std::min(size_, size);
        const int ret = hal_exflash_read(offs_, (uint8_t*)data, n);
        if (ret != 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }
        return n;
    }

    int skip(size_t size) override {
        if (!size_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        const size_t n = std::min(size_, size);
        offs_ += n;
        size_ -= n;
        return n;
    }

    int waitEvent(unsigned flags, unsigned timeout) override {
        if (!flags) {
            return 0;
        }
        if (!(flags & InputStream::READABLE)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!size_) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        return InputStream::READABLE;
    }

private:
    size_t size_;
    size_t offs_;
};

std::unique_ptr<XmodemSender> sender;
std::unique_ptr<ExtFlashStream> extFlashStrm;

#ifdef DEBUG_BUILD
const RttLogHandler logHandler(LOG_LEVEL_ALL);
#endif // DEBUG_BUILD

int logVersion() {
    char ver[16] = {};
    uint16_t mver = 0;
    CHECK(argonNcpAtClient()->getVersion(ver, sizeof(ver)));
    CHECK(argonNcpAtClient()->getModuleVersion(&mver));
    Log.info("Current version: %s (%hu)", ver, mver);
    return 0;
}

int init() {
    const size_t SIZE = 0;
    static_assert(SIZE > 0, "File size is not specified");
    const size_t OFFSET = 0x00200000;
    extFlashStrm.reset(new ExtFlashStream(OFFSET, SIZE));
    sender->init(argonNcpAtClient()->getStream(), extFlashStrm.get(), SIZE);
    CHECK(logVersion());
    CHECK(argonNcpAtClient()->startUpdate(SIZE));
    return 0;
}

} // unnamed

void setup() {
    sender.reset(new XmodemSender);
    if (init() != 0) {
        sender.reset();
        Log.error("Failed to talk to NCP");
    }
}

void loop() {
    if (sender) {
        const int ret = sender->run();
        if (ret != XmodemSender::RUNNING) {
            if (ret == XmodemSender::DONE) {
                Log.info("XMODEM transfer finished");
            } else {
                Log.error("XMODEM transfer failed: %d", ret);
            }
            Log.info("Update result: %d", argonNcpAtClient()->finishUpdate());
            sender.reset();
            // Invalidate AT Client state, because the ESP32 was reset
            argonNcpAtClient()->reset();
            logVersion();
        }
    }
}

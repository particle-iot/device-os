#include "application.h"

#include "exflash_hal.h"

#include "xmodem_sender.h"
#include "check.h"
#include "../../../../services/inc/stream.h"
#include "network/ncp.h"

#include <cstdarg>

SYSTEM_MODE(SEMI_AUTOMATIC);

namespace {

// Input stream generating text data
class TextStream: public InputStream {
public:
    explicit TextStream(size_t size) :
            size_(size),
            c_('A') {
    }

    int read(char* data, size_t size) override {
        // XmodemSender shouldn't try to read more bytes than necessary
        if (size > size_) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        for (size_t i = 0; i < size; ++i) {
            data[i] = c_++;
            if (c_ > 'Z') {
                c_ = 'A';
            }
        }
        size_ -= size;
        return size;
    }

private:
    size_t size_;
    char c_;
};

// Input stream reading data from the external flash
class ExtFlashStream: public InputStream {
public:
    explicit ExtFlashStream(size_t offs, size_t size) :
            size_(size),
            offs_(offs) {
    }

    int read(char* data, size_t size) override {
        if (size > size_) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        const int ret = hal_exflash_read(offs_, (uint8_t*)data, size);
        if (ret != 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }
        size_ -= size;
        offs_ += size;
        return size;
    }

private:
    size_t size_;
    size_t offs_;
};

std::unique_ptr<XmodemSender> sender;
std::unique_ptr<ExtFlashStream> extFlashStrm;
std::unique_ptr<TextStream> textStrm;

#ifdef DEBUG_BUILD
const RttLogHandler logHandler(LOG_LEVEL_ALL);
#endif // DEBUG_BUILD

void initTextStream() {
    const size_t SIZE = 3500;
    textStrm.reset(new TextStream(SIZE));
    sender->init(argonNcpAtClient()->getStream(), textStrm.get(), SIZE);
}

int logVersion() {
    char ver[16] = {};
    uint16_t mver = 0;
    CHECK(argonNcpAtClient()->getVersion(ver, sizeof(ver)));
    CHECK(argonNcpAtClient()->getModuleVersion(&mver));
    Log.info("Current version: %s (%hu)", ver, mver);
    return 0;
}

int initExtFlashStream() {
    const size_t SIZE = 800976;
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
    // initTextStream();
    if (initExtFlashStream()) {
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

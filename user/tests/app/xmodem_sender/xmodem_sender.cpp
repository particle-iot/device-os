#include "application.h"

#include "xmodem_sender.h"
#include "../../../../services/inc/stream.h"
#include "file.h"

SYSTEM_MODE(SEMI_AUTOMATIC)

namespace {

const size_t FILE_SIZE = 3500;

class DummyFile: public InputFile {
public:
    DummyFile() :
            size_(0) {
    }

    ~DummyFile() {
        destroy();
    }

    int init(size_t size) {
        data_.reset(new(std::nothrow) char[size]);
        if (!data_) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        // Fill the buffer with some data
        char c = 'A';
        for (size_t i = 0; i < size; ++i) {
            data_[i] = c++;
            if (c > 'Z') {
                c = 'A';
            }
        }
        size_ = size;
        return 0;
    }

    void destroy() {
        data_.reset();
        size_ = 0;
    }

    int read(char* data, size_t size, size_t offs) override {
        if (offs + size > size_) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        memcpy(data, data_.get() + offs, size);
        return size;
    }

    int size() const override {
        return size_;
    }

private:
    std::unique_ptr<char[]> data_;
    size_t size_;
};

class Serial1Stream: public particle::Stream {
public:
    ~Serial1Stream() {
        destroy();
    }

    int init() {
        Serial1.begin(115200);
        // FIXME: XMODEM transfer fails if the non-blocking mode is enabled
        // Serial1.blockOnOverrun(false);
        return 0;
    }

    void destroy() {
        Serial1.end();
    }

    int read(char* data, size_t size) override {
        return Serial1.readBytes(data, size);
    }

    int write(const char* data, size_t size) override {
        return Serial1.write((const uint8_t*)data, size);
    }
};

std::unique_ptr<XmodemSender> g_sender;
std::unique_ptr<Serial1Stream> g_stream;
std::unique_ptr<DummyFile> g_file;

const RttLogHandler logHandler(LOG_LEVEL_ALL);

} // unnamed

void setup() {
    g_file.reset(new(std::nothrow) DummyFile);
    g_file->init(FILE_SIZE);
    g_stream.reset(new(std::nothrow) Serial1Stream);
    g_stream->init();
    g_sender.reset(new(std::nothrow) XmodemSender);
    g_sender->init(g_stream.get(), g_file.get());
}

void loop() {
    if (g_sender) {
        const int ret = g_sender->run();
        if (ret != XmodemSender::RUNNING) {
            if (ret == XmodemSender::DONE) {
                Log.info("XMODEM transfer finished");
            } else if (ret < 0) {
                Log.error("XMODEM transfer failed: %d", ret);
            }
            g_sender.reset();
        }
    }
}

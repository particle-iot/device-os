#include "inflate.h"

#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>

#include <functional>
#include <random>
#include <string>
#include <vector>

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

namespace {

const unsigned DEFAULT_WINDOW_BITS = 15;

class Options {
public:
    Options() :
            windowBits_(DEFAULT_WINDOW_BITS) {
    }

    Options& windowBits(unsigned count) {
        windowBits_ = count;
        return *this;
    }

    unsigned windowBits() const {
        return windowBits_;
    }

private:
    unsigned windowBits_;
};

class Inflate {
public:
    typedef std::function<size_t(const char*, size_t)> OutputFn;

    Inflate() :
            ctx_(nullptr) {
        init();
    }

    ~Inflate() {
        destroy();
    }

    void init(const Options& opts = Options()) {
        destroy();
        inflate_opts inflOpts = {};
        inflOpts.window_bits = opts.windowBits();
        REQUIRE(inflate_create(&ctx_, &inflOpts, outputCallback, this) == 0);
    }

    void destroy() {
        inflate_destroy(ctx_);
        outputData_ = std::string();
        outputFn_ = OutputFn();
        ctx_ = nullptr;
    }

    int input(const char* data, size_t* size, bool hasMore) {
        REQUIRE(ctx_ != nullptr);
        return inflate_input(ctx_, data, size, hasMore ? INFLATE_HAS_MORE_INPUT : 0);
    }

    int input(const char* data, size_t size, bool hasMore) {
        return input(data, &size, hasMore);
    }

    const std::string& output() const {
        return outputData_;
    }

    void outputFn(OutputFn fn) {
        outputFn_ = std::move(fn);
    }

    inflate_ctx* instance() const {
        return ctx_;
    }

private:
    OutputFn outputFn_;
    std::string outputData_;
    inflate_ctx* ctx_;

    static int outputCallback(const char* data, size_t size, void* userData) {
        auto self = (Inflate*)userData;
        self->outputData_.append(data, size);
        if (self->outputFn_) {
            return self->outputFn_(data, size);
        }
        return size;
    }
};

std::string deflateRaw(const std::string& data, const Options& opts = Options()) {
    using namespace boost::iostreams;

    std::istringstream src(data);
    std::ostringstream dest;
    filtering_ostreambuf strm;
    zlib_params params;
    params.window_bits = opts.windowBits();
    params.noheader = true;
    strm.push(zlib_compressor(params));
    strm.push(dest);
    copy(src, strm);
    return dest.str();
}

std::default_random_engine& randomGen() {
    static thread_local std::default_random_engine rand((std::random_device())());
    return rand;
}

std::string genRandomData(size_t size) {
    std::uniform_int_distribution<unsigned> dist(0, 255);
    std::string d;
    d.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        d += (char)dist(randomGen());
    }
    return d;
}

std::string genRandomData(size_t minSize, size_t maxSize) {
    std::uniform_int_distribution<unsigned> dist(minSize, maxSize);
    return genRandomData(dist(randomGen()));
}

std::string genCompressedData(size_t size, const Options& opts = Options()) {
    const size_t minChunkSize = 5;
    const size_t maxChunkSize = 80;
    const size_t chunkCount = 50;
    std::vector<std::string> chunks;
    chunks.reserve(chunkCount);
    for (size_t i = 0; i < chunkCount; ++i) {
        chunks.push_back(genRandomData(minChunkSize, maxChunkSize));
    }
    std::uniform_int_distribution<unsigned> dist(0, chunkCount * 3);
    std::string d;
    d.reserve(size);
    while (d.size() < size) {
        std::string chunk;
        const size_t index = dist(randomGen());
        if (index < chunks.size()) {
            chunk = chunks[index];
        } else {
            chunk = genRandomData(minChunkSize, maxChunkSize);
        }
        d += chunk;
    }
    d.resize(size);
    return deflateRaw(d, opts);
}

std::string genCompressedData(size_t minSize, size_t maxSize, const Options& opts = Options()) {
    std::uniform_int_distribution<unsigned> dist(minSize, maxSize);
    return genCompressedData(dist(randomGen()), opts);
}

std::string genCompressedData(const Options& opts = Options()) {
    return genCompressedData(50000, 1000000, opts);
}

int dummyOutputCallback(const char* data, size_t size, void* userData) {
    return size;
}

} // namespace

TEST_CASE("inflate_create()") {
    SECTION("creates a decompressor instance") {
        inflate_ctx* ctx = nullptr;
        int r = inflate_create(&ctx, nullptr, dummyOutputCallback, nullptr);
        CHECK(r == 0);
        CHECK(ctx != nullptr);
        inflate_destroy(ctx);
    }

    SECTION("can be configured with a specific window size") {
        inflate_ctx* ctx = nullptr;
        inflate_opts opts = {};
        opts.window_bits = INFLATE_MAX_WINDOW_BITS;
        int r = inflate_create(&ctx, &opts, dummyOutputCallback, nullptr);
        CHECK(r == 0);
        CHECK(ctx != nullptr);
        inflate_destroy(ctx);
    }

    SECTION("fails if the number of window bits is out of the allowed range") {
        inflate_ctx* ctx = nullptr;
        inflate_opts opts = {};
        opts.window_bits = 7;
        int r = inflate_create(&ctx, &opts, dummyOutputCallback, nullptr);
        CHECK(r < 0);
        CHECK(ctx == nullptr);
        opts.window_bits = 16;
        r = inflate_create(&ctx, &opts, dummyOutputCallback, nullptr);
        CHECK(r < 0);
        CHECK(ctx == nullptr);
    }

    SECTION("fails if the output callback is NULL") {
        inflate_ctx* ctx = nullptr;
        int r = inflate_create(&ctx, nullptr, nullptr, nullptr);
        CHECK(r < 0);
        CHECK(ctx == nullptr);
    }
}

TEST_CASE("inflate_input()") {
    Inflate infl;

    SECTION("decompresses raw Deflate data") {
        auto d = genCompressedData();
        int r = infl.input(d.data(), d.size(), false);
        CHECK(r == INFLATE_DONE);
        CHECK(infl.output() == d);
    }
}

#pragma once

#include <optional>
#include <cstddef>

namespace particle::constrained {

const unsigned MAX_REQUEST_TYPE_OR_RESULT_CODE = 127;
const unsigned MAX_REQUEST_ID = 8191;
const unsigned MAX_BLOCK_NUMBER = 63;

const size_t MAX_FRAME_HEADER_SIZE = 4;

enum class FrameType: unsigned {
    REQUEST = 0,
    REQUEST_NO_RESPONSE = 1,
    REQUEST_RESPONSE_BLOCK = 2,
    RESPONSE = 3
};

class FrameHeader {
public:
    FrameHeader() :
            reqTypeOrResult_(0) {
    }

    FrameHeader& frameType(FrameType type) {
        frameType_ = type;
        return *this;
    }

    FrameType frameType() const {
        return frameType_.value_or(FrameType());
    }

    bool hasFrameType() const {
        return frameType_.has_value();
    }

    FrameHeader& requestTypeOrResultCode(unsigned val) {
        reqTypeOrResult_ = val;
        return *this;
    }

    unsigned requestTypeOrResultCode() const {
        return reqTypeOrResult_;
    }

    FrameHeader& requestId(unsigned id) {
        reqId_ = id;
        return *this;
    }

    unsigned requestId() const {
        return reqId_.value_or(0);
    }

    bool hasRequestId() const {
        return reqId_.has_value();
    }

    FrameHeader& blockNumber(unsigned num) {
        blockNum_ = num;
        return *this;
    }

    unsigned blockNumber() const {
        return blockNum_.value_or(0);
    }

    bool hasBlockNumber() const {
        return blockNum_.has_value();
    }

    FrameHeader& more(bool more) {
        more_ = more;
        return *this;
    }

    bool more() const {
        return more_.value_or(false);
    }

    bool hasMore() const {
        return more_.has_value();
    }

private:
    std::optional<FrameType> frameType_;
    std::optional<unsigned> reqId_;
    std::optional<unsigned> blockNum_;
    std::optional<bool> more_;
    unsigned reqTypeOrResult_;
};

int encodeFrameHeader(char* data, size_t size, const FrameHeader& header);
int decodeFrameHeader(const char* data, size_t size, FrameHeader& header);

} // namespace particle::constrained

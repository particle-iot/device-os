#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cassert>

#include <spark_wiring_error.h>

#include <endian_util.h>

#include "frame_codec.h"

namespace particle::constrained {

namespace {

const uint32_t REQUEST_ID_FLAG = 0x80000000u;
const uint32_t BLOCK_NUMBER_FLAG = 0x00800000u;
const uint32_t MORE_FLAG = 0x00000040u;

} // namespace

int encodeFrameHeader(char* data, size_t size, const FrameHeader& h) {
    if (h.requestTypeOrResultCode() > MAX_REQUEST_TYPE_OR_RESULT_CODE) {
        return Error::INVALID_ARGUMENT;
    }
    uint32_t v = h.requestTypeOrResultCode() << 24;
    size_t n = 1;
    if (h.hasRequestId()) {
        if (h.requestId() > MAX_REQUEST_ID || !h.hasFrameType()) {
            return Error::INVALID_ARGUMENT;
        }
        auto frameType = static_cast<unsigned>(h.frameType());
        assert(frameType <= 3);
        v |= REQUEST_ID_FLAG | (frameType << 21) | (h.requestId() << 8);
        n += 2;
        if (h.hasBlockNumber()) {
            if (h.blockNumber() > MAX_BLOCK_NUMBER || !h.hasMore()) {
                return Error::INVALID_ARGUMENT;
            }
            v |= h.blockNumber();
            if (h.more()) {
                v |= MORE_FLAG;
            }
            ++n;
        } else if (h.frameType() == FrameType::REQUEST_RESPONSE_BLOCK) {
            // Frame of this type must have a block number field
            return Error::INVALID_ARGUMENT;
        }
    }
    v = nativeToBigEndian(v);
    std::memcpy(data, &v, std::min(n, size));
    return n;
}

int decodeFrameHeader(const char* data, size_t size, FrameHeader& header) {
    if (!size) {
        return Error::NOT_ENOUGH_DATA;
    }
    uint32_t v = 0;
    std::memcpy(&v, data, std::min(size, sizeof(v)));
    v = bigEndianToNative(v);
    size_t n = 1;
    FrameHeader h;
    h.requestTypeOrResultCode((v & 0x7f000000u) >> 24);
    if (v & REQUEST_ID_FLAG) {
        if (size < 3) {
            return Error::NOT_ENOUGH_DATA;
        }
        auto frameType = (v & 0x00600000u) >> 21;
        h.frameType(static_cast<FrameType>(frameType));
        h.requestId((v & 0x001fff00u) >> 8);
        n += 2;
        if (v & BLOCK_NUMBER_FLAG) {
            if (size < 4) {
                return Error::NOT_ENOUGH_DATA;
            }
            h.blockNumber(v & 0x3f);
            h.more(v & MORE_FLAG);
            ++n;
        } else if (h.frameType() == FrameType::REQUEST_RESPONSE_BLOCK) {
            // Frame of this type must have a block number field
            return Error::BAD_DATA;
        }
    }
    header = std::move(h);
    return n;
}

} // namespace particle::constrained

#include <cstring>
#include <cassert>

#include <pb_encode.h>
#include <pb_decode.h>

#include <spark_wiring_error.h>

#include "protobuf.h"

namespace particle::util {

int encodeProtobuf(Buffer& buf, const void* msg, const pb_msgdesc_t* desc) {
    pb_ostream_t strm = {};
    strm.state = &buf;
    strm.max_size = SIZE_MAX;
    strm.callback = [](pb_ostream_t* strm, const uint8_t* data, size_t size) {
        auto buf = static_cast<Buffer*>(strm->state);
        if (!buf->resize(buf->size() + size)) {
            return false;
        }
        std::memcpy(buf->data() + buf->size() - size, data, size);
        return true;
    };
    if (!pb_encode(&strm, desc, msg)) {
        return Error::ENCODING_FAILED;
    }
    return strm.bytes_written;
}

int decodeProtobuf(const Buffer& buf, void* msg, const pb_msgdesc_t* desc) {
    pb_istream_t strm = {};
    strm.state = const_cast<Buffer*>(&buf);
    strm.bytes_left = buf.size();
    strm.callback = [](pb_istream_t* strm, uint8_t* data, size_t size) {
        auto buf = static_cast<const Buffer*>(strm->state);
        assert(strm->bytes_left <= buf->size() && size <= strm->bytes_left);
        std::memcpy(data, buf->data() + buf->size() - strm->bytes_left, size);
        return true;
    };
    if (!pb_decode(&strm, desc, msg)) {
        return Error::BAD_DATA;
    }
    assert(strm.bytes_left <= buf.size());
    return buf.size() - strm.bytes_left;
}

} // namespace particle::util

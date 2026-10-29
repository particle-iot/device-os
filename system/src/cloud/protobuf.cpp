#include "protobuf.h"
#include "coap_util.h"
#include "nanopb_misc.h"
#include "check.h"

#include <pb_decode.h>

#include <limits>
#include <memory>
#include <cstring>
#include <cstdint>

#define PB_CLOUD(_name) particle_cloud_##_name

namespace particle::system {

namespace {

enum FindFieldResult {
    FIND_FIELD_NOT_FOUND = 1,
    FIND_FIELD_READ_MORE = 2
};

int findField(pb_istream_t& stream, uint32_t fieldTag, pb_wire_type_t fieldType, bool hasMore) {
    for (;;) {
        auto type = pb_wire_type_t();
        uint32_t tag = 0;
        bool eof = false;
        if (!pb_decode_tag(&stream, &type, &tag, &eof)) {
            if (hasMore) {
                return FIND_FIELD_READ_MORE;
            }
            if (eof) {
                return FIND_FIELD_NOT_FOUND;
            }
            return SYSTEM_ERROR_BAD_DATA;
        }
        if (tag == fieldTag) {
            if (type != fieldType) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            return 0;
        }
        if (!pb_skip_field(&stream, type)) {
            if (hasMore) {
                return FIND_FIELD_READ_MORE;
            }
            return SYSTEM_ERROR_BAD_DATA;
        }
    }
}

int findVarintField(uint32_t& val, uint32_t tag, const char* buf, size_t size, bool hasMore) {
    pb_istream_t stream = {};
    if (!pb_istream_from_buffer_ex(&stream, (const pb_byte_t*)buf, size, nullptr /* reserved */)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    int r = findField(stream, tag, PB_WT_VARINT, hasMore);
    if (r < 0 || r == FIND_FIELD_NOT_FOUND || r == FIND_FIELD_READ_MORE) {
        return r;
    }
    if (!pb_decode_varint32(&stream, &val)) {
        if (hasMore) {
            return FIND_FIELD_READ_MORE;
        }
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int findSignedVarintField(int& val, uint32_t tag, const char* buf, size_t size, bool hasMore) {
    pb_istream_t stream = {};
    if (!pb_istream_from_buffer_ex(&stream, (const pb_byte_t*)buf, size, nullptr /* reserved */)) {
        return SYSTEM_ERROR_INTERNAL;
    }
    int r = findField(stream, tag, PB_WT_VARINT, hasMore);
    if (r < 0 || r == FIND_FIELD_NOT_FOUND || r == FIND_FIELD_READ_MORE) {
        return r;
    }
    int64_t v = 0;
    if (!pb_decode_svarint(&stream, &v)) {
        if (hasMore) {
            return FIND_FIELD_READ_MORE;
        }
        return SYSTEM_ERROR_BAD_DATA;
    }
    if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    val = v;
    return 0;
}

} // namespace

int sendProtobufRequest(const char* path, coap_method method, const PB_CLOUD(Request)& pbReq, void* arg,
        coap_response_callback respCb, coap_ack_callback ackCb, coap_error_callback errorCb) {
    CoapMessagePtr coapReq;
    CHECK(coap_begin_request(&coapReq, path, method, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));

    pb_ostream_t pbOut = {};
    CHECK(pb_ostream_from_coap_message(&pbOut, coapReq.get(), nullptr /* reserved */));
    if (!pb_encode(&pbOut, &PB_CLOUD(Request_msg), &pbReq)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_request(coapReq.get(), respCb, ackCb, errorCb, arg, nullptr /* reserved */));
    coapReq.release(); // CoAP API now owns the message
    return 0;
}

int decodeProtobufRequest(coap_message* msg, PB_CLOUD(Request)& pbReq) {
    pb_istream_t pbIn = {};
    CHECK(pb_istream_from_coap_message(&pbIn, msg, nullptr /* reserved */));
    if (!pb_decode(&pbIn, &PB_CLOUD(Request_msg), &pbReq)) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int getProtobufRequestType(coap_message* msg, int& type) {
    char buf[128] = {};
    int r = coap_peek_block(msg, buf, sizeof(buf), nullptr /* reserved */);
    if (r < 0) {
        if (r == SYSTEM_ERROR_END_OF_STREAM) {
            // If the request is empty, the `type` field is implicitly 0 which is not a valid
            // request type (see cloud.proto)
            return SYSTEM_ERROR_BAD_DATA;
        }
        return r;
    }
    size_t bytesRead = r;
    size_t bytesAvail = CHECK(coap_peek_block(msg, nullptr /* data */, COAP_BLOCK_SIZE, nullptr /* reserved */));
    bool hasMore = bytesRead < bytesAvail;

    uint32_t val = 0;
    r = CHECK(findVarintField(val, PB_CLOUD(Request_type_tag), buf, bytesRead, hasMore));
    // Normally, the first few bytes of the request always contain the `type` field as the server
    // seems to encode fields in the order of their tag numbers. The Protobuf spec doesn't guarantee
    // that though, hence the fallback
    if (r == FIND_FIELD_READ_MORE) {
        std::unique_ptr<char[]> buf(new(std::nothrow) char[bytesAvail]);
        if (!buf) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        CHECK(coap_peek_block(msg, buf.get(), bytesAvail, nullptr /* reserved */));
        r = CHECK(findVarintField(val, PB_CLOUD(Request_type_tag), buf.get(), bytesAvail, false /* hasMore */));
    }
    if (r == FIND_FIELD_NOT_FOUND) {
        // The `type` field is implicitly 0 which is not a valid request type
        return SYSTEM_ERROR_BAD_DATA;
    }
    if (val > (unsigned)std::numeric_limits<int>::max()) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    type = val;
    return 0;
}

int sendProtobufResponse(int reqId, coap_status status, const PB_CLOUD(Response)& pbResp, void* arg,
        coap_ack_callback ackCb, coap_error_callback errorCb) {
    CoapMessagePtr coapResp;
    CHECK(coap_begin_response(&coapResp, status, reqId, 0 /* flags */, nullptr /* reserved */));

    pb_ostream_t pbOut = {};
    CHECK(pb_ostream_from_coap_message(&pbOut, coapResp.get(), nullptr /* reserved */));
    if (!pb_encode(&pbOut, &PB_CLOUD(Response_msg), &pbResp)) {
        return SYSTEM_ERROR_ENCODING_FAILED;
    }
    CHECK(coap_end_response(coapResp.get(), ackCb, errorCb, arg, nullptr /* reserved */));
    coapResp.release(); // CoAP API now owns the message
    return 0;
}

int sendEmptyProtobufResponse(int reqId, coap_status status, int result, const char* msg, void* arg,
        coap_ack_callback ackCb, coap_error_callback errorCb) {
    PB_CLOUD(Response) pbResp = {};
    pbResp.result = result;
    if (msg) {
        pbResp.message.arg = const_cast<char*>(msg);
        pbResp.message.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
            auto msg = (const char*)*arg;
            if (!pb_encode_tag_for_field(stream, field) ||
                    !pb_encode_string(stream, (const uint8_t*)msg, std::strlen(msg))) {
                return false;
            }
            return true;
        };
    }
    CHECK(sendProtobufResponse(reqId, status, pbResp, arg, ackCb, errorCb));
    return 0;
}

int decodeProtobufResponse(coap_message* msg, PB_CLOUD(Response)& pbResp) {
    pb_istream_t pbIn = {};
    CHECK(pb_istream_from_coap_message(&pbIn, msg, nullptr /* reserved */));
    if (!pb_decode(&pbIn, &PB_CLOUD(Response_msg), &pbResp)) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int getProtobufResponseResult(coap_message* msg, int status, int& result) {
    size_t bytesRead = 0;
    char buf[128] = {};
    int r = coap_peek_block(msg, buf, sizeof(buf), nullptr /* reserved */);
    if (r >= 0) {
        bytesRead = r;
    } else if (r != SYSTEM_ERROR_END_OF_STREAM) { // Empty payload implies that `result` is 0
        return r;
    }

    int val = 0;
    if (bytesRead > 0) {
        size_t bytesAvail = CHECK(coap_peek_block(msg, nullptr /* data */, COAP_BLOCK_SIZE, nullptr /* reserved */));
        bool hasMore = bytesRead < bytesAvail;

        int r = CHECK(findSignedVarintField(val, PB_CLOUD(Response_result_tag), buf, bytesRead, hasMore));
        if (r == FIND_FIELD_READ_MORE) {
            std::unique_ptr<char[]> buf(new(std::nothrow) char[bytesAvail]);
            if (!buf) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            CHECK(coap_peek_block(msg, buf.get(), bytesAvail, nullptr /* reserved */));
            CHECK(findSignedVarintField(val, PB_CLOUD(Response_result_tag), buf.get(), bytesAvail, false /* hasMore */));
        }
    }

    int cls = COAP_CODE_CLASS(status);
    if (val == PB_CLOUD(Response_Result_OK) && cls != 2 /* Success 2.xx */) {
        // Protobuf payload (or the lack of it) indicates success but the CoAP response code doesn't
        LOG(WARN, "Unexpected response result %d for CoAP code %d.%02d", val, cls, (int)COAP_CODE_DETAIL(status));
        val = PB_CLOUD(Response_Result_ERROR);
    }
    result = val;
    return 0;
}

int checkProtobufResponseResult(coap_message* msg, int status) {
    int cls = COAP_CODE_CLASS(status);
    int result = 0;
    int r = getProtobufResponseResult(msg, status, result);
    if (r < 0) {
        LOG(ERROR, "Error parsing Protobuf response: %d", r);
        if (cls != 2) { // Success 2.xx
            LOG(ERROR, "Cloud request failed, CoAP code: %d.%02d", cls, (int)COAP_CODE_DETAIL(status));
            return SYSTEM_ERROR_SERVER_REQUEST_FAILED;
        }
        return r;
    }
    if (result != PB_CLOUD(Response_Result_OK) || cls != 2) {
        LOG(ERROR, "Cloud request failed, CoAP code: %d.%02d, result: %d", cls, (int)COAP_CODE_DETAIL(status), result);
        return SYSTEM_ERROR_SERVER_REQUEST_FAILED;
    }
    return 0;
}

} // namespace particle::system

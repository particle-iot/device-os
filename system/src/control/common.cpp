#include "common.h"

#if SYSTEM_CONTROL_ENABLED

#include "nanopb_misc.h"
#include "spark_wiring_platform.h"

using namespace particle;

namespace particle {
namespace control {
namespace common {

int encodeReplyMessage(ctrl_request* req, const pb_field_t* fields, const void* src) {
    pb_ostream_t* stream = nullptr;
    size_t sz = 0;
    int ret = SYSTEM_ERROR_UNKNOWN;

    // Calculate size
    bool res = pb_get_encoded_size(&sz, fields, src);
    if (!res) {
        goto cleanup;
    }

    // Allocate reply data
    ret = system_ctrl_alloc_reply_data(req, sz, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        goto cleanup;
    }
    ret = SYSTEM_ERROR_UNKNOWN;

    // Allocate ostream
    stream = pb_ostream_init(nullptr);
    if (stream == nullptr) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto cleanup;
    }

    res = pb_ostream_from_buffer_ex(stream, (pb_byte_t*)req->reply_data, req->reply_size, nullptr);
    if (!res) {
        goto cleanup;
    }

    res = pb_encode(stream, fields, src);
    if (res) {
        ret = SYSTEM_ERROR_NONE;
    }

cleanup:
    if (stream != nullptr) {
        pb_ostream_free(stream, nullptr);
    }
    if (ret != SYSTEM_ERROR_NONE) {
        system_ctrl_alloc_reply_data(req, 0, nullptr);
    }
    return ret;
}

int decodeRequestMessage(ctrl_request* req, const pb_field_t* fields, void* dst) {
    pb_istream_t* stream = nullptr;
    int ret = SYSTEM_ERROR_UNKNOWN;
    bool res = false;

    if (req->request_size == 0) {
        // Nothing to decode
        ret = SYSTEM_ERROR_BAD_DATA;
        goto cleanup;
    }

    stream = pb_istream_init(nullptr);
    if (stream == nullptr) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto cleanup;
    }

    res = pb_istream_from_buffer_ex(stream, (const pb_byte_t*)req->request_data, req->request_size, nullptr);
    if (!res) {
        goto cleanup;
    }

    res = pb_decode_noinit(stream, fields, dst);
    if (res) {
        ret = SYSTEM_ERROR_NONE;
    } else {
        ret = SYSTEM_ERROR_BAD_DATA;
    }

cleanup:
    if (stream != nullptr) {
        pb_istream_free(stream, nullptr);
    }
    return ret;
}

int protoIpFromHal(particle_ctrl_IPAddress* ip, const HAL_IPAddress* sip) {
#if HAL_IPv6
    if (sip->v == 4) {
        ip->protocol = particle_ctrl_IPAddress_Protocol_IPv4;
        ip->address.size = sizeof(sip->ipv4);
        memcpy(ip->address.bytes, &sip->ipv4, sizeof(sip->ipv4));
    } else if (sip->v == 6) {
        ip->protocol = particle_ctrl_IPAddress_Protocol_IPv6;
        ip->address.size = sizeof(sip->ipv6);
        memcpy(ip->address.bytes, &sip->ipv6, sizeof(sip->ipv6));
    } else {
        // Do not encode
        return 1;
    }
#else
    ip->protocol = particle_ctrl_IPAddress_Protocol_IPv4;
    ip->address.size = sizeof(sip->ipv4);
    memcpy(ip->address.bytes, &sip->ipv4, sizeof(sip->ipv4));
#endif // HAL_IPv6
    return 0;
}

int halIpFromProto(particle_ctrl_IPAddress* ip, HAL_IPAddress* halip) {
    if (ip->protocol == particle_ctrl_IPAddress_Protocol_IPv4) {
#if HAL_IPv6
        halip->v = 4;
#endif
        if (ip->address.size == sizeof(halip->ipv4)) {
            memcpy(&halip->ipv4, ip->address.bytes, sizeof(halip->ipv4));
        }
    }
#if HAL_IPv6
    else if (ip->protocol == particle_ctrl_IPAddress_Protocol_IPv6) {
        halip->v = 6;
        if (ip->address.size == sizeof(halip->ipv6)) {
            memcpy(&halip->ipv6, ip->address.bytes, sizeof(halip->ipv6));
        }
    }
#endif
    return 0;
}

} } } /* namespace particle::control::common */

#endif /* #if SYSTEM_CONTROL_ENABLED */

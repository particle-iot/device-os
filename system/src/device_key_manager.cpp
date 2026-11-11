#include "cloud/protobuf.h"
#include "device_key_manager.h"
#include "system_cloud_internal.h"
#include "system_mode.h"
#include "eckeygen.h"
#include "mbedtls_util.h"
#include "file_util.h"
#include "random.h"
#include "check.h"

#include <pb_encode.h>

#include <algorithm>
#include <memory>
#include <cstring>

// This is a core feature so keep it buildable on all platforms including GCC/newhal, even if not
// fully supported
#if (PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL) || defined(UNIT_TEST)
#include "dct.h"
#define HAS_DCT 1
#else
#define HAS_DCT 0
#endif

#define PB_CLOUD(_name) particle_cloud_##_name

namespace particle::system {

namespace {

const auto CANDIDATE_KEY_FILE = "/sys/new_device_key";

#if HAS_DCT

const size_t MAX_DEVICE_PRIVATE_KEY_SIZE = DCT_ALT_DEVICE_PRIVATE_KEY_SIZE; // 192
const size_t MAX_DEVICE_PUBLIC_KEY_SIZE = DCT_ALT_DEVICE_PUBLIC_KEY_SIZE; // 128

// Saves the device key to its default storage
int saveNewDeviceKey(const char* data, size_t size) {
    if (size > MAX_DEVICE_PRIVATE_KEY_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    int r = dct_write_app_data(data, DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, size);
    if (r != 0) { // `r` is not a system_error_t
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

#else // !HAS_DCT

const size_t MAX_DEVICE_PRIVATE_KEY_SIZE = 1024;
const size_t MAX_DEVICE_PUBLIC_KEY_SIZE = 1024;

int saveNewDeviceKey(const char* data, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
}

#endif // !HAS_DCT

} // namespace

int EcpPrivateKey::getPublicKey(EcpPublicKey& pub) const {
    if (!isValid()) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const auto bufSize = MAX_DEVICE_PUBLIC_KEY_SIZE; // FIXME: This is a generic key class
    std::unique_ptr<char[]> pubData(new(std::nothrow) char[bufSize]);
    if (!pubData) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = extract_public_ec_key_length((uint8_t*)pubData.get(), bufSize, (const uint8_t*)this->data(), this->size());
    if (r < 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t pubSize = r;
    // extract_public_ec_key_length() writes the key to the end of the buffer
    std::memmove(pubData.get(), pubData.get() + bufSize - pubSize, pubSize);
    pub = EcpPublicKey(std::move(pubData), pubSize);
    return pubSize;
}

int DeviceKeyManager::setCandidateKey(const EcpPrivateKey& key) {
    if (!key.isValid()) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    fs::File f;
    char tmpPath[TEMP_PATH_LEN + 1] = {};
    CHECK(createTempFile(f, tmpPath, sizeof(tmpPath), LFS_O_WRONLY));
    CHECK(f.write(key.data(), key.size()));
    CHECK(f.close());

    CHECK(fs::rename(tmpPath, CANDIDATE_KEY_FILE));
    hasCandidateKey_ = true;
    return 0;
}

int DeviceKeyManager::getCandidateKey(EcpPrivateKey& key) {
    CHECK(loadCandidateKey(key));
    return 0;
}

int DeviceKeyManager::acceptCandidateKey() {
    EcpPrivateKey k;
    CHECK(loadCandidateKey(k));
    CHECK(saveNewDeviceKey(k.data(), k.size()));
    CHECK(rmrf(CANDIDATE_KEY_FILE));
    hasCandidateKey_ = false;
    return 0;
}

int DeviceKeyManager::clearCandidateKey() {
    if (hasCandidateKey_.has_value() && !hasCandidateKey_.value()) {
        return 0;
    }
    CHECK(rmrf(CANDIDATE_KEY_FILE));
    hasCandidateKey_ = false;
    return 0;
}

int DeviceKeyManager::generateKey(EcpPrivateKey& key) {
    const size_t bufSize = MAX_DEVICE_PRIVATE_KEY_SIZE;
    std::unique_ptr<char[]> privData(new(std::nothrow) char[bufSize]);
    if (!privData) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = gen_ec_key((uint8_t*)privData.get(), bufSize, mbedtls_default_rng, nullptr /* p_rng */);
    if (r <= 0) {
        return SYSTEM_ERROR_CRYPTO;
    }
    size_t privSize = r;
    key = EcpPrivateKey(std::move(privData), privSize);
    return 0;
}

DeviceKeyManager& DeviceKeyManager::instance() {
    static DeviceKeyManager mgr;
    return mgr;
}

int DeviceKeyManager::loadCandidateKey(EcpPrivateKey& key) {
    if (hasCandidateKey_.has_value() && !hasCandidateKey_.value()) {
        return SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND;
    }
    fs::File f;
    int r = f.open(CANDIDATE_KEY_FILE, LFS_O_RDONLY);
    if (r < 0) {
        if (r == SYSTEM_ERROR_FILESYSTEM_NOENT) {
            hasCandidateKey_ = false;
            return SYSTEM_ERROR_CRYPTO_KEY_NOT_FOUND;
        }
        return r;
    }

    size_t privSize = CHECK(f.size());
    if (!privSize) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    if (privSize > MAX_DEVICE_PRIVATE_KEY_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    std::unique_ptr<char[]> privData(new(std::nothrow) char[privSize]);
    if (!privData) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    size_t bytesRead = CHECK(f.read(privData.get(), privSize));
    if (bytesRead != privSize) {
        return SYSTEM_ERROR_BAD_DATA;
    }

    key = EcpPrivateKey(std::move(privData), privSize);
    hasCandidateKey_ = true;
    return 0;
}

int DeviceKeyChangeRequestHandler::handle(CoapMessagePtr& req) {
    int r = handlePrepareRequest(req);
    if (r < 0) {
        LOG(ERROR, "Error handling PrepareDeviceKeyChangeRequest: %d", r);
        return r;
    }
    return Result::RUNNING;
}

void DeviceKeyChangeRequestHandler::reset() {
    privKey_ = EcpPrivateKey();
}

int DeviceKeyChangeRequestHandler::handlePrepareRequest(CoapMessagePtr& req) {
    Random::genSecure(keyId_, sizeof(keyId_));

    // Reply with the key ID to the server
    PB_CLOUD(Response) pbResp = {};
    pbResp.which_data = PB_CLOUD(Response_config_prepare_device_key_change_tag);
    auto& pbInnerResp = pbResp.data.config_prepare_device_key_change;

    static_assert(sizeof(pbInnerResp.key_id.bytes) == sizeof(keyId_));
    std::memcpy(pbInnerResp.key_id.bytes, keyId_, sizeof(keyId_));
    pbInnerResp.key_id.size = sizeof(keyId_);

    CHECK(sendProtobufResponse(COAP_STATUS_CHANGED, pbResp, this, [](int reqId, void* arg) { // ack_cb
        auto self = static_cast<DeviceKeyChangeRequestHandler*>(arg);
        int r = self->sendFinalizeRequest();
        if (r < 0) {
            LOG(ERROR, "Error sending FinalizeDeviceKeyChangeRequest: %d", r);
            self->notifyComplete(r);
        }
        return 0;
    }, [](int err, int reqId, void* arg) { // error_cb
        auto self = static_cast<DeviceKeyChangeRequestHandler*>(arg);
        LOG(ERROR, "Error sending PrepareDeviceKeyChangeResponse: %d", err);
        self->notifyComplete(err);
    }));
    return 0;
}

int DeviceKeyChangeRequestHandler::sendFinalizeRequest() {
    // Generate a new device key
    CHECK(DeviceKeyManager::instance().generateKey(privKey_));
    EcpPublicKey pubKey;
    CHECK(privKey_.getPublicKey(pubKey));

    // Send the public key to the server
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_CONFIG_FINALIZE_DEVICE_KEY_CHANGE);
    pbReq.which_data = PB_CLOUD(Request_config_finalize_device_key_change_tag);
    auto& pbInnerReq = pbReq.data.config_finalize_device_key_change;

    static_assert(sizeof(pbInnerReq.key_id.bytes) == sizeof(keyId_));
    std::memcpy(pbInnerReq.key_id.bytes, keyId_, sizeof(keyId_));
    pbInnerReq.key_id.size = sizeof(keyId_);

    pbInnerReq.public_key.arg = &pubKey;
    pbInnerReq.public_key.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        auto pubKey = (const EcpPublicKey*)*arg;
        if (!pb_encode_tag_for_field(stream, field) ||
                !pb_encode_string(stream, (const uint8_t*)pubKey->data(), pubKey->size())) {
            return false;
        }
        return true;
    };

    CHECK(sendProtobufRequest(PROTOBUF_REQUEST_URI, PROTOBUF_REQUEST_METHOD, pbReq, this,
            [](coap_message* msg, int status, int reqId, void* arg) { // resp_cb
        CoapMessagePtr coapResp(msg);
        auto self = static_cast<DeviceKeyChangeRequestHandler*>(arg);
        int r = self->handleFinalizeResponse(status, coapResp);
        if (r < 0) {
            LOG(ERROR, "Error handling FinalizeDeviceKeyChangeResponse: %d", r);
            self->notifyComplete(r);
        }
        return 0;
    }, nullptr /* ack_cb */, [](int err, int reqId, void* arg) { // error_cb
        auto self = static_cast<DeviceKeyChangeRequestHandler*>(arg);
        LOG(ERROR, "Error sending FinalizeDeviceKeyChangeRequest: %d", err);
        self->notifyComplete(err);
    }));
    return 0;
}

int DeviceKeyChangeRequestHandler::handleFinalizeResponse(int status, CoapMessagePtr& resp) {
    CHECK(checkProtobufResponseResult(resp.get(), status));
    // Save the new key as a candidate key
    int r = DeviceKeyManager::instance().setCandidateKey(privKey_);
    if (r < 0) {
        LOG(ERROR, "Error saving candidate device key: %d", r);
        return r;
    }

    LOG(INFO, "Saved candidate device key");
    notifyComplete();

    // Discard the DTLS session and reset to reload the device key
    clearSessionData();
    system_reset(SYSTEM_RESET_MODE_NORMAL, RESET_REASON_CONFIG_UPDATE, 0 /* value */,
            SYSTEM_RESET_FLAG_NO_WAIT, nullptr /* reserved */);
    return 0; // Unreachable
}

} // namespace particle::system

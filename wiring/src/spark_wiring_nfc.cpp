#include "spark_wiring_nfc.h"

#if Wiring_NFC

#include "nfc_hal.h"
#include "system_error.h"
#include "logging.h"
LOG_SOURCE_CATEGORY("nfc")

namespace particle {

size_t NdefRecord::getEncodedSize() {
    // Record header: TNF + typeLength + payloadLength + idLength + typeData + idData
    size_t length = 1 + 1 + 4 + (id_.size() ? 1 : 0) + type_.size() + id_.size();
    // Record payload
    length += payload_.size();

    return length;
}

size_t NdefRecord::getEncodedData(void *buf, size_t size) {
    if (buf == nullptr || size == 0) {
        return 0;
    }

    size_t length = 0;
    uint8_t *pdata = static_cast<uint8_t *>(buf);

    memset(buf, 0, size);
    pdata[length++] = flagsAndTnf_.bytedata;
    pdata[length++] = type_.size();
    pdata[length + 0] = (payload_.size() >> 24) & 0xFF;
    pdata[length + 1] = (payload_.size() >> 16) & 0xFF;
    pdata[length + 2] = (payload_.size() >> 8) & 0xFF;
    pdata[length + 3] = payload_.size() & 0xFF;
    length += 4;
    if (id_.size()) {
        pdata[length++] = id_.size();
    }
    memcpy(&pdata[length], type_.data(), type_.size());
    length += type_.size();
    memcpy(&pdata[length], payload_.data(), payload_.size());
    length += payload_.size();
    if (id_.size()) {
        memcpy(&pdata[length], id_.data(), id_.size());
        length += id_.size();
    }

    return length;
}

TextRecord::TextRecord(const char *text,const char *encoding) {
    if (text == nullptr || encoding == nullptr) {
        return;
    }

    setTnf(NdefRecord::TNF_WELL_KNOWN);

    uint8_t ndefType[1] = {NdefRecord::NDEF_TYPE_TEXT};
    setType(ndefType, sizeof(ndefType));

    // Payload: encoding length + encoding data + text data
    uint8_t payload[1 + strlen(encoding) + strlen(text)];
    uint8_t index = 0;
    payload[index++] = strlen(encoding);
    memcpy(&payload[index], encoding, strlen(encoding));
    index += strlen(encoding);
    memcpy(&payload[index], text, strlen(text));
    setPayload(payload, sizeof(payload));
}

UriRecord::UriRecord(const char *uri, UriRecord::NfcUriType type) {
    if (uri == nullptr) {
        return;
    }

    setTnf(NdefRecord::TNF_WELL_KNOWN);

    uint8_t ndefType[1] = {NdefRecord::NDEF_TYPE_URI};
    setType(ndefType, sizeof(ndefType));

    // Payload: uri type + uri
    uint8_t payload[1 + strlen(uri)];
    payload[0] = type;
    memcpy(&payload[1], uri, strlen(uri));
    setPayload(payload, sizeof(payload));
}

LauchappRecord::LauchappRecord(const char *androidPackageName) {
    if (androidPackageName == nullptr) {
        return;
    }

    /* Record Payload Type for NFC NDEF Android Application Record */
    static constexpr const char ndef_android_launchapp_rec_type[] = "android.com:pkg";

    std::shared_ptr<NdefRecord> ndefr(new NdefRecord());
    setTnf(NdefRecord::TNF_EXTERNAL_TYPE);

    setType(ndef_android_launchapp_rec_type, strlen(ndef_android_launchapp_rec_type));
    setPayload(androidPackageName, strlen(androidPackageName));
}

size_t NdefMessage::getEncodedSize() {
    size_t length = 0;
    for (auto record : records_) {
        if (record.get()) {
            length += record->getEncodedSize();
        }
    }

    return length;
}

size_t NdefMessage::getEncodedData(void *data, size_t numBytes) {
    if (data == nullptr || numBytes == 0) {
        return 0;
    }

    size_t copyLength = 0;
    size_t tmpRecordLength = 0;

    for (auto record : records_) {
        if (record.get()) {
            tmpRecordLength = record->getEncodedSize();
            if (copyLength + tmpRecordLength > numBytes) {
                break;
            } else {
                uint8_t *pdata = static_cast<uint8_t *>(data);
                record->getEncodedData(&pdata[copyLength], tmpRecordLength);
                copyLength += tmpRecordLength;
            }
        }
    }

    return copyLength;
}

int NdefMessage::addTextRecord(const char *text, const char *encoding) {
    if (text == nullptr || encoding == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<NdefRecord> ndefr(new TextRecord(text, encoding));
    records_.append(ndefr);
    arrangeRecords();

    return records_.size() - 1;
}

int NdefMessage::addUriRecord(const char *uri, UriRecord::NfcUriType type) {
    if (uri == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<NdefRecord> ndefr(new UriRecord(uri, type));
    records_.append(ndefr);
    arrangeRecords();

    return records_.size() - 1;
}

int NdefMessage::addLauchAppRecord(const char *androidPackageName) {
    if (androidPackageName == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    std::shared_ptr<NdefRecord> ndefr(new LauchappRecord(androidPackageName));
    records_.append(ndefr);
    arrangeRecords();

    return records_.size() - 1;
}

void NdefMessage::arrangeRecords(void) {
    if (records_.size()) {
        for (int i = 0; i < records_.size(); i++) {
            records_[i]->setFirst(false);
            records_[i]->setLast(false);
        }
        records_[0]->setFirst(true);
        records_[records_.size() - 1]->setLast(true);
    }
}

int NfcTagType2::start(nfc_event_callback_t cb) {
    if (!enable_) {
        enable_ = true;
        hal_nfc_type2_init();
    }

    size_t msgLength = ndefMessage_.getEncodedSize();
    std::unique_ptr<uint8_t[]> msgBuf(new uint8_t[msgLength]);
    ndefMessage_.getEncodedData(msgBuf.get(), msgLength);
    encode_.clear();
    encode_.append(msgBuf.get(), msgLength);

    hal_nfc_type2_set_payload(encode_.data(), encode_.size());
    hal_nfc_type2_set_callback(cb);
    hal_nfc_type2_start_emulation();

    return 0;
}

int NfcTagType2::stop() {
    encode_.clear();
    hal_nfc_type2_stop_emulation();

    return 0;
}

NfcTagType2 NFC;

} // namespace particle

#endif

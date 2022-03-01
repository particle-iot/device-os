#include "spark_wiring_nfc.h"

#if Wiring_NFC

#include "nfc_hal.h"
#include "system_error.h"
#include "logging.h"
LOG_SOURCE_CATEGORY("nfc");

namespace particle {

size_t Record::getEncodedData(Vector<uint8_t>& vector) const {
    vector.append(flagsAndTnf_.bytedata);
    vector.append(type_.size());
    vector.append((payload_.size() >> 24) & 0xFF);
    vector.append((payload_.size() >> 16) & 0xFF);
    vector.append((payload_.size() >> 8) & 0xFF);
    vector.append(payload_.size() & 0xFF);
    if (id_.size()) {
        vector.append(id_.size());
    }
    vector.append(type_);
    vector.append(payload_);
    vector.append(id_);

    return getEncodedSize();
}

TextRecord::TextRecord(const char* text, const char* encoding) {
    if (text == nullptr || encoding == nullptr) {
        return;
    }

    setTnf(Tnf::TNF_WELL_KNOWN);

    uint8_t ndefType = static_cast<uint8_t>(NdefType::NDEF_TYPE_TEXT);
    setType(&ndefType, sizeof(ndefType));

    // Payload: encoding length + encoding data + text data
    Vector<uint8_t> payload;
    payload.append(static_cast<uint8_t>(strlen(encoding)));
    payload.append(reinterpret_cast<const uint8_t*>(encoding), strlen(encoding));
    payload.append(reinterpret_cast<const uint8_t*>(text), strlen(text));
    setPayload(payload);
}

UriRecord::UriRecord(const char* uri, NfcUriType type) {
    if (uri == nullptr) {
        return;
    }

    setTnf(Tnf::TNF_WELL_KNOWN);

    uint8_t ndefType = static_cast<uint8_t>(NdefType::NDEF_TYPE_URI);
    setType(&ndefType, sizeof(ndefType));

    // Payload: uri type + uri
    Vector<uint8_t> payload;
    payload.append(static_cast<uint8_t>(type));
    payload.append(reinterpret_cast<const uint8_t*>(uri), strlen(uri));
    setPayload(payload);
}

LauchAppRecord::LauchAppRecord(const char* androidPackageName) {
    if (androidPackageName == nullptr) {
        return;
    }

    /* Record Payload Type for NFC NDEF Android Application Record */
    static constexpr const char ndefAndroidLaunchappRecType[] = "android.com:pkg";

    setTnf(Tnf::TNF_EXTERNAL_TYPE);
    setType(ndefAndroidLaunchappRecType, strlen(ndefAndroidLaunchappRecType));
    setPayload(androidPackageName, strlen(androidPackageName));
}

size_t NdefMessage::getEncodedSize() const {
    size_t length = 0;
    for (const auto record : records_) {
        if (record.get()) {
            length += record->getEncodedSize();
        }
    }

    return length;
}

size_t NdefMessage::getEncodedData(Vector<uint8_t>& vector) const {
    for (const auto record : records_) {
        if (record.get()) {
            record->getEncodedData(vector);
            LOG_DEBUG(TRACE, "record size: %d", record->getEncodedSize());
        }
    }

    return vector.size();
}

int NdefMessage::addRecord(Record& record) {
    std::shared_ptr<Record> r(new (std::nothrow) Record(record));
    if (!records_.append(r)) {
        return -1;
    }
    arrangeRecords();
    return records_.size() - 1;
}

int NdefMessage::addTextRecord(const char* text, const char* encoding) {
    if (text == nullptr || encoding == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    std::unique_ptr<TextRecord> textRecord(new (std::nothrow) TextRecord(text, encoding));
    Record* record = static_cast<Record*>(textRecord.get());

    return addRecord(*record);
}

int NdefMessage::addUriRecord(const char* uri, NfcUriType type) {
    if (uri == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    std::unique_ptr<UriRecord> uriRecord(new (std::nothrow) UriRecord(uri, type));
    Record* record = static_cast<Record*>(uriRecord.get());

    return addRecord(*record);
}

int NdefMessage::addLauchAppRecord(const char* androidPackageName) {
    if (androidPackageName == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    std::unique_ptr<LauchAppRecord> lauchAppRecord(new (std::nothrow) LauchAppRecord(androidPackageName));
    Record* record = static_cast<Record*>(lauchAppRecord.get());

    return addRecord(*record);
}

void NdefMessage::arrangeRecords(void) {
    if (records_.size()) {
        for (const auto record : records_) {
            record->setFirst(false);
            record->setLast(false);
        }
        records_[0]->setFirst(true);
        records_[records_.size() - 1]->setLast(true);
    }
}

int NfcTagType2::on(nfc_event_callback_t cb, void* context) {
    if (!enable_) {
        enable_ = true;
        hal_nfc_type2_init(nullptr);
    }
    encode_.clear();
    ndefMessage_.getEncodedData(encode_);
    LOG_DEBUG(TRACE, "size: %d", encode_.size());
    hal_nfc_type2_stop_emulation(nullptr);
    hal_nfc_type2_set_payload(encode_.data(), encode_.size());
    hal_nfc_type2_set_callback(cb, context);
    hal_nfc_type2_start_emulation(nullptr);

    callback_ = cb;
    context_ = context;

    return 0;
}

int NfcTagType2::off() {
    encode_.clear();
    hal_nfc_type2_stop_emulation(nullptr);

    return 0;
}

int NfcTagType2::update() {
    off();
    on(callback_, context_);

    return 0;
}

} // namespace particle

#endif

/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief NDEF message and record format
 * 
 * @details NDEF data is structured in messages. Each message consists of one or more records,
 *          which are made up of a header and the record payload. The Record header contains
 *          metadata about, amongst others, the payload type and length. The Record payload
 *          constitutes the actual content of the record.
 *
 *          [ NfcTagType2 ] --> [ NdefMessage ] --> [ NdefRecord ]
 *                                                  [ NdefRecord ]
 * 
 *          +-------------+-------------+-------------+
 *          | NDEF Record | NDEF Record | NDEF Record |
 *          +------+------+-------------+-------------+
 *              |
 *              v
 *          +------+--------+---------------------+
 *          | Record Header |   Record Payload    |
 *          +------+--------+---------------------+
 *              |
 *              v
 *          +------+-----+-------------+----------------+-----------+--------------+------------+
 *          | Flag & TNF | Type Length | Payload Length | ID Length | Payload Type | Payload ID |
 *          |   1 Byte   |   1 Byte    |   1 or 4 Byte  | (optional)|  (optional)  | (optional) |
 *          |            |             |                |   1 Byte  |   variable   |   variable |
 *          +------------+-------------+----------------+-----------+--------------+------------+
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>

#include "spark_wiring_platform.h"
#include "spark_wiring_vector.h"
#include "nfc_hal.h"

#if Wiring_NFC

namespace particle {

using spark::Vector;



class NdefRecord {
public:
    enum NdefType {
        NDEF_TYPE_TEXT = 0x54,
        NDEF_TYPE_URI  = 0x55
    };

    enum TNF {
        TNF_EMPTY           = 0x00,
        TNF_WELL_KNOWN      = 0x01,
        TNF_MIME_MEDIA      = 0x02,
        TNF_ABSOLUTE_URI    = 0x03,
        TNF_EXTERNAL_TYPE   = 0x04,
        TNF_UNKNOWN         = 0x05,
        TNF_UNCHANGED       = 0x06,
        TNF_RESERVED        = 0x07
    };

    typedef union {
        struct {
            TNF         tnf:3;
            uint8_t     il:1;
            uint8_t     sr:1;
            uint8_t     cf:1;
            uint8_t     me:1;
            uint8_t     mb:1;
        } bitdata;
        uint8_t bytedata;
    } FlagsAndTnf;

public:
    NdefRecord() : flagsAndTnf_{} {}
    ~NdefRecord() {}

    /**
     * @brief Get raw data size
     */
    size_t getEncodedSize();

    /**
     * @brief Get raw data
     * 
     * @retval Number of bytes copied
     */
    size_t getEncodedData(void *buf, size_t size);

    /**
     * @brief Get TNF field
     * 
     * @retval TNF, @ref FlagsAndTnf
     */
    TNF getTnf() {
        return flagsAndTnf_.bitdata.tnf; 
    }

    /**
     * @brief Get type filed length
     */
    size_t getTypeLength() {
        return type_.size();
    }

    /**
     * @brief Get type filed data
     * 
     * @retval Number of bytes copied
     */
    size_t getType(void *buf, size_t size) {
        if (buf == nullptr || size == 0) {
            return 0;
        }
        size_t len = static_cast<size_t>(type_.size()) > size ? size : type_.size();
        memcpy(buf, type_.data(), len);
        return len;
    }

    /**
     * @brief Get id filed length
     */
    size_t getIdLength() {
        return id_.size();
    }

    /**
     * @brief Get id filed data
     */
    size_t getId(void *buf, size_t size) {
        if (buf == nullptr || size == 0) {
            return 0;
        }
        size_t len = static_cast<size_t>(id_.size()) > size ? size : id_.size();
        memcpy(buf, id_.data(), len);
        return len;
    }

    /**
     * @brief Get payload filed length
     */
    size_t getPayloadLength() {
        return payload_.size();
    }

    /**
     * @brief Get id filed data
     */
    size_t getPayload(void *buf, size_t size) {
        if (buf == nullptr || size == 0) {
            return 0;
        }
        size_t len = static_cast<size_t>(payload_.size()) > size ? size : payload_.size();
        memcpy(buf, payload_.data(), len);
        return len;
    }

    /**
     * @brief Set TNF(Type Name Format): Specifies the structure of the Payload Type field and how to interpret it.
     */
    void setTnf(TNF tnf) {
        flagsAndTnf_.bitdata.tnf = tnf;
    }

    /**
     * @brief Specify the position of the NDEF record within the message,
     *        flag MB (Message Begin) 
     */
    void setFirst(bool firstRecord) {
        flagsAndTnf_.bitdata.mb = firstRecord;
    }

    /**
     * @brief Specify the position of the NDEF record within the message,
     *        flag ME(Message End)
     */
    void setLast(bool lastRecord) {
        flagsAndTnf_.bitdata.me = lastRecord;
    }

    /**
     * @brief Set type field data
     * 
     * @retval length of type field
     */
    size_t setType(const void *type, size_t numBytes) {
        if (type == nullptr || numBytes > 0xFF) {
            // type length filed is only 1 byte
            return 0;
        }
        type_.insert(type_.size(), static_cast<const uint8_t *>(type), numBytes);
        return numBytes;
    }

    /**
     * @brief Set id field data，When id field is not empty IL flag should be set
     * 
     * @retval length of id field
     */
    size_t setId(const void *id, size_t numBytes) {
        if (id == nullptr || numBytes > 0xFF) {
            // id length filed is only 1 byte
            return 0;
        }
        id_.insert(id_.size(), static_cast<const uint8_t *>(id), numBytes);
        flagsAndTnf_.bitdata.il = 1;
        return numBytes;
    }

    /**
     * @brief Set payload data
     * 
     * @retval length of payload
     */
    size_t setPayload(const void *payload, size_t numBytes) {
        payload_.insert(payload_.size(), static_cast<const uint8_t *>(payload), numBytes);
        return numBytes;
    }

private:
    FlagsAndTnf         flagsAndTnf_;
    Vector<uint8_t>     type_;
    Vector<uint8_t>     id_;
    Vector<uint8_t>     payload_;
};

class TextRecord : public NdefRecord {
public:
    TextRecord(const char *text, const char *encoding);
    ~TextRecord() {}
};

class UriRecord : public NdefRecord {
public:
    enum NfcUriType {
        NFC_URI_NONE          = 0x00,  /**< No prepending is done. */
        NFC_URI_HTTP_WWW      = 0x01,  /**< "http://www." */
        NFC_URI_HTTPS_WWW     = 0x02,  /**< "https://www." */
        NFC_URI_HTTP          = 0x03,  /**< "http:" */
        NFC_URI_HTTPS         = 0x04,  /**< "https:" */
        NFC_URI_TEL           = 0x05,  /**< "tel:" */
        NFC_URI_MAILTO        = 0x06,  /**< "mailto:" */
        NFC_URI_FTP_ANONYMOUS = 0x07,  /**< "ftp://anonymous:anonymous@" */
        NFC_URI_FTP_FTP       = 0x08,  /**< "ftp://ftp." */
        NFC_URI_FTPS          = 0x09,  /**< "ftps://" */
        NFC_URI_SFTP          = 0x0A,  /**< "sftp://" */
        NFC_URI_SMB           = 0x0B,  /**< "smb://" */
        NFC_URI_NFS           = 0x0C,  /**< "nfs://" */
        NFC_URI_FTP           = 0x0D,  /**< "ftp://" */
        NFC_URI_DAV           = 0x0E,  /**< "dav://" */
        NFC_URI_NEWS          = 0x0F,  /**< "news:" */
        NFC_URI_TELNET        = 0x10,  /**< "telnet://" */
        NFC_URI_IMAP          = 0x11,  /**< "imap:" */
        NFC_URI_RTSP          = 0x12,  /**< "rtsp://" */
        NFC_URI_URN           = 0x13,  /**< "urn:" */
        NFC_URI_POP           = 0x14,  /**< "pop:" */
        NFC_URI_SIP           = 0x15,  /**< "sip:" */
        NFC_URI_SIPS          = 0x16,  /**< "sips:" */
        NFC_URI_TFTP          = 0x17,  /**< "tftp:" */
        NFC_URI_BTSPP         = 0x18,  /**< "btspp://" */
        NFC_URI_BTL2CAP       = 0x19,  /**< "btl2cap://" */
        NFC_URI_BTGOEP        = 0x1A,  /**< "btgoep://" */
        NFC_URI_TCPOBEX       = 0x1B,  /**< "tcpobex://" */
        NFC_URI_IRDAOBEX      = 0x1C,  /**< "irdaobex://" */
        NFC_URI_FILE          = 0x1D,  /**< "file://" */
        NFC_URI_URN_EPC_ID    = 0x1E,  /**< "urn:epc:id:" */
        NFC_URI_URN_EPC_TAG   = 0x1F,  /**< "urn:epc:tag:" */
        NFC_URI_URN_EPC_PAT   = 0x20,  /**< "urn:epc:pat:" */
        NFC_URI_URN_EPC_RAW   = 0x21,  /**< "urn:epc:raw:" */
        NFC_URI_URN_EPC       = 0x22,  /**< "urn:epc:" */
        NFC_URI_URN_NFC       = 0x23,  /**< "urn:nfc:" */
        NFC_URI_RFU           = 0xFF   /**< No prepending is done. Reserved for future use. */
    };

public:
    UriRecord(const char *uri, UriRecord::NfcUriType type);
    ~UriRecord() {}
};

class LauchappRecord : public NdefRecord{
public:
    LauchappRecord(const char *androidPackageName);
    ~LauchappRecord() {}
};

class NdefMessage{
public:
    NdefMessage() {}
    ~NdefMessage() {}

    /**
     * @brief Get raw data size of all records
     * 
     * @retval length of raw data
     */
    size_t getEncodedSize();

    /**
     * @brief Get raw data of all records
     * 
     * @retval length of raw data
     */
    size_t getEncodedData(void *data, size_t numBytes);

    /**
     * @brief Add a customized record
     * 
     * @retval index of the record
     */
    int addRecord(std::shared_ptr<NdefRecord> record) {
        records_.append(record);
        arrangeRecords();
        return records_.size() - 1;
    }

    /**
     * @brief Add a text record
     * 
     * @retval index of the record
     */
    int addTextRecord(const char *text, const char *encoding);

    /**
     * @brief Add a URI record
     * 
     * @retval index of the record
     */
    int addUriRecord(const char *uri, UriRecord::NfcUriType type);

    /**
     * @brief Add a lauch application record, only support Android
     * 
     * @retval index of the record
     */
    int addLauchAppRecord(const char *androidPackageName);

    /**
     * @brief Get the count of all records
     */
    size_t getRecordCount() const {
        return records_.size();
    };

    /**
     * @brief Get record by index
     * 
     * @retval return the record, or nullptr
     */
    std::shared_ptr<NdefRecord> & getRecord(int index) {
        // FIXME: parameter check?
        return records_[index];
    };

    /**
     * @brief Get record by index
     */
    std::shared_ptr<NdefRecord> & operator[](int index) {
        return getRecord(index);
    }

private:
    /**
     * @brief Arrange the records to reset MB and ME flag
     */
    void arrangeRecords(void);

private:
    Vector<std::shared_ptr<NdefRecord>> records_;
};

class NfcTagType2 {
public:
    NfcTagType2(): enable_(false) {}
    ~NfcTagType2() {}

    // TODO: UID configuration is not implemented by Nordic SDK, for future support
    size_t getUidLength();
    size_t getUid(void *uid, size_t numBytes);
    size_t setUid(void *uid, size_t numBytes);

    NdefMessage& getNdefMessage() {
        return ndefMessage_;
    }

    int start(nfc_event_callback_t cb=nullptr);
    int stop();

private:
    bool                 enable_;
    Vector<uint8_t>      uid_;
    Vector<uint8_t>      encode_;
    NdefMessage          ndefMessage_;
};

extern NfcTagType2 NFC;

} // namespace particle

#endif /* Wiring_NFC*/

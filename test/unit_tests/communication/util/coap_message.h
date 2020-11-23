/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "coap_defs.h"

#include <string>
#include <vector>
#include <map>

namespace particle {

namespace protocol {

namespace test {

class CoapMessageOption {
public:
    explicit CoapMessageOption(CoapOption number);
    explicit CoapMessageOption(unsigned number);
    CoapMessageOption(CoapOption number, unsigned value);
    CoapMessageOption(unsigned number, unsigned value);
    CoapMessageOption(CoapOption number, std::string value);
    CoapMessageOption(unsigned number, std::string value);
    CoapMessageOption(CoapOption number, const char* data, size_t size);
    CoapMessageOption(unsigned number, const char* data, size_t size);

    unsigned number() const;

    const std::string& toString() const;
    unsigned toUInt() const;
    bool isEmpty() const;

private:
    std::string data_;
    int num_;

    static std::string encodeUInt(unsigned value);
    static unsigned decodeUInt(const std::string& data);
};

class CoapMessage {
public:
    CoapMessage();

    CoapMessage& type(CoapType type);
    CoapType type() const;
    bool hasType() const;

    CoapMessage& code(CoapCode code);
    CoapMessage& code(unsigned code);
    unsigned code() const;
    bool hasCode() const;

    CoapMessage& id(CoapMessageId id);
    CoapMessageId id() const;
    bool hasId() const;

    CoapMessage& token(std::string data);
    CoapMessage& token(const char* data, size_t size);
    const std::string& token() const;
    bool hasToken() const;

    CoapMessage& option(CoapMessageOption option);
    CoapMessage& option(CoapOption number, unsigned value);
    CoapMessage& option(unsigned number, unsigned value);
    CoapMessage& option(CoapOption number, std::string value);
    CoapMessage& option(unsigned number, std::string value);
    CoapMessage& option(CoapOption number, const char* data, size_t size);
    CoapMessage& option(unsigned number, const char* data, size_t size);
    CoapMessage& emptyOption(CoapOption number);
    CoapMessage& emptyOption(unsigned number);
    CoapMessageOption option(CoapOption opt) const;
    CoapMessageOption option(unsigned opt) const;
    std::vector<CoapMessageOption> options(CoapOption number) const;
    std::vector<CoapMessageOption> options(unsigned number) const;
    std::vector<CoapMessageOption> options() const;
    bool hasOption(CoapOption number) const;
    bool hasOption(unsigned number) const;
    bool hasOptions() const;

    CoapMessage& payload(std::string data);
    CoapMessage& payload(const char* data, size_t size);
    const std::string& payload() const;
    bool hasPayload() const;

    std::string encode() const;

    static CoapMessage decode(const std::string& data);
    static CoapMessage decode(const char* data, size_t size);

private:
    std::multimap<unsigned, CoapMessageOption> opts_;
    CoapType type_;
    unsigned code_;
    CoapMessageId id_;
    std::string token_;
    std::string payload_;
    bool hasType_;
    bool hasCode_;
    bool hasId_;
    bool hasToken_;
    bool hasPayload_;
};

inline CoapMessageOption::CoapMessageOption(CoapOption number) :
        CoapMessageOption((unsigned)number) {
}

inline CoapMessageOption::CoapMessageOption(unsigned number) :
        num_(number) {
}

inline CoapMessageOption::CoapMessageOption(CoapOption number, unsigned value) :
        CoapMessageOption((unsigned)number, value) {
}

inline CoapMessageOption::CoapMessageOption(unsigned number, unsigned value) :
        data_(encodeUInt(value)),
        num_(number) {
}

inline CoapMessageOption::CoapMessageOption(CoapOption number, std::string value) :
        CoapMessageOption((unsigned)number, std::move(value)) {
}

inline CoapMessageOption::CoapMessageOption(unsigned number, std::string value) :
        data_(std::move(value)),
        num_(number) {
}

inline CoapMessageOption::CoapMessageOption(CoapOption number, const char* data, size_t size) :
        CoapMessageOption((unsigned)number, data, size) {
}

inline CoapMessageOption::CoapMessageOption(unsigned number, const char* data, size_t size) :
        data_(data, size),
        num_(number) {
}

inline unsigned CoapMessageOption::number() const {
    return num_;
}

inline const std::string& CoapMessageOption::toString() const {
    return data_;
}

inline unsigned CoapMessageOption::toUInt() const {
    return decodeUInt(data_);
}

inline bool CoapMessageOption::isEmpty() const {
    return data_.empty();
}

inline CoapMessage::CoapMessage() :
    type_(CoapType::CON),
    code_(0),
    id_(0),
    hasType_(false),
    hasCode_(false),
    hasId_(false),
    hasToken_(false),
    hasPayload_(false) {
}

inline CoapMessage& CoapMessage::type(CoapType type) {
    type_ = type;
    hasType_ = true;
    return *this;
}

inline CoapType CoapMessage::type() const {
    if (!hasType_) {
        throw std::runtime_error("CoAP message type is missing");
    }
    return type_;
}

inline bool CoapMessage::hasType() const {
    return hasType_;
}

inline CoapMessage& CoapMessage::code(CoapCode code) {
    return this->code((unsigned)code);
}

inline CoapMessage& CoapMessage::code(unsigned code) {
    code_ = code;
    hasCode_ = true;
    return *this;
}

inline unsigned CoapMessage::code() const {
    if (!hasCode_) {
        throw std::runtime_error("CoAP message code is missing");
    }
    return code_;
}

inline bool CoapMessage::hasCode() const {
    return hasCode_;
}

inline CoapMessage& CoapMessage::id(CoapMessageId id) {
    id_ = id;
    hasId_ = true;
    return *this;
}

inline CoapMessageId CoapMessage::id() const {
    if (!hasId_) {
        throw std::runtime_error("CoAP message ID is missing");
    }
    return id_;
}

inline bool CoapMessage::hasId() const {
    return hasId_;
}

inline CoapMessage& CoapMessage::token(std::string data) {
    token_ = std::move(data);
    hasToken_ = true;
    return *this;
}

inline CoapMessage& CoapMessage::token(const char* data, size_t size) {
    return token(std::string(data, size));
}

inline const std::string& CoapMessage::token() const {
    if (!hasToken_) {
        throw std::runtime_error("CoAP message has no token");
    }
    return token_;
}

inline bool CoapMessage::hasToken() const {
    return hasToken_;
}

inline CoapMessage& CoapMessage::payload(std::string data) {
    payload_ = std::move(data);
    hasPayload_ = true;
    return *this;
}

inline CoapMessage& CoapMessage::payload(const char* data, size_t size) {
    return payload(std::string(data, size));
}

inline const std::string& CoapMessage::payload() const {
    if (!hasPayload_) {
        throw std::runtime_error("CoAP message has no payload data");
    }
    return payload_;
}

inline bool CoapMessage::hasPayload() const {
    return hasPayload_;
}

inline CoapMessage& CoapMessage::option(CoapMessageOption option) {
    opts_.emplace(option.number(), std::move(option));
    return *this;
}

inline CoapMessage& CoapMessage::option(CoapOption number, unsigned value) {
    return option((unsigned)number, value);
}

inline CoapMessage& CoapMessage::option(unsigned number, unsigned value) {
    return option(CoapMessageOption(number, value));
}

inline CoapMessage& CoapMessage::option(CoapOption number, std::string value) {
    return option((unsigned)number, std::move(value));
}

inline CoapMessage& CoapMessage::option(unsigned number, std::string value) {
    return option(CoapMessageOption(number, std::move(value)));
}

inline CoapMessage& CoapMessage::option(CoapOption number, const char* data, size_t size) {
    return option((unsigned)number, data, size);
}

inline CoapMessage& CoapMessage::option(unsigned number, const char* data, size_t size) {
    return option(CoapMessageOption(number, data, size));
}

inline CoapMessage& CoapMessage::emptyOption(CoapOption number) {
    return emptyOption((unsigned)number);
}

inline CoapMessage& CoapMessage::emptyOption(unsigned number) {
    return option(CoapMessageOption(number));
}

inline CoapMessageOption CoapMessage::option(CoapOption number) const {
    return option((unsigned)number);
}

inline CoapMessageOption CoapMessage::option(unsigned number) const {
    const auto it = opts_.find(number);
    if (it == opts_.end()) {
        throw std::runtime_error("CoAP option not found");
    }
    return it->second;
}

inline std::vector<CoapMessageOption> CoapMessage::options(CoapOption number) const {
    return options((unsigned)number);
}

inline std::vector<CoapMessageOption> CoapMessage::options(unsigned number) const {
    const auto r = opts_.equal_range(number);
    if (r.first == r.second) {
        throw std::runtime_error("CoAP option not found");
    }
    std::vector<CoapMessageOption> opts;
    for (auto it = r.first; it != r.second; ++it) {
        opts.push_back(it->second);
    }
    return opts;
}

inline std::vector<CoapMessageOption> CoapMessage::options() const {
    std::vector<CoapMessageOption> opts;
    for (auto it = opts_.begin(); it != opts_.end(); ++it) {
        opts.push_back(it->second);
    }
    return opts;
}

inline bool CoapMessage::hasOption(CoapOption number) const {
    return hasOption((unsigned)number);
}

inline bool CoapMessage::hasOption(unsigned number) const {
    return opts_.count(number) > 0;
}

inline bool CoapMessage::hasOptions() const {
    return !opts_.empty();
}

inline CoapMessage CoapMessage::decode(const std::string& data) {
    return decode(data.data(), data.size());
}

} // namespace test

} // namespace protocol

} // namespace particle

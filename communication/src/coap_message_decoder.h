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

#include <cstddef>

namespace particle {

namespace protocol {

class CoapOptionIterator;

/**
 * A class for decoding CoAP messages.
 */
class CoapMessageDecoder {
public:
    CoapMessageDecoder();

    CoapType type() const;
    unsigned code() const;
    CoapMessageId id() const;

    const char* token() const;
    size_t tokenSize() const;
    bool hasToken() const;

    const char* payload() const;
    size_t payloadSize() const;
    bool hasPayload() const;

    CoapOptionIterator options() const;
    CoapOptionIterator findOption(CoapOption opt) const;
    CoapOptionIterator findOption(unsigned opt) const;
    bool hasOption(CoapOption opt) const;
    bool hasOption(unsigned opt) const;
    bool hasOptions() const;

    // TODO: Add convenience methods for decoding URI path and query options

    int decode(const char* data, size_t size);

private:
    // We want all fields that identify the message, such as the ID and token, to remain valid
    // even if the data in the source buffer is not valid anymore
    char token_[MAX_COAP_TOKEN_SIZE];
    CoapType type_;
    CoapMessageId id_;
    const char* opts_;
    const char* payload_;
    size_t tokenSize_;
    size_t optsSize_;
    size_t payloadSize_;
    unsigned code_;
};

/**
 * A class for iterating over a CoAP message's options.
 */
class CoapOptionIterator {
public:
    CoapOptionIterator();

    unsigned option() const;

    // Note: The data is never null-terminated
    const char* data() const;
    size_t size() const;

    unsigned toUInt() const;

    bool next();

    // Returns true if this iterator points to a valid option
    operator bool() const;

private:
    const char* nextOpt_;
    const char* optData_;
    size_t bufSize_;
    size_t optSize_;
    unsigned opt_;

    CoapOptionIterator(const char* data, size_t size);

    void reset();

    friend class CoapMessageDecoder;
};

inline CoapType CoapMessageDecoder::type() const {
    return type_;
}

inline unsigned CoapMessageDecoder::code() const {
    return code_;
}

inline CoapMessageId CoapMessageDecoder::id() const {
    return id_;
}

inline const char* CoapMessageDecoder::token() const {
    return (tokenSize_ > 0) ? token_ : nullptr;
}

inline size_t CoapMessageDecoder::tokenSize() const {
    return tokenSize_;
}

inline bool CoapMessageDecoder::hasToken() const {
    return tokenSize_ > 0;
}

inline const char* CoapMessageDecoder::payload() const {
    return payload_;
}

inline size_t CoapMessageDecoder::payloadSize() const {
    return payloadSize_;
}

inline bool CoapMessageDecoder::hasPayload() const {
    return payloadSize_ > 0;
}

inline CoapOptionIterator CoapMessageDecoder::options() const {
    return CoapOptionIterator(opts_, optsSize_);
}

inline CoapOptionIterator CoapMessageDecoder::findOption(CoapOption opt) const {
    return findOption((unsigned)opt);
}

inline bool CoapMessageDecoder::hasOption(CoapOption opt) const {
    return hasOption((unsigned)opt);
}

inline bool CoapMessageDecoder::hasOption(unsigned opt) const {
    return findOption(opt);
}

inline bool CoapMessageDecoder::hasOptions() const {
    return opts_;
}

inline CoapOptionIterator::CoapOptionIterator() :
        CoapOptionIterator(nullptr, 0) {
}

inline CoapOptionIterator::CoapOptionIterator(const char* data, size_t size) :
        nextOpt_(data),
        optData_(nullptr),
        bufSize_(size),
        optSize_(0),
        opt_(0) {
}

inline unsigned CoapOptionIterator::option() const {
    return opt_;
}

inline const char* CoapOptionIterator::data() const {
    return optSize_ ? optData_ : nullptr;
}

inline size_t CoapOptionIterator::size() const {
    return optSize_;
}

inline CoapOptionIterator::operator bool() const {
    return optData_;
}

} // namespace protocol

} // namespace particle

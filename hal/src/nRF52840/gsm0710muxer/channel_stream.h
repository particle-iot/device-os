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

#ifndef GSM0710_MUXER_CHANNEL_STREAM_H
#define GSM0710_MUXER_CHANNEL_STREAM_H

#include "gsm0710muxer/muxer.h"
#include "stream.h"
#include "ringbuffer.h"
#include "check.h"
#include "logging.h"
#include "system_error.h"
#include "concurrent_hal.h"
#include <memory>

namespace particle {

template <typename MuxerT>
class MuxerChannelStream : virtual public Stream {
public:
    MuxerChannelStream(MuxerT* muxer, uint8_t channel);
    virtual ~MuxerChannelStream();

    int init(size_t rxBufSize);

    static int channelDataCb(const uint8_t* data, size_t size, void* ctx);

    virtual int read(char* data, size_t size) override;
    virtual int peek(char* data, size_t size) override;
    virtual int skip(size_t size) override;
    virtual int availForRead() override;

    virtual int write(const char* data, size_t size) override;
    virtual int flush() override;
    virtual int availForWrite() override;

    virtual int waitEvent(unsigned flags, unsigned timeout = 0) override;

private:
    MuxerT* muxer_;
    uint8_t channel_;
    size_t rxBufSize_ = 0;
    std::unique_ptr<particle::services::RingBuffer<char> > rxBuf_;
    std::unique_ptr<char[]> rxBufData_;
    os_semaphore_t sem_ = nullptr;
};

template <typename MuxerT>
MuxerChannelStream<MuxerT>::MuxerChannelStream(MuxerT* muxer, uint8_t channel)
        : muxer_(muxer),
          channel_(channel) {

}

template <typename MuxerT>
MuxerChannelStream<MuxerT>::~MuxerChannelStream() {
    if (sem_) {
        os_semaphore_destroy(sem_);
        sem_ = nullptr;
    }
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::init(size_t rxBufSize) {
    if (rxBufSize != rxBufSize_ || !rxBufData_) {
        rxBufSize_ = rxBufSize;
        rxBufData_.reset(new (std::nothrow) char[rxBufSize]);
        CHECK_TRUE(rxBufData_, SYSTEM_ERROR_NO_MEMORY);
    }

    if (!rxBuf_) {
        rxBuf_.reset(new (std::nothrow) particle::services::RingBuffer<char>());
        CHECK_TRUE(rxBuf_, SYSTEM_ERROR_NO_MEMORY);
    }

    if (!sem_) {
        CHECK_TRUE(os_semaphore_create(&sem_, 1, 0) == 0, SYSTEM_ERROR_NO_MEMORY);
    }

    rxBuf_->init(rxBufData_.get(), rxBufSize_);
    return 0;
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::channelDataCb(const uint8_t* data, size_t size, void* ctx) {
    auto self = (MuxerChannelStream<MuxerT>*)ctx;
    auto wasEmpty = self->rxBuf_->empty();
    auto r = self->rxBuf_->put((const char*)data, size);
    if (r == SYSTEM_ERROR_TOO_LARGE) {
        LOG_DEBUG(WARN, "No space in muxer channel stream rx buffer while trying to put %d bytes (%d available)",
                (int)size, self->rxBuf_->space());
    }
    if (wasEmpty) {
        os_semaphore_give(self->sem_, true);
    }
    return 0;
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::read(char* data, size_t size) {
    size_t canRead = CHECK(rxBuf_->data());
    size_t willRead = std::min(canRead, size);
    return rxBuf_->get(data, willRead);
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::peek(char* data, size_t size) {
    size_t canPeek = CHECK(rxBuf_->data());
    size_t willPeek = std::min(canPeek, size);
    return rxBuf_->peek(data, willPeek);
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::skip(size_t size) {
    size_t canRead = CHECK(rxBuf_->data());
    size_t willRead = std::min(canRead, size);
    return rxBuf_->get(nullptr, willRead);
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::availForRead() {
    return rxBuf_->data();
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::write(const char* data, size_t size) {
    if (size == 0) {
        return 0;
    }
    auto r = muxer_->writeChannel(channel_, (const uint8_t*)data, size);
    if (!r) {
        return size;
    } else if (r == gsm0710::GSM0710_ERROR_FLOW_CONTROL) {
        return 0;
    }
    return r;
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::flush() {
    return 0;
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::availForWrite() {
    return muxer_->getMaxFrameSize();
}

template <typename MuxerT>
int MuxerChannelStream<MuxerT>::waitEvent(unsigned flags, unsigned timeout) {
    if (!flags) {
        return 0;
    }
    if (!(flags & (Stream::READABLE | Stream::WRITABLE))) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    unsigned f = 0;
    const auto t = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        if (availForRead() > 0) {
            f |= Stream::READABLE;
        }
        // FIXME: check modem status flags
        if (availForWrite() > 0) {
            f |= Stream::WRITABLE;
        }
        if (f &= flags) {
            break;
        }
        if (timeout > 0 && HAL_Timer_Get_Milli_Seconds() - t >= timeout) {
            return SYSTEM_ERROR_TIMEOUT;
        }
        os_semaphore_take(sem_, HAL_Timer_Get_Milli_Seconds() - t, false);
    }
    return f;
}

} // particle
#endif // GSM0710_MUXER_CHANNEL_STREAM_H

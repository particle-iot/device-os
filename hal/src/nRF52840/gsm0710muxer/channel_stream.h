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

namespace detail {
const auto MUXER_CHANNEL_SUSPEND_THRESHOLD = 3; // 1/3
const auto MUXER_CHANNEL_RESUME_THRESHOLD = 2; // 1/2
} // detail

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

    void enabled(bool enabled);
    bool enabled() const;

private:
    void suspend();
    void resume();

private:
    MuxerT* muxer_;
    uint8_t channel_;
    size_t rxBufSize_ = 0;
    std::unique_ptr<particle::services::RingBuffer<char> > rxBuf_;
    std::unique_ptr<char[]> rxBufData_;
    os_semaphore_t sem_ = nullptr;
    volatile bool flow_ = false;
    volatile bool enabled_ = true;
};

template <typename MuxerT>
inline MuxerChannelStream<MuxerT>::MuxerChannelStream(MuxerT* muxer, uint8_t channel)
        : muxer_(muxer),
          channel_(channel) {

}

template <typename MuxerT>
inline MuxerChannelStream<MuxerT>::~MuxerChannelStream() {
    if (sem_) {
        os_semaphore_destroy(sem_);
        sem_ = nullptr;
    }
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::init(size_t rxBufSize) {
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
inline int MuxerChannelStream<MuxerT>::channelDataCb(const uint8_t* data, size_t size, void* ctx) {
    auto self = (MuxerChannelStream<MuxerT>*)ctx;
    auto wasEmpty = self->rxBuf_->empty();
    auto r = self->rxBuf_->put((const char*)data, size);
    if (r == SYSTEM_ERROR_TOO_LARGE) {
        LOG_DEBUG(WARN, "No space in muxer channel stream rx buffer while trying to put %d bytes (%d available)",
                (int)size, self->rxBuf_->space());
    }
    self->suspend();
    if (wasEmpty) {
        os_semaphore_give(self->sem_, false);
    }
    return 0;
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::read(char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t canRead = CHECK(rxBuf_->data());
    size_t willRead = std::min(canRead, size);
    auto r = rxBuf_->get(data, willRead);
    resume();
    return r;
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::peek(char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t canPeek = CHECK(rxBuf_->data());
    size_t willPeek = std::min(canPeek, size);
    return rxBuf_->peek(data, willPeek);
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::skip(size_t size) {
    return read(nullptr, size);
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::availForRead() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return rxBuf_->data();
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::write(const char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
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
inline int MuxerChannelStream<MuxerT>::flush() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return 0;
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::availForWrite() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return muxer_->getMaxFrameSize();
}

template <typename MuxerT>
inline int MuxerChannelStream<MuxerT>::waitEvent(unsigned flags, unsigned timeout) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
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
        if (!enabled_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }
    return f;
}

template <typename MuxerT>
inline void MuxerChannelStream<MuxerT>::suspend() {
    ssize_t space = rxBuf_->space();
    if (space < 0) {
        return;
    }
    if (!flow_ && (size_t)space <= (rxBufSize_ / detail::MUXER_CHANNEL_SUSPEND_THRESHOLD)) {
        muxer_->suspendChannel(channel_);
        flow_ = true;
    }
}

template <typename MuxerT>
inline void MuxerChannelStream<MuxerT>::resume() {
    ssize_t space = rxBuf_->space();
    if (space < 0) {
        return;
    }
    if (flow_ && (size_t)space >= (rxBufSize_ / detail::MUXER_CHANNEL_RESUME_THRESHOLD)) {
        muxer_->resumeChannel(channel_);
        flow_ = false;
    }
}

template <typename MuxerT>
inline void MuxerChannelStream<MuxerT>::enabled(bool enabled) {
    const auto wasEnabled = enabled_;
    enabled_ = enabled;
    if (!enabled && wasEnabled) {
        os_semaphore_give(sem_, false);
    }
}

template <typename MuxerT>
inline bool MuxerChannelStream<MuxerT>::enabled() const {
    return enabled_;
}

} // particle

#endif // GSM0710_MUXER_CHANNEL_STREAM_H

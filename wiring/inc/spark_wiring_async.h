/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#ifndef SPARK_WIRING_ASYNC_H
#define SPARK_WIRING_ASYNC_H

#include "spark_wiring_ticks.h"
#include "spark_wiring_error.h"

#include "system_cloud.h"

#include <functional>
#include <memory>
#include <atomic>

#if (ATOMIC_POINTER_LOCK_FREE != 2) || (ATOMIC_INT_LOCK_FREE != 2) || (ATOMIC_BOOL_LOCK_FREE != 2)
#error "std::atomic is not always lock-free for required types"
#endif

namespace spark {

namespace detail {

// Future state
enum class FutureState: int {
    RUNNING,
    SUCCEEDED,
    FAILED,
    CANCELLED
};

// Completion callback types
template<typename ResultT>
struct FutureCallbackTypes {
    typedef std::function<void(const ResultT&)> OnSuccess;
    typedef std::function<void(const Error&)> OnError;
};

// Completion callback types. Specialization for void result type
template<>
struct FutureCallbackTypes<void> {
    typedef std::function<void()> OnSuccess;
    typedef std::function<void(const Error&)> OnError;
};

// Base class for FutureData
template<typename ResultT>
struct FutureDataBase {
    std::atomic<bool> done; // Signals that future is in a final state
    std::atomic<FutureState> state; // Future state
    std::atomic<typename FutureCallbackTypes<ResultT>::OnSuccess*> onSuccess; // Pointer to callback for succeeded operation
    std::atomic<typename FutureCallbackTypes<ResultT>::OnError*> onError; // Pointer to callback for failed operation

    explicit FutureDataBase(FutureState state) :
            done(state != FutureState::RUNNING),
            state(state),
            onSuccess(nullptr),
            onError(nullptr) {
    }

    virtual ~FutureDataBase() {
        delete onSuccess.load(std::memory_order_relaxed);
        delete onError.load(std::memory_order_relaxed);
    }
};

// Internal future implementation
template<typename ResultT>
struct FutureData: FutureDataBase<ResultT> {
    union {
        ResultT result;
        Error error;
    };

    explicit FutureData(FutureState state) :
            FutureDataBase<ResultT>(state) {
    }

    virtual ~FutureData() {
        // Call destructor of the appropriate unnamed enum's field
        const FutureState s = this->state.load(std::memory_order_relaxed);
        if (s == FutureState::SUCCEEDED) {
            result.~ResultT();
        } else if (s == FutureState::FAILED) {
            error.~Error();
        }
    }
};

// Internal future implementation. Specialization for void result type
template<>
struct FutureData<void>: FutureDataBase<void> {
    Error error;

    explicit FutureData(FutureState state) :
            FutureDataBase<void>(state) {
    }
};

template<typename ResultT>
using FutureDataPtr = std::shared_ptr<FutureData<ResultT>>;

// Event loop and threading abstraction. Used for unit testing
struct FutureContext {
    static void processApplicationEvents() {
        spark_process();
    }

    // Asynchronously invokes callback in the application context
    static void invokeApplicationCallback(void (*callback)(void* data), void* data) {
        // TODO
    }

    // Returns true if current thread is the application thread
    static bool isApplicationThreadCurrent() {
        // TODO
        return true;
    }
};

// Helper function for invokeFutureCallback()
void futureCallbackWrapper(void* data);

// Invokes callback in the application context
template<typename ContextT, typename FunctionT, typename... ArgsT>
inline void invokeFutureCallback(const std::function<FunctionT>& callback, ArgsT&&... args) {
    if (ContextT::isApplicationThreadCurrent()) {
        callback(std::forward<ArgsT>(args)...); // Invoke synchronously
    } else {
        auto funcPtr = new std::function<void()>(std::bind(callback, std::forward<ArgsT>(args)...));
        ContextT::invokeApplicationCallback(futureCallbackWrapper, funcPtr);
    }
}

// Convenience overload taking callback from its atomic wrapper
template<typename ContextT, typename FunctionT, typename... ArgsT>
inline void invokeFutureCallback(std::atomic<std::function<FunctionT>*>& callback, ArgsT&&... args) {
    std::function<FunctionT>* callbackPtr = callback.exchange(nullptr, std::memory_order_acquire);
    if (callbackPtr) {
        invokeFutureCallback<ContextT>(*callbackPtr, std::forward<ArgsT>(args)...);
        delete callbackPtr;
    }
}

} // namespace spark::detail

template<typename ResultT, typename ContextT>
class Future;

template<typename ResultT, typename ContextT>
class Promise;

// Base class for Promise
template<typename ResultT, typename ContextT>
class PromiseBase {
public:
    // Future state
    typedef detail::FutureState State;

    // Completion callbacks
    typedef typename detail::FutureCallbackTypes<ResultT>::OnSuccess OnSuccessCallback;
    typedef typename detail::FutureCallbackTypes<ResultT>::OnError OnErrorCallback;

    PromiseBase() :
            d_(new detail::FutureData<ResultT>(State::RUNNING)) {
    }

    explicit PromiseBase(detail::FutureDataPtr<ResultT> data) :
            d_(data) {
    }

    void setError(Error error) {
        State s = State::RUNNING; // Expected state
        if (d_->state.compare_exchange_strong(s, State::FAILED, std::memory_order_relaxed)) {
            d_->error = std::move(error);
            detail::invokeFutureCallback<ContextT>(d_->onError, d_->error);
            d_->done.store(true, std::memory_order_release);
        }
    }

    bool isDone() const {
        return d_->done.load(std::memory_order_relaxed);
    }

    Future<ResultT, ContextT> future() const {
        return Future<ResultT, ContextT>(d_);
    }

    // Wraps this promise into an object that can be passed to a C function
    void* toData() const {
        // TODO: Use custom reference counting object to avoid unnecessary memory allocation
        return new detail::FutureDataPtr<ResultT>(d_);
    }

    // Unwraps promise from an object created via toData() method
    static Promise<ResultT, ContextT> fromData(void* data) {
        auto d = static_cast<detail::FutureDataPtr<ResultT>*>(data);
        const Promise<ResultT, ContextT> p(*d);
        delete d;
        return p;
    }

protected:
    detail::FutureDataPtr<ResultT> d_;
};

template<typename ResultT, typename ContextT = detail::FutureContext>
class Promise: public PromiseBase<ResultT, ContextT> {
public:
    using typename PromiseBase<ResultT, ContextT>::State;
    using typename PromiseBase<ResultT, ContextT>::OnSuccessCallback;

    using PromiseBase<ResultT, ContextT>::PromiseBase;

    void setResult(ResultT result) {
        State s = State::RUNNING; // Expected state
        if (this->d_->state.compare_exchange_strong(s, State::SUCCEEDED, std::memory_order_relaxed)) {
            this->d_->result = std::move(result);
            detail::invokeFutureCallback<ContextT>(this->d_->onSuccess, this->d_->result);
            this->d_->done.store(true, std::memory_order_release);
        }
    }

    // System completion callback (see services/inc/completion_handler.h). This function is provided for
    // convenience, do not use it with complex result types that require ABI compatibility checks
    static void systemCallback(int error, void* result, void* data, void* reserved) {
        auto p = Promise<ResultT, ContextT>::fromData(data);
        if (error != spark::Error::NONE) {
            p.setError((spark::Error::Type)error);
        } else if (result) {
            p.setResult(*static_cast<const ResultT*>(result));
        } else {
            p.setResult(ResultT());
        }
    }
};

template<typename ContextT>
class Promise<void, ContextT>: public PromiseBase<void, ContextT> {
public:
    using typename PromiseBase<void, ContextT>::State;
    using typename PromiseBase<void, ContextT>::OnSuccessCallback;

    using PromiseBase<void, ContextT>::PromiseBase;

    void setResult() {
        State s = State::RUNNING; // Expected state
        if (this->d_->state.compare_exchange_strong(s, State::SUCCEEDED, std::memory_order_relaxed)) {
            detail::invokeFutureCallback<ContextT>(this->d_->onSuccess);
            this->d_->done.store(true, std::memory_order_release);
        }
    }

    static void systemCallback(int error, void* result, void* data, void* reserved) {
        auto p = Promise<void, ContextT>::fromData(data);
        if (error != spark::Error::NONE) {
            p.setError((spark::Error::Type)error);
        } else {
            p.setResult();
        }
    }
};

// Base class for Future
template<typename ResultT, typename ContextT>
class FutureBase {
public:
    // Future state
    typedef detail::FutureState State;

    // Completion callbacks
    typedef typename detail::FutureCallbackTypes<ResultT>::OnSuccess OnSuccessCallback;
    typedef typename detail::FutureCallbackTypes<ResultT>::OnError OnErrorCallback;

    // Constructs cancelled future
    FutureBase() :
            d_(new detail::FutureData<ResultT>(State::CANCELLED)) {
    }

    explicit FutureBase(detail::FutureDataPtr<ResultT> data) :
            d_(data) {
    }

    Future<ResultT, ContextT>& wait(int timeout = 0) {
        // TODO: Waiting for a future in non-default application thread is not supported
        if (ContextT::isApplicationThreadCurrent()) {
            const system_tick_t t = (timeout > 0) ? millis() : 0;
            for (;;) {
                const bool done = d_->done.load(std::memory_order_relaxed);
                if (done || (timeout > 0 && millis() - t >= (system_tick_t)timeout)) {
                    break;
                }
                ContextT::processApplicationEvents();
            }
        }
        return *static_cast<Future<ResultT, ContextT>*>(this);
    }

    bool cancel() {
        State s = State::RUNNING; // Expected state
        if (!d_->state.compare_exchange_strong(s, State::CANCELLED, std::memory_order_relaxed)) {
            return false;
        }
        d_->done.store(true, std::memory_order_release);
        return true;
    }

    Error error() const {
        if (!d_->done.load(std::memory_order_acquire) || d_->state.load(std::memory_order_relaxed) != State::FAILED) {
            return Error();
        }
        return d_->error;
    }

    State state() const {
        return d_->state.load(std::memory_order_relaxed);
    }

    bool isSucceeded() const {
        return state() == State::SUCCEEDED;
    }

    bool isFailed() const {
        return state() == State::FAILED;
    }

    bool isCancelled() const {
        return state() == State::CANCELLED;
    }

    bool isDone() const {
        return d_->done.load(std::memory_order_relaxed);
    }

    Future<ResultT, ContextT>& onError(OnErrorCallback callback) {
        OnErrorCallback* callbackPtr = new OnErrorCallback(std::move(callback)); // New callback
        callbackPtr = d_->onError.exchange(callbackPtr, std::memory_order_release);
        delete callbackPtr; // Delete old callback
        // Ensure that the newly assigned callback is invoked for an already completed future
        if (d_->done.load(std::memory_order_acquire) && d_->state.load(std::memory_order_relaxed) == State::FAILED) {
            detail::invokeFutureCallback<ContextT>(d_->onError, d_->error);
        }
        return *static_cast<Future<ResultT, ContextT>*>(this);
    }

    static Future<ResultT, ContextT> makeFailed(Error error) {
        auto d = std::make_shared<detail::FutureData<ResultT>>(State::FAILED);
        d->error = std::move(error);
        return Future<ResultT, ContextT>(d);
    }

protected:
    detail::FutureDataPtr<ResultT> d_;
};

template<typename ResultT, typename ContextT = detail::FutureContext>
class Future: public FutureBase<ResultT, ContextT> {
public:
    using typename FutureBase<ResultT, ContextT>::State;
    using typename FutureBase<ResultT, ContextT>::OnSuccessCallback;

    using FutureBase<ResultT, ContextT>::FutureBase;

    ResultT result() const {
        if (!this->d_->done.load(std::memory_order_acquire) || this->d_->state.load(std::memory_order_relaxed) != State::SUCCEEDED) {
            return ResultT();
        }
        return this->d_->result;
    }

    Future<ResultT, ContextT>& onSuccess(OnSuccessCallback callback) {
        OnSuccessCallback* callbackPtr = new OnSuccessCallback(std::move(callback)); // New callback
        callbackPtr = this->d_->onSuccess.exchange(callbackPtr, std::memory_order_release);
        delete callbackPtr; // Delete old callback
        // Ensure that the newly assigned callback is invoked for an already completed future
        if (this->d_->done.load(std::memory_order_acquire) && this->d_->state.load(std::memory_order_relaxed) == State::SUCCEEDED) {
            detail::invokeFutureCallback<ContextT>(this->d_->onSuccess, this->d_->result);
        }
        return *this;
    }
};

template<typename ContextT>
class Future<void, ContextT>: public FutureBase<void, ContextT> {
public:
    using typename FutureBase<void, ContextT>::State;
    using typename FutureBase<void, ContextT>::OnSuccessCallback;

    using FutureBase<void, ContextT>::FutureBase;

    Future<void, ContextT>& onSuccess(OnSuccessCallback callback) {
        OnSuccessCallback* callbackPtr = new OnSuccessCallback(std::move(callback)); // New callback
        callbackPtr = this->d_->onSuccess.exchange(callbackPtr, std::memory_order_release);
        delete callbackPtr; // Delete old callback
        // Ensure that the newly assigned callback is invoked for an already completed future
        if (this->d_->done.load(std::memory_order_acquire) && this->d_->state.load(std::memory_order_relaxed) == State::SUCCEEDED) {
            detail::invokeFutureCallback<ContextT>(this->d_->onSuccess);
        }
        return *this;
    }
};

} // namespace spark

#endif // SPARK_WIRING_ASYNC_H

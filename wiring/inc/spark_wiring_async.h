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
#include "system_task.h"

#include <functional>
#include <memory>
#include <atomic>

#if (ATOMIC_POINTER_LOCK_FREE != 2) || (ATOMIC_CHAR_LOCK_FREE != 2) || (ATOMIC_BOOL_LOCK_FREE != 2)
#error "std::atomic is not always lock-free for required types"
#endif

namespace particle {

namespace detail {

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

// Helper function for FutureImplBase::invokeCallback()
void futureCallbackWrapper(void* data);

// Internal future implementation. Base class for FutureImpl
template<typename ResultT, typename ContextT>
class FutureImplBase {
public:
    // Future state
    enum class State: char {
        RUNNING,
        SUCCEEDED,
        FAILED,
        CANCELLED
    };

    // Completion callback types
    typedef typename detail::FutureCallbackTypes<ResultT>::OnSuccess OnSuccessCallback;
    typedef typename detail::FutureCallbackTypes<ResultT>::OnError OnErrorCallback;

    ~FutureImplBase() {
        delete onSuccess_.load(std::memory_order_relaxed);
        delete onError_.load(std::memory_order_relaxed);
    }

    bool wait(int timeout = 0) const {
        // TODO: Waiting for a future in a non-default application thread is not supported
        if (ContextT::isApplicationThreadCurrent()) {
            const system_tick_t t = (timeout > 0) ? millis() : 0;
            for (;;) {
                if (isDone()) { // We can use relaxed ordering here, as long as the future's result is not examined
                    return true;
                }
                if (timeout > 0 && millis() - t >= (system_tick_t)timeout) {
                    return false;
                }
                ContextT::processApplicationEvents();
            }
        }
        return false;
    }

    // This method attempts to switch the future into cancelled state and doesn't affect pending operation
    bool cancel() {
        if (changeState(State::CANCELLED)) {
            releaseDone();
            return true;
        }
        return false;
    }

    bool isSucceeded() const {
        wait();
        return state() == State::SUCCEEDED;
    }

    bool isFailed() const {
        wait();
        return state() == State::FAILED;
    }

    bool isCancelled() const {
        return state() == State::CANCELLED;
    }

    bool isDone() const {
        return done_.load(std::memory_order_relaxed);
    }

protected:
    std::atomic<State> state_; // Future state
    std::atomic<bool> done_; // Flag signaling that future is in a final state
    std::atomic<typename FutureCallbackTypes<ResultT>::OnSuccess*> onSuccess_; // User callback for succeeded operation
    std::atomic<typename FutureCallbackTypes<ResultT>::OnError*> onError_; // User callback for failed operation

    explicit FutureImplBase(State state) :
            state_(state),
            done_(state != State::RUNNING),
            onSuccess_(nullptr),
            onError_(nullptr) {
    }

    bool changeState(State state) {
        State s = State::RUNNING; // Expected state
        return state_.compare_exchange_strong(s, state, std::memory_order_relaxed);
    }

    void releaseDone() {
        done_.store(true, std::memory_order_release);
    }

    bool acquireDone() const {
        return done_.load(std::memory_order_acquire);
    }

    State state() const {
        return state_.load(std::memory_order_relaxed);
    }

    template<typename FunctionT>
    static void setCallback(std::atomic<std::function<FunctionT>*>& wrapper, std::function<FunctionT>&& callback) {
        auto callbackPtr = new std::function<FunctionT>(callback); // New callback
        callbackPtr = wrapper.exchange(callbackPtr, std::memory_order_acq_rel);
        delete callbackPtr; // Delete old callback
    }

    // Takes a callback from its atomic wrapper and invokes it
    template<typename FunctionT, typename... ArgsT>
    static void invokeCallback(std::atomic<std::function<FunctionT>*>& wrapper, ArgsT&&... args) {
        std::function<FunctionT>* callbackPtr = wrapper.exchange(nullptr, std::memory_order_acq_rel);
        if (callbackPtr) {
            invokeCallback(*callbackPtr, std::forward<ArgsT>(args)...);
            delete callbackPtr;
        }
    }

    // Invokes std::function in the application context
    template<typename FunctionT, typename... ArgsT>
    static void invokeCallback(const std::function<FunctionT>& callback, ArgsT&&... args) {
        if (ContextT::isApplicationThreadCurrent()) {
            callback(std::forward<ArgsT>(args)...); // Synchronous call
        } else {
            // Bind all arguments and wrap resulting function into a pointer
            auto callbackPtr = new std::function<void()>(std::bind(callback, std::forward<ArgsT>(args)...));
            ContextT::invokeApplicationCallback(futureCallbackWrapper, callbackPtr);
        }
    }
};

// Internal future implementation
template<typename ResultT, typename ContextT>
class FutureImpl: public FutureImplBase<ResultT, ContextT> {
public:
    using typename FutureImplBase<ResultT, ContextT>::State;
    using typename FutureImplBase<ResultT, ContextT>::OnSuccessCallback;
    using typename FutureImplBase<ResultT, ContextT>::OnErrorCallback;

    explicit FutureImpl(State state) :
            FutureImplBase<ResultT, ContextT>(state) {
    }

    explicit FutureImpl(ResultT result) :
            FutureImplBase<ResultT, ContextT>(State::SUCCEEDED),
            result_(std::move(result)) {
    }

    explicit FutureImpl(Error error) :
            FutureImplBase<ResultT, ContextT>(State::FAILED),
            error_(std::move(error)) {
    }

    explicit FutureImpl(Error::Type error) :
            FutureImpl(Error(error)) {
    }

    ~FutureImpl() {
        // Call destructor of the appropriate unnamed enum's field
        const State s = this->state();
        if (s == State::SUCCEEDED) {
            result_.~ResultT();
        } else if (s == State::FAILED) {
            error_.~Error();
        }
    }

    void setResult(ResultT result) {
        if (this->changeState(State::SUCCEEDED)) {
            new(&result_) ResultT(std::move(result));
            this->releaseDone();
            this->invokeCallback(this->onSuccess_, result_);
        }
    }

    ResultT result(ResultT defaultValue = ResultT()) const {
        this->wait();
        if (this->acquireDone() && this->isSucceeded()) {
            return result_;
        }
        return std::move(defaultValue);
    }

    void setError(Error error) {
        if (this->changeState(State::FAILED)) {
            new(&error_) Error(std::move(error));
            this->releaseDone();
            this->invokeCallback(this->onError_, error_);
        }
    }

    Error error() const {
        this->wait();
        if (this->acquireDone() && this->isFailed()) {
            return error_;
        }
        return Error::NONE;
    }

    void onSuccess(OnSuccessCallback callback) {
        this->setCallback(this->onSuccess_, std::move(callback));
        // Ensure that the newly assigned callback is invoked for already completed future
        if (this->acquireDone() && this->isSucceeded()) {
            this->invokeCallback(this->onSuccess_, result_);
        }
    }

    void onError(OnErrorCallback callback) {
        this->setCallback(this->onError_, std::move(callback));
        if (this->acquireDone() && this->isFailed()) {
            this->invokeCallback(this->onError_, error_);
        }
    }

private:
    union {
        ResultT result_;
        Error error_;
    };
};

// Internal future implementation. Specialization for void result type
template<typename ContextT>
class FutureImpl<void, ContextT>: public FutureImplBase<void, ContextT> {
public:
    using typename FutureImplBase<void, ContextT>::State;
    using typename FutureImplBase<void, ContextT>::OnSuccessCallback;
    using typename FutureImplBase<void, ContextT>::OnErrorCallback;

    explicit FutureImpl(State state) :
            FutureImplBase<void, ContextT>(state) {
    }

    explicit FutureImpl(Error error) :
            FutureImplBase<void, ContextT>(State::FAILED),
            error_(std::move(error)) {
    }

    explicit FutureImpl(Error::Type error) :
            FutureImpl(Error(error)) {
    }

    void setResult() {
        if (this->changeState(State::SUCCEEDED)) {
            this->releaseDone();
            this->invokeCallback(this->onSuccess_);
        }
    }

    void setError(Error error) {
        if (this->changeState(State::FAILED)) {
            error_ = std::move(error);
            this->releaseDone();
            this->invokeCallback(this->onError_, error_);
        }
    }

    Error error() const {
        this->wait();
        if (this->acquireDone() && this->isFailed()) {
            return error_;
        }
        return Error::NONE;
    }

    void onSuccess(OnSuccessCallback callback) {
        this->setCallback(this->onSuccess_, std::move(callback));
        // Ensure that the newly assigned callback is invoked for already completed future
        if (this->acquireDone() && this->isSucceeded()) {
            this->invokeCallback(this->onSuccess_);
        }
    }

    void onError(OnErrorCallback callback) {
        this->setCallback(this->onError_, std::move(callback));
        if (this->acquireDone() && this->isFailed()) {
            this->invokeCallback(this->onError_, error_);
        }
    }

private:
    Error error_;
};

template<typename ResultT, typename ContextT>
using FutureImplPtr = std::shared_ptr<FutureImpl<ResultT, ContextT>>;

// Event loop and threading abstraction. Used for unit testing
struct FutureContext {
    // Runs the application's event loop
    static void processApplicationEvents() {
        spark_process();
    }

    // Asynchronously invokes a callback in the application context
    static bool invokeApplicationCallback(void (*callback)(void* data), void* data) {
        return (application_thread_invoke(callback, data, nullptr) == 0);
    }

    // Returns true if current thread is the application thread
    static bool isApplicationThreadCurrent() {
        return (application_thread_current(nullptr) != 0);
    }
};

} // namespace particle::detail

template<typename ResultT, typename ContextT>
class Future;

template<typename ResultT, typename ContextT>
class Promise;

// Base class for Promise. Promise allows to store result of an asynchronous operation that later
// can be acquired via Future
template<typename ResultT, typename ContextT>
class PromiseBase {
public:
    PromiseBase() :
            p_(new detail::FutureImpl<ResultT, ContextT>(State::RUNNING)) {
    }

    explicit PromiseBase(detail::FutureImplPtr<ResultT, ContextT> ptr) :
            p_(std::move(ptr)) {
    }

    void setError(Error error) {
        p_->setError(std::move(error));
    }

    bool isDone() const {
        return p_->isDone();
    }

    Future<ResultT, ContextT> future() const {
        return Future<ResultT, ContextT>(p_);
    }

    // Wraps this promise into an object pointer that can be passed to a C function
    void* dataPtr() const {
        // TODO: Use custom reference counting object to avoid unnecessary memory allocation
        return new detail::FutureImplPtr<ResultT, ContextT>(p_);
    }

    // Unwraps promise from an object pointer created via dataPtr() method
    static Promise<ResultT, ContextT> fromDataPtr(void* data) {
        auto d = static_cast<detail::FutureImplPtr<ResultT, ContextT>*>(data);
        const Promise<ResultT, ContextT> p(*d);
        delete d;
        return p;
    }

protected:
    typedef typename detail::FutureImpl<ResultT, ContextT>::State State;

    detail::FutureImplPtr<ResultT, ContextT> p_;
};

template<typename ResultT, typename ContextT = detail::FutureContext>
class Promise: public PromiseBase<ResultT, ContextT> {
public:
    using PromiseBase<ResultT, ContextT>::PromiseBase;

    void setResult(ResultT result) {
        this->p_->setResult(std::move(result));
    }

    // System completion callback (see services/inc/completion_handler.h). This function is provided for
    // convenience, do not use it with complex result types that require ABI compatibility checks
    static void defaultCallback(int error, const void* data, void* callbackData, void* reserved) {
        auto p = Promise<ResultT, ContextT>::fromDataPtr(callbackData);
        if (error != Error::NONE) {
            p.setError(Error((Error::Type)error, (const char*)data));
        } else if (data) {
            p.setResult(*static_cast<const ResultT*>(data));
        } else {
            p.setResult(ResultT());
        }
    }
};

// Specialization for void result type
template<typename ContextT>
class Promise<void, ContextT>: public PromiseBase<void, ContextT> {
public:
    using PromiseBase<void, ContextT>::PromiseBase;

    void setResult() {
        this->p_->setResult();
    }

    static void defaultCallback(int error, const void* data, void* callbackData, void* reserved) {
        auto p = Promise<void, ContextT>::fromDataPtr(callbackData);
        if (error != Error::NONE) {
            p.setError(Error((Error::Type)error, (const char*)data));
        } else {
            p.setResult();
        }
    }
};

// Base class for Future. Future allows to access result of an asynchronous operation
template<typename ResultT, typename ContextT>
class FutureBase {
public:
    // Completion callback types
    typedef typename detail::FutureImpl<ResultT, ContextT>::OnSuccessCallback OnSuccessCallback;
    typedef typename detail::FutureImpl<ResultT, ContextT>::OnErrorCallback OnErrorCallback;

    // Construct failed future
    explicit FutureBase(Error error) :
            p_(new detail::FutureImpl<ResultT, ContextT>(std::move(error))) {
    }

    explicit FutureBase(Error::Type error) :
            FutureBase(Error(error)) {
    }

    explicit FutureBase(detail::FutureImplPtr<ResultT, ContextT> ptr) :
            p_(std::move(ptr)) {
    }

    bool wait(int timeout = 0) const {
        return p_->wait(timeout);
    }

    bool cancel() {
        return p_->cancel();
    }

    Error error() const {
        return p_->error();
    }

    bool isSucceeded() const {
        return p_->isSucceeded();
    }

    bool isFailed() const {
        return p_->isFailed();
    }

    bool isCancelled() const {
        return p_->isCancelled();
    }

    bool isDone() const {
        return p_->isDone();
    }

    Future<ResultT, ContextT>& onSuccess(OnSuccessCallback callback) {
        p_->onSuccess(std::move(callback));
        return *static_cast<Future<ResultT, ContextT>*>(this);
    }

    Future<ResultT, ContextT>& onError(OnErrorCallback callback) {
        p_->onError(std::move(callback));
        return *static_cast<Future<ResultT, ContextT>*>(this);
    }

protected:
    typedef typename detail::FutureImpl<ResultT, ContextT>::State State;

    detail::FutureImplPtr<ResultT, ContextT> p_;
};

template<typename ResultT, typename ContextT = detail::FutureContext>
class Future: public FutureBase<ResultT, ContextT> {
public:
    using FutureBase<ResultT, ContextT>::FutureBase;

    // Constructs succeeded future
    explicit Future(ResultT result = ResultT()) :
            FutureBase<ResultT, ContextT>(std::make_shared<detail::FutureImpl<ResultT, ContextT>>(std::move(result))) {
    }

    ResultT result() const {
        return this->p_->result();
    }

    ResultT result(ResultT defaultValue) const {
        return this->p_->result(std::move(defaultValue));
    }

    operator ResultT() const {
        return result();
    }
};

// Specialization for void result type
template<typename ContextT>
class Future<void, ContextT>: public FutureBase<void, ContextT> {
public:
    using FutureBase<void, ContextT>::FutureBase;

    // Constructs succeeded future
    Future() :
            FutureBase<void, ContextT>(std::make_shared<detail::FutureImpl<void, ContextT>>(State::SUCCEEDED)) {
    }

private:
    using typename FutureBase<void, ContextT>::State;
};

// Helper class that can be used to make existent functions, that use their own special return
// values for error handling, asynchronous in an API-compatible way
template<typename ResultT, ResultT defaultValue, typename ContextT = detail::FutureContext>
class AdaptedFuture: public Future<ResultT, ContextT> {
public:
    using Future<ResultT, ContextT>::Future;

    explicit AdaptedFuture(ResultT result = ResultT()) :
            Future<ResultT, ContextT>(std::move(result)) {
    }

    AdaptedFuture(Future<ResultT, ContextT> future) :
            Future<ResultT, ContextT>(std::move(future)) {
    }

    ResultT result() const {
        return this->p_->result(defaultValue);
    }

    operator ResultT() const {
        return result();
    }
};

} // namespace particle

#endif // SPARK_WIRING_ASYNC_H

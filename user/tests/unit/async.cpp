#include "spark_wiring_async.h"

#include "completion_handler.h"

#include "tools/catch.h"

#include <boost/optional.hpp>

#include <thread>
#include <deque>

namespace {

using namespace particle;

// Event loop and threading abstraction
class Context {
public:
    typedef std::function<void()> Event;

    void processEvents() {
        if (!events_.empty()) {
            Event event = events_.front();
            events_.pop_front();
            event();
        }
    }

    Context& postEvent(Event event) {
        events_.push_back(std::move(event));
        return *this;
    }

    void reset() {
        events_.clear();
    }

    static Context* instance() {
        static Context ctx;
        return &ctx;
    }

    // Methods called by the Future implementation
    static void processApplicationEvents() {
        instance()->processEvents();
    }

    static void invokeApplicationCallback(void (*callback)(void* data), void* data) {
        instance()->postEvent([=]() {
            callback(data);
        });
    }

    static bool isApplicationThreadCurrent() {
        return true; // TODO
    }

private:
    std::deque<Event> events_;
};

template<typename ResultT>
using Future = particle::Future<ResultT, Context>;

template<typename ResultT, ResultT defaultValue>
using AdaptedFuture = particle::AdaptedFuture<ResultT, defaultValue, Context>;

template<typename ResultT>
using Promise = particle::Promise<ResultT, Context>;

inline void postEvent(Context::Event event) {
    Context::instance()->postEvent(std::move(event));
}

inline void resetContext() {
    Context::instance()->reset();
}

// Helper class wrapping CompletionHandler's result data
template<typename ResultT>
class CompletionData {
public:
    CompletionData() = default;

    ResultT result() const {
        return result_.value_or(ResultT());
    }

    bool hasResult() const {
        return (bool)result_;
    }

    Error error() const {
        return error_.value_or(Error::NONE);
    }

    bool hasError() const {
        return (bool)error_;
    }

    CompletionHandler handler() {
        return CompletionHandler(callback, this);
    }

    void reset() {
        result_ = boost::none;
        error_ = boost::none;
    }

private:
    boost::optional<ResultT> result_;
    boost::optional<Error> error_;

    // Completion callback
    static void callback(int error, const void* data, void* callback_data, void* reserved) {
        auto d = static_cast<CompletionData*>(callback_data);
        if (error != Error::NONE) {
            d->error_ = Error((Error::Type)error, (const char*)data);
            d->result_ = boost::none;
        } else {
            d->result_ = data ? *static_cast<const ResultT*>(data) : ResultT();
            d->error_ = boost::none;
        }
    }
};

} // namespace

TEST_CASE("Future<void>") {
    using Future = ::Future<void>;
    using Promise = ::Promise<void>;

    resetContext();

    SECTION("constructing succeeded future") {
        Future f;
        CHECK(f.isDone() == true);
        CHECK(f.isSucceeded() == true);
        CHECK(f.isFailed() == false);
        CHECK(f.isCancelled() == false);
        CHECK(f.error() == Error::NONE);
    }

    SECTION("constructing failed future") {
        Future f(Error::UNKNOWN);
        CHECK(f.isDone() == true);
        CHECK(f.isSucceeded() == false);
        CHECK(f.isFailed() == true);
        CHECK(f.isCancelled() == false);
        CHECK(f.error() == Error::UNKNOWN);
    }

    SECTION("constructing future via promise") {
        Promise p;
        Future f = p.future();
        CHECK(f.isDone() == false);
        CHECK(f.isCancelled() == false);
    }

    SECTION("making succeeded future via promise") {
        Promise p;
        Future f = p.future();
        p.setResult();
        CHECK(f.isDone() == true);
        CHECK(f.isSucceeded() == true);
        CHECK(f.isFailed() == false);
        CHECK(f.isCancelled() == false);
        CHECK(f.error() == Error::NONE);
    }

    SECTION("making failed future via promise") {
        Promise p;
        Future f = p.future();
        p.setError(Error::UNKNOWN);
        CHECK(f.isDone() == true);
        CHECK(f.isSucceeded() == false);
        CHECK(f.isFailed() == true);
        CHECK(f.isCancelled() == false);
        CHECK(f.error() == Error::UNKNOWN);
    }

    SECTION("waiting for succeeded operation") {
        // Explicit waiting via Future::wait()
        Promise p1;
        Future f1 = p1.future();
        postEvent([&p1]() {
            p1.setResult();
        });
        f1.wait();
        CHECK(f1.isSucceeded() == true);
        // Future::isSucceeded() waits until future is completed
        Promise p2;
        Future f2 = p2.future();
        postEvent([&p2]() {
            p2.setResult();
        });
        CHECK(f2.isSucceeded() == true);
    }

    SECTION("waiting for failed operation") {
        // Explicit waiting via Future::wait()
        Promise p1;
        Future f1 = p1.future();
        postEvent([&p1]() {
            p1.setError(Error::UNKNOWN);
        });
        f1.wait();
        CHECK(f1.isFailed() == true);
        CHECK(f1.error() == Error::UNKNOWN);
        // Future::error() waits until future is completed
        Promise p2;
        Future f2 = p2.future();
        postEvent([&p2]() {
            p2.setError(Error::UNKNOWN);
        });
        CHECK(f2.error() == Error::UNKNOWN);
        // Future::isFailed() waits until future is completed
        Promise p3;
        Future f3 = p3.future();
        postEvent([&p3]() {
            p3.setError(Error::UNKNOWN);
        });
        CHECK(f3.isFailed() == true);
    }

    SECTION("waiting for timed out operation") {
        Promise p;
        Future f = p.future();
        CHECK(f.wait(50) == false); // 50 ms
        CHECK(f.isDone() == false);
    }

    SECTION("callback for succeeded operation") {
        Promise p;
        Future f = p.future();
        bool called = false;
        f.onSuccess([&called]() {
            called = true;
        });
        p.setResult();
        CHECK(called == true);
    }

    SECTION("callback for failed operation") {
        Promise p;
        Future f = p.future();
        Error error(Error::NONE);
        f.onError([&error](Error e) {
            error = e;
        });
        p.setError(Error::UNKNOWN);
        CHECK(error == Error::UNKNOWN);
    }

    SECTION("callbacks are invoked immediately for already completed future") {
        // Succeeded operation
        Future f1;
        bool called = false;
        f1.onSuccess([&called]() {
            called = true;
        });
        CHECK(called == true);
        // Failed operation
        Future f2(Error::UNKNOWN);
        Error error(Error::NONE);
        f2.onError([&error](Error e) {
            error = e;
        });
        CHECK(error == Error::UNKNOWN);
    }

    SECTION("callbacks are not invoked for cancelled future") {
        // Succeeded operation
        Promise p1;
        Future f1 = p1.future();
        bool called = false;
        f1.onSuccess([&called]() {
            called = true;
        });
        CHECK(f1.cancel() == true);
        p1.setResult();
        CHECK(called == false);
        // Failed operation
        Promise p2;
        Future f2 = p2.future();
        Error error(Error::NONE);
        f2.onError([&error](Error e) {
            error = e;
        });
        CHECK(f2.cancel() == true);
        p2.setError(Error::UNKNOWN);
        CHECK(error == Error::NONE);
    }
}

TEST_CASE("Future<int>") {
    using Future = ::Future<int>;
    using Promise = ::Promise<int>;

    resetContext();

    SECTION("constructing succeeded future") {
        Future f1;
        CHECK(f1.isSucceeded() == true);
        CHECK(f1.result() == 0); // Default-constructed value
        CHECK(f1.result(1) == 0); // User-provided default value is ignored
        CHECK(f1 == 0); // Implicit conversion
        Future f2(1);
        CHECK(f2.result() == 1); // User-provided value
        CHECK(f2 == 1);
    }

    SECTION("constructing failed future") {
        Future f(Error::UNKNOWN);
        CHECK(f.isFailed() == true);
        CHECK(f.result() == 0); // Default-constructed value
        CHECK(f.result(1) == 1); // User-provided default value
    }

    SECTION("making succeeded future via promise") {
        Promise p;
        Future f = p.future();
        p.setResult(1);
        CHECK(f.isSucceeded() == true);
        CHECK(f.result() == 1);
    }

    SECTION("making failed future via promise") {
        Promise p;
        Future f = p.future();
        p.setError(Error::UNKNOWN);
        CHECK(f.isFailed() == true);
        CHECK(f.result() == 0); // Default-constructed value
    }

    SECTION("waiting for succeeded operation") {
        // Explicit waiting via Future::wait()
        Promise p1;
        Future f1 = p1.future();
        postEvent([&p1]() {
            p1.setResult(1);
        });
        f1.wait();
        CHECK(f1.isSucceeded() == true);
        CHECK(f1.result() == 1);
        // Future::result() waits until future is completed
        Promise p2;
        Future f2 = p2.future();
        postEvent([&p2]() {
            p2.setResult(1);
        });
        CHECK(f1.result() == 1);
        // Conversion operator waits until future is completed
        Promise p3;
        Future f3 = p3.future();
        postEvent([&p3]() {
            p3.setResult(1);
        });
        CHECK((int)f3 == 1);
    }

    SECTION("callback for succeeded operation") {
        Promise p;
        Future f = p.future();
        int result = 0;
        f.onSuccess([&result](int r) {
            result = r;
        });
        p.setResult(1);
        CHECK(result == 1);
    }
}

TEST_CASE("AdaptedFuture<int>") {
    using Future = ::Future<int>;
    using AdaptedFuture = ::AdaptedFuture<int, 1>; // Default value is 1

    SECTION("constructing failed future") {
        AdaptedFuture af(Error::UNKNOWN);
        CHECK(af.isFailed() == true);
        CHECK(af.result() == 1); // User-defined default value
        CHECK(af == 1); // Implicit conversion
    }

    SECTION("converting from basic future") {
        // Copying succeeded future
        Future f1;
        AdaptedFuture af1(f1);
        CHECK(af1.isSucceeded() == true);
        CHECK(af1.result() == 0); // Original value
        // Assigning failed future
        Future f2(Error::UNKNOWN);
        AdaptedFuture af2 = f2;
        af2 = f2;
        CHECK(af2.isFailed() == true);
        CHECK(af2.result() == 1); // User-defined default value
    }

    SECTION("converting to basic future") {
        // Copying succeeded future
        AdaptedFuture af1;
        Future f1(af1);
        CHECK(f1.isSucceeded() == true);
        CHECK(f1.result() == 0); // Original value
        // Assigning failed future
        AdaptedFuture af2(Error::UNKNOWN);
        Future f2;
        f2 = af2;
        CHECK(f2.isFailed() == true);
        CHECK(f2.result() == 0); // Information about user-defined default value has been lost
    }
}

TEST_CASE("CompletionHandler") {
    SECTION("using default-constructed handler") {
        CHECK((bool)CompletionHandler() == false);
        CompletionHandler().setResult(); // Shouldn't crash :)
        CompletionHandler().setError(Error::UNKNOWN); // ditto
    }

    SECTION("invoking handler with a result data") {
        CompletionData<int> d;
        CompletionHandler h = d.handler();
        CHECK((bool)h == true);
        h.setResult(1);
        CHECK(d.result() == 1);
        CHECK(d.hasError() == false);
    }

    SECTION("invoking handler with an error") {
        CompletionData<int> d1, d2;
        CompletionHandler h1 = d1.handler();
        h1.setError(Error::UNKNOWN); // Set error code
        Error e1 = d1.error();
        CHECK(e1.type() == Error::UNKNOWN);
        CHECK(d1.hasResult() == false);
        CompletionHandler h2 = d2.handler();
        h2.setError(Error::INTERNAL, "abc"); // Set error code and message
        Error e2 = d2.error();
        CHECK(e2.type() == Error::INTERNAL);
        CHECK(strcmp(e2.message(), "abc") == 0);
    }

    SECTION("handler can be invoked only once") {
        // Invoking already completed handler with an error code
        CompletionData<int> d1;
        CompletionHandler h1 = d1.handler();
        h1.setResult(1);
        h1.setError(Error::UNKNOWN);
        CHECK(d1.result() == 1);
        CHECK(d1.hasError() == false);
        // Invoking already completed handler with a result data
        CompletionData<int> d2;
        CompletionHandler h2 = d2.handler();
        h2.setError(Error::UNKNOWN);
        h2.setResult(1);
        CHECK(d2.hasResult() == false);
        CHECK(d2.error() == Error::UNKNOWN);
    }

    SECTION("handler instance can be moved via move constructor") {
        CompletionData<int> d;
        CompletionHandler h1 = d.handler();
        CompletionHandler h2(std::move(h1));
        h1.setResult();
        CHECK(d.hasResult() == false); // Callback ownership has been transferred to h2
        h2.setResult();
        CHECK(d.hasResult() == true);
    }

    SECTION("handler instance can be moved via move assignment") {
        CompletionData<int> d;
        CompletionHandler h1 = d.handler();
        CompletionHandler h2 = d.handler();
        h1 = std::move(h2);
        // It's an internal error if a completion handler wasn't invoked during its lifetime
        CHECK(d.error() == Error::INTERNAL);
        h1.setError(Error::UNKNOWN);
        CHECK(d.error() == Error::UNKNOWN);
        h2.setResult();
        CHECK(d.hasResult() == false); // Callback ownership has been transferred to h1
    }

    SECTION("completion callback is invoked on handler destruction") {
        CompletionData<int> d;
        {
            CompletionHandler h = d.handler();
        }
        CHECK(d.error() == Error::INTERNAL);
    }
}

TEST_CASE("CompletionHandlerList") {
    SECTION("constructing handler list") {
        CompletionHandlerList l;
        CHECK(l.size() == 0);
        CHECK(l.isEmpty() == true);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
    }

    SECTION("adding handlers") {
        CompletionHandlerList l;
        // Adding an invalid handler
        CHECK(l.addHandler(CompletionHandler()) == false); // Returns false
        CHECK(l.size() == 0); // Invalid handlers are ignored
        CHECK(l.isEmpty() == true);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
        // Adding more handlers
        CompletionData<int> d1, d2, d3;
        CHECK(l.addHandler(d1.handler(), 20) == true); // Handler 1, timeout: 20
        CHECK(l.size() == 1);
        CHECK(l.isEmpty() == false);
        CHECK(l.nearestTimeout() == 20); // Handler 1 is a nearest handler to expire
        CHECK(l.addHandler(d2.handler(), 10) == true); // Handler 2, timeout: 10
        CHECK(l.size() == 2);
        CHECK(l.isEmpty() == false);
        CHECK(l.nearestTimeout() == 10); // Handler 2 is a nearest handler to expire
        CHECK(l.addHandler(d3.handler(), 30) == true); // Handler 3, timeout: 30
        CHECK(l.size() == 3);
        CHECK(l.isEmpty() == false);
        CHECK(l.nearestTimeout() == 10); // Handler 2 is still a nearest handler to expire
    }

    SECTION("clearing handler list") {
        CompletionHandlerList l;
        CompletionData<int> d1, d2;
        CHECK(l.addHandler(d1.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(l.addHandler(d2.handler(), 20) == true); // Handler 2, timeout: 20
        l.clear();
        // All handlers should have been invoked with ABORTED error
        CHECK(d1.error() == Error::ABORTED);
        CHECK(d2.error() == Error::ABORTED);
        CHECK(l.size() == 0);
        CHECK(l.isEmpty() == true);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
    }

    SECTION("setting result data") {
        CompletionHandlerList l;
        CompletionData<int> d1, d2;
        CHECK(l.addHandler(d1.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(l.addHandler(d2.handler(), 20) == true); // Handler 2, timeout: 20
        l.setResult(1);
        CHECK(d1.result() == 1);
        CHECK(d2.result() == 1);
        CHECK(l.size() == 0);
        CHECK(l.isEmpty() == true);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
    }

    SECTION("setting error code") {
        CompletionHandlerList l;
        CompletionData<int> d1, d2;
        CHECK(l.addHandler(d1.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(l.addHandler(d2.handler(), 20) == true); // Handler 2, timeout: 20
        l.setError(Error::UNKNOWN);
        CHECK(d1.error() == Error::UNKNOWN);
        CHECK(d2.error() == Error::UNKNOWN);
        CHECK(l.size() == 0);
        CHECK(l.isEmpty() == true);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
    }

    SECTION("waiting for handlers expiration") {
        CompletionHandlerList l;
        CompletionData<int> d1, d2;
        l.addHandler(d1.handler(), 10); // Handler 1, timeout: 10
        l.addHandler(d2.handler(), 20); // Handler 2, timeout: 20
        CHECK(l.update(5) == 0); // No handlers have expired
        CHECK(l.size() == 2);
        CHECK(l.nearestTimeout() == 5); // Handler 1 is a nearest handler to expire
        CHECK(l.update(5) == 1); // 1 handler has expired (handler 1)
        CHECK(d1.error() == Error::TIMEOUT);
        CHECK(d2.hasError() == false);
        CHECK(l.size() == 1);
        CHECK(l.nearestTimeout() == 10); // Handler 2 is a nearest handler to expire
        CHECK(l.update(15) == 1); // 1 handler has expired (handler 2)
        CHECK(d2.error() == Error::TIMEOUT);
        CHECK(l.size() == 0);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
    }

    SECTION("modifying handler list while waiting for handlers expiration") {
        CompletionHandlerList l;
        CompletionData<int> d1, d2;
        l.addHandler(d1.handler(), 10); // Handler 1, timeout: 10
        l.addHandler(d2.handler(), 20); // Handler 2, timeout: 20
        CHECK(l.update(5) == 0); // No handlers have expired
        CHECK(l.size() == 2);
        CHECK(l.nearestTimeout() == 5); // Handler 1 is a nearest handler to expire
        CompletionData<int> d3, d4;
        l.addHandler(d3.handler(), 0); // Handler 3, timeout: 0
        l.addHandler(d4.handler(), 20); // Handler 4, timeout: 20
        CHECK(l.nearestTimeout() == 0); // Handler 3 is a nearest handler to expire
        CHECK(l.update(10) == 2); // 2 handlers have expired (handlers 1 and 3)
        CHECK(d1.error() == Error::TIMEOUT);
        CHECK(d3.error() == Error::TIMEOUT);
        CHECK(l.size() == 2);
        CHECK(l.nearestTimeout() == 5); // Handler 2 is a nearest handler to expire
        l.setResult(1); // Set result for handlers 2 and 4
        CHECK(d2.result() == 1);
        CHECK(d4.result() == 1);
        CHECK(l.size() == 0);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
        CompletionData<int> d5, d6;
        l.addHandler(d5.handler(), 10); // Handler 5, timeout: 10
        l.addHandler(d6.handler(), 20); // Handler 6, timeout: 20
        l.setError(Error::UNKNOWN); // Set error for handlers 5 and 6
        CHECK(d5.error() == Error::UNKNOWN);
        CHECK(d6.error() == Error::UNKNOWN);
        CHECK(l.size() == 0);
        CHECK(l.nearestTimeout() == CompletionHandlerList::MAX_TIMEOUT);
    }
}

TEST_CASE("CompletionHandlerMap") {
    using CompletionHandlerMap = ::CompletionHandlerMap<int>;

    SECTION("constructing handler map") {
        CompletionHandlerMap m;
        CHECK(m.size() == 0);
        CHECK(m.isEmpty() == true);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("adding and removing handlers") {
        CompletionHandlerMap m;
        // Adding an invalid handler
        CHECK(m.addHandler(1, CompletionHandler()) == false); // Returns false
        CHECK(m.size() == 0); // Invalid handlers are ignored
        CHECK(m.isEmpty() == true);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
        // Adding more handlers
        CompletionData<int> d1, d2, d3;
        CHECK(m.addHandler(3, d1.handler(), 30) == true); // Handler 3, timeout: 30
        CHECK(m.addHandler(1, d2.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(m.addHandler(2, d3.handler(), 20) == true); // Handler 2, timeout: 20
        CHECK((m.hasHandler(1) && m.hasHandler(2) && m.hasHandler(3)));
        CHECK(m.size() == 3);
        CHECK(m.isEmpty() == false);
        CHECK(m.nearestTimeout() == 10); // Handler 1 is a nearest handler to expire
        // Removing handler 1
        m.takeHandler(1);
        CHECK((!m.hasHandler(1) && m.hasHandler(2) && m.hasHandler(3)));
        CHECK(m.size() == 2);
        CHECK(m.isEmpty() == false);
        CHECK(m.nearestTimeout() == 20); // Handler 2 is a nearest handler to expire
        // Removing handler 2
        m.takeHandler(2);
        CHECK((!m.hasHandler(1) && !m.hasHandler(2) && m.hasHandler(3)));
        CHECK(m.size() == 1);
        CHECK(m.isEmpty() == false);
        CHECK(m.nearestTimeout() == 30); // Handler 3 is a nearest handler to expire
        // Removing handler 3
        m.takeHandler(3);
        CHECK((!m.hasHandler(1) && !m.hasHandler(2) && !m.hasHandler(3)));
        CHECK(m.size() == 0);
        CHECK(m.isEmpty() == true);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("clearing handler map") {
        CompletionHandlerMap m;
        CompletionData<int> d1, d2;
        m.addHandler(1, d1.handler(), 10); // Handler 1, timeout: 10
        m.addHandler(2, d2.handler(), 20); // Handler 2, timeout: 20
        m.clear();
        // Both handlers should have been invoked with ABORTED error
        CHECK(d1.error() == Error::ABORTED);
        CHECK(d2.error() == Error::ABORTED);
        CHECK(m.size() == 0);
        CHECK(m.isEmpty() == true);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("setting result data") {
        CompletionHandlerMap m;
        CompletionData<int> d1, d2;
        CHECK(m.addHandler(1, d1.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(m.addHandler(2, d2.handler(), 20) == true); // Handler 2, timeout: 20
        m.setResult(2, 1); // Set result for handler 2
        CHECK(d2.result() == 1);
        CHECK(d1.hasResult() == false);
        CHECK(m.size() == 1);
        CHECK(m.nearestTimeout() == 10); // Handler 1 is a nearest handler to expire
        m.setResult(1, 1); // Set result for handler 1
        CHECK(d1.result() == 1);
        CHECK(m.size() == 0);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("setting error code") {
        CompletionHandlerMap m;
        CompletionData<int> d1, d2;
        CHECK(m.addHandler(1, d1.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(m.addHandler(2, d2.handler(), 20) == true); // Handler 2, timeout: 20
        m.setError(2, Error::UNKNOWN); // Set error for handler 2
        CHECK(d2.error() == Error::UNKNOWN);
        CHECK(d1.hasError() == false);
        CHECK(m.size() == 1);
        CHECK(m.nearestTimeout() == 10); // Handler 1 is a nearest handler to expire
        m.setError(1, Error::UNKNOWN); // Set error for handler 1
        CHECK(d1.error() == Error::UNKNOWN);
        CHECK(m.size() == 0);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("waiting for handlers expiration") {
        CompletionHandlerMap m;
        CompletionData<int> d1, d2;
        m.addHandler(1, d1.handler(), 10); // Handler 1, timeout: 10
        m.addHandler(2, d2.handler(), 20); // Handler 2, timeout: 20
        CHECK(m.update(5) == 0); // No handlers have expired
        CHECK((m.hasHandler(1) && m.hasHandler(2)));
        CHECK(m.size() == 2);
        CHECK(m.nearestTimeout() == 5);
        CHECK(m.update(5) == 1); // 1 handler has expired (handler 1)
        CHECK(d1.error() == Error::TIMEOUT);
        CHECK(d2.hasError() == false);
        CHECK((!m.hasHandler(1) && m.hasHandler(2)));
        CHECK(m.size() == 1);
        CHECK(m.nearestTimeout() == 10); // Handler 2 is a nearest handler to expire
        CHECK(m.update(15) == 1);
        CHECK(d2.error() == Error::TIMEOUT);
        CHECK((!m.hasHandler(1) && !m.hasHandler(2)));
        CHECK(m.size() == 0);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("modifying handler map while waiting for handlers expiration") {
        CompletionHandlerMap m;
        CompletionData<int> d1, d2, d3, d4, d5, d6;
        m.addHandler(1, d1.handler(), 10); // Handler 1, timeout: 10
        m.addHandler(2, d2.handler(), 20); // Handler 2, timeout: 20
        m.addHandler(3, d3.handler(), 30); // Handler 3, timeout: 30
        m.addHandler(4, d4.handler(), 40); // Handler 4, timeout: 40
        m.addHandler(5, d5.handler(), 50); // Handler 5, timeout: 50
        m.addHandler(6, d6.handler(), 60); // Handler 6, timeout: 60
        CHECK(m.update(5) == 0); // No handlers have expired
        CHECK(m.size() == 6);
        CHECK(m.nearestTimeout() == 5); // Handler 1 is a nearest handler to expire
        m.takeHandler(1); // Remove handler 1
        CHECK(d1.error() == Error::INTERNAL);
        CHECK(m.hasHandler(1) == false);
        CHECK(m.size() == 5);
        CHECK(m.nearestTimeout() == 15); // Handler 2 is a nearest handler to expire
        CHECK(m.update(20) == 1); // 1 handler has expired (handler 2)
        CHECK(d2.error() == Error::TIMEOUT);
        CHECK(m.hasHandler(2) == false);
        CHECK(m.size() == 4);
        CHECK(m.nearestTimeout() == 5); // Handler 3 is a nearest handler to expire
        CompletionData<int> d7, d8;
        m.addHandler(7, d7.handler(), 0); // Handler 7, timeout: 0
        m.addHandler(8, d8.handler(), 10); // Handler 8, timeout: 10
        CHECK(m.size() == 6);
        CHECK(m.nearestTimeout() == 0); // Handler 7 is a nearest handler to expire
        CHECK(m.update(10) == 3); // 3 handlers have expired (handlers 3, 7 and 8)
        CHECK(d3.error() == Error::TIMEOUT);
        CHECK(d7.error() == Error::TIMEOUT);
        CHECK(d8.error() == Error::TIMEOUT);
        CHECK((!m.hasHandler(3) && !m.hasHandler(7) && !m.hasHandler(8)));
        CHECK(m.size() == 3);
        CHECK(m.nearestTimeout() == 5); // Handler 4 is a nearest handler to expire
        m.setResult(4, 1); // Set result for handler 4
        CHECK(d4.result() == 1);
        CHECK(m.hasHandler(4) == false);
        CHECK(m.size() == 2);
        CHECK(m.nearestTimeout() == 15); // Handler 5 is a nearest handler to expire
        m.setError(5, Error::UNKNOWN); // Set error for handler 5
        CHECK(d5.error() == Error::UNKNOWN);
        CHECK(m.hasHandler(5) == false);
        CHECK(m.size() == 1);
        CHECK(m.nearestTimeout() == 25); // Handler 6 is a nearest handler to expire
        CHECK(m.update(100) == 1); // 1 handler has expired (handler 6)
        CHECK(d6.error() == Error::TIMEOUT);
        CHECK(m.hasHandler(6) == false);
        CHECK(m.size() == 0);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }
}

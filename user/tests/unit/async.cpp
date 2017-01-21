#include "spark_wiring_async.h"

#include "completion_handler.h"

#include "tools/catch.h"

#include <boost/optional.hpp>

#include <thread>
#include <deque>

namespace {

using spark::Error;

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
using Future = spark::Future<ResultT, Context>;

template<typename ResultT>
using Promise = spark::Promise<ResultT, Context>;

inline void postEvent(Context::Event event) {
    Context::instance()->postEvent(std::move(event));
}

inline void resetContext() {
    Context::instance()->reset();
}

using particle::CompletionHandler;
using particle::CompletionHandlerMap;

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

    system_error error() const {
        return error_.value_or(SYSTEM_ERROR_NONE);
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
    boost::optional<system_error> error_;

    // Completion callback
    static void callback(int error, void* result, void* data, void* reserved) {
        auto h = static_cast<CompletionData*>(data);
        if (error != SYSTEM_ERROR_NONE) {
            h->error_ = (system_error)error;
            h->result_ = boost::none;
        } else {
            h->result_ = result ? *static_cast<const ResultT*>(result) : ResultT();
            h->error_ = boost::none;
        }
    }
};

} // namespace

TEST_CASE("Future<void>") {
    using Future = ::Future<void>;
    using Promise = ::Promise<void>;

    resetContext();

    SECTION("constructing via default constructor") {
        Future f;
        CHECK(f.state() == Future::State::CANCELLED);
        CHECK(f.isSucceeded() == false);
        CHECK(f.isFailed() == false);
        CHECK(f.isCancelled() == true);
        CHECK(f.isDone() == true);
        CHECK(f.error().type() == Error::NONE);
    }

    SECTION("constructing via promise") {
        Promise p;
        Future f = p.future();
        CHECK(f.state() == Future::State::RUNNING);
        CHECK(f.isSucceeded() == false);
        CHECK(f.isFailed() == false);
        CHECK(f.isCancelled() == false);
        CHECK(f.isDone() == false);
        CHECK(f.error().type() == Error::NONE);
    }

    SECTION("making succeeded future") {
        Promise p;
        Future f = p.future();
        p.setResult();
        CHECK(f.state() == Future::State::SUCCEEDED);
        CHECK(f.isSucceeded() == true);
        CHECK(f.isFailed() == false);
        CHECK(f.isCancelled() == false);
        CHECK(f.isDone() == true);
        CHECK(f.error().type() == Error::NONE);
    }

    SECTION("making succeeded future (convenience method)") {
        Future f = Future::makeSucceeded();
        CHECK(f.isSucceeded() == true);
    }

    SECTION("making failed future") {
        Promise p;
        Future f = p.future();
        p.setError(Error::UNKNOWN);
        CHECK(f.state() == Future::State::FAILED);
        CHECK(f.isSucceeded() == false);
        CHECK(f.isFailed() == true);
        CHECK(f.isCancelled() == false);
        CHECK(f.isDone() == true);
        CHECK(f.error().type() == Error::UNKNOWN);
    }

    SECTION("making failed future (convenience method)") {
        Future f = Future::makeFailed(Error::UNKNOWN);
        CHECK(f.isFailed() == true);
        CHECK(f.error().type() == Error::UNKNOWN);
    }

    SECTION("waiting for succeeded operation") {
        Promise p;
        Future f = p.future();
        postEvent([&p]() {
            p.setResult();
        });
        f.wait();
        CHECK(f.isSucceeded() == true);
    }

    SECTION("waiting for failed operation") {
        Promise p;
        Future f = p.future();
        postEvent([&p]() {
            p.setError(Error::UNKNOWN);
        });
        f.wait();
        CHECK(f.isFailed() == true);
        CHECK(f.error().type() == Error::UNKNOWN);
    }

    SECTION("waiting for timed out operation") {
        Promise p;
        Future f = p.future();
        CHECK(f.wait(50).isDone() == false); // 50 ms
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
        CHECK(error.type() == Error::UNKNOWN);
    }

    SECTION("callbacks are invoked immediately for already completed future") {
        // Succeeded operation
        Promise p1;
        Future f1 = p1.future();
        p1.setResult();
        bool called = false;
        f1.onSuccess([&called]() {
            called = true;
        });
        CHECK(called == true);
        // Failed operation
        Future f2 = Future::makeFailed(Error::UNKNOWN);
        Error error(Error::NONE);
        f2.onError([&error](Error e) {
            error = e;
        });
        CHECK(error.type() == Error::UNKNOWN);
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
        CHECK(error.type() == Error::NONE);
    }
}

TEST_CASE("Future<int>") {
    using Future = ::Future<int>;
    using Promise = ::Promise<int>;

    resetContext();

    SECTION("constructing via default constructor") {
        Future f;
        CHECK(f.isCancelled() == true);
        CHECK(f.result() == 0); // Default-constructed value
    }

    SECTION("constructing via promise") {
        Promise p;
        Future f = p.future();
        CHECK(f.isDone() == false);
        CHECK(f.result() == 0); // Default-constructed value
    }

    SECTION("making succeeded future") {
        Promise p;
        Future f = p.future();
        p.setResult(1);
        CHECK(f.isSucceeded() == true);
        CHECK(f.result() == 1);
    }

    SECTION("making succeeded future (convenience method)") {
        Future f = Future::makeSucceeded(1);
        CHECK(f.isSucceeded() == true);
        CHECK(f.result() == 1);
    }

    SECTION("making failed future") {
        Promise p;
        Future f = p.future();
        p.setError(Error::UNKNOWN);
        CHECK(f.isFailed() == true);
        CHECK(f.result() == 0); // Default-constructed value
    }

    SECTION("waiting for succeeded operation") {
        Promise p;
        Future f = p.future();
        postEvent([&p]() {
            p.setResult(1);
        });
        f.wait();
        CHECK(f.isSucceeded() == true);
        CHECK(f.result() == 1);
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

TEST_CASE("CompletionHandler") {
    SECTION("using default-constructed handler") {
        CHECK((bool)CompletionHandler() == false);
        CompletionHandler().setResult(); // Shouldn't crash :)
        CompletionHandler().setError(SYSTEM_ERROR_UNKNOWN); // ditto
    }

    SECTION("invoking handler with result data") {
        CompletionData<int> d;
        CompletionHandler h = d.handler();
        CHECK((bool)h == true);
        int result = 1;
        h.setResult(&result); // Set result data
        CHECK(d.result() == 1);
        CHECK(d.hasError() == false);
    }

    SECTION("invoking handler with error code") {
        CompletionData<int> d;
        CompletionHandler h = d.handler();
        h.setError(SYSTEM_ERROR_UNKNOWN); // Set error code
        CHECK(d.hasResult() == false);
        CHECK(d.error() == SYSTEM_ERROR_UNKNOWN);
    }

    SECTION("handler can be invoked only once") {
        CompletionData<int> d;
        // Invoking already completed handler with an error code
        CompletionHandler h1 = d.handler();
        int result = 1;
        h1.setResult(&result);
        h1.setError(SYSTEM_ERROR_UNKNOWN);
        CHECK(d.result() == 1);
        CHECK(d.hasError() == false);
        // Invoking already completed handler with a result data
        d.reset();
        CompletionHandler h2 = d.handler();
        h2.setError(SYSTEM_ERROR_UNKNOWN);
        h2.setResult(&result);
        CHECK(d.hasResult() == false);
        CHECK(d.error() == SYSTEM_ERROR_UNKNOWN);
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
        CHECK(d.error() == SYSTEM_ERROR_INTERNAL);
        h1.setError(SYSTEM_ERROR_UNKNOWN);
        CHECK(d.error() == SYSTEM_ERROR_UNKNOWN);
        h2.setResult();
        CHECK(d.hasResult() == false); // Callback ownership has been transferred to h1
    }

    SECTION("completion callback is invoked on handler destruction") {
        CompletionData<int> d;
        {
            CompletionHandler h = d.handler();
        }
        CHECK(d.error() == SYSTEM_ERROR_INTERNAL);
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
        CompletionData<int> d;
        CHECK(m.addHandler(3, d.handler(), 30) == true); // Handler 3, timeout: 30
        CHECK(m.addHandler(1, d.handler(), 10) == true); // Handler 1, timeout: 10
        CHECK(m.addHandler(2, d.handler(), 20) == true); // Handler 2, timeout: 20
        CHECK((m.hasHandler(1) && m.hasHandler(2) && m.hasHandler(3)));
        CHECK(m.size() == 3);
        CHECK(m.isEmpty() == false);
        CHECK(m.nearestTimeout() == 10); // Handler 1 is a next handler to expire
        // Removing handler 1
        m.takeHandler(1);
        CHECK((!m.hasHandler(1) && m.hasHandler(2) && m.hasHandler(3)));
        CHECK(m.size() == 2);
        CHECK(m.isEmpty() == false);
        CHECK(m.nearestTimeout() == 20); // Handler 2 is a next handler to expire
        // Removing handler 2
        m.takeHandler(2);
        CHECK((!m.hasHandler(1) && !m.hasHandler(2) && m.hasHandler(3)));
        CHECK(m.size() == 1);
        CHECK(m.isEmpty() == false);
        CHECK(m.nearestTimeout() == 30); // Handler 3 is a next handler to expire
        // Removing handler 3
        m.takeHandler(3);
        CHECK((!m.hasHandler(1) && !m.hasHandler(2) && !m.hasHandler(3)));
        CHECK(m.size() == 0);
        CHECK(m.isEmpty() == true);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("clearing handler map") {
        CompletionHandlerMap m;
        CompletionData<int> d;
        m.addHandler(1, d.handler(), 10); // Handler 1, timeout: 10
        m.addHandler(2, d.handler(), 20); // Handler 2, timeout: 20
        m.clear();
        CHECK(m.size() == 0);
        CHECK(m.isEmpty() == true);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("waiting for handler expiration") {
        // Adding a single handler and waiting until it expires
        CompletionHandlerMap m;
        CompletionData<int> d;
        m.addHandler(1, d.handler(), 10); // Handler 1, timeout: 10
        CHECK(m.update(5) == 0); // No handlers have expired
        CHECK(m.hasHandler(1) == true);
        CHECK(m.nearestTimeout() == 5);
        CHECK(m.update(5) == 1); // 1 handler has expired (Handler 1)
        CHECK(d.error() == SYSTEM_ERROR_TIMEOUT);
        CHECK(m.hasHandler(1) == false);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
    }

    SECTION("modifying handler map while waiting for handlers expiration") {
        CompletionHandlerMap m;
        CompletionData<int> d;
        m.addHandler(1, d.handler(), 10); // Handler 1, timeout: 10
        m.addHandler(2, d.handler(), 20); // Handler 2, timeout: 20
        m.addHandler(3, d.handler(), 30); // Handler 3, timeout: 30
        m.addHandler(4, d.handler(), 40); // Handler 4, timeout: 40
        m.addHandler(5, d.handler(), 50); // Handler 5, timeout: 50
        m.addHandler(6, d.handler(), 60); // Handler 6, timeout: 60
        CHECK(m.update(5) == 0); // No handlers have expired
        CHECK(m.nearestTimeout() == 5); // Handler 1 is a next handler to expire
        m.takeHandler(1); // Remove Handler 1
        CHECK(m.nearestTimeout() == 15); // Handler 2 is a next handler to expire
        CHECK(m.update(20) == 1); // 1 handler has expired (Handler 2)
        CHECK(m.hasHandler(2) == false);
        CHECK(m.nearestTimeout() == 5); // Handler 3 is a next handler to expire
        m.addHandler(1, d.handler(), 0); // Handler 1, timeout: 0
        m.addHandler(2, d.handler(), 10); // Handler 2, timeout: 10
        CHECK(m.nearestTimeout() == 0); // Handler 1 is a next handler to expire
        CHECK(m.update(10) == 3); // 3 handlers have expired (Handler 1, Handler 2, Handler 3)
        CHECK((!m.hasHandler(1) && !m.hasHandler(2) && !m.hasHandler(3)));
        CHECK(m.nearestTimeout() == 5); // Handler 4 is a next handler to expire
        int result = 1;
        m.setResult(4, &result); // Set result for Handler 4
        CHECK(m.hasHandler(4) == false);
        CHECK(m.nearestTimeout() == 15); // Handler 5 is a next handler to expire
        CHECK(d.result() == 1);
        d.reset();
        m.setError(5, SYSTEM_ERROR_UNKNOWN); // Set error for Handler 5
        CHECK(m.hasHandler(5) == false);
        CHECK(m.nearestTimeout() == 25); // Handler 6 is a next handler to expire
        CHECK(d.error() == SYSTEM_ERROR_UNKNOWN);
        d.reset();
        CHECK(m.update(100) == 1); // 1 handler has expired (Handler 6)
        CHECK(m.hasHandler(6) == false);
        CHECK(m.nearestTimeout() == CompletionHandlerMap::MAX_TIMEOUT);
        CHECK(m.isEmpty() == true);
    }
}

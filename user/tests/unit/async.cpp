#include "spark_wiring_async.h"

#include "completion_handler.h"

#include "tools/catch.h"

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

    // Methods called by Future implementation
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

// Adapter class allowing to use C++ lambda as completion_callback
class CompletionCallback {
public:
    typedef std::function<void(int error, void* result)> Function;

    CompletionCallback(Function func) :
            func_(std::move(func)) {
    }

    operator completion_callback() const {
        return callback;
    }

private:
    static void callback(int error, void* result, void* data, void* reserved) {
        if (data) {
            auto d = static_cast<const CompletionCallback*>(data);
            d->func_(error, result);
        }
    }

    Function func_;
};

inline CompletionCallback completionCallback(CompletionCallback::Function func) {
    return CompletionCallback(std::move(func));
}

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
        CompletionHandler().setResult(); // Shouldn't crash
        CompletionHandler().setError(SYSTEM_ERROR_UNKNOWN); // ditto
    }

    SECTION("invoking handler with result data") {
        int result = 0;
        int error = SYSTEM_ERROR_UNKNOWN;
        auto cb = completionCallback([&](int e, void* r) {
            // Copy callback arguments to local variables
            result = *static_cast<const int*>(r);
            error = e;
        });
        CompletionHandler h(cb, &cb); // Wrap completion_callback to CompletionHandler
        CHECK((bool)h == true);
        int val = 1;
        h.setResult(&val); // Set result data
        CHECK(result == 1);
        CHECK(error == SYSTEM_ERROR_NONE);
    }

    SECTION("invoking handler with error code") {
        bool hasResult = false;
        int error = SYSTEM_ERROR_NONE;
        auto cb = completionCallback([&](int e, void* r) {
            hasResult = (bool)r;
            error = e;
        });
        CompletionHandler h(cb, &cb);
        h.setError(SYSTEM_ERROR_UNKNOWN); // Set error code
        CHECK(hasResult == false);
        CHECK(error == SYSTEM_ERROR_UNKNOWN);
    }

    SECTION("handler can be invoked only once") {
        bool hasResult = false;
        int error = SYSTEM_ERROR_NONE;
        auto cb = completionCallback([&](int e, void* r) {
            hasResult = (bool)r;
            error = e;
        });
        // Invoking already completed handler with error code
        CompletionHandler h1(cb, &cb);
        int val = 1;
        h1.setResult(&val);
        h1.setError(SYSTEM_ERROR_UNKNOWN);
        CHECK(hasResult == true);
        CHECK(error == SYSTEM_ERROR_NONE);
        // Invoking already completed handler with result data
        CompletionHandler h2(cb, &cb);
        h2.setError(SYSTEM_ERROR_UNKNOWN);
        h2.setResult(&val);
        CHECK(hasResult == false);
        CHECK(error == SYSTEM_ERROR_UNKNOWN);
    }

    SECTION("handler instance can be moved via move constructor") {
        bool called = false;
        auto cb = completionCallback([&](int, void*) {
            called = true;
        });
        CompletionHandler h1(cb, &cb);
        CompletionHandler h2(std::move(h1));
        h1.setResult();
        CHECK(called == false); // Callback ownership has been transferred to h2
        h2.setResult();
        CHECK(called == true);
    }

    SECTION("handler instance can be moved via move assignment") {
        int error = SYSTEM_ERROR_NONE;
        auto cb = completionCallback([&](int e, void*) {
            error = e;
        });
        CompletionHandler h1(cb, &cb);
        CompletionHandler h2(cb, &cb);
        h1 = std::move(h2);
        // It's an error if a completion handler wasn't invoked during its lifetime
        CHECK(error == SYSTEM_ERROR_INTERNAL);
        h1.setError(SYSTEM_ERROR_UNKNOWN);
        CHECK(error == SYSTEM_ERROR_UNKNOWN);
        h2.setResult();
        CHECK(error == SYSTEM_ERROR_UNKNOWN); // Callback ownership has been transferred to h1
    }

    SECTION("completion callback is invoked on handler destruction") {
        int error = SYSTEM_ERROR_NONE;
        {
            auto cb = completionCallback([&](int e, void*) {
                error = e;
            });
            CompletionHandler h(cb, &cb);
        }
        // It's an error if a completion handler wasn't invoked during its lifetime
        CHECK(error == SYSTEM_ERROR_INTERNAL);
    }
}

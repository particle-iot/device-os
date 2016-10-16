#include "spark_wiring_async.h"

#include "catch.hpp"

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
        // TODO
    }

    static bool isApplicationThreadCurrent() {
        // TODO
        return true;
    }

private:
    std::deque<Event> events_;
};

inline void postEvent(Context::Event event) {
    Context::instance()->postEvent(std::move(event));
}

inline void resetContext() {
    Context::instance()->reset();
}

template<typename ResultT>
using Future = spark::Future<ResultT, Context>;

template<typename ResultT>
using Promise = spark::Promise<ResultT, Context>;

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

    SECTION("waiting for succeeded operation (single thread)") {
        Promise p;
        Future f = p.future();
        postEvent([&p]() {
            p.setResult();
        });
        f.wait();
        CHECK(f.isSucceeded() == true);
    }

    SECTION("waiting for failed operation (single thread)") {
        Promise p;
        Future f = p.future();
        postEvent([&p]() {
            p.setError(Error::UNKNOWN);
        });
        f.wait();
        CHECK(f.isFailed() == true);
        CHECK(f.error().type() == Error::UNKNOWN);
    }

    SECTION("waiting for timed out operation (single thread)") {
        Promise p;
        Future f = p.future();
        CHECK(f.wait(50).isDone() == false); // 50 ms
    }

    SECTION("callback for succeeded operation (single thread)") {
        Promise p;
        Future f = p.future();
        bool called = false;
        f.onSuccess([&called]() {
            called = true;
        });
        postEvent([&p]() {
            p.setResult();
        });
        f.wait();
        CHECK(called == true);
    }

    SECTION("callback for failed operation (single thread)") {
        Promise p;
        Future f = p.future();
        Error error(Error::NONE);
        f.onError([&error](Error e) {
            error = e;
        });
        postEvent([&p]() {
            p.setError(Error::UNKNOWN);
        });
        f.wait();
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

    SECTION("making failed future") {
        Promise p;
        Future f = p.future();
        p.setError(Error::UNKNOWN);
        CHECK(f.isFailed() == true);
        CHECK(f.result() == 0); // Default-constructed value
    }

    SECTION("waiting for succeeded operation (single thread)") {
        Promise p;
        Future f = p.future();
        postEvent([&p]() {
            p.setResult(1);
        });
        f.wait();
        CHECK(f.isSucceeded() == true);
        CHECK(f.result() == 1);
    }

    SECTION("callback for succeeded operation (single thread)") {
        Promise p;
        Future f = p.future();
        int result = 0;
        f.onSuccess([&result](int r) {
            result = r;
        });
        postEvent([&p]() {
            p.setResult(1);
        });
        f.wait();
        CHECK(result == 1);
    }
}

#include "spark_wiring_cloud.h"

#include "spark_wiring_ledger.h"

#include <functional>
#include "system_cloud.h"
#include "check.h"

namespace {

using namespace particle;

void publishCompletionCallback(int error, const void* data, void* callbackData, void* reserved) {
    auto p = Promise<bool>::fromDataPtr(callbackData);
    if (error != Error::NONE) {
        p.setError(Error((Error::Type)error, (const char*)data));
    } else {
        p.setResult(true);
    }
}

inline bool subscribeWithContentType(const char* name, EventHandler handler, void* handlerData) {
    spark_subscribe_param param = {};
    param.size = sizeof(param);
    param.flags |= SUBSCRIBE_FLAG_BINARY_DATA;

    return spark_subscribe(name, handler, handlerData, MY_DEVICES, nullptr /* device_id_deprecated */, &param);
}

void subscribeWithContentTypeCallbackWrapper(void* arg, const char* name, const char* data, size_t dataSize, int contentType) {
    auto cb = (EventHandlerWithContentType)arg;
    cb(name, data, dataSize, (ContentType)contentType);
}

void subscribeWithContentTypeFunctionWrapper(void* arg, const char* name, const char* data, size_t dataSize, int contentType) {
    auto fn = (EventHandlerWithContentTypeFn*)arg;
    (*fn)(name, data, dataSize, (ContentType)contentType);
}

} // namespace

spark_cloud_disconnect_options CloudDisconnectOptions::toSystemOptions() const
{
    spark_cloud_disconnect_options opts = {};
    opts.size = sizeof(opts);
    opts.flags = flags_;
    opts.graceful = graceful_;
    opts.timeout = timeout_;
    opts.clear_session = clearSession_;
    opts.reconnect_immediately = reconnect_;
    return opts;
}

CloudDisconnectOptions CloudDisconnectOptions::fromSystemOptions(const spark_cloud_disconnect_options* options)
{
    bool clearSession = false;
    if (options->size >= offsetof(spark_cloud_disconnect_options, clear_session) +
            sizeof(spark_cloud_disconnect_options::clear_session)) {
        clearSession = options->clear_session;
    }
    return CloudDisconnectOptions(options->flags, options->timeout, options->graceful, clearSession, options->reconnect_immediately);
}

int CloudClass::call_raw_user_function(void* data, const char* param, void* reserved)
{
    user_function_int_str_t* fn = (user_function_int_str_t*)(data);
    String p(param);
    return (*fn)(p);
}

int CloudClass::call_std_user_function(void* data, const char* param, void* reserved)
{
    user_std_function_int_str_t* fn = (user_std_function_int_str_t*)(data);
    return (*fn)(String(param));
}

void CloudClass::call_wiring_event_handler(const void* handler_data, const char *event_name, const char *data)
{
    wiring_event_handler_t* fn = (wiring_event_handler_t*)(handler_data);
    (*fn)(event_name, data);
}

bool CloudClass::register_function(cloud_function_t fn, void* data, const char* funcKey)
{
    cloud_function_descriptor desc = {};
    desc.size = sizeof(desc);
    desc.fn = fn;
    desc.data = (void*)data;
    desc.funcKey = funcKey;
    return spark_function(NULL, (user_function_int_str_t*)&desc, NULL);
}

Future<bool> CloudClass::publish_event(const char* name, const char* data, size_t size, ContentType type, int ttl,
        PublishFlags flags) {
    if (!connected()) {
        return Future<bool>(Error::INVALID_STATE);
    }
    spark_send_event_data d = {};
    d.size = sizeof(spark_send_event_data);
    d.data_size = size;
    d.content_type = static_cast<int>(type);

    // Completion handler
    Promise<bool> p;
    d.handler_callback = publishCompletionCallback;
    d.handler_data = p.dataPtr();

    if (!spark_send_event(name, data, ttl, flags.value(), &d) && !p.isDone()) {
        // Set generic error code in case completion callback wasn't invoked for some reason
        p.setError(Error::UNKNOWN);
        p.fromDataPtr(d.handler_data); // Free wrapper object
    }

    return p.future();
}

int CloudClass::publishVitals(system_tick_t period_s_) {
    return spark_publish_vitals(period_s_, nullptr);
}

void CloudClass::disconnect(const CloudDisconnectOptions& options) {
    const auto opts = options.toSystemOptions();
    spark_cloud_disconnect(&opts, nullptr /* reserved */);
}

void CloudClass::setDisconnectOptions(const CloudDisconnectOptions& options) {
    const auto opts = options.toSystemOptions();
    spark_set_connection_property(SPARK_CLOUD_DISCONNECT_OPTIONS, 0 /* value */, &opts, nullptr /* reserved */);
}

int CloudClass::maxEventDataSize() {
    size_t size = 0;
    size_t n = sizeof(size);
    CHECK(spark_get_connection_property(SPARK_CLOUD_MAX_EVENT_DATA_SIZE, &size, &n, nullptr /* reserved */));
    return size;
}

int CloudClass::maxVariableValueSize() {
    size_t size = 0;
    size_t n = sizeof(size);
    CHECK(spark_get_connection_property(SPARK_CLOUD_MAX_VARIABLE_VALUE_SIZE, &size, &n, nullptr /* reserved */));
    return size;
}

int CloudClass::maxFunctionArgumentSize() {
    size_t size = 0;
    size_t n = sizeof(size);
    CHECK(spark_get_connection_property(SPARK_CLOUD_MAX_FUNCTION_ARGUMENT_SIZE, &size, &n, nullptr /* reserved */));
    return size;
}

#if Wiring_Ledger

Ledger CloudClass::ledger(const char* name) {
    ledger_instance* instance = nullptr;
    int r = ledger_get_instance(&instance, name, nullptr);
    if (r < 0) {
        LOG(ERROR, "ledger_get_instance() failed: %d", r);
        return Ledger();
    }
    return Ledger(instance, false /* addRef */);
}

int CloudClass::useLedgersImpl(const Vector<const char*>& usedNames) {
    Vector<String> allNames;
    CHECK(Ledger::getNames(allNames));
    int result = 0;
    for (auto& name: allNames) {
        bool found = false;
        for (auto usedName: usedNames) {
            if (name == usedName) {
                found = true;
                break;
            }
        }
        if (!found) {
            int r = Ledger::remove(name);
            if (r < 0 && result >= 0) {
                result = r;
            }
        }
    }
    return result;
}

#endif // Wiring_Ledger

bool CloudClass::subscribe(const char* name, EventHandlerWithContentType handler) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    auto h = (EventHandler)subscribeWithContentTypeCallbackWrapper;
#pragma GCC diagnostic pop
    return subscribeWithContentType(name, h, (void*)handler);
}

bool CloudClass::subscribe(const char* name, EventHandlerWithContentTypeFn handler) {
    auto fnPtr = new(std::nothrow) EventHandlerWithContentTypeFn(std::move(handler));
    if (!fnPtr) {
        return false;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    auto h = (EventHandler)subscribeWithContentTypeFunctionWrapper;
#pragma GCC diagnostic pop
    return subscribeWithContentType(name, h, fnPtr);
}

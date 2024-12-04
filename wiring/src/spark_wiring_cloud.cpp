#include <functional>
#include <cstdio>

#include "spark_wiring_cloud.h"
#include "spark_wiring_ledger.h"
#include "spark_wiring_variant.h"
#include "spark_wiring_print.h"

#include "system_cloud.h"

#include "check.h"

namespace {

using namespace particle;

bool parseVariantFromCbor(Variant& v, const char* data, size_t size) {
    InputBufferStream stream(data, size);
    int r = decodeFromCBOR(v, stream);
    if (r < 0) {
        LOG(ERROR, "Failed to parse CBOR: %d", r);
        return false;
    }
    return true;
}

void publishCompletionCallback(int error, const void* data, void* callbackData, void* reserved) {
    auto p = Promise<bool>::fromDataPtr(callbackData);
    if (error != Error::NONE) {
        p.setError(Error((Error::Type)error, (const char*)data));
    } else {
        p.setResult(true);
    }
}

inline bool subscribeWithFlags(const char* name, EventHandler handler, void* handlerData, int flags) {
    spark_subscribe_param param = {};
    param.size = sizeof(param);
    param.flags = flags;

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

void subscribeWithVariantCallbackWrapper(void* arg, const char* name, const char* data, size_t dataSize, int contentType) {
    Variant v;
    if (!parseVariantFromCbor(v, data, dataSize)) {
        return;
    }
    auto cb = (EventHandlerWithVariant)arg;
    cb(name, std::move(v));
}

void subscribeWithVariantFunctionWrapper(void* arg, const char* name, const char* data, size_t dataSize, int contentType) {
    Variant v;
    if (!parseVariantFromCbor(v, data, dataSize)) {
        return;
    }
    auto fn = (EventHandlerWithVariantFn*)arg;
    (*fn)(name, std::move(v));
}

template<typename T>
inline EventHandler eventHandlerCast(T* fn) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    return (EventHandler)fn;
#pragma GCC diagnostic pop
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

Future<bool> CloudClass::publish_event(const char* name, const char* data, size_t size, int type, int ttl,
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

Future<bool> CloudClass::publish(const char* name, const Variant& data, PublishFlags flags) {
    String s;
    OutputStringStream stream(s);
    int r = encodeToCBOR(data, stream);
    if (r < 0) {
        return Future<bool>((Error::Type)r);
    }
    return publish_event(name, s.c_str(), s.length(), static_cast<int>(protocol::CoapContentFormat::PARTICLE_STRUCTURED),
            DEFAULT_CLOUD_EVENT_TTL, flags);
}

bool CloudClass::publish(CloudEvent event) {
    int r = event.publish();
    if (r < 0) {
        return false;
    }
    return true;
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
    auto h = eventHandlerCast(subscribeWithContentTypeCallbackWrapper);
    return subscribeWithFlags(name, h, (void*)handler, SUBSCRIBE_FLAG_BINARY_DATA);
}

bool CloudClass::subscribe(const char* name, EventHandlerWithContentTypeFn handler) {
    auto fnPtr = new(std::nothrow) EventHandlerWithContentTypeFn(std::move(handler));
    if (!fnPtr) {
        return false;
    }
    auto h = eventHandlerCast(subscribeWithContentTypeFunctionWrapper);
    return subscribeWithFlags(name, h, fnPtr, SUBSCRIBE_FLAG_BINARY_DATA);
}

bool CloudClass::subscribe(const char* name, particle::EventHandlerWithVariant handler) {
    auto h = eventHandlerCast(subscribeWithVariantCallbackWrapper);
    return subscribeWithFlags(name, h, (void*)handler, SUBSCRIBE_FLAG_CBOR_DATA);
}

bool CloudClass::subscribe(const char* name, particle::EventHandlerWithVariantFn handler) {
    auto fnPtr = new(std::nothrow) EventHandlerWithVariantFn(std::move(handler));
    if (!fnPtr) {
        return false;
    }
    auto h = eventHandlerCast(subscribeWithVariantFunctionWrapper);
    return subscribeWithFlags(name, h, fnPtr, SUBSCRIBE_FLAG_CBOR_DATA);
}

bool CloudClass::subscribe(const char* name, EventHandlerWithCloudEvent* handler, const SubscribeOptions& opts) {
    std::function<EventHandlerWithCloudEvent> fn(handler);
    if (!fn) {
        return false;
    }
    return subscribe(name, std::move(fn), opts);
}

bool CloudClass::subscribe(const char* name, std::function<EventHandlerWithCloudEvent> handler, const SubscribeOptions& opts) {
    int flags = SUBSCRIBE_FLAG_LARGE_EVENT;
    if (opts.structured()) {
        flags |= SUBSCRIBE_FLAG_CBOR_DATA;
    }
    bool ok = subscribeWithFlags(name, nullptr /* handler */, nullptr /* handlerData */, flags);
    if (!ok) {
        return false;
    }
    int r = CloudEvent::subscribe(name, std::move(handler), opts);
    if (r < 0) {
        return false;
    }
    return true;
}

void CloudClass::unsubscribe() {
    spark_unsubscribe(nullptr /* reserved */);
    CloudEvent::unsubscribeAll();
}

namespace particle {

size_t getEventDataSize(const EventData& data) {
    return getCBORSize(data);
}

} // namespace particle

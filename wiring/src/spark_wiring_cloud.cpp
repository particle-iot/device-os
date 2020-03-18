#include "spark_wiring_cloud.h"

#include <functional>
#include "system_cloud.h"

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

} // namespace

spark_cloud_disconnect_options CloudDisconnectOptions::toSystemOptions() const
{
    spark_cloud_disconnect_options opts = {};
    opts.size = sizeof(opts);
    opts.flags = optionFlags_;
    opts.graceful = disconnectGracefully_;
    opts.timeout = disconnectTimeout_;
    return opts;
}

CloudDisconnectOptions CloudDisconnectOptions::fromSystemOptions(const spark_cloud_disconnect_options* options)
{
    return CloudDisconnectOptions(options->flags, options->timeout, options->graceful);
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

Future<bool> CloudClass::publish_event(const char *eventName, const char *eventData, int ttl, PublishFlags flags) {
    if (!connected()) {
        return Future<bool>(Error::INVALID_STATE);
    }
    spark_send_event_data d = { sizeof(spark_send_event_data) };

    // Completion handler
    Promise<bool> p;
    d.handler_callback = publishCompletionCallback;
    d.handler_data = p.dataPtr();

    if (!spark_send_event(eventName, eventData, ttl, flags.value(), &d) && !p.isDone()) {
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
    spark_set_connection_property(SPARK_CLOUD_CONNECTION_PROPERTY_DISCONNECT_OPTIONS, 0 /* data */, &opts, nullptr /* reserved */);
}

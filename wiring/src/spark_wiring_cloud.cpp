#include "spark_wiring_cloud.h"

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
    cloud_function_descriptor desc;
    memset(&desc, 0, sizeof(desc));
    desc.size = sizeof(desc);
    desc.fn = fn;
    desc.data = (void*)data;
    desc.funcKey = funcKey;
    return spark_function(NULL, (user_function_int_str_t*)&desc, NULL);
}

spark::Future<void> CloudClass::publish(const char *eventName, const char *eventData, int ttl, uint32_t flags) {
#ifndef SPARK_NO_CLOUD
    spark_send_event_data d = { sizeof(spark_send_event_data) };

    // Completion handler
    spark::Promise<void> p;
    d.handler_callback = p.systemCallback;
    d.handler_data = p.dataPtr();

    if (!spark_send_event(eventName, eventData, ttl, flags, &d) && !p.isDone()) {
        // Set generic error code in case completion callback wasn't invoked for some reason
        p.setError(spark::Error::UNKNOWN);
        p.fromDataPtr(d.handler_data); // Free wrapper object
    }

    return p.future();
#else
    return spark::Future<void>::makeFailed(spark::Error::NOT_SUPPORTED);
#endif
}

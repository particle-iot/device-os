#include "spark_wiring_cloud.h"

CloudClass Spark;

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

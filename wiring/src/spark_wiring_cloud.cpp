#include "spark_wiring_cloud.h"

CloudClass Spark;

int CloudClass::call_raw_user_function(void* data, const char* param, void* reserved)
{
    user_function_t fn = user_function_t(data);
    return fn(param);
}

int CloudClass::call_std_user_function(void* data, const char* param, void* reserved)
{
    user_std_function_t* fn = (user_std_function_t*)(data);
    return (*fn)(param);
}

bool CloudClass::register_function(cloud_function_t fn, void* data, const char* funcKey)
{
    cloud_function_descriptor desc;
    desc.size = sizeof(desc);
    desc.fn = fn;
    desc.data = (void*)data;
    desc.funcKey = funcKey;    
    return spark_function(&desc, NULL);    
}

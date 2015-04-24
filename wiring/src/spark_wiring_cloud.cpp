#include "spark_wiring_cloud.h"

SparkClass Spark;

int SparkClass::call_raw_user_function(void* data, const char* param, void* reserved)
{
    user_function_t fn = user_function_t(data);
    return fn(param);
}

int SparkClass::call_std_user_function(void* data, const char* param, void* reserved)
{
    user_std_function_t* fn = (user_std_function_t*)(data);
    return (*fn)(param);
}

bool SparkClass::register_function(cloud_function_t fn, void* data, const char* funcKey)
{
    cloud_function_descriptor desc;
    desc.size = sizeof(desc);
    desc.fn = fn;
    desc.data = (void*)data;
    desc.funcKey = funcKey;    
    return spark_function(&desc, NULL);    
}

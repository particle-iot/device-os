#include <string.h>
#include "UnitTest++.h"
#include "spark_descriptor.h"

struct FunctionFixture {
  static int execute_a_function(const char *func_key, const char *arg, SparkDescriptor::FunctionResultCallback callback, void*);
  static int get_number_of_funcs(void);
  static void copy_a_function_key(char *destination, int function_index);
};

int FunctionFixture::execute_a_function(const char *func_key,
                                        const char *arg,
                                        SparkDescriptor::FunctionResultCallback callback, void*)
{
  const char *prevent_warning;
  prevent_warning = func_key;
  prevent_warning = arg;
  int result = 99;
  callback(&result, SparkReturnType::INT);
  return 0;
}

int FunctionFixture::get_number_of_funcs(void)
{
  return 3;
}

void FunctionFixture::copy_a_function_key(char *destination, int function_index)
{
  const char *function_keys[] = {
    "brew\0\0\0\0\0\0\0\0", "clean\0\0\0\0\0\0\0", "bean_alert\0\0" };
  memcpy(destination, function_keys[function_index], 12);
}

SUITE(Descriptor)
{
  TEST_FIXTURE(FunctionFixture, DescriptorKnowsNumberOfRegisteredFunctions)
  {
    SparkDescriptor descriptor;
    descriptor.num_functions = get_number_of_funcs;
    CHECK_EQUAL(3, descriptor.num_functions());
  }

  TEST_FIXTURE(FunctionFixture, DescriptorCanCallRegisteredFunction)
  {
    SparkDescriptor descriptor;
    descriptor.call_function = execute_a_function;
    const char *function_key = "brew";
    const char *arg = "32,240";
    int return_value;
    descriptor.call_function(function_key, arg, [&](const void* result, SparkReturnType::Enum resultType)->bool {
        CHECK_EQUAL(SparkReturnType::INT, resultType);
        return_value = *((const int*)result);
        return true;
    },  NULL);
    CHECK_EQUAL(99, return_value);
  }
}

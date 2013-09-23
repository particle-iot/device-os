#include "UnitTest++.h"
#include "spark_descriptor.h"

struct FunctionFixture {
  static int execute_a_function(unsigned char *func_key, unsigned char *arg);
};

int FunctionFixture::execute_a_function(unsigned char *func_key, unsigned char *arg)
{
  unsigned char *prevent_warning;
  prevent_warning = func_key;
  prevent_warning = arg;
  return 99;
}

SUITE(Descriptor)
{
  TEST(DescriptorKnowsNumberOfRegisteredFunctions)
  {
    unsigned char num_funcs = 3;
    SparkDescriptor descriptor;
    descriptor.num_funcs_ptr = &num_funcs;
    CHECK_EQUAL(3, *descriptor.num_funcs_ptr);
  }

  TEST(DescriptorCanAccessArrayOfFunctionKeys)
  {
    const char *function_keys[] = { "brew", "clean", "bean_alert" };
    SparkDescriptor descriptor;
    descriptor.function_keys = function_keys;
    CHECK_EQUAL("bean_alert", descriptor.function_keys[2]);
  }

  TEST_FIXTURE(FunctionFixture, DescriptorCanCallRegisteredFunction)
  {
    SparkDescriptor descriptor;
    descriptor.call_function = execute_a_function;
    unsigned char *function_key = (unsigned char *)"brew";
    unsigned char *arg= (unsigned char *)"32,240";
    int return_value = descriptor.call_function(function_key, arg);
    CHECK_EQUAL(99, return_value);
  }
}

#include "UnitTest++.h"
#include "spark_descriptor.h"

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
}

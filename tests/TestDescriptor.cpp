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
}

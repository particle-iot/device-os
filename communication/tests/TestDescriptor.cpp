#include <string.h>
#include "UnitTest++.h"
#include "particle_descriptor.h"

struct FunctionFixture {
  static int execute_a_function(const char *func_key, const char *arg);
  static int get_number_of_funcs(void);
  static void copy_a_function_key(char *destination, int function_index);
};

int FunctionFixture::execute_a_function(const char *func_key,
                                        const char *arg)
{
  const char *prevent_warning;
  prevent_warning = func_key;
  prevent_warning = arg;
  return 99;
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
    ParticleDescriptor descriptor;
    descriptor.num_functions = get_number_of_funcs;
    CHECK_EQUAL(3, descriptor.num_functions());
  }

  TEST_FIXTURE(FunctionFixture, DescriptorCanAccessArrayOfFunctionKeys)
  {
    ParticleDescriptor descriptor;
    descriptor.copy_function_key = copy_a_function_key;
    char buf[12];
    descriptor.copy_function_key(buf, 2);
    CHECK_EQUAL("bean_alert\0\0", buf);
  }

  TEST_FIXTURE(FunctionFixture, DescriptorCanCallRegisteredFunction)
  {
    ParticleDescriptor descriptor;
    descriptor.call_function = execute_a_function;
    const char *function_key = "brew";
    const char *arg = "32,240";
    int return_value = descriptor.call_function(function_key, arg);
    CHECK_EQUAL(99, return_value);
  }
}

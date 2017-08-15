#include "platform_diagnostic.h"

__attribute__((__noinline__)) void* platform_get_current_pc(void)
{
  return __builtin_return_address(0);
}

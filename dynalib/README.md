# Dynamic Link Interfaces

## Overview

## ABI Compatibility

Callers are always assumed to be at the same version or older than their dependent library. It is a general contract that dynamically linked interfaces should backwards compatible with older callers.  Since callers cannot be newer than the dynamically linked interface, there is no general requirement for forward compatibility.

### General Recommendataions for Achieving ABI compatibility

When designing a new function that is part of a dynamic link interface, consider the following points:

* Structs passed in to functions may need to evolve over time. 
Adding a `size` parameter that is filled by the caller ensures that as these structs evolve, the function can check the size of the struct that the client used. This ensures the implementation doesn't use members of the structs that were added after the caller was compiled, since these will not have been allocated by the caller.

* If the function's parameters are numerous or are expected to
change over time, wrapping them up in a struct can make evolution of the API easier, since ABI compatibilty is maintained when
adding new members to the struct.

* As generalization of the point above, consider adding an additional `void*` parameter named `reserved` that is set to `nullptr` by clients. This allows the function to be extended in future releases without breaking the ABI.  This can be useful even when
the function is already taking one or more structs as arguments since the new data may not be a good semantic fit for the existing types.

** The reserved parameter can be used to later add a new struct type to the API. The implementation checks that the pointer is not `nullptr` and if the struct has multiple versions, checks the size before access.

## You can't break ABI compatibility, but what if you REALLY want to?

If a change is required that would break ABI compatibility, this
is done by adding a new function to the dynamic link interface with the new revised signature. The function is added after all the other functions, i.e. at a new index.

The existing function can be renamed and given a suffix to indicate that it is no longer current, such as `_old` , while existing callers in system firmware and wiring are changed to use the new function name (which can be the same as the old function name.) Since dynamic link functions are invoked by index, the old function continues to exist at the same index and it will continue to be used by old clients, even though it has been renamed. New clients will use the new function found at the new index.

The HAL I2C interface on Gen2 platforms went through this type of revision, since it needed to take an additional parameter for the I2D peripheral selector, that was not added to the original I2C dynamic interface.  For good measure, a `void*` reserved parameter was also added in anticipation of future changes required.


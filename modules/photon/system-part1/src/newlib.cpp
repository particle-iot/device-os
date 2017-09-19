#include "../../../../hal/src/stm32/newlib.cpp"

extern "C" {

// Defining this in hal/src/stm32/newlib.cpp causes linker errors
void *__dso_handle = NULL;

} // extern "C"

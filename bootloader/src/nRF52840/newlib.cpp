#include <cstddef>

extern "C" {

// Exceptions are disabled on the nRF52840 platforms, and default implementations of these functions
// pull malloc() and free() into the bootloader
void* __cxa_allocate_exception(size_t) throw() {
    return nullptr;
}

void __cxa_free_exception(void*) throw() {
}

} // extern "C"

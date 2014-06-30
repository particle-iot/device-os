

#include <stdatomic.h>
#include "spi_bus.h"

volatile uint8_t spi_bus_lock = 0;

int try_acquire_spi_bus(uint8_t owner) {
    return 1; //spi_bus_lock==owner || __sync_bool_compare_and_swap(&spi_bus_lock, 0, owner);
}

void acquire_spi_bus(uint8_t owner) {
    //while (!try_acquire_spi_bus(owner));
}

void release_spi_bus(uint8_t owner) {
    //while (!__sync_bool_compare_and_swap(&spi_bus_lock, owner, 0));
}

uint8_t current_bus_owner() { return spi_bus_lock; }


#include <stdint.h>
#include <stdatomic.h>
#include "spi_bus.h"
#include "debug.h"
#include "hw_config.h"

#ifndef SPI_BUS_ARBITER
#define SPI_BUS_ARBITER 1
#endif

volatile int spi_bus_lock = 0;

void reset_bus() { spi_bus_lock = 0; }

int try_acquire_spi_bus(int owner) {
#if SPI_BUS_ARBITER
    __sync_synchronize();
    return spi_bus_lock==owner || __sync_bool_compare_and_swap(&spi_bus_lock, 0, owner);
#else
    return 1;
#endif
}

void acquire_spi_bus(int owner) {
#if SPI_BUS_ARBITER
    while (!try_acquire_spi_bus(owner));
#endif
}

int try_release_spi_bus(int owner) {
#if SPI_BUS_ARBITER
    __sync_synchronize();
    return spi_bus_lock==0 || __sync_bool_compare_and_swap(&spi_bus_lock, owner, 0);
#else
    return 1;
#endif
}

void release_spi_bus(int owner) {
#if SPI_BUS_ARBITER
    while (!try_release_spi_bus(owner));
#endif
}

int current_bus_owner() { return spi_bus_lock; }

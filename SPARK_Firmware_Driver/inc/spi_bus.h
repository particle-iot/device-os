/* 
 * File:   spi_bus.h
 * Author: mat
 *
 * Created on 30 June 2014, 14:25
 */

#ifndef SPI_BUS_H
#define	SPI_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

int try_acquire_spi_bus(uint8_t owner);
    
void acquire_spi_bus(uint8_t owner);

void release_spi_bus(uint8_t owner);

uint8_t current_bus_owner();

#ifdef __cplusplus
}
#endif

#endif	/* SPI_BUS_H */


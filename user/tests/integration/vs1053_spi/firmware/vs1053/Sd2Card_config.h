#ifndef __SD2CARD_CONFIG_H__
#define __SD2CARD_CONFIG_H__

#include "application.h"
//#include "spark_wiring.h"

#define SD_SPI_NUMBER	2					/* Specify HardwareSPI number */

/* For Maple families, Maple rev5, etc */
#	define SD_CHIP_SELECT_PIN	A2			/* for Spark Core SPI CS pin */
	//#define SPI_LOW_CLOCK	SPI_281_250KHZ	/* = SPI_HALF_SPEED */
#	define SPI_LOW_CLOCK	SPI_4_5MHZ		/* = SPI_HALFL_SPEED */

/* High speed */
//#	define SPI_HIGH_CLOCK	SPI_4_5MHZ		/* = SPI_FULL_SPEED */
#	define SPI_HIGH_CLOCK	SPI_9MHZ		/* = SPI_FULL_SPEED */
//#	define SPI_HIGH_CLOCK	SPI_18MHZ		/* = SPI_FULL_SPEED */

/* Select output device for print() function */
#	define SERIAL_DEVICE	0				/* 0:SerialUSB  1=: Serial UART device */
#	if SERIAL_DEVICE == 0  	
#		define Serial Serial				/* Use USB CDC port */
#		define BPS_9600		/* Nothing */ 
#		define BPS_115200 	/* Nothing */ 
#	else
#		define Serial Serial1				/* Specify serial device, "Serial1" or "Serial2" or "Serial3" */
#		define BPS_9600		9600
#		define BPS_115200 	115200
#	endif

//#define SPI_SPEED_UP						/* Enable read/write speed up option */


//define SPI_DMA		/* Do not enable this line, no checked, experiment, dangerous */


#endif /* __SD2CARD_CONFIG_H__ */


/******************************************************************************
LSM9DS1_Types.h
SparkFunLSM9DS1 Particle Library - LSM9DS1 Types and Enumerations
Jim Lindblom @ SparkFun Electronics
Original Creation Date: April 21, 2015
https://github.com/sparkfun/SparkFun_LSM9DS1_Particle_Library

This file defines all types and enumerations used by the LSM9DS1 class.

Development environment specifics:
	IDE: Particle Build
	Hardware Platform: Particle Photon
	                   SparkFun Photon IMU Shield

This code is released under the MIT license.

Distributed as-is; no warranty is given.
******************************************************************************/

#ifndef __LSM9DS1_Types_H__
#define __LSM9DS1_Types_H__

#include "LSM9DS1_Registers.h"

// The LSM9DS1 functions over both I2C or SPI. This library supports both.
// But the interface mode used must be sent to the LSM9DS1 constructor. Use
// one of these two as the first parameter of the constructor.
enum interface_mode
{
	IMU_MODE_SPI,
	IMU_MODE_I2C,
};

// accel_scale defines all possible FSR's of the accelerometer:
enum accel_scale
{
	A_SCALE_2G,	// 00:  2g
	A_SCALE_16G,// 01:  16g
	A_SCALE_4G,	// 10:  4g
	A_SCALE_8G	// 11:  8g
};

// gyro_scale defines the possible full-scale ranges of the gyroscope:
enum gyro_scale
{
	G_SCALE_245DPS,		// 00:  245 degrees per second
	G_SCALE_500DPS,		// 01:  500 dps
	G_SCALE_2000DPS,	// 11:  2000 dps
};

// mag_scale defines all possible FSR's of the magnetometer:
enum mag_scale
{
	M_SCALE_4GS, 	// 00:  4Gs
	M_SCALE_8GS,	// 01:  8Gs
	M_SCALE_12GS,	// 10:  12Gs
	M_SCALE_16GS,	// 11:  16Gs
};

// gyro_odr defines all possible data rate/bandwidth combos of the gyro:
enum gyro_odr
{
	//! TODO 
	G_ODR_PD,	// Power down (0)
	G_ODR_149,	// 14.9 Hz (1)
	G_ODR_595,	// 59.5 Hz (2)
	G_ODR_119,	// 119 Hz (3)
	G_ODR_238,	// 238 Hz (4)
	G_ODR_476,	// 476 Hz (5)
	G_ODR_952	// 952 Hz (6)
};
// accel_oder defines all possible output data rates of the accelerometer:
enum accel_odr
{
	XL_POWER_DOWN, 	// Power-down mode (0x0)
	XL_ODR_10,		// 10 Hz (0x1)
	XL_ODR_50,		// 50 Hz (0x02)
	XL_ODR_119,		// 119 Hz (0x3)
	XL_ODR_238,		// 238 Hz (0x4)
	XL_ODR_476,		// 476 Hz (0x5)
	XL_ODR_952		// 952 Hz (0x6)
};

// accel_abw defines all possible anti-aliasing filter rates of the accelerometer:
enum accel_abw
{
	A_ABW_408,		// 408 Hz (0x0)
	A_ABW_211,		// 211 Hz (0x1)
	A_ABW_105,		// 105 Hz (0x2)
	A_ABW_50,		//  50 Hz (0x3)
};


// mag_odr defines all possible output data rates of the magnetometer:
enum mag_odr
{
	M_ODR_0625,	// 0.625 Hz (0)
	M_ODR_125,	// 1.25 Hz (1)
	M_ODR_250,	// 2.5 Hz (2)
	M_ODR_5,	// 5 Hz (3)
	M_ODR_10,	// 10 Hz (4)
	M_ODR_20,	// 20 Hz (5)
	M_ODR_40,	// 40 Hz (6)
	M_ODR_80	// 80 Hz (7)
};

enum interrupt_select
{
	XG_INT1 = INT1_CTRL,
	XG_INT2 = INT2_CTRL
};

enum interrupt_generators
{
	INT_DRDY_XL = (1<<0),	// Valid for INT1 & INT2
	INT_DRDY_G = (1<<1),	// Valid for INT1 & INT2
	INT2_DRDY_TEMP = (1<<2),// Replaces INT_BOOT (int1)
	INT_FTH = (1<<3),		// Valid for INT1 & INT2
	INT_OVR = (1<<4),		// Valid for INT1 & INT2
	INT_FSS5 = (1<<5),		// Valid for INT1 & INT2
	INT1_IG_XL = (1<<6),	// ONLY VALID for INT1
	INT1_IG_G = (1<<7)		// Replaces INT2_INACT (int2)
};	

enum accel_interrupt_generator
{
	XLIE_XL = (1<<0),
	XHIE_XL = (1<<1),
	YLIE_XL = (1<<2),
	YHIE_XL = (1<<3),
	ZLIE_XL = (1<<4),
	ZHIE_XL = (1<<5),
	GEN_6D = (1<<6)
};

enum gyro_interrupt_generator
{
	XLIE_G = (1<<0),
	XHIE_G = (1<<1),
	YLIE_G = (1<<2),
	YHIE_G = (1<<3),
	ZLIE_G = (1<<4),
	ZHIE_G = (1<<5)
};

enum mag_interrupt_generator
{
	ZIEN = (1<<5),
	YIEN = (1<<6),
	XIEN = (1<<7)
};

enum h_lactive
{
	INT_ACTIVE_HIGH,
	INT_ACTIVE_LOW
};

enum pp_od
{
	INT_PUSH_PULL,
	INT_OPEN_DRAIN
};

enum fifoMode_type
{
	FIFO_OFF = 0,
	FIFO_THS = 1,
	FIFO_CONT_TRIGGER = 3,
	FIFO_OFF_TRIGGER = 4,
	FIFO_CONT = 5
};

struct gyroSettings
{
	// Gyroscope settings:
	uint8_t enabled;
	uint16_t scale;	// Changed this to 16-bit
	uint8_t sampleRate;
	// New gyro stuff:
	uint8_t bandwidth;
	uint8_t lowPowerEnable;
	uint8_t HPFEnable;
	uint8_t HPFCutoff;
	uint8_t flipX;
	uint8_t flipY;
	uint8_t flipZ;
	uint8_t orientation;
	uint8_t enableX;
	uint8_t enableY;
	uint8_t enableZ;
	uint8_t latchInterrupt;
};

struct deviceSettings
{
    uint8_t commInterface; // Can be I2C, SPI 4-wire or SPI 3-wire
    uint8_t agAddress;	// I2C address or SPI CS pin
	uint8_t mAddress;	// I2C address or SPI CS pin
};

struct accelSettings
{
	// Accelerometer settings:
    uint8_t enabled;
    uint8_t scale;
    uint8_t sampleRate;
	// New accel stuff:
	uint8_t enableX;
	uint8_t enableY;
	uint8_t enableZ;
	int8_t  bandwidth;
	uint8_t highResEnable;
	uint8_t highResBandwidth;
};

struct magSettings
{
	// Magnetometer settings:
    uint8_t enabled;
    uint8_t scale;
    uint8_t sampleRate;
	// New mag stuff:
	uint8_t tempCompensationEnable;
	uint8_t XYPerformance;
	uint8_t ZPerformance;
	uint8_t lowPowerEnable;
	uint8_t operatingMode;
};

struct temperatureSettings
{
	// Temperature settings
    uint8_t enabled;
};

struct IMUSettings
{
	deviceSettings device;
	
	gyroSettings gyro;
	accelSettings accel;
	magSettings mag;
	
	temperatureSettings temp;
};

#endif
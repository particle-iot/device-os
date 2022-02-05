/** 
 * @brief MAX17043 fuel gauge custom battery model configuration
 *
 * @copyright Copyright (c) 2022 Particle Industries, Inc.
 */

#pragma once

// For now we only support a single concrete Maxim fuel gauge,
// on a subset of all our device platforms.



#if(!HAL_PLATFORM_FUELGAUGE_MAX17043)
#error This definition only works with Maxim fuel gauge
#endif 

#define BATTERY_MODEL_DATA_LENGTH   (64)

typedef struct {
    bool valid_config;
    uint8_t EmptyAdjustment;  // 
    uint8_t FullAdjustment;   // 
    uint8_t RCOMP0;           // Starting RCOMP value 
    float TempCoUp;           // Temperature (hot) coeffiecient for RCOMP. Used in UPDATE RCOMP step 
    float TempCoDown;         // Temperature (cold) coeffiecient for RCOMP. Used in UPDATE RCOMP step. 
    uint32_t OCVTest;         // OCV Test value in decimal. Used in step 7 
    uint8_t SOCCheckA;        // SOCCheck low value. Used to verify model 
    uint8_t SOCCheckB;        // SOCCheck high value. Used to verify model 
    uint8_t bits;             // 18 or 19 bit model. See Calculating SOC for details. 
    // uint8_t evkit_data1[32];  // 32 bytes used for EVKit software only. Discard this data  
    uint8_t model_data[BATTERY_MODEL_DATA_LENGTH];   // 64 bytes of Model Data begin here. Write these bytes in this order to the first table address at 0x40h. 
    // uint8_t evkit_data2[32];  // 32 bytes used for EVKit software only. Discard this data  
} MaximFuelGaugeConfig_t;

#define PlatformFuelGaugeConfig_t   MaximFuelGaugeConfig_t

// Generic Particle 103450 battery model config
const MaximFuelGaugeConfig_t MODEL_CONFIG_LP103450 = {
    .valid_config = true,
    .EmptyAdjustment=0,
    .FullAdjustment=100,
    .RCOMP0 = 49,
    .TempCoUp = -0.484375,
    .TempCoDown = -4.125,
    .OCVTest = 55488,
    .SOCCheckA = 106,
    .SOCCheckB = 108,
    .bits = 18,
    //RCOMPSeg = 0x0200
    .model_data = {
        0xA9, 0xB0, 0xB7, 0x10, 0xB8, 0x80, 0xB9, 0xE0, 0xBA, 0xC0, 0xBB, 0xA0, 0xBC, 0xB0, 0xBD, 0x90,
        0xBE, 0x50, 0xBF, 0x20, 0xC1, 0x80, 0xC2, 0x80, 0xC3, 0x20, 0xC8, 0xC0, 0xCD, 0x00, 0xCE, 0xC0,
        0x01, 0xE0, 0x17, 0xB0, 0x07, 0xC0, 0x16, 0x00, 0x1A, 0x00, 0x1F, 0x30, 0x26, 0x50, 0x16, 0x00,
        0x1F, 0xE0, 0x0E, 0x80, 0x08, 0x20, 0x1A, 0xD0, 0x0A, 0xA0, 0x0C, 0xE0, 0x02, 0xF0, 0x02, 0xF0,
    }
};

// Generic LG 21700 battery model config
const MaximFuelGaugeConfig_t MODEL_CONFIG_LG21700 = {
    .valid_config = true,
    .EmptyAdjustment=0,
    .FullAdjustment=100,
    .RCOMP0 = 92,
    .TempCoUp = -0.453125,
    .TempCoDown = -0.8125,
    .OCVTest = 58560,
    .SOCCheckA = 203,
    .SOCCheckB = 205,
    .bits = 19,
    .model_data = {
        0x88, 0x70, 0xAA, 0x10, 0xAD, 0x90, 0xB0, 0x60, 0xB3, 0xF0, 0xB7, 0x00, 0xB8, 0xF0, 0xBC, 0x50,
        0xBF, 0xE0, 0xC2, 0x00, 0xC4, 0x60, 0xC7, 0x40, 0xCA, 0xD0, 0xCC, 0x40, 0xCD, 0x00, 0xDA, 0xC0, 
        0x00, 0x40, 0x07, 0x00, 0x0C, 0x00, 0x10, 0x40, 0x13, 0x00, 0x1D, 0x60, 0x19, 0x20, 0x1A, 0xE0,
        0x13, 0xC0, 0x15, 0x80, 0x11, 0xC0, 0x13, 0x20, 0x3D, 0x00, 0x5E, 0x60, 0x01, 0x20, 0x01, 0x20,
    }
};








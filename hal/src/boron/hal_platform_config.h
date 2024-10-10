#pragma once

#include "hal_platform_nrf52840_config.h"
#include "platforms.h"

#define HAL_PLATFORM_NCP (1)
#define HAL_PLATFORM_NCP_AT (1)
#define HAL_PLATFORM_CELLULAR (1)
#define HAL_PLATFORM_CELLULAR_SERIAL (HAL_USART_SERIAL2)
#define HAL_PLATFORM_CELLULAR_LOW_POWER (1)
#define HAL_PLATFORM_SETUP_BUTTON_UX (1)
#define HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX (1)
#define HAL_PLATFORM_SPI_NUM (2)
#define HAL_PLATFORM_I2C_NUM (2)
#define HAL_PLATFORM_USART_NUM (2)
#define HAL_PLATFORM_NCP_COUNT (1)
#define HAL_PLATFORM_BROKEN_MTU (1)
#define HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME (9*60*1000)

#if PLATFORM_ID == PLATFORM_BORON

#define HAL_PLATFORM_ASSETS_OTA_OVERLAP (1)

#define HAL_PLATFORM_POWER_MANAGEMENT (1)
#define HAL_PLATFORM_PMIC_BQ24195 (1)
#define HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD (4)
#define HAL_PLATFORM_FUELGAUGE_MAX17043 (1)
#define HAL_PLATFORM_PMIC_BQ24195_I2C (HAL_I2C_INTERFACE2)
#define HAL_PLATFORM_FUELGAUGE_MAX17043_I2C (HAL_I2C_INTERFACE2)
#define HAL_PLATFORM_RADIO_ANTENNA_INTERNAL (1)
#define HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL (1)
#define HAL_PLATFORM_FLASH_MX25L3233F (1)

#if HAL_PLATFORM_ETHERNET
#define HAL_PLATFORM_ETHERNET_WIZNETIF_CS_PIN_DEFAULT    (D5)
#define HAL_PLATFORM_ETHERNET_WIZNETIF_RESET_PIN_DEFAULT (D3)
#define HAL_PLATFORM_ETHERNET_WIZNETIF_INT_PIN_DEFAULT   (D4)
#endif // HAL_PLATFORM_ETHERNET

#elif PLATFORM_ID == PLATFORM_ESOMX

#define HAL_PLATFORM_POWER_MANAGEMENT (1)
#define HAL_PLATFORM_PMIC_BQ24195 (1)
#define HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD (4)
#define HAL_PLATFORM_FUELGAUGE_MAX17043 (1)
#define HAL_PLATFORM_PMIC_BQ24195_I2C (HAL_I2C_INTERFACE2)
#define HAL_PLATFORM_FUELGAUGE_MAX17043_I2C (HAL_I2C_INTERFACE2)
#define HAL_PLATFORM_RADIO_ANTENNA_INTERNAL (0)
#define HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL (1)
#define HAL_PLATFORM_FLASH_MX25R6435FZNIL0 (1)
#define HAL_PLATFORM_CELLULAR_MODEM_VOLTAGE_TRANSLATOR (0)

#if HAL_PLATFORM_ETHERNET
#define HAL_PLATFORM_ETHERNET_WIZNETIF_CS_PIN_DEFAULT    (B0)
#define HAL_PLATFORM_ETHERNET_WIZNETIF_RESET_PIN_DEFAULT (B1)
#define HAL_PLATFORM_ETHERNET_WIZNETIF_INT_PIN_DEFAULT   (D2)
#endif // HAL_PLATFORM_ETHERNET

#else // PLATFORM_ID != PLATFORM_ESOMX (i.e. PLATFORM_BSOM)

#define HAL_PLATFORM_ASSETS_OTA_OVERLAP (1)

#define HAL_PLATFORM_PMIC_BQ24195 (1)
#define HAL_PLATFORM_PMIC_BQ24195_I2C (HAL_I2C_INTERFACE1)
#define HAL_PLATFORM_ETHERNET_FEATHERWING_SPI_CLOCK (16000000)
#define HAL_PLATFORM_FUELGAUGE_MAX17043 (1)
#define HAL_PLATFORM_FUELGAUGE_MAX17043_I2C (HAL_I2C_INTERFACE1)
#define HAL_PLATFORM_HW_FORM_FACTOR_SOM (1)
#define HAL_PLATFORM_POWER_MANAGEMENT (1)
#define HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL (1)
#define HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD (4)
#define HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL (1)
#define HAL_PLATFORM_FLASH_MX25L3233F (1)

#if HAL_PLATFORM_ETHERNET
#define HAL_PLATFORM_ETHERNET_WIZNETIF_CS_PIN_DEFAULT    (D8)
#define HAL_PLATFORM_ETHERNET_WIZNETIF_RESET_PIN_DEFAULT (A7)
#define HAL_PLATFORM_ETHERNET_WIZNETIF_INT_PIN_DEFAULT   (D22)
#endif // HAL_PLATFORM_ETHERNET

#define HAL_PLATFORM_IF_INIT_POSTPONE (1)

#endif // PLATFORM_ID != PLATFORM_BORON

#if PLATFORM_ID == PLATFORM_ESOMX
#define PRODUCT_SERIES "E-SoMX"
#else
#define PRODUCT_SERIES "boron"
#endif

#define HAL_PLATFORM_PPP_SERVER (1)
#define HAL_PLATFORM_PPP_SERVER_USART (HAL_USART_SERIAL1)
//#define HAL_PLATFORM_PPP_SERVER_USART_BAUDRATE (460800)
#define HAL_PLATFORM_PPP_SERVER_USART_BAUDRATE (921600)
#define HAL_PLATFORM_PPP_SERVER_USART_FLAGS (SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS) // For hw flow control (SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS)

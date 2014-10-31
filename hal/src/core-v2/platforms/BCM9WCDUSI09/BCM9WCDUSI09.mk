#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Platform_BCM9WCDUSI09

WLAN_CHIP            := 43362
WLAN_CHIP_REVISION   := A2
HOST_MCU_FAMILY      := STM32F2xx
HOST_MCU_VARIANT     := STM32F2x5
HOST_MCU_PART_NUMBER := STM32F215RGT6

INTERNAL_MEMORY_RESOURCES = $(ALL_RESOURCES)

ifndef BUS
BUS := SDIO
endif

VALID_BUSES := SDIO

ifeq ($(BUS),SDIO)
WIFI_IMAGE_DOWNLOAD := direct
GLOBAL_DEFINES      += WWD_DIRECT_RESOURCES
else
$(error This platform does not support SPI)
endif

# Global includes
GLOBAL_INCLUDES  := .

# Global defines
# HSE_VALUE = STM32 crystal frequency = 26MHz (needed to make UART work correctly)
GLOBAL_DEFINES += HSE_VALUE=26000000
GLOBAL_DEFINES += $$(if $$(NO_CRLF_STDIO_REPLACEMENT),,CRLF_STDIO_REPLACEMENT)

# Components
$(NAME)_COMPONENTS += drivers/spi_flash

# Source files
$(NAME)_SOURCES := platform.c

# WICED APPS
# APP0 and FILESYSTEM_IMAGE are reserved main app and resources file system
# FR_APP := resources/sflash/snip_ota_fr-BCM943362WCD6.stripped.elf
# DCT_IMAGE :=
# OTA_APP :=
# FILESYSTEM_IMAGE :=
# WIFI_FIRMWARE :=
# APP0 :=
# APP1 :=
# APP2 :=

# WICED APPS LOOKUP TABLE
APPS_LUT_HEADER_LOC := 0x0000
APPS_START_SECTOR := 1

ifneq ($(APP),bootloader)
ifneq ($(MAIN_COMPONENT_PROCESSING),1)
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | IMPORTANT NOTES                                                                                     | )
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | Wi-Fi MAC Address                                                                                   | )
$(info |    The target Wi-Fi MAC address is defined in <WICED-SDK>/generated_mac_address.txt                 | )
$(info |    Ensure each target device has a unique address.                                                  | )
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | MCU & Wi-Fi Power Save                                                                              | )
$(info |    It is *critical* that applications using WICED Powersave API functions connect an accurate 32kHz | )
$(info |    reference clock to the sleep clock input pin of the WLAN chip. Please read the WICED Powersave   | )
$(info |    Application Note located in the documentation directory if you plan to use powersave features.   | )
$(info +-----------------------------------------------------------------------------------------------------+ )
endif
endif

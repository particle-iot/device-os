SYSTEM_PART2_MODULE_VERSION ?= 2
SYSTEM_PART2_MODULE_PATH ?= $(PROJECT_ROOT)/modules/photon/system-part2

ifeq ($(MINIMAL),y)
GLOBAL_DEFINES += SYSTEM_MINIMAL
endif

# we use wiring (mostly for Stream, String, USBSerial) but we don't want
# I2C and SPI objects
GLOBAL_DEFINES += SPARK_WIRING_NO_I2C SPARK_WIRING_NO_SPI


include $(call rwildcard,$(SYSTEM_PART2_MODULE_PATH)/,include.mk)
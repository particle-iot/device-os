MODULE=freertos
FREERTOS_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(FREERTOS_BUILD_PATH_EXT)

# depends only on headers. Will it work?
DEPENDENCIES = hal platform services

# FIXME
ifeq ($(SOFTDEVICE_PRESENT),y)
DEPENDENCIES += dynalib services third_party/nrf5_sdk
endif

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

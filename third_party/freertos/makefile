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

DEPENDENCIES += dynalib services third_party/ambd_sdk

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

# Disable LTO for FreeRTOS sources to keep some of the symbols in a single coalescent location
CONLYFLAGS_ORIGINAL := $(CONLYFLAGS)
CONLYFLAGS = $(filter-out $(LTO_FLAGS),$(CONLYFLAGS_ORIGINAL))

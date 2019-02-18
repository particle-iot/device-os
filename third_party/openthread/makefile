MODULE=openthread
OPENTHREAD_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(OPENTHREAD_BUILD_PATH_EXT)

# depends only on headers. Will it work?
DEPENDENCIES = crypto dynalib hal platform services

ifeq ($(PLATFORM_OPENTHREAD),nrf52840)
DEPENDENCIES += third_party/nrf5_sdk
endif

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

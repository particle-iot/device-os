MODULE=nrf5_sdk
NRF5_SDK_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(NRF5_SDK_BUILD_PATH_EXT)

# depends only on headers. Will it work?
DEPENDENCIES = dynalib platform services third_party/freertos hal

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

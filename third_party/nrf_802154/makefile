MODULE=nrf_802154
NRF_802154_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(NRF_802154_BUILD_PATH_EXT)

# depends only on headers. Will it work?
DEPENDENCIES = hal platform services third_party/nrf5_sdk

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

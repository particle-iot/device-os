MODULE=mbedtls
MBEDTLS_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(MBEDTLS_BUILD_PATH_EXT)

# depends only on headers. Will it work?
DEPENDENCIES = crypto platform hal

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

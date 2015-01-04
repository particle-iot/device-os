# Makefile for building modules

PROJECT_ROOT=..
MODULE_PATH=.
COMMON_BUILD=$(PROJECT_ROOT)/build
BUILD_PATH_BASE=$(COMMON_BUILD)/target


include $(COMMON_BUILD)/product-id.mk
include $(COMMON_BUILD)/module.mk

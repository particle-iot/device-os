# Makefile for building modules

# assume project is at top level directory if PROJECT_ROOT not specified
PROJECT_ROOT ?= ..
MODULE_PATH=.
COMMON_BUILD=$(PROJECT_ROOT)/build
BUILD_PATH_BASE?=$(COMMON_BUILD)/target


include $(COMMON_BUILD)/platform-id.mk
include $(COMMON_BUILD)/checks.mk

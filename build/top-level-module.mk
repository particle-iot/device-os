# Makefile for building modules

# assume project is at top level directory if PROJECT_ROOT not specified
# current_dir set in arm-tlk.mk
PROJECT_ROOT ?= $(current_dir)/..
# module_dir is optional, and will include final trailing slash
MODULE_PATH?=$(abspath $(module_dir).)
COMMON_BUILD=$(PROJECT_ROOT)/build
BUILD_PATH_BASE?=$(COMMON_BUILD)/target


include $(COMMON_BUILD)/platform-id.mk
include $(COMMON_BUILD)/checks.mk

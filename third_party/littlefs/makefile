MODULE=littlefs
LITTLEFS_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(LITTLEFS_BUILD_PATH_EXT)

DEPENDENCIES = hal platform dynalib services

# FIXME
# ifneq ("$(BOOTLOADER_MODULE)","1")
# DEPENDENCIES += services
# endif

PROJECT_ROOT ?= ../..
include ../../build/arm-tlm.mk

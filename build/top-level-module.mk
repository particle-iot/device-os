# Makefile for building modules

PROJECT_ROOT=..
MODULE_PATH=../$(MODULE)
COMMON_BUILD=$(PROJECT_ROOT)/build
BUILD_PATH_BASE=$(COMMON_BUILD)/target


# Define the build path, this is where all of the dependancies and
# object files will be placed.
# Note: Currently set to <project>/build/obj directory and set relative to
# the dir which makefile is invoked. If the makefile is moved to the project
# root, BUILD_PATH = build can be used to store the build products in 
# the build directory.
BUILD_PATH = $(BUILD_PATH_BASE)/$(MODULE)


include $(COMMON_BUILD)/module.mk
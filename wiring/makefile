
MODULE=wiring
WIRING_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a
BUILD_PATH_EXT=$(ARCH)

DEPENDENCIES = hal services communication

# This is required to compile main.cpp, which depends upon hal/syshealth macros.
# these macros are defined as direct reads/writes to backup registers.
# This is temporary - syshalth macros will delegate to HAL functions so this module
# no longer depends upon platform.
DEPENDENCIES += platform

include ../build/arm-tlm.mk


MODULE=communication
COMMUNICATION_MODULE_PATH=.

# ensure the unit test is not built
MODULE_BUILD_PATH=$(COMMUNICATION_MODULE_PATH)/src

# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(COMMUNICATION_BUILD_PATH_EXT)

DEPENDENCIES = hal dynalib services wiring crypto

include ../build/arm-tlm.mk

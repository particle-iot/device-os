
MODULE=system
SYSTEM_MODULE_PATH=.
BUILD_PATH_EXT = $(SYSTEM_BUILD_PATH_EXT)

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif
DEPENDENCIES = platform hal services communication wiring dynalib
TARGET_TYPE = a


include ../build/arm-tlm.mk

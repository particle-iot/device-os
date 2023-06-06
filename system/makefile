
MODULE=system
SYSTEM_MODULE_PATH=.
BUILD_PATH_EXT = $(SYSTEM_BUILD_PATH_EXT)

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif
DEPENDENCIES = platform hal services communication wiring dynalib crypto proto_defs third_party/arduinojson
TARGET_TYPE = a

MAKE_DEPENDENCIES =

include ../build/arm-tlm.mk


MODULE=system
SYSTEM_MODULE_PATH=.
BUILD_PATH_EXT = $(SYSTEM_BUILD_PATH_EXT)

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif
DEPENDENCIES = platform hal services third_party/nanopb third_party/miniz communication wiring dynalib crypto
TARGET_TYPE = a

MAKE_DEPENDENCIES = third_party/miniz

include ../build/arm-tlm.mk

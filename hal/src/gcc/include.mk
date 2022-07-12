# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_GCC_PATH = $(TARGET_HAL_PATH)/src/gcc
HAL_INCL_NETWORK_PATH = $(TARGET_HAL_PATH)/network
HAL_INCL_NETWORK_UTIL_PATH = $(TARGET_HAL_PATH)/network/util

INCLUDE_DIRS += $(HAL_SRC_GCC_PATH)
INCLUDE_DIRS += $(HAL_INCL_NETWORK_UTIL_PATH)
INCLUDE_DIRS += $(HAL_INCL_NETWORK_PATH)

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -lc


# additional libraries required by gcc build
ifdef SYSTEMROOT
LIBS += boost_system-mgw48-mt-1_57 ws2_32 wsock32
else
LIBS += boost_system
endif
LIBS += boost_program_options boost_random boost_thread boost_json

LIB_DIRS += $(BOOST_ROOT)/stage/lib

# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
HAL_TEST_FLAVOR+=-driver
endif
endif

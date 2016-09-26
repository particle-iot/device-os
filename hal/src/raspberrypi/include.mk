# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_RASPBERRYPI_PATH = $(TARGET_HAL_PATH)/src/raspberrypi

# if we are being compiled with platform as a dependency, then also include
# implementation headers.
ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_SRC_RASPBERRYPI_PATH)
endif

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -lc

# additional libraries required by raspberrypi build
LIBS += boost_system boost_program_options boost_random boost_thread

LIB_DIRS += $(BOOST_ROOT)/stage/lib

endif




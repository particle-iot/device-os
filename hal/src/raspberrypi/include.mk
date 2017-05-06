
HAL_INCL_RASPBERRYPI_PATH = $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)
INCLUDE_DIRS += $(HAL_INCL_RASPBERRYPI_PATH)

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -lc

# additional libraries required by raspberrypi build
LIBS += boost_system boost_program_options boost_random boost_thread

LIB_DIRS += $(BOOST_ROOT)/stage/lib

endif






ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -lc

# additional libraries required by raspberrypi build
LIBS += boost_system boost_program_options boost_random boost_thread

LIB_DIRS += $(BOOST_ROOT)/stage/lib

endif




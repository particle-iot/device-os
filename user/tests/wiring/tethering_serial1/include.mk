ifneq (,$(filter $(TEST),wiring/tethering_serial1 integration/wiring/tethering_serial1))
ifndef LIBIPERF_TEST_INCLUDE_MK
LIBIPERF_TEST_INCLUDE_MK := 1

LIBIPERF_DEPENDENCY = third_party/libiperf

include $(PROJECT_ROOT)/$(LIBIPERF_DEPENDENCY)/import.mk

LIBS += $(LIBIPERF_MODULE_NAME)
LIB_DIRS += $(LIBIPERF_LIB_DIR)

endif
endif

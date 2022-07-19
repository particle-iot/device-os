ifeq ($(APP),tinker)
COREMARK_DEPENDENCY = third_party/coremark

include $(PROJECT_ROOT)/$(COREMARK_DEPENDENCY)/import.mk

LIBS += $(notdir $(COREMARK_DEPENDENCY))
LIB_DIRS += $(COREMARK_LIB_DIR)
endif

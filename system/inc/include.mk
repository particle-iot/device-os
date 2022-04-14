
# Add include to all objects built for this target
INCLUDE_DIRS += $(SYSTEM_MODULE_PATH)/inc

SYSTEM_DEPS =
SYSTEM_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(SYSTEM_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
include $(SYSTEM_DEPS_INCLUDE_SCRIPTS)

ifneq ($(filter system,$(LIBS)),)
SYSTEM_LIB_DEP +=
LIBS += $(notdir $(SYSTEM_DEPS))
LIB_DIRS +=
endif

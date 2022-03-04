PLATFORM_DEPS = third_party/ambd_sdk
ifneq ("$(ARM_CPU)","cortex-m23")
PLATFORM_DEPS += third_party/littlefs third_party/miniz
endif
PLATFORM_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(PLATFORM_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
include $(PLATFORM_DEPS_INCLUDE_SCRIPTS)

LIBS += $(notdir $(PLATFORM_DEPS))

PLATFORM_LIB_DEPS = $(AMBD_SDK_LIB_DEP)
ifneq ("$(ARM_CPU)","cortex-m23")
PLATFORM_LIB_DEPS += $(LITTLEFS_LIB_DEP) $(MINIZ_LIB_DEP)
LIB_DIRS += $(LITTLEFS_LIB_DIR) $(MINIZ_LIB_DIR)
endif

PLATFORM_LIB_DEP += $(PLATFORM_LIB_DEPS)

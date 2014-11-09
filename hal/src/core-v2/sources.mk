
HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_COREV2_PATH = $(TARGET_HAL_PATH)/src/core-v2

ifeq ("$(USE_WICED_SDK)","1")

# private includes - WICED is not exposed to the HAL clients
# find all .h files, convert to directory and remove duplicates
HAL_WICED_INCLUDE_DIRS += $(dir $(call rwildcard,$(HAL_SRC_COREV2_PATH)/,*.h))
HAL_WICED_INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/wiced
HAL_WICED_INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/wiced/WWD
HAL_WICED_INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/wiced/network/LwIP/ver1.4.0.rc1/src/include
HAL_WICED_INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/wiced/network/LwIP/ver1.4.0.rc1/src/include/ipv4
INCLUDE_DIRS += $(sort $(HAL_WICED_INCLUDE_DIRS))

endif

# make the make variable also a preprocessor define
CFLAGS += -DUSE_WICED_SDK=$(USE_WICED_SDK)


templatedir=$(HAL_SRC_TEMPLATE_PATH)
overridedir=$(HAL_SRC_COREV2_PATH)

# C source files included in this build.
# Use files from the template unless they are overridden by files in the 
# core-v2 folder. Also manually exclude some files that have changed from c->cpp.

CSRC += $(call target_files,$(templatedir)/,*.c)
CPPSRC += $(call target_files,$(templatedir)/,*.cpp)

# find the overridden list of files (without extension)
overrides_abs = $(call rwildcard,$(overridedir)/,*.cpp)
overrides_abs += $(call rwildcard,$(overridedir)/,*.c)
overrides = $(basename $(patsubst $(overridedir)/%,%,$(overrides_abs)))

remove_c = $(addsuffix .c,$(addprefix $(templatedir)/,$(overrides)))
remove_cpp = $(addsuffix .cpp,$(addprefix $(templatedir)/,$(overrides)))

# remove files from template that have the same basename as an overridden file
# e.g. if template contains core_hal.c, and gcc contains core_hal.cpp, the gcc module
# will override
CSRC := $(filter-out $(remove_c),$(CSRC))
CPPSRC := $(filter-out $(remove_cpp),$(CPPSRC))

CSRC += $(call target_files,$(overridedir)/,*.c)
CPPSRC += $(call target_files,$(overridedir)/,*.cpp)

# ASM source files included in this build.
ASRC +=




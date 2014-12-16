
HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_COREV2_PATH = $(TARGET_HAL_PATH)/src/core-v2

# private includes - WICED is not exposed to the HAL clients

HAL_WICED_INCLUDE_DIRS +=   include
HAL_WICED_INCLUDE_DIRS +=   platforms/$(PLATFORM_NET)			    
HAL_WICED_INCLUDE_DIRS +=   libraries/daemons/DNS_redirect \
			    libraries/utilities/ring_buffer
HAL_WICED_INCLUDE_DIRS +=   wiced
HAL_WICED_INCLUDE_DIRS +=   wiced/network/$(HAL_WICED_NETWORK) \
			    wiced/network/$(HAL_WICED_NETWORK)/WWD \
			    wiced/network/$(HAL_WICED_NETWORK)/WICED
HAL_WICED_INCLUDE_DIRS +=   wiced/platform/ARM_CM3 \
			    wiced/platform/ARM_CM3/CMSIS \
			    wiced/platform/GCC \
			    wiced/platform/include \
			    wiced/platform/MCU \
			    wiced/platform/MCU/STM32F2xx \
			    wiced/platform/MCU/STM32F2xx/peripherals \
			    wiced/platform/MCU/STM32F2xx/WAF
HAL_WICED_INCLUDE_DIRS +=   wiced/RTOS/$(HAL_WICED_RTOS) \
			    wiced/RTOS/$(HAL_WICED_RTOS)/WWD \
			    wiced/RTOS/$(HAL_WICED_RTOS)/WICED
HAL_WICED_INCLUDE_DIRS +=   wiced/WWD


ifeq "$(HAL_WICED_NETWORK)" "LwIP"
HAL_WICED_INCLUDE_DIRS +=   wiced/network/LwIP/ver1.4.0.rc1/src/include \
			    wiced/network/LwIP/ver1.4.0.rc1/src/include/ipv4 \
			    wiced/network/LwIP/WWD/FreeRTOS
endif
ifeq "$(HAL_WICED_NETWORK)" "NetX"
HAL_WICED_INCLUDE_DIRS += wiced/network/NetX/ver5.5_sp1
endif
ifeq "$(HAL_WICED_RTOS)" "ThreadX"
HAL_WICED_INCLUDE_DIRS +=   wiced/RTOS/ThreadX/ver5.6 \
			    wiced/RTOS/ThreadX/ver5.6/Cortex_M3_M4/GCC
endif
ifeq "$(HAL_WICED_RTOS)" "FreeRTOS"
HAL_WICED_INCLUDE_DIRS +=   wiced/RTOS/FreeRTOS/WWD/ARM_CM3 \
			    wiced/RTOS/FreeRTOS/ver7.5.2/Source/Include \
			    wiced/RTOS/FreeRTOS/ver7.5.2/Source/portable/GCC/ARM_CM3
			    
endif



INCLUDE_DIRS += $(addprefix $(HAL_SRC_COREV2_PATH)/,$(sort $(HAL_WICED_INCLUDE_DIRS)))
INCLUDE_DIRS += $(dir $(call rwildcard,$(HAL_SRC_COREV2_PATH)/wiced/security,*.h))
INCLUDE_DIRS += $(dir $(call rwildcard,$(HAL_SRC_COREV2_PATH)/wiced/WWD,*.h))
$(info "wiced inc $(INCLUDE_DIRS)")

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




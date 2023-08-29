# Skip to next 100 every v0.x.0 release (e.g. 108 for v0.6.2 to 200 for v0.7.0-rc.1)
# Bump by 1 for every prerelease or release with the same v0.x.* base.
COMMON_MODULE_VERSION ?= 5501
SYSTEM_PART1_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)

RELEASE_080_MODULE_VERSION_BASE ?= 300
RELEASE_070_MODULE_VERSION ?= 207
RELEASE_110_MODULE_VERSION ?= 1102


# Bump by 1 if Tinker has been updated
USER_PART_MODULE_VERSION ?= 6

# Skip to next 100 every v0.x.0 release (e.g. 11 for v0.6.2 to 100 for v0.7.0-rc.1),
# but only if the bootloader has changed since the last v0.x.0 release.
# Bump by 1 for every updated bootloader image for a release with the same v0.x.* base.
BOOTLOADER_VERSION ?= 2300

ifeq ($(PLATFORM_MCU),rtl872x)
PREBOOTLOADER_MBR_VERSION ?= 2
PREBOOTLOADER_PART1_VERSION ?= 7
endif

# The version of the bootloader that the system firmware requires
# NOTE: this will force the device into safe mode until this dependency is met, which is why
# this version usually lags behind the current bootloader version, to avoid non-mandatory updates.
ifeq ($(PLATFORM_GEN),3)
ifeq ($(PLATFORM_MCU),rtl872x)
BOOTLOADER_DEPENDENCY = 2300
else # ifeq ($(PLATFORM_MCU),rtl872x)
BOOTLOADER_DEPENDENCY = 2300
endif # ifeq ($(PLATFORM_GEN),3)
else
# Some sensible default
BOOTLOADER_DEPENDENCY = 0
endif

ifeq ($(PLATFORM_MCU),rtl872x)
PREBOOTLOADER_PART1_DEPENDENCY = 7
endif

ifeq ($(PLATFORM_GEN),3)
ifeq ($(PLATFORM_MCU),nRF52840)
# SoftDevice S140 7.0.1
SOFTDEVICE_DEPENDENCY = 202

# FIXME: There is a compiler optimization or XIP access error in some earlier Device OS releases preventing
# OTA updates on Argon specifically to some 3.x versions due to a failed dependency check even though the
# dependencies are satisfied. Swapping two current dependencies seems to help.
ifneq ($(PLATFORM_ID),12)
SYSTEM_PART1_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_BOOTLOADER},0,${BOOTLOADER_DEPENDENCY}
else
SYSTEM_PART1_MODULE_DEPENDENCY2 ?= ${MODULE_FUNCTION_BOOTLOADER},0,${BOOTLOADER_DEPENDENCY}
endif
# /FIXME

ifeq (,$(filter $(PLATFORM_ID),26))
# FIXME: There is a compiler optimization or XIP access error in some earlier Device OS releases preventing
# OTA updates on Argon specifically to some 3.x versions due to a failed dependency check even though the
# dependencies are satisfied. Swapping two current dependencies seems to help.
ifneq ($(PLATFORM_ID),12)
SYSTEM_PART1_MODULE_DEPENDENCY2 ?= ${MODULE_FUNCTION_RADIO_STACK},0,${SOFTDEVICE_DEPENDENCY}
else
SYSTEM_PART1_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_RADIO_STACK},0,${SOFTDEVICE_DEPENDENCY}
endif
# /FIXME
else
# There is no need to carry SoftDevice dependency on Tracker, since they are manufactured
# with the latest one. Update NCP firmware instead.
ESP32_NCP_DEPENDENCY = 7
SYSTEM_PART1_MODULE_DEPENDENCY2 ?= ${MODULE_FUNCTION_NCP_FIRMWARE},0,${ESP32_NCP_DEPENDENCY}
endif
endif
endif # ($(PLATFORM_MCU),nRF52840)

ifeq ($(PLATFORM_MCU),rtl872x)
SYSTEM_PART1_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_BOOTLOADER},0,${BOOTLOADER_DEPENDENCY}
BOOTLOADER_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_BOOTLOADER},2,${PREBOOTLOADER_PART1_DEPENDENCY}
endif

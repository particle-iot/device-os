# Skip to next 100 every v0.x.0 release (e.g. 108 for v0.6.2 to 200 for v0.7.0-rc.1)
# Bump by 1 for every prerelease or release with the same v0.x.* base.
COMMON_MODULE_VERSION ?= 1512
SYSTEM_PART1_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)
SYSTEM_PART2_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)
SYSTEM_PART3_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)

RELEASE_080_MODULE_VERSION_BASE ?= 300
RELEASE_070_MODULE_VERSION ?= 207
RELEASE_110_MODULE_VERSION ?= 1102

ifneq (,$(filter $(PLATFORM_ID),6 8))
ifeq ($(shell test $(SYSTEM_PART2_MODULE_VERSION) -ge $(RELEASE_080_MODULE_VERSION_BASE); echo $$?),0)
# If this is >= 0.8.x release, Photon and P1 system-part1
# needs to have a dependency on system-part2 of at least 0.7.0
# in order to ensure the device remains online during OTA or Ymodem upgrade
# when trasitioning from uncompressed to compressed wifi firmware.
SYSTEM_PART1_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_SYSTEM_PART},2,${RELEASE_070_MODULE_VERSION}
endif
endif


# Bump by 1 if Tinker has been updated
USER_PART_MODULE_VERSION ?= 6

RELEASE_064_MODULE_VERSION=110

ifneq (,$(filter $(PLATFORM_ID),10))
# make system part1 dependent upon 0.6.4 so that the bootloader is not repeatedly upgraded
SYSTEM_PART3_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_SYSTEM_PART},2,${RELEASE_064_MODULE_VERSION}
endif

# Skip to next 100 every v0.x.0 release (e.g. 11 for v0.6.2 to 100 for v0.7.0-rc.1),
# but only if the bootloader has changed since the last v0.x.0 release.
# Bump by 1 for every updated bootloader image for a release with the same v0.x.* base.
BOOTLOADER_VERSION ?= 502

# The version of the bootloader that the system firmware requires
# NOTE: this will force the device into safe mode until this dependency is met, which is why
# this version usually lags behind the current bootloader version, to avoid non-mandatory updates.
ifeq ($(PLATFORM_GEN),2)
BOOTLOADER_DEPENDENCY = 400
else ifeq ($(PLATFORM_GEN),3)
# FIXME: External QSPI flash initialization issue on B5 SoM
ifeq ($(PLATFORM_ID),25)
BOOTLOADER_DEPENDENCY = 502
else
BOOTLOADER_DEPENDENCY = 501
endif
else
# Some sensible default
BOOTLOADER_DEPENDENCY = 0
endif

ifeq ($(PLATFORM_GEN),3)
# SoftDevice S140 7.0.1
SOFTDEVICE_DEPENDENCY = 202
endif

# Gen 3 platforms require an intermediate update through 1.1.0 to avoid IRQ priority configuration
# issue in DeviceOS < 1.1.0 which presents itself with >= 501 bootloaders
ifneq (,$(filter $(PLATFORM_ID),12 13 14 22 23))
BOOTLOADER_MODULE_DEPENDENCY=${MODULE_FUNCTION_SYSTEM_PART},1,${RELEASE_110_MODULE_VERSION}
endif

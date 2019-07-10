# Skip to next 100 every v0.x.0 release (e.g. 108 for v0.6.2 to 200 for v0.7.0-rc.1)
# Bump by 1 for every prerelease or release with the same v0.x.* base.
COMMON_MODULE_VERSION ?= 1213
SYSTEM_PART1_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)
SYSTEM_PART2_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)
SYSTEM_PART3_MODULE_VERSION ?= $(COMMON_MODULE_VERSION)

RELEASE_080_MODULE_VERSION_BASE ?= 300
RELEASE_070_RC5_MODULE_VERSION ?= 204

ifneq (,$(filter $(PLATFORM_ID),6 8))
ifeq ($(shell test $(SYSTEM_PART2_MODULE_VERSION) -ge $(RELEASE_080_MODULE_VERSION_BASE); echo $$?),0)
# If this is >= 0.8.x release, Photon and P1 system-part1
# needs to have a dependency on system-part2 of at least 0.7.0-rc.4
# in order to ensure the device remains online during OTA or Ymodem upgrade
# when trasitioning from uncompressed to compressed wifi firmware.
SYSTEM_PART1_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_SYSTEM_PART},2,${RELEASE_070_RC5_MODULE_VERSION}
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
BOOTLOADER_VERSION ?= 311

# The version of the bootloader that the system firmware requires
# NOTE: this will force the device into safe mode until this dependency is met, which is why
# this version usually lags behind the current bootloader version, to avoid non-mandatory updates.
ifeq ($(PLATFORM_GEN),2)
BOOTLOADER_DEPENDENCY = 311
else ifeq ($(PLATFORM_GEN),3)
BOOTLOADER_DEPENDENCY = 311
else
# Some sensible default
BOOTLOADER_DEPENDENCY = 0
endif

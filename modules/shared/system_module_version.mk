# Skip to next 100 every v0.x.0 release (e.g. 108 for v0.6.2 to 200 for v0.7.0-rc.1)
# Bump by 1 for every prerelease or release with the same v0.x.* base.
SYSTEM_PART1_MODULE_VERSION ?= 207
SYSTEM_PART2_MODULE_VERSION ?= 207
SYSTEM_PART3_MODULE_VERSION ?= 207

# Bump by 1 if Tinker has been updated
USER_PART_MODULE_VERSION ?= 5

# CONSTANT (DO NOT CHANGE)
RELEASE_064_MODULE_VERSION=110

ifneq (,$(filter $(PLATFORM_ID),10))
# make system part1 dependent upon 0.6.4 so that the bootloader is not repeatedly upgraded
SYSTEM_PART3_MODULE_DEPENDENCY ?= ${MODULE_FUNCTION_SYSTEM_PART},2,${RELEASE_064_MODULE_VERSION}
endif

# Skip to next 100 every v0.x.0 release (e.g. 11 for v0.6.2 to 100 for v0.7.0-rc.1),
# but only if the bootloader has changed since the last v0.x.0 release.
# Bump by 1 for every updated bootloader image for a release with the same v0.x.* base.
BOOTLOADER_VERSION ?= 101

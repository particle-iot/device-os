# makefile to build all dynamically linked modules for the given platform

PROJECT_ROOT = ..

# this ensures that the character xFF is represented as a single byte rather than a 2 byte encoding
export LC_CTYPE=C

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

COMMON_BUILD=$(PROJECT_ROOT)/build
BUILD_PATH_BASE=$(COMMON_BUILD)/target

include $(PROJECT_ROOT)/build/verbose.mk
include $(PROJECT_ROOT)/build/platform-id.mk
include $(PROJECT_ROOT)/build/macros.mk
include $(PROJECT_ROOT)/build/module-defaults.mk
include $(PROJECT_ROOT)/build/arm-tools.mk

$(call check_modular)

projects = $(sort $(wildcard $(current_dir)/$(PLATFORM_DYNALIB_MODULES)/*))

ifneq ("$(findstring system,$(MAKECMDGOALS))","")
MAKECMDGOALS := $(subst system,,$(MAKECMDGOALS))
export USER_PART=n
endif

# when building update-zero, enable minimum build since there is insufficient space
ifneq ("$(findstring update-zero,$(MAKECMDGOALS))","")
$(info Enabling minimal build for update Zero)
export MINIMAL=y
export PRODUCT_FIRMWARE_VERSION=$(VERSION)
export COMPILE_LTO=n
SUBDIR_GOALS = all APP=tinker
else
SUBDIR_GOALS := $(MAKECMDGOALS)
endif

INVOKEDGOALS := $(MAKECMDGOALS)
MAKECMDGOALS := all
MODULE=combined

# only consider projects containing a makefile
ifeq ($(USER_PART),n)
MODULE_EXCLUDE=user-part
endif
makefiles = $(foreach module,$(projects),$(and $(strip $(subst $(MODULE_EXCLUDE),,$(notdir $(module)))),$(wildcard $(module)/makefile)))
$(info $(makefiles))

all $(INVOKEDGOALS): make_deps

UPDATE_ZERO = $(BUILD_PATH)/update-zero-$(VERSION_STRING)-p$(PLATFORM_ID).bin
MODULAR_DIR=.
SYSTEM_PART1_BIN=$(COMMON_BUILD)/target/system-part1/platform-$(PLATFORM_ID)-m$(LTO)/system-part1.bin
SYSTEM_PART2_BIN=$(COMMON_BUILD)/target/system-part2/platform-$(PLATFORM_ID)-m$(LTO)/system-part2.bin

dd_silent = 2>/dev/null

# builds the minimal update zero image.
update-zero: make_deps update-zero-build update-zero-validate

update-zero-build:
	$(call echo, Buliding Update Zero binary to $(UPDATE_ZERO))
	$(VERBOSE)mkdir -p $(dir $(UPDATE_ZERO))
	-$(VERBOSE)rm $(UPDATE_ZERO)
	$(VERBOSE)dd if=/dev/zero ibs=1 count=393212 $(dd_silent) | tr "\000" "\377" > $(UPDATE_ZERO)
	$(VERBOSE)dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(UPDATE_ZERO) conv=notrunc $(dd_slient)
	$(VERBOSE)dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(UPDATE_ZERO) seek=256 conv=notrunc $(dd_silent)
	# 5FFFC is the maximum length (384k-4 bytes). Place in end address in module_info struct
	$(VERBOSE)echo fcff0708 | $(XXD) -r -p | dd bs=1 of=$(UPDATE_ZERO) seek=392 conv=notrunc $(dd_silent)
	# change the module function from system-part modular (04) to monolithic (03 since that's what the OTA verifier is expecting.
	$(VERBOSE)echo 03 | $(XXD) -r -p | dd bs=1 of=$(UPDATE_ZERO) seek=402 conv=notrunc $(dd_silent)
	$(VERBOSE)$(CRC) $(UPDATE_ZERO) | cut -c 1-10 | $(XXD) -r -p >> $(UPDATE_ZERO)

update-zero-validate:
	$(call assert_filesize,$(UPDATE_ZERO),393216)


combined:

combined-app:

$(makefiles):
	$(call,echo,'Making module $@')
	$(VERBOSE)$(MAKE) -C $(dir $@) $(SUBDIR_GOALS) $(MAKE_ARGS) $(MAKEOVERRIDES)

make_deps: $(makefiles)

.PHONY: system stflash program-dfu clean make_deps $(makefiles) update-zero

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

SUBDIR_GOALS := $(MAKECMDGOALS)
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

dd_silent = 2>/dev/null

combined:

combined-app:

$(makefiles):
	$(call,echo,'Making module $@')
ifneq (,$(filter clean,$(SUBDIR_GOALS)))
	$(VERBOSE)$(MAKE) -C $(dir $@) clean $(MAKE_ARGS) $(MAKEOVERRIDES)
ifneq (,$(strip $(filter-out clean,$(SUBDIR_GOALS))))
	$(VERBOSE)$(MAKE) -C $(dir $@) $(filter-out clean,$(SUBDIR_GOALS)) $(MAKE_ARGS) $(MAKEOVERRIDES)
endif
else
	$(VERBOSE)$(MAKE) -C $(dir $@) $(SUBDIR_GOALS) $(MAKE_ARGS) $(MAKEOVERRIDES)
endif

make_deps: $(makefiles)

.NOTPARALLEL:

.PHONY: system program-dfu clean make_deps $(makefiles)

PROJECT_ROOT ?= ../..
COMMON_BUILD=$(PROJECT_ROOT)/build

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

include $(PROJECT_ROOT)/build/verbose.mk
include $(PROJECT_ROOT)/build/platform-id.mk
include $(PROJECT_ROOT)/build/macros.mk
include $(PROJECT_ROOT)/build/module-defaults.mk
include $(PROJECT_ROOT)/build/arm-tools.mk

projects = $(sort $(wildcard $(current_dir)/src/$(PLATFORM_MCU)/*))
makefiles = $(foreach module,$(projects),$(and $(strip $(subst $(MODULE_EXCLUDE),,$(notdir $(module)))),$(wildcard $(module)/makefile)))

SUBDIR_GOALS := $(MAKECMDGOALS)

all: $(makefiles)

$(makefiles):
	$(call,echo,'Making module $@')
	$(VERBOSE)$(MAKE) -C $(dir $@) $(SUBDIR_GOALS) $(MAKE_ARGS) $(MAKEOVERRIDES)


.PHONY: clean all $(makefiles)

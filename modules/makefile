# makefile to build all dynamically linked modules for the given platform

PROJECT_ROOT = ..

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

include $(PROJECT_ROOT)/build/verbose.mk
include $(PROJECT_ROOT)/build/platform-id.mk

ifeq (,$(PLATFORM_DYNALIB_MODULES))
$(error "Platform $(PLATFORM_ID) does not support dynamic modules")
endif

projects = $(sort $(wildcard $(current_dir)/$(PLATFORM_DYNALIB_MODULES)/*))

# only consider projects containing a makefile
makefiles = $(foreach module,$(projects),$(wildcard $(module)/makefile))

SUBDIR_GOALS := $(MAKECMDGOALS)
MAKECMDGOALS := all

all: make_deps
	
clean program-dfu stflash: $(makefiles)
	
$(makefiles):
	$(call,echo,'Making module $@')
	$(VERBOSE)$(MAKE) -C $(dir $@) $(SUBDIR_GOALS) $(MAKE_ARGS) $(MAKEOVERRIDES) 

		
make_deps: $(makefiles)

.PHONY: stflash program-dfu clean make_deps $(makefiles)


# put deliverable products at the end
MAKE_DEPENDENCIES=communication hal platform services wiring bootloader main
PROJECT_ROOT = .
COMMON_BUILD=build
BUILD_PATH_BASE=$(COMMON_BUILD)/target


all: make_deps

include $(COMMON_BUILD)/common-tools.mk
include $(COMMON_BUILD)/recurse.mk
include $(COMMON_BUILD)/verbose.mk

clean: clean_deps
	$(VERBOSE)$(RMDIR) $(BUILD_PATH_BASE)


.PHONY: all
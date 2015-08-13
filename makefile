
# put deliverable products at the end
MAKE_DEPENDENCIES=communication hal platform services wiring bootloader main
PROJECT_ROOT = .
COMMON_BUILD=build
BUILD_PATH_BASE=$(COMMON_BUILD)/target

ifdef SPARK_PRODUCT_ID
PRODUCT_ID=$(SPARK_PRODUCT_ID)
endif

include $(COMMON_BUILD)/platform-id.mk

ifdef PRODUCT_ID
msg_ext =, product ID: $(PRODUCT_ID)
endif

msg = Building firmware for $(PRODUCT_DESC), platform ID: $(PLATFORM_ID)$(msg_ext)

$(info $(msg))

all: make_deps

include $(COMMON_BUILD)/common-tools.mk
include $(COMMON_BUILD)/recurse.mk
include $(COMMON_BUILD)/verbose.mk

clean: clean_deps
	$(VERBOSE)$(RMDIR) $(BUILD_PATH_BASE)


.PHONY: all

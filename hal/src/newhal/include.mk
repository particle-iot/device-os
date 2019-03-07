
# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_NEWHAL_PATH = $(TARGET_HAL_PATH)/src/newhal

INCLUDE_DIRS += $(HAL_SRC_NEWHAL_PATH)

HAL_LINK ?= $(findstring hal,$(MAKE_DEPENDENCIES))

# if hal is used as a make dependency (linked) then add linker commands
ifneq (,$(HAL_LINK))
LINKER_FILE=$(HAL_SRC_NEWHAL_PATH)/linker.ld
LINKER_DEPS=$(LINKER_FILE)

LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -T$(LINKER_FILE)
# support for external linker file
# LD_FLAGS += -L/some/directory
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
endif



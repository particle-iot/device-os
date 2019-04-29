ifeq (,$(SYSTEM_PART1_MODULE_VERSION))
$(error SYSTEM_PART1_MODULE_VERSION not defined)
endif


# TARGET_FILE sets the file stem of the target. It's typically only specified when building applications.
ifdef TARGET_FILE
TARGET_FILE_NAME = $(TARGET_FILE)
endif

# TARGET_DIR is the externally specified target directory
ifdef TARGET_DIR
TARGET_PATH = $(TARGET_DIR)
endif


ifdef APP
# when TARGET_FILE is defined on the command line,
TARGET_FILE_NAME ?= $(notdir $(APP))
TARGET_DIR_NAME ?= applications/$(APP)
endif

ifdef APPDIR
# APPDIR is where the sources are found
# if TARGET_DIR is not defined defaults to $(APPDIR)/target
# if TARGET_FILE_NAME is not defined, defaults to the name of the $(APPDIR)
TARGET_FILE_NAME ?= $(notdir $(APPDIR))
TARGET_DIR_NAME ?= $(APPDIR)/target
# do not use $(BUILD_PATH) since the TARGET_DIR specifies fully where the output should go
ifdef TARGET_DIR
TARGET_PATH = $(TARGET_DIR)/
else
TARGET_PATH = $(TARGET_DIR_NAME)/
endif
BUILD_PATH = $(TARGET_PATH)/obj
endif

ifdef TEST
TARGET_FILE_NAME ?= $(notdir $(TEST))
TARGET_DIR_NAME ?= test/$(TEST)
endif

GLOBAL_DEFINES += MODULE_VERSION=$(USER_PART_MODULE_VERSION)
GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_USER_PART)
GLOBAL_DEFINES += MODULE_INDEX=1
GLOBAL_DEFINES += MODULE_DEPENDENCY=${MODULE_FUNCTION_SYSTEM_PART},1,${SYSTEM_PART1_MODULE_VERSION}
GLOBAL_DEFINES += MODULE_DEPENDENCY2=0,0,0

LINKER_FILE=$(USER_PART_MODULE_PATH)/linker.ld
LINKER_DEPS += $(LINKER_FILE)
LINKER_DEPS += $(SYSTEM_PART1_MODULE_PATH)/module_system_part1_export.ld
NANO_SUFFIX ?= _nano

LDFLAGS += -lnosys --specs=nano.specs
LDFLAGS += -L$(SYSTEM_PART1_MODULE_PATH)
LDFLAGS += -L$(USER_PART_MODULE_PATH)
LDFLAGS += -L$(TARGET_BASE_DIR)
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,--defsym,USER_FIRMWARE_IMAGE_SIZE=$(USER_FIRMWARE_IMAGE_SIZE)
LDFLAGS += -Wl,--defsym,USER_FIRMWARE_IMAGE_LOCATION=$(USER_FIRMWARE_IMAGE_LOCATION)
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
LDFLAGS += -Wl,--defsym,__STACKSIZE__=2048
LDFLAGS += -Wl,--defsym,__STACK_SIZE=2048

# used the -v flag to get gcc to output the commands it passes to the linker when --specs=nano.specs is provided
# LDFLAGS += -lstdc++$(NANO_SUFFIX) -lm -Wl,--start-group -lgcc -Wl,--end-group -Wl,--start-group -lgcc  $(LIBG_TWEAK) -Wl,--end-group

BUILTINS_EXCLUDE = malloc free realloc
CFLAGS += $(addprefix -fno-builtin-,$(BUILTINS_EXCLUDE))

INCLUDE_DIRS += $(SHARED_MODULAR)/inc/user-part
USER_PART_MODULE_SRC_PATH = $(USER_PART_MODULE_PATH)/src

CPPSRC += $(call target_files,$(USER_PART_MODULE_SRC_PATH),*.cpp)
CSRC += $(call target_files,$(USER_PART_MODULE_SRC_PATH),*.c)

all:

# remove *malloc*.o from the standard library. No longer used.
$(LIBG_TWEAK) :
	$(VERBOSE)-rm "$(LIBG_TWEAK)"
	$(VERBOSE)-rm "$(LIBG_TWEAK).tmp"
	$(VERBOSE)cp "`$(CPP) -print-sysroot`/lib/armv7-m/libg_nano.a" $(LIBG_TWEAK).tmp
	$(VERBOSE)$(AR) d "$(LIBG_TWEAK).tmp" lib_a-nano-mallocr.o
	$(VERBOSE)cp "$(LIBG_TWEAK).tmp" "$(LIBG_TWEAK)"


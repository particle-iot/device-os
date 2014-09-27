MAKEOVERRIDES:=$(filter-out TARGET%,$(MAKEOVERRIDES))

# Recursive wildcard function - finds matching files in a directory tree
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
target_files = $(patsubst $(MODULE_PATH)/%,%,$(call rwildcard,$(MODULE_PATH)/$1,$2))

# import this modules symbols
include $(MODULE_PATH)/import.mk

include $(call rwildcard,$(MODULE_PATH)/,build.mk)

# pull in the include.mk files from each dependency, and make them relative to
# the dependency module directory
DEPS_INCLUDE_SCRIPTS =$(foreach module,$(DEPENDENCIES),../$(module)/import.mk)
include $(DEPS_INCLUDE_SCRIPTS)	

# propagate the clean goal, otherwise use the default goal
ifeq ("$(MAKECMDGOALS)","clean")
   SUBDIR_GOALS = clean
endif

	
ifeq ("$(DEBUG_BUILD)","y") 
CFLAGS += -DDEBUG_BUILD
else
CFLAGS += -DRELEASE_BUILD
endif

# add include directories
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS)) -I.
# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -ffunction-sections -Wall -Werror -Wno-switch -fmessage-length=0
CFLAGS += -DSPARK=1

LDFLAGS += $(patsubst %,-L%,$(LIB_DIRS))
LDFLAGS += $(patsubst %,-l%,$(LIBS))

# Assembler flags
ASFLAGS += -x assembler-with-cpp -fmessage-length=0

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(ASRC:.S=.o))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(ASRC:.S=.o.d))

ifeq ("$(TARGET_TYPE)","a") 
TARGET_FILE_PREFIX = lib
endif

ifneq ("$(TARGET_DIR)","")
TARGET_DIR := $(TARGET_DIR)/
endif

TARGET_FILE ?= $(MODULE)
TARGET_DIR ?= ./
TARGET_BASE ?= $(BUILD_PATH)/$(TARGET_DIR)$(TARGET_FILE_PREFIX)$(TARGET_FILE)
TARGET ?= $(TARGET_BASE).$(TARGET_TYPE)

# All Target
all: subdirs $(TARGET)

elf: $(TARGET_BASE).elf
bin: $(TARGET_BASE).bin
hex: $(TARGET_BASE).hex


# Program the core using dfu-util. The core should have been placed
# in bootloader mode before invoking 'make program-dfu'
program-dfu: $(TARGET_BASE).bin
	@echo Flashing using dfu:
	$(DFU) -d 1d50:607f -a 0 -s 0x08005000:leave -D $<

# Program the core using the cloud. SPARK_CORE_ID and SPARK_ACCESS_TOKEN must
# have been defined in the environment before invoking 'make program-cloud'
program-cloud: $(TARGET_BASE).bin
	@echo Flashing using cloud API, CORE_ID=$(SPARK_CORE_ID):
	$(CURL) -X PUT -F file=@$< -F file_type=binary $(CLOUD_FLASH_URL)

# Display size
size: $(TARGET_BASE).elf
	@echo Invoking: ARM GNU Print Size
	$(SIZE) --format=berkeley $<
	@echo

# Create a hex file from ELF file
%.hex : %.elf
	@echo Invoking: ARM GNU Create Flash Image
	$(OBJCOPY) -O ihex $< $@
	@echo

# Create a bin file from ELF file
%.bin : %.elf
	@echo Invoking: ARM GNU Create Flash Image
	$(OBJCOPY) -O binary $< $@
	@echo

$(TARGET_BASE).elf : $(ALLOBJ)
	@echo Building target: $@
	@echo Invoking: ARM GCC C++ Linker
	$(MKDIR) $(dir $@)
	$(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	@echo

# Tool invocations
$(TARGET_BASE).a : $(ALLOBJ)	
	@echo 'Building target: $@'
	@echo 'Invoking: ARM GCC Archiver'
	$(MKDIR) $(dir $@)
	$(AR) -r $@ $^
	@echo ' '

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(MODULE_PATH)/%.c
	@echo Building file: $<
	@echo Invoking: ARM GCC C Compiler
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

# Assember to build .o from .S in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(MODULE_PATH)/%.S
	@echo Building file: $<
	@echo Invoking: ARM GCC Assembler
	$(MKDIR) $(dir $@)
	$(CC) $(ASFLAGS) -c -o $@ $<
	@echo
	
# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(MODULE_PATH)/%.cpp
	@echo Building file: $<
	@echo Invoking: ARM GCC CPP Compiler
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	@echo

# Other Targets
clean:  subdirs
	$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGET)
	$(RMDIR) $(BUILD_PATH)
	@echo ' '

# allow recursive invocation across dependencies to make
subdirs: $(MAKE_DEPENDENCIES)
	
$(MAKE_DEPENDENCIES):
	$(MAKE) -C ../$@ $(SUBDIR_GOALS) $(MAKE_ARGS)

.PHONY: all clean elf bin hex size program-dfu program-cloud subdirs $(MAKE_DEPENDENCIES)
.SECONDARY:

# Include auto generated dependency files
ifneq ("MAKECMDGOALS","clean")
-include $(ALLDEPS)
endif



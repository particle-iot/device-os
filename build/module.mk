# This is the common makefile used to build all top-level modules
# It contains common recipes for bulding C/CPP/asm files to objects, and
# to combine those objects into libraries or elf files.

include $(COMMON_BUILD)/verbose.mk

SOURCE_PATH ?= $(MODULE_PATH)

# Recursive wildcard function - finds matching files in a directory tree
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
target_files = $(patsubst $(SOURCE_PATH)/%,%,$(call rwildcard,$(SOURCE_PATH)/$1,$2))

# import this module's symbols
include $(MODULE_PATH)/import.mk

include $(call rwildcard,$(MODULE_PATH)/,build.mk)

# pull in the include.mk files from each dependency, and make them relative to
# the dependency module directory
DEPS_INCLUDE_SCRIPTS =$(foreach module,$(DEPENDENCIES),../$(module)/import.mk)
include $(DEPS_INCLUDE_SCRIPTS)	
	
ifeq ("$(DEBUG_BUILD)","y") 
CFLAGS += -DDEBUG_BUILD
else
CFLAGS += -DRELEASE_BUILD
endif

ifdef SPARK_TEST_DRIVER
CFLAGS += -DSPARK_TEST_DRIVER=$(SPARK_TEST_DRIVER)
endif

# add include directories
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS)) -I.
# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -ffunction-sections -Wall -Werror -Wno-switch -fmessage-length=0
CFLAGS += -fno-strict-aliasing
CFLAGS += -DSPARK=1

CONLYFLAGS += -Wno-pointer-sign

LDFLAGS += $(patsubst %,-L%,$(LIB_DIRS))
# include libraries twice to avoid gcc library craziness
LDFLAGS += -Wl,--start-group $(patsubst %,-l%,$(LIBS)) -Wl,--end-group

# Assembler flags
ASFLAGS += -x assembler-with-cpp -fmessage-length=0

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o)))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o.d)))


ifeq ("$(TARGET_TYPE)","a") 
TARGET_FILE_PREFIX = lib
endif

# TARGET_FILE_NAME is the file name (minus extension) of the target produced
# TARGET_NAME is the final filename, including any prefix
TARGET_FILE_NAME ?= $(MODULE)
TARGET_NAME ?= $(TARGET_FILE_PREFIX)$(TARGET_FILE_NAME)
TARGET_PATH ?= $(BUILD_PATH)/$(TARGET_DIR_NAME)

# add trailing slash
ifneq ("$(TARGET_PATH)","$(dir $(TARGET_PATH))")
TARGET_SEP = /
endif

TARGET_BASE ?= $(TARGET_PATH)$(TARGET_SEP)$(TARGET_NAME)
TARGET ?= $(TARGET_BASE).$(TARGET_TYPE)

# add BUILD_PATH_EXT with a preceeding slash if not empty.
BUILD_PATH ?= $(BUILD_PATH_BASE)/$(MODULE)$(and $(BUILD_PATH_EXT),/$(BUILD_PATH_EXT))


# All Target
all: $(MAKE_DEPENDENCIES) $(TARGET)

elf: $(TARGET_BASE).elf
bin: $(TARGET_BASE).bin
hex: $(TARGET_BASE).hex
lst: $(TARGET_BASE).lst
exe: $(TARGET_BASE).exe
	@echo Built x-compile executable at $(TARGET_BASE).exe
none: 

# Program the core using dfu-util. The core should have been placed
# in bootloader mode before invoking 'make program-dfu'
program-dfu: $(TARGET_BASE).dfu
	@echo Flashing using dfu:
	$(DFU) -d 1d50:607f -a 0 -s 0x08005000:leave -D $<

# Program the core using the cloud. SPARK_CORE_ID and SPARK_ACCESS_TOKEN must
# have been defined in the environment before invoking 'make program-cloud'
program-cloud: $(TARGET_BASE).bin
	@echo Flashing using cloud API, CORE_ID=$(SPARK_CORE_ID):
	$(CURL) -X PUT -F file=@$< -F file_type=binary $(CLOUD_FLASH_URL)

# Display size
size: $(TARGET_BASE).elf
	$(call,echo,'Invoking: ARM GNU Print Size')
	$(VERBOSE)$(SIZE) --format=berkeley $<
	$(call,echo,)

# create a object listing from the elf file
%.lst: %.elf
	$(call,echo,'Invoking: ARM GNU Create Listing')
	$(VERBOSE)$(OBJDUMP) -h -S $< > $@
	$(call,echo,'Finished building: $@')
	$(call,echo,)

# Create a hex file from ELF file
%.hex : %.elf
	$(call,echo,'Invoking: ARM GNU Create Flash Image')
	$(VERBOSE)$(OBJCOPY) -O ihex $< $@
	$(call,echo,)


# Create a DFU file from bin file
%.dfu: %.bin
	@cp $< $@
	$(DFUSUFFIX) -v 1d50 -p 607f -a $@

# Create a bin file from ELF file
%.bin : %.elf
	$(call,echo,'Invoking: ARM GNU Create Flash Image')
	$(VERBOSE)$(OBJCOPY) -O binary $< $@
	$(call,echo,)

$(TARGET_BASE).exe $(TARGET_BASE).elf : $(ALLOBJ) $(LIB_DEPS)
	$(call,echo,'Building target: $@')
	$(call,echo,'Invoking: ARM GCC C++ Linker')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(call,echo,)	


# Tool invocations
$(TARGET_BASE).a : $(ALLOBJ)
	$(call,echo,'Building target: $@')
	$(call,echo,'Invoking: ARM GCC Archiver')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(AR) -cr $@ $^
	$(call,echo,)

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(SOURCE_PATH)/%.c
	$(call,echo,'Building file: $<')
	$(call,echo,'Invoking: ARM GCC C Compiler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CC) $(CFLAGS) $(CONLYFLAGS) -c -o $@ $<
	$(call,echo,)

# Assember to build .o from .S in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(COMMON_BUILD)/arm/%.S
	$(call,echo,'Building file: $<')
	$(call,echo,'Invoking: ARM GCC Assembler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CC) $(ASFLAGS) -c -o $@ $<
	$(call,echo,)
	
# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(SOURCE_PATH)/%.cpp
	$(call,echo,'Building file: $<')
	$(call,echo,'Invoking: ARM GCC CPP Compiler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	$(call,echo,)

# Other Targets
clean: clean_deps
	$(VERBOSE)$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGET)
	$(VERBOSE)$(RMDIR) $(BUILD_PATH)
	$(call,echo,)

.PHONY: all none elf bin hex size program-dfu program-cloud
.SECONDARY:

include $(COMMON_BUILD)/recurse.mk


# Include auto generated dependency files
ifneq ("MAKECMDGOALS","clean")
-include $(ALLDEPS)
endif



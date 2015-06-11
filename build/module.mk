# This is the common makefile used to build all top-level modules
# It contains common recipes for bulding C/CPP/asm files to objects, and
# to combine those objects into libraries or elf files.
include $(COMMON_BUILD)/macros.mk

SOURCE_PATH ?= $(MODULE_PATH)

# Recursive wildcard function - finds matching files in a directory tree
target_files = $(patsubst $(SOURCE_PATH)/%,%,$(call rwildcard,$(SOURCE_PATH)/$1,$2))

# import this module's symbols
include $(MODULE_PATH)/import.mk

# pull in the include.mk files from each dependency, and make them relative to
# the dependency module directory
DEPS_INCLUDE_SCRIPTS =$(foreach module,$(DEPENDENCIES),$(PROJECT_ROOT)/$(module)/import.mk)
include $(DEPS_INCLUDE_SCRIPTS)

include $(call rwildcard,$(MODULE_PATH)/,build.mk)

# Uncomment the following to enable serial bitrate specific dfu/ymodem flasher in code
START_DFU_FLASHER_SERIAL_SPEED=14400
# Uncommenting this increase the size of the firmware image because of ymodem addition
START_YMODEM_FLASHER_SERIAL_SPEED=28800

QUOTE='
ifneq (,$(GLOBAL_DEFINES))
MAKE_ARGS+=$(QUOTE)GLOBAL_DEFINES=$(GLOBAL_DEFINES)$(QUOTE)
CFLAGS += $(addprefix -D,$(GLOBAL_DEFINES))
endif

ifdef TEACUP
CFLAGS += -DTEACUP
endif

ifeq ("$(DEBUG_BUILD)","y")
CFLAGS += -DDEBUG_BUILD
COMPILE_LTO ?= n
else
CFLAGS += -DRELEASE_BUILD
endif

ifdef SPARK_TEST_DRIVER
CFLAGS += -DSPARK_TEST_DRIVER=$(SPARK_TEST_DRIVER)
endif

ifeq ("$(SPARK_CLOUD)","n")
CFLAGS += -DSPARK_NO_CLOUD
endif

ifeq ("$(SPARK_WIFI)","n")
CFLAGS += -DSPARK_NO_WIFI
endif


# disable COMPILE_LTO when JTAG is enabled since it obfuscates the symbol mapping
# breaking step debugging
ifeq ($(USE_SWD),y)
COMPILE_LTO ?= n
endif

ifeq ($(USE_SWD_JTAG),y)
COMPILE_LTO ?= n
endif

WARNINGS_AS_ERRORS ?= y
ifeq ($(WARNINGS_AS_ERRORS),y)
CFLAGS += -Werror
endif

# add include directories
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS)) -I.
# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d
# Removed "-fdata-sections" as firmware doesn't work as expected
CFLAGS += -ffunction-sections -fdata-sections -Wall -Wno-switch -Wno-error=deprecated-declarations -fmessage-length=0
CFLAGS += -fno-strict-aliasing
CFLAGS += -DSPARK=1

ifdef START_DFU_FLASHER_SERIAL_SPEED
CFLAGS += -DSTART_DFU_FLASHER_SERIAL_SPEED=$(START_DFU_FLASHER_SERIAL_SPEED)
endif
ifdef START_YMODEM_FLASHER_SERIAL_SPEED
CFLAGS += -DSTART_YMODEM_FLASHER_SERIAL_SPEED=$(START_YMODEM_FLASHER_SERIAL_SPEED)
endif

CONLYFLAGS += -Wno-pointer-sign -std=gnu99

LDFLAGS += $(LIBS_EXT)
LDFLAGS += $(patsubst %,-L%,$(LIB_DIRS))
LDFLAGS += -Wl,--whole-archive $(patsubst %,-l%,$(LIBS)) -Wl,--no-whole-archive
LDFLAGS += -L$(COMMON_BUILD)/arm/linker

# Assembler flags
ASFLAGS += -x assembler-with-cpp -fmessage-length=0

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o)))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o.d)))

ifeq (y,$(MODULAR_FIRMWARE))
MODULAR_EXT = -m
endif

COMPILE_LTO ?= y
ifeq (y,$(COMPILE_LTO))
LTO_EXT = -lto
MAKE_ARGS += COMPILE_LTO=$(COMPILE_LTO)
endif

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

BUILD_TARGET_PLATFORM = platform-$(PLATFORM_ID)$(MODULAR_EXT)$(LTO_EXT)
BUILD_PATH_EXT ?= $(BUILD_TARGET_PLATFORM)


# All Target
all: $(MAKE_DEPENDENCIES) $(TARGET)

elf: $(TARGET_BASE).elf
bin: $(TARGET_BASE).bin
hex: $(TARGET_BASE).hex
lst: $(TARGET_BASE).lst
exe: $(TARGET_BASE).exe
	@echo Built x-compile executable at $(TARGET_BASE).exe
none:
	;

st-flash: $(TARGET_BASE).bin
	@echo Flashing $< using st-flash to address $(PLATFORM_DFU)
	st-flash write $< $(PLATFORM_DFU)

# Program the core using dfu-util. The core should have been placed
# in bootloader mode before invoking 'make program-dfu'
program-dfu: $(TARGET_BASE).dfu
ifdef START_DFU_FLASHER_SERIAL_SPEED
# SPARK_SERIAL_DEV should be set something like /dev/tty.usbxxxx and exported
#ifndef SPARK_SERIAL_DEV
ifeq ("$(wildcard $(SPARK_SERIAL_DEV))","")
	@echo Serial device SPARK_SERIAL_DEV : $(SPARK_SERIAL_DEV) not available
else
	@echo Entering dfu bootloader mode:
	stty -f $(SPARK_SERIAL_DEV) $(START_DFU_FLASHER_SERIAL_SPEED)
	sleep 1
endif
endif
	@echo Flashing using dfu:
	$(DFU) -d $(USBD_VID_SPARK):$(USBD_PID_DFU) -a 0 -s $(PLATFORM_DFU)$(if $(PLATFORM_DFU_LEAVE),:leave) -D $<

# Program the core using the cloud. SPARK_CORE_ID and SPARK_ACCESS_TOKEN must
# have been defined in the environment before invoking 'make program-cloud'
program-cloud: $(TARGET_BASE).bin
	@echo Flashing using cloud API, CORE_ID=$(SPARK_CORE_ID):
	$(CURL) -X PUT -F file=@$< -F file_type=binary $(CLOUD_FLASH_URL)

program-serial: $(TARGET_BASE).bin
ifdef START_YMODEM_FLASHER_SERIAL_SPEED
# Program core/photon using serial ymodem flasher.
# Install 'sz' tool using: 'brew install lrzsz' on MAC OS X
# SPARK_SERIAL_DEV should be set something like /dev/tty.usbxxxx and exported
ifeq ("$(wildcard $(SPARK_SERIAL_DEV))","")
	@echo Serial device SPARK_SERIAL_DEV : $(SPARK_SERIAL_DEV) not available
else
	@echo Entering serial programmer mode:
	stty -f $(SPARK_SERIAL_DEV) $(START_YMODEM_FLASHER_SERIAL_SPEED)
	sleep 1
	@echo Flashing using serial ymodem protocol:
# Got some issue currently in getting 'sz' working
	sz -b -v --ymodem $< > $(SPARK_SERIAL_DEV) < $(SPARK_SERIAL_DEV)
endif
endif

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
	$(DFUSUFFIX) -v $(subst 0x,,$(USBD_VID_SPARK)) -p $(subst 0x,,$(USBD_PID_DFU)) -a $@

# generated by running xxd -p crc_block
CRC_LEN = 4
CRC_BLOCK_LEN = 38
DEFAULT_SHA_256 = 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20
MOD_INFO_SUFFIX_LEN ?= 2800
MOD_INFO_SUFFIX = $(DEFAULT_SHA_256)$(MOD_INFO_SUFFIX_LEN)
CRC_BLOCK_CONTENTS = $(MOD_INFO_SUFFIX)78563412

ifneq (WINDOWS,$(MAKE_OS))
SHA_256 = shasum -a 256
else
SHA_256 = $(COMMON_BUILD)/bin/win32/sha256sum
endif

ifeq (WINDOWS,$(MAKE_OS))
filesize=`stat --print %s $1`
else
ifeq (LINUX, $(MAKE_OS))
filesize=`stat -c %s $1`
else
filesize=`stat -f%z $1`
endif
endif

# Create a bin file from ELF file
%.bin : %.elf
	$(call echo,'Invoking: ARM GNU Create Flash Image')
	$(VERBOSE)$(OBJCOPY) -O binary $< $@.pre_crc
	if [ -s $@.pre_crc ]; then \
	head -c $$(($(call filesize,$@.pre_crc) - $(CRC_BLOCK_LEN))) $@.pre_crc > $@.no_crc && \
	tail -c $(CRC_BLOCK_LEN) $@.pre_crc > $@.crc_block && \
	test "$(CRC_BLOCK_CONTENTS)" = `xxd -p -c 500 $@.crc_block` && \
	$(SHA_256) $@.no_crc | cut -c 1-65 | $(XXD) -r -p | dd bs=1 of=$@.pre_crc seek=$$(($(call filesize,$@.pre_crc) - $(CRC_BLOCK_LEN))) conv=notrunc && \
	head -c $$(($(call filesize,$@.pre_crc) - $(CRC_LEN))) $@.pre_crc > $@.no_crc && \
	 $(CRC) $@.no_crc | cut -c 1-10 | $(XXD) -r -p | dd bs=1 of=$@.pre_crc seek=$$(($(call filesize,$@.pre_crc) - $(CRC_LEN))) conv=notrunc;\
	fi
	-rm $@
	mv $@.pre_crc $@
	$(call echo,)


$(TARGET_BASE).exe $(TARGET_BASE).elf : $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: ARM GCC C++ Linker')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(call echo,)


# Tool invocations
$(TARGET_BASE).a : $(ALLOBJ)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: ARM GCC Archiver')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(AR) -cr $@ $^
	$(call echo,)

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(SOURCE_PATH)/%.c
	$(call echo,'Building file: $<')
	$(call echo,'Invoking: ARM GCC C Compiler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CC) $(CFLAGS) $(CONLYFLAGS) -c -o $@ $<
	$(call echo,)

# Assember to build .o from .S in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(COMMON_BUILD)/arm/%.S
	$(call echo,'Building file: $<')
	$(call echo,'Invoking: ARM GCC Assembler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CC) $(ASFLAGS) -c -o $@ $<
	$(call echo,)

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(SOURCE_PATH)/%.cpp
	$(call echo,'Building file: $<')
	$(call echo,'Invoking: ARM GCC CPP Compiler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	$(call echo,)

# Other Targets
clean: clean_deps
	$(VERBOSE)$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGET)
	$(VERBOSE)$(RMDIR) $(BUILD_PATH)
	$(call,echo,)

.PHONY: all none elf bin hex size program-dfu program-cloud st-flash program-serial
.SECONDARY:

include $(COMMON_BUILD)/recurse.mk


# Include auto generated dependency files
ifneq ("MAKECMDGOALS","clean")
-include $(ALLDEPS)
endif



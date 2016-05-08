# This is the common makefile used to build all top-level modules
# It contains common recipes for bulding C/CPP/asm files to objects, and
# to combine those objects into libraries or elf files.
include $(COMMON_BUILD)/macros.mk

SOURCE_PATH ?= $(MODULE_PATH)

# Recursive wildcard function - finds matching files in a directory tree
target_files = $(patsubst $(SOURCE_PATH)/%,%,$(call rwildcard,$(SOURCE_PATH)/$1,$2))
here_files = $(call wildcard,$(SOURCE_PATH)/$1$2)

# import this module's symbols
include $(MODULE_PATH)/import.mk

# pull in the include.mk files from each dependency, and make them relative to
# the dependency module directory
DEPS_INCLUDE_SCRIPTS =$(foreach module,$(DEPENDENCIES),$(PROJECT_ROOT)/$(module)/import.mk)
include $(DEPS_INCLUDE_SCRIPTS)

include $(COMMON_BUILD)/module-defaults.mk

include $(call rwildcard,$(MODULE_PATH)/,build.mk)

# add trailing slash
ifneq ("$(TARGET_PATH)","$(dir $(TARGET_PATH))")
TARGET_SEP = /
endif

TARGET_FILE_NAME ?= $(MODULE)

ifneq (,$(GLOBAL_DEFINES))
CFLAGS += $(addprefix -D,$(GLOBAL_DEFINES))
export GLOBAL_DEFINES
endif

# fixes build errors on ubuntu with arm gcc 5.3.1
# GNU_SOURCE is needed for isascii/toascii
# WINSOCK_H stops select.h from being used which conflicts with CC3000 headers
CFLAGS += -D_GNU_SOURCE -D_WINSOCK_H

# Global category name for logging
ifneq (,$(LOG_MODULE_CATEGORY))
CFLAGS += -DLOG_MODULE_CATEGORY="\"$(LOG_MODULE_CATEGORY)\""
endif

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o)))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o.d)))

CLOUD_FLASH_URL ?= https://api.spark.io/v1/devices/$(SPARK_CORE_ID)\?access_token=$(SPARK_ACCESS_TOKEN)

# All Target
all: $(TARGET) postbuild

build_dependencies: $(MAKE_DEPENDENCIES)

elf: $(TARGET_BASE).elf
bin: $(TARGET_BASE).bin
hex: $(TARGET_BASE).hex
lst: $(TARGET_BASE).lst
exe: $(TARGET_BASE)$(EXECUTABLE_EXTENSION)
	@echo Built x-compile executable at $(TARGET_BASE)$(EXECUTABLE_EXTENSION)
none:
	;

st-flash: $(TARGET_BASE).bin
	@echo Flashing $< using st-flash to address $(PLATFORM_DFU)
	st-flash write $< $(PLATFORM_DFU)

ifneq ("$(OPENOCD_HOME)","")

program-openocd: $(TARGET_BASE).bin
	@echo Flashing $< using openocd to address $(PLATFORM_DFU)
	$(OPENOCD_HOME)/openocd -f $(OPENOCD_HOME)/tcl/interface/ftdi/particle-ftdi.cfg -f $(OPENOCD_HOME)/tcl/target/stm32f2x.cfg  -c "init; reset halt" -c "flash protect 0 0 11 off" -c "program $< $(PLATFORM_DFU) reset exit"

endif

# Program the core using dfu-util. The core should have been placed
# in bootloader mode before invoking 'make program-dfu'
program-dfu: $(TARGET_BASE).dfu
ifdef START_DFU_FLASHER_SERIAL_SPEED
# PARTICLE_SERIAL_DEV should be set something like /dev/tty.usbxxxx and exported
#ifndef PARTICLE_SERIAL_DEV
ifeq ("$(wildcard $(PARTICLE_SERIAL_DEV))","")
	@echo Serial device PARTICLE_SERIAL_DEV : $(PARTICLE_SERIAL_DEV) not available
else
	@echo Entering dfu bootloader mode:
	$(SERIAL_SWITCHER) $(START_DFU_FLASHER_SERIAL_SPEED) $(PARTICLE_SERIAL_DEV)
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
# PARTICLE_SERIAL_DEV should be set something like /dev/tty.usbxxxx and exported
ifeq ("$(wildcard $(PARTICLE_SERIAL_DEV))","")
	@echo Serial device PARTICLE_SERIAL_DEV : $(PARTICLE_SERIAL_DEV) not available
else
	@echo Entering serial programmer mode:
	$(SERIAL_SWITCHER) $(START_YMODEM_FLASHER_SERIAL_SPEED) $(PARTICLE_SERIAL_DEV)
	sleep 1
	@echo Flashing using serial ymodem protocol:
# Got some issue currently in getting 'sz' working
	sz -b -v --ymodem $< > $(PARTICLE_SERIAL_DEV) < $(PARTICLE_SERIAL_DEV)
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

# OS X + debian systems have shasum, RHEL + windows have sha256sum
SHASUM_COMMAND_VERSION := $(shell shasum --version 2>/dev/null)
SHA256SUM_COMMAND_VERSION := $(shell sha256sum --version 2>/dev/null)
ifdef SHASUM_COMMAND_VERSION
SHA_256 = shasum -a 256
else ifdef SHA256SUM_COMMAND_VERSION
SHA_256 = sha256sum
else ifeq (WINDOWS,$(MAKE_OS))
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
	$(VERBOSE)if [ -s $@.pre_crc ]; then \
	head -c $$(($(call filesize,$@.pre_crc) - $(CRC_BLOCK_LEN))) $@.pre_crc > $@.no_crc && \
	tail -c $(CRC_BLOCK_LEN) $@.pre_crc > $@.crc_block && \
	test "$(CRC_BLOCK_CONTENTS)" = `xxd -p -c 500 $@.crc_block` && \
	$(SHA_256) $@.no_crc | cut -c 1-65 | $(XXD) -r -p | dd bs=1 of=$@.pre_crc seek=$$(($(call filesize,$@.pre_crc) - $(CRC_BLOCK_LEN))) conv=notrunc $(VERBOSE_REDIRECT) && \
	head -c $$(($(call filesize,$@.pre_crc) - $(CRC_LEN))) $@.pre_crc > $@.no_crc && \
	 $(CRC) $@.no_crc | cut -c 1-10 | $(XXD) -r -p | dd bs=1 of=$@.pre_crc seek=$$(($(call filesize,$@.pre_crc) - $(CRC_LEN))) conv=notrunc $(VERBOSE_REDIRECT);\
	fi
	$(VERBOSE)[ ! -f $@ ] || rm $@
	$(VERBOSE)mv $@.pre_crc $@
	$(call echo,)


$(TARGET_BASE).elf : build_dependencies $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: ARM GCC C++ Linker')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(call echo,)

$(TARGET_BASE)$(EXECUTABLE_EXTENSION) : build_dependencies $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: GCC C++ Linker')
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

.PHONY: all postbuild none elf bin hex size program-dfu program-cloud st-flash program-serial
.SECONDARY:

include $(COMMON_BUILD)/recurse.mk


# Include auto generated dependency files
ifneq ("MAKECMDGOALS","clean")
-include $(ALLDEPS)
endif



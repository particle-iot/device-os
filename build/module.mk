# This is the common makefile used to build all top-level modules
# It contains common recipes for bulding C/CPP/asm files to objects, and
# to combine those objects into libraries or elf files.
include $(COMMON_BUILD)/macros.mk

SOURCE_PATH ?= $(MODULE_PATH)

# import this module's symbols
include $(MODULE_PATH)/import.mk

# FIXME: find a better place for this
ifneq (,$(filter $(PLATFORM_ID),12 13 15 22 23 25 26 37))
ifneq ("$(BOOTLOADER_MODULE)","1")
export SOFTDEVICE_PRESENT=y
CFLAGS += -DSOFTDEVICE_PRESENT=1
ASFLAGS += -DSOFTDEVICE_PRESENT=1
export SOFTDEVICE_VARIANT=s140
CFLAGS += -D$(shell echo $(SOFTDEVICE_VARIANT) | tr a-z A-Z)
PLATFORM_FREERTOS =
endif
endif

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

ifneq (,$(NO_GNU_EXTENSIONS))
# Stick to POSIX-conforming API
CFLAGS += -D_POSIX_C_SOURCE=200809
else
# Enable GNU extensions
CFLAGS += -D_GNU_SOURCE
endif

# Global category name for logging
ifneq (,$(LOG_MODULE_CATEGORY))
CFLAGS += -DLOG_MODULE_CATEGORY="\"$(LOG_MODULE_CATEGORY)\""
endif

# Adds the sources from the specified library directories
# v1 libraries include all sources
LIBCPPSRC += $(call target_files_dirs,$(MODULE_LIBSV1),,*.cpp)
LIBCSRC += $(call target_files_dirs,$(MODULE_LIBSV1),,*.c)

# v2 libraries only include sources in the "src" dir
LIBCPPSRC += $(call target_files_dirs,$(MODULE_LIBSV2)/,src/,*.cpp)
LIBCSRC += $(call target_files_dirs,$(MODULE_LIBSV2)/,src/,*.c)


CPPSRC += $(LIBCPPSRC)
CSRC += $(LIBCSRC)

# add all module libraries as include directories
INCLUDE_DIRS += $(MODULE_LIBSV1)

# v2 libraries contain their sources under a "src" folder
INCLUDE_DIRS += $(addsuffix /src,$(MODULE_LIBSV2))

# $(info cppsrc $(CPPSRC))
# $(info csrc $(CSRC))


# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o)))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))

# $(info allobj $(ALLOBJ))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(patsubst $(COMMON_BUILD)/arm/%,%,$(ASRC:.S=.o.d)))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))

CLOUD_FLASH_URL ?= https://api.particle.io/v1/devices/$(PARTICLE_DEVICE_ID)

# All Target
all: prebuild $(MAKE_DEPENDENCIES) $(TARGET) postbuild

elf: $(TARGET_BASE).elf
bin: $(TARGET_BASE).bin
hex: $(TARGET_BASE).hex
lst: $(TARGET_BASE).lst
exe: $(TARGET_BASE)$(EXECUTABLE_EXTENSION)
	@echo Built x-compile executable at $(TARGET_BASE)$(EXECUTABLE_EXTENSION)
none:
	;

ifeq ($(PLATFORM_MCU),rtl872x)
.PHONY: rtl-flash
rtl_module_start_address = $(subst 0x08,0x00,$(call get_module_start_address))
rtl-flash: | $(TARGET_BASE).bin
	$(PROJECT_ROOT)/scripts/flash.sh $(PROJECT_ROOT)/scripts/rtl872x.tcl $(TARGET_BASE).bin $(call rtl_module_start_address)
endif

# Program the device using dfu-util. The device should have been placed
# in bootloader mode before invoking 'make program-dfu'
program-dfu: $(MAKE_DEPENDENCIES) $(TARGET_BASE).dfu
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
ifneq ($(PLATFORM_DFU),)
	$(DFU) -d $(USBD_VID_PARTICLE):$(USBD_PID_DFU) -a 0 -s $(PLATFORM_DFU)$(if $(PLATFORM_DFU_LEAVE),:leave) -D $(lastword $^)
else
	$(DFU) -d $(USBD_VID_PARTICLE):$(USBD_PID_DFU) -a 0 -s $(call get_module_start_address)$(if $(PLATFORM_DFU_LEAVE),:leave) -D $(lastword $^)
endif

# Program the device using the cloud. PARTICLE_DEVICE_ID and PARTICLE_ACCESS_TOKEN must
# have been defined in the environment before invoking 'make program-cloud'
program-cloud: $(MAKE_DEPENDENCIES) $(TARGET_BASE).bin
	@echo Flashing using cloud API, CORE_ID=$(PARTICLE_DEVICE_ID):
	$(CURL) -X PUT -F file=@$(lastword $^) -F file_type=binary $(CLOUD_FLASH_URL) -H "Authorization: Bearer $(PARTICLE_ACCESS_TOKEN)"

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
	$(DFUSUFFIX) -v $(subst 0x,,$(USBD_VID_PARTICLE)) -p $(subst 0x,,$(USBD_PID_DFU)) -a $@

# Geometry of module_info_suffix_base_t (see dynalib/inc/module_info.h): the base part is
# always the last bytes of the module suffix and ends with a 32 byte SHA-256 followed by the
# 2 byte suffix size. Everything else (the position of the suffix and of the CRC) is derived
# from the linker symbols, see the %.bin rule below.
# Note that the stored suffix size counts the CRC on platforms without suffix extensions and
# does not on platforms with them, see MODULE_INFO_SUFFIX_EXTRA in dynalib/inc/module_info.inc.
CRC_LEN = 4
SHA_256_LEN = 32
SUFFIX_SIZE_FIELD_LEN = 2
# This is platform-specific and comes from platform-id.mk
MODULE_SUFFIX_PRODUCT_DATA_OFFSET_FROM_END ?= 0

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
	[ ! -f $@.product ] || rm $@.product
	$(VERBOSE)$(OBJCOPY) $< --dump-section '.module_info_product=$@.product' > /dev/null 2>&1 || true
	$(VERBOSE)if [ -s $@.product ] && [ $(MODULE_SUFFIX_PRODUCT_DATA_OFFSET_FROM_END) -ne 0 ]; then \
		$(OBJCOPY) $< --dump-section '.module_info_suffix=$@.suffix' && \
		$(OBJCOPY) $< --remove-section '.module_info_product' && \
		dd bs=1 if=$@.product of=$@.suffix seek=$$(($(call filesize,$@.suffix) - $(MODULE_SUFFIX_PRODUCT_DATA_OFFSET_FROM_END))) conv=notrunc $(VERBOSE_REDIRECT) && \
		$(OBJCOPY) $< --update-section '.module_info_suffix=$@.suffix'; \
	fi
	$(VERBOSE)$(OBJCOPY) -O binary $< $@.pre_crc
	$(VERBOSE)if $(OBJDUMP) -h $< -j '.module_info_legacy' $(VERBOSE_REDIRECT) >/dev/null 2>&1; then \
		([ ! -f $@.legacy_info ] || rm $@.legacy_info) && \
		dd if=$@.pre_crc of=$@.legacy_info bs=1 skip=$$(($(call get_symbol_address,$<,link_module_start_legacy) - $(call get_symbol_address,$<,link_module_start))) \
			conv=notrunc count=$$(($(call get_symbol_address,$<,link_module_info_crc_legacy_start) - $(call get_symbol_address,$<,link_module_start_legacy))) $(VERBOSE_REDIRECT) && \
		$(CRC) $@.legacy_info | cut -c 1-10 | $(XXD) -r -p | \
			dd bs=1 of=$@.pre_crc seek=$$(($(call get_symbol_address,$<,link_module_info_crc_legacy_start) - $(call get_symbol_address,$<,link_module_start))) conv=notrunc $(VERBOSE_REDIRECT) && \
		dd bs=1 if=$@.pre_crc of=$@.crc_legacy_updated skip=$$(($(call get_symbol_address,$<,link_module_info_crc_legacy_start) - $(call get_symbol_address,$<,link_module_start))) count=$(CRC_LEN) $(VERBOSE_REDIRECT) && \
		$(OBJCOPY) $< --update-section '.module_info_crc_legacy=$@.crc_legacy_updated'; \
	fi
	$(VERBOSE)if [ -s $@.pre_crc ]; then \
	eval `$(READELF) -W -s $< | $(AWK) '\
		$$8 == "link_module_start" { print "mod_start=0x" $$2 } \
		$$8 == "link_module_info_suffix_start" { print "suffix_start=0x" $$2 } \
		$$8 == "link_module_info_suffix_end" { print "suffix_end=0x" $$2 } \
		$$8 == "link_module_info_crc_start" { print "crc_start=0x" $$2 } \
		$$8 == "link_module_info_crc_end" { print "crc_end=0x" $$2 }'`; \
	if [ -n "$$mod_start" ] && [ -n "$$suffix_start" ] && [ -n "$$suffix_end" ] && [ -n "$$crc_start" ] && [ -n "$$crc_end" ]; then \
	sha_offset=$$(($$suffix_end - $$mod_start - $(SHA_256_LEN) - $(SUFFIX_SIZE_FIELD_LEN))) && \
	crc_offset=$$(($$crc_start - $$mod_start)) && \
	suffix_size=$$(($$suffix_end - $$suffix_start)) && \
	stored_size=$$((0x`$(XXD) -s $$(($$sha_offset + $(SHA_256_LEN))) -l $(SUFFIX_SIZE_FIELD_LEN) -p $@.pre_crc | sed 's/\(..\)\(..\)/\2\1/'`)) && \
	test $$(($$crc_end - $$crc_start)) -eq $(CRC_LEN) && \
	test $(call filesize,$@.pre_crc) -eq $$(($$crc_offset + $(CRC_LEN))) && \
	{ test $$stored_size -eq $$suffix_size || test $$stored_size -eq $$(($$suffix_size + $(CRC_LEN))); } && \
	head -c $$sha_offset $@.pre_crc > $@.no_crc && \
	$(SHA_256) $@.no_crc | cut -c 1-65 | $(XXD) -r -p | dd bs=1 of=$@.pre_crc seek=$$sha_offset conv=notrunc $(VERBOSE_REDIRECT) && \
	head -c $$crc_offset $@.pre_crc > $@.no_crc && \
	$(CRC) $@.no_crc | cut -c 1-10 | $(XXD) -r -p | dd bs=1 of=$@.pre_crc seek=$$crc_offset conv=notrunc $(VERBOSE_REDIRECT) && \
	$(OBJCOPY) $< --dump-section '.module_info_suffix=$@.suffix_updated' && \
	dd bs=1 if=$@.pre_crc of=$@.suffix_updated skip=$$sha_offset seek=$$(($$suffix_size - $(SHA_256_LEN) - $(SUFFIX_SIZE_FIELD_LEN))) count=$(SHA_256_LEN) conv=notrunc $(VERBOSE_REDIRECT) && \
	dd bs=1 if=$@.pre_crc of=$@.crc_updated skip=$$crc_offset count=$(CRC_LEN) $(VERBOSE_REDIRECT) && \
	$(OBJCOPY) $< --update-section '.module_info_suffix=$@.suffix_updated' --update-section '.module_info_crc=$@.crc_updated'; \
	fi; \
	fi
	$(VERBOSE)[ ! -f $@ ] || rm $@
	$(VERBOSE)mv $@.pre_crc $@
	# The ELF is updated with the hash and CRC above, so make sure the binary stays the
	# newer file, otherwise this rule would run again on every build. mv keeps the mtime
	# of the temporary file, which predates the update of the ELF.
	$(VERBOSE)touch $@
	$(call echo,)


$(TARGET_BASE).elf : $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: ARM GCC C++ Linker')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(call echo,)

$(TARGET_BASE)$(EXECUTABLE_EXTENSION) : $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: GCC C++ Linker')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(call echo,)


# Tool invocations
$(TARGET_BASE).a : $(ALLOBJ)
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: ARM GCC Archiver')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(AR) -cr $@ $^
	$(call echo,)

define build_C_file
	$(call echo,'Building c file: $<')
	$(call echo,'Invoking: ARM GCC C Compiler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CC) $(CFLAGS) $(CONLYFLAGS) -D__FILE_NAME__=\"$(notdir $<)\" -c -o $@ $<
	$(call echo,)
endef

define build_CPP_file
	$(call echo,'Building cpp file: $<')
	$(call echo,'Invoking: ARM GCC CPP Compiler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CC) $(CFLAGS) $(CPPFLAGS) -D__FILE_NAME__=\"$(notdir $<)\" -c -o $@ $<
	$(call echo,)
endef

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(SOURCE_PATH)/%.c
	$(build_C_file)

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(SOURCE_PATH)/%.cpp
	$(build_CPP_file)

define build_LIB_files
$(BUILD_PATH)/$(notdir $1)/%.o : $1/%.c
	$$(build_C_file)

$(BUILD_PATH)/$(notdir $1)/%.o : $1/%.cpp
	$$(build_CPP_file)
endef

# define rules for each library
# only the sources added for each library are built (so for v2 libraries only files under "src" are built.)
$(foreach lib,$(MODULE_LIBSV1) $(MODULE_LIBSV2),$(eval $(call build_LIB_files,$(lib))))

# Assember to build .o from .S in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(COMMON_BUILD)/arm/%.S
	$(call echo,'Building file: $<')
	$(call echo,'Invoking: ARM GCC Assembler')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CC) $(ASFLAGS) -c -o $@ $<
	$(call echo,)


# Other Targets
clean: clean_deps
	$(VERBOSE)$(foreach cleanfile,$(ALLOBJ) $(ALLDEPS) $(TARGET),$(shell $(RM) $(cleanfile)))
	$(VERBOSE)$(RMDIR) $(BUILD_PATH)
	$(call,echo,)

.PHONY: all prebuild postbuild none elf bin hex size program-dfu program-cloud program-serial
.SECONDARY:

# Disable implicit builtin rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(COMMON_BUILD)/recurse.mk

ifneq ($(strip $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)),)
$(sort $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS)): | $(MAKE_DEPENDENCIES) prebuild
endif
ifneq ($(strip $(MAKE_DEPENDENCIES)),)
$(MAKE_DEPENDENCIES): | prebuild
endif
postbuild: | $(TARGET)

ifneq (,$(filter clean,$(MAKECMDGOALS)))
ifneq (,$(filter-out clean,$(MAKECMDGOALS)))
.NOTPARALLEL:
endif
endif

ifneq ("$(findstring elf_fi,$(TARGET))","")
$(TARGET_BASE).bin $(TARGET_BASE).hex $(TARGET_BASE).lst size: | $(TARGET_BASE)_fi.elf
.NOTPARALLEL:
endif

ifneq (,$(filter bin,$(TARGET)))
$(TARGET_BASE).hex $(TARGET_BASE).lst size: | $(TARGET_BASE).bin
endif


# Include auto generated dependency files
ifneq ("MAKECMDGOALS","clean")
-include $(ALLDEPS)
endif



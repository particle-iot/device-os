MODULE=user-part
USER_PART_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = $(USER_FIRMWARE_IMAGE_LOCATION)
PLATFORM_DFU_LEAVE = y
# disable LTO build on 2015q2 - see issue #471
export COMPILE_LTO=n

# communication used only for header declarations
DEPENDENCIES = user dynalib services services-dynalib hal hal-dynalib system system-dynalib rt-dynalib wiring communication-dynalib modules/xenon/system-part1 platform wiring_globals
MAKE_DEPENDENCIES = user hal-dynalib services-dynalib system-dynalib rt-dynalib wiring communication-dynalib platform wiring_globals
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(USER_LIB_DEP) $(SERVICES_DYNALIB_LIB_DEP) $(HAL_DYNALIB_LIB_DEP) $(SYSTEM_DYNALIB_LIB_DEP) $(RT_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(PLATFORM_LIB_DEP) $(WIRING_GLOBALS_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))


TARGET= elf elf_fi bin lst hex size


include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)

MODULE_USER_MEMORY_FILE_GEN = $(TARGET_BASE_DIR)module_user_memory.ld
export MODULE_USER_MEMORY_FILE_GEN

elf_fi: $(TARGET_BASE)_fi.elf


prebuild: $(SHARED_MODULAR)/build_linker_script.mk
	$(call echo,)
	$(VERBOSE)$(MKDIR) $(dir $(MODULE_USER_MEMORY_FILE_GEN))
	$(VERBOSE)$(MAKE) -f $(SHARED_MODULAR)/build_linker_script.mk PREBUILD=1
	$(call echo,)


postbuild:
	$(VERBOSE)$(RM) $(MODULE_USER_MEMORY_FILE_GEN)


$(TARGET_BASE)_fi.elf: $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS) $(TARGET_BASE).elf $(SHARED_MODULAR)/build_linker_script.mk
	$(VERBOSE)$(RM) $(MODULE_USER_MEMORY_FILE_GEN)
	$(VERBOSE)$(MKDIR) $(dir $(MODULE_USER_MEMORY_FILE_GEN))
	$(VERBOSE)$(MAKE) -f $(SHARED_MODULAR)/build_linker_script.mk INTERMEDIATE_ELF=$(TARGET_BASE).elf
	$(call echo,)
	$(call echo, Generating the final ELF file...)
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(VERBOSE)mv -f $@ $(TARGET_BASE).elf
	$(call echo,)

.PHONY: elf_fi


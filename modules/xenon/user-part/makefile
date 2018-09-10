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


TARGET= prebuild elf bin lst hex size


include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)

WRITE_FILE_CREATE =$(file >$(1),$(2))
WRITE_FILE_APPEND =$(file >>$(1),$(2))

COMMA := ,

prebuild:
	@echo Generating fake linker script ...
	$(call WRITE_FILE_CREATE, module_user_memory.ld,/* Memory layout constants */)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_app_flash_origin = 0xD4000;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_app_flash_length = 128K;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,/* The SRAM Origin is system_part1_module_ram_end$(COMMA) and extends to system_static_ram_start */)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_sram_origin = 0x20040000 - 56K;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_sram_length = 56K;)

final_linker_script.ld: makefile build_linker_script.mk
	$(MAKE) -f build_linker_script.mk TARGET_ELF=$(TARGET_BASE).elf
	
$(TARGET_BASE)_fi.elf : $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS) final_linker_script.ld
	$(call echo,'Building target: $@')
	$(call echo,'Invoking: ARM GCC C++ Linker')
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)$(CCACHE) $(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	$(call echo,)

postbuild: $(TARGET_BASE)_fi.elf
	@rm module_user_memory.ld

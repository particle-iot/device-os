#
## Building the combined Image
# edit WICED_SDK to point to the directory containing the photon-wiced repo, with the
# `feature/combined-fw` branch checked out
# edit FIRMWARE to point to the directory containing the firmware-private repo with the
# `feature/hal` branch checked out
# run make -f wiced_test_mfg_combined.mk
# This will build the artefacts to $(FIRMEARE)/build/target/photon-rc2/

ifeq (,$(PLATFORM_ID))
$(error PLATFORM_ID not defined!)
endif


# redefine these for your environment
TOOLCHAIN_PREFIX=arm-none-eabi-
BASE?=../../../..
WICED_SDK?=$(BASE)/WICED/WICED-SDK-3.1.1/WICED-SDK
FIRMWARE?=$(BASE)/firmware
COMMON_BUILD=$(FIRMWARE)/build

include $(COMMON_BUILD)/macros.mk
include $(COMMON_BUILD)/os.mk
include $(COMMON_BUILD)/version.mk

ifeq (6,$(PLATFORM_ID))
CMD=test.mfg_test-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO
BUILD_NAME=test_mfg_test-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO
SUFFIX=_BM-09_$(VERSION_STRING)
else
CMD=test.mfg_test-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO
BUILD_NAME=test_mfg_test-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO
SUFFIX=_BM-14_$(VERSION_STRING)
endif


SERVER_PUB_KEY=cloud_public.der
FIRMWARE_BUILD=$(FIRMWARE)/build
TARGET_PARENT=$(FIRMWARE_BUILD)/target
OUT=$(FIRMWARE_BUILD)/releases/release-$(VERSION_STRING)-p$(PLATFORM_ID)
DCT_MEM=$(OUT)/dct_pad.bin
DCT_PREP=dct_prep.bin
ERASE_SECTOR=$(OUT)/erase_sector.bin
BOOTLOADER_BIN=$(FIRMWARE_BUILD)/target/bootloader/platform-$(PLATFORM_ID)-lto/bootloader.bin
BOOTLOADER_MEM=$(OUT)/bootloader_pad$(SUFFIX).bin
BOOTLOADER_DIR=$(FIRMWARE)/bootloader

FIRMWARE_BIN=$(FIRMWARE_BUILD)/target/main/platform-$(PLATFORM_ID)/main.bin
FIRMWARE_ELF=$(FIRMWARE_BUILD)/target/main/platform-$(PLATFORM_ID)/main.elf
FIRMWARE_MEM=$(OUT)/main_pad$(SUFFIX).bin
FIRMWARE_DIR=$(FIRMWARE)/main
COMBINED_MEM=$(OUT)/combined$(SUFFIX).bin
COMBINED_ELF=$(OUT)/combined$(SUFFIX).elf

#LTO=-lto
MODULAR_DIR=$(FIRMWARE)/modules
SYSTEM_PART1_BIN=$(FIRMWARE_BUILD)/target/system-part1/platform-$(PLATFORM_ID)-m$(LTO)/system-part1.bin
SYSTEM_PART2_BIN=$(FIRMWARE_BUILD)/target/system-part2/platform-$(PLATFORM_ID)-m$(LTO)/system-part2.bin
SYSTEM_MEM=$(OUT)/system_pad$(SUFFIX).bin

USER_BIN=$(FIRMWARE_BUILD)/target/user-part/platform-$(PLATFORM_ID)-m$(LTO)/user-part.bin
USER_MEM=$(OUT)/user-part.bin
USER_DIR=$(FIRMWARE)/modules/photon/user-part

MFG_TEST_BIN=$(WICED_SDK)/build/$(BUILD_NAME)/binary/$(BUILD_NAME).bin
MFG_TEST_MEM=$(OUT)/mfg_test_pad$(SUFFIX).bin
MFG_TEST_DIR=Apps/test/mfg_test

PRODUCT_ID?=$(PLATFORM_ID)

WL_DEP=
ifneq ("$(MAKE_OS)","OSX")
  WL_DEP=wl
endif

CRC=crc32
XXD=xxd
OPTS=

all: combined-full

setup:
	-mkdir $(TARGET_PARENT)
	-mkdir $(TARGET)
	-mkdir $(OUT)
clean:
	-rm -rf $(TARGET_PARENT)
	-rm $(MFG_TEST_BIN)
	-rm $(BOOTLOADER_MEM)
	-rm $(DCT_MEM)
	cd "$(WICED_SDK)"; "./make" clean

bootloader:
	@echo building bootloader to $(BOOTLOADER_MEM)
	-rm $(BOOTLOADER_MEM)
	$(MAKE) -C $(BOOTLOADER_DIR) PLATFORM_ID=$(PLATFORM_ID) all
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377"  > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc

# add the prepared dct image into the flash image
dct:
	@echo building DCT to $(DCT_MEM)
	-rm $(DCT_MEM)
	dd if=/dev/zero ibs=1k count=112 | tr "\000" "\377" > $(DCT_MEM)
#	tr "\000" "\377" < /dev/zero | dd of=$(DCT_MEM) ibs=1k count=112
	dd if=$(DCT_PREP) of=$(DCT_MEM) conv=notrunc
	# inject the version string in to the DCT image
	dd if=/dev/zero bs=1 count=32 of=$(DCT_MEM) seek=9406 conv=notrunc
	echo -n $(VERSION_STRING) | dd bs=1 of=$(DCT_MEM) seek=9406 conv=notrunc


$(MFG_TEST_BIN):
	cd "$(WICED_SDK)"; "./make" $(CMD) $(OPTS)
	@echo Appending: CRC32 to the Flash Image
	cp $@ $@.no_crc
	$(CRC) $@.no_crc | cut -c 1-10 | $(XXD) -r -p >> $@

$(MFG_TEST_MEM): $(MFG_TEST_BIN)
	@echo building WICED test tool to $(MFT_TEST_MEM)
	-rm $(MFG_TEST_MEM)
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(MFG_TEST_MEM)
#	tr "\000" "\377" < /dev/zero | dd of=$(MFG_TEST_MEM) ibs=1k count=384
	dd if=$(MFG_TEST_BIN) of=$(MFG_TEST_MEM) conv=notrunc

mfg_test: $(MFG_TEST_MEM)

firmware:
	@echo building main firmware $(FIRMWARE_MEM)
	-rm $(FIRMWARE_MEM)
	$(MAKE) -C $(FIRMWARE_DIR) PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) PRODUCT_ID=$(PRODUCT_ID) MODULAR=n all
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(FIRMWARE_MEM)
#	tr "\000" "\377" < /dev/zero | dd of=$(FIRMWARE_MEM) ibs=1k count=384
	dd if=$(FIRMWARE_BIN) of=$(FIRMWARE_MEM) conv=notrunc
	cp $(FIRMWARE_ELF) $(OUT)

user: 
	@echo building factory default modular user app to $(USER_MEM)
	-rm $(USER_MEM)
	$(MAKE) -C $(USER_DIR) PLATFORM_ID=$(PLATFORM_ID)  PRODUCT_ID=$(PRODUCT_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) all
	cp $(USER_BIN) $(USER_MEM)

system-minimal:
	# The system module is composed of part1 and part2 concatenated together
	# adjust the module_info end address and the final CRC
	@echo building minimal modular system firmware to $(SYSTEM_MEM)
	-rm $(SYSTEM_MEM)
	$(MAKE) -C $(MODULAR_DIR) COMPILE_LTO=n MINIMAL=y PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) PRODUCT_ID=$(PRODUCT_ID) all
	dd if=/dev/zero ibs=1 count=393212 | tr "\000" "\377" > $(SYSTEM_MEM)
#	tr "\000" "\377" < /dev/zero | dd of=$(SYSTEM_MEM) ibs=1 count=393212
	dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(SYSTEM_MEM) conv=notrunc
	dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(SYSTEM_MEM) seek=256 conv=notrunc
	# 5FFFC is the maximum length (384k-4 bytes). Place in end address in module_info struct
	echo fcff0708 | $(XXD) -r -p | dd bs=1 of=$(SYSTEM_MEM) seek=392 conv=notrunc
	# change the module function from system-part modular (04) to monolithic (03 since that's what the factory reset is expecting.
	echo 03 | $(XXD) -r -p | dd bs=1 of=$(SYSTEM_MEM) seek=402 conv=notrunc
	$(CRC) $(SYSTEM_MEM) | cut -c 1-10 | $(XXD) -r -p >> $(SYSTEM_MEM)

system-full:
	# The system module is composed of part1 and part2 concatenated together
	@echo building full modular system firmware to $(SYSTEM_MEM)
	-rm $(SYSTEM_MEM)
	$(MAKE) -C $(MODULAR_DIR) COMPILE_LTO=n PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) PRODUCT_ID=$(PRODUCT_ID) all
	# 512k - 4 bytes for the CRC
	dd if=/dev/zero ibs=1 count=524284 | tr "\000" "\377" > $(SYSTEM_MEM)
	dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(SYSTEM_MEM) conv=notrunc
	dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(SYSTEM_MEM) seek=256 conv=notrunc
	# 7FFFC is the maximum length (512k-4 bytes). Place in end address in module_info struct, in little endian order
	echo fcff0908 | $(XXD) -r -p | dd bs=1 of=$(SYSTEM_MEM) seek=392 conv=notrunc
	# append the CRC for the combined module
	$(CRC) $(SYSTEM_MEM) | cut -c 1-10 | $(XXD) -r -p >> $(SYSTEM_MEM)


	
wl:
	cd "$(WICED_SDK)/$(MFG_TEST_DIR)"; make
	cp $(WICED_SDK)/$(MFG_TEST_DIR)/wl43362A2.exe $(OUT)/wl.exe

combined-minimal: setup bootloader dct mfg_test user system-minimal $(WL_DEP) checks-minimal
	@echo Building combined image to $(COMBINED_MEM)
	-rm $(COMBINED_MEM)	
	cat $(BOOTLOADER_MEM) $(DCT_MEM) $(MFG_TEST_MEM) $(SYSTEM_MEM) $(USER_MEM) > $(COMBINED_MEM)

	# Generate combined.elf from combined.bin
	${TOOLCHAIN_PREFIX}ld -b binary -r -o $(OUT)/temp.elf $(COMBINED_MEM)
	${TOOLCHAIN_PREFIX}objcopy --rename-section .data=.text --set-section-flags .data=alloc,code,load $(OUT)/temp.elf
	${TOOLCHAIN_PREFIX}ld $(OUT)/temp.elf -T ../stm32/combined_bin_to_elf.ld -o $(COMBINED_ELF)
	${TOOLCHAIN_PREFIX}strip -s $(COMBINED_ELF)
	-rm -rf $(OUT)/temp.elf

user-full: user
	# the application image (tinker) is injected into the 2nd EEPROM page
	dd if=$(USER_MEM) bs=1k of=$(DCT_MEM) seek=32 conv=notrunc


combined-full: setup bootloader dct mfg_test user-full system-full $(WL_DEP) user-full checks-full
	@echo Building combined full image to $(COMBINED_MEM)
	-rm $(COMBINED_MEM)
	cat $(BOOTLOADER_MEM) $(DCT_MEM) $(MFG_TEST_MEM) $(SYSTEM_MEM) > $(COMBINED_MEM)

	# Generate combined.elf from combined.bin
	${TOOLCHAIN_PREFIX}ld -b binary -r -o $(OUT)/temp.elf $(COMBINED_MEM)
	${TOOLCHAIN_PREFIX}objcopy --rename-section .data=.text --set-section-flags .data=alloc,code,load $(OUT)/temp.elf
	${TOOLCHAIN_PREFIX}ld $(OUT)/temp.elf -T ../stm32/combined_bin_to_elf.ld -o $(COMBINED_ELF)
	${TOOLCHAIN_PREFIX}strip -s $(COMBINED_ELF)
	-rm -rf $(OUT)/temp.elf


st-flash: combined-full
	st-flash write $(COMBINED_MEM) 0x8000000

openocd-flash: 
	openocd -f $(OPENOCD_HOME)/tcl/interface/ftdi/particle-ftdi.cfg -f $(OPENOCD_HOME)/tcl/target/stm32f2x.cfg  -c "init; reset halt" -c "flash protect 0 0 11 off" -c "program $(COMBINED_MEM) 0x08000000 reset exit"

checks-common:
	$(call assert_filesize,$(BOOTLOADER_MEM),16384)
	$(call assert_filebyte,$(FIRMWARE_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(BOOTLOADER_MEM),400,0$(PLATFORM_ID))
	$(call assert_filesize,$(DCT_MEM),114688)
	$(call assert_filesize,$(MFG_TEST_MEM),393216)
	$(call assert_filebyte,$(MFG_TEST_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(SYSTEM_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(SYSTEM_MEM),400,0$(PLATFORM_ID))

checks-minimal: checks-common
	$(call assert_filesize,$(SYSTEM_MEM),393216)

checks-full: checks-common
	$(call assert_filesize,$(SYSTEM_MEM),524288)
	# check that system part2 has platform ID set (256k + 400)
	$(call assert_filebyte,$(SYSTEM_MEM),262544,0$(PLATFORM_ID))


.PHONY: wl mfg_test clean all bootloader dct mfg_test firmware $(MFG_TEST_BIN) $(MFG_TEST_MEM) prep_dct write_version checks system-minimal system-full

DFU_USB_ID=2b04:d006
DFU_DCT = dfu-util -d $(DFU_USB_ID) -a 1 --dfuse-address
DFU_FLASH = dfu-util -d $(DFU_USB_ID) -a 0 --dfuse-address
# Run this after doing a factory reset on the combined image and putting the
# device in DFU mode.
# This will create a blank DCT (with pre-generated keys)
# The this script erases the generated keys, with 0xFF
# And writes the server public key to the appropriate place
prep_dct:
	dd if=/dev/zero ibs=4258 count=1 | tr "\000" "\377" > $(ERASE_SECTOR)
#	tr "\000" "\377" < /dev/zero | dd of=$(ERASE_SECTOR) ibs=4258 count=1
	$(DFU_DCT) 1:4258 -D $(ERASE_SECTOR)
	$(DFU_DCT) 2082 -D $(SERVER_PUB_KEY)
	#st-flash read $(DCT_PREP) 0x8004000 0x8000
	#$(DFU_FLASH) 0x4000:0x8000 -U $(DCT_PREP)

# Feb 24 2015 - steps to build dct_prep.bin file
# flash the combined image
# enter dfu mode
# use st-flash GUI tool to erase DCT sectors 0x8004000 and 0x8008000
# use the prep_dct goal to write the cloud public key
# use st-flash GUI tool to save the memory contents of sectors 0x8004000-0x800C0000 (32K)


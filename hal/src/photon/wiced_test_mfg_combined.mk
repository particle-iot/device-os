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

_assert_lessthan = $(if $(call test,$2,-lt,$3),$(error "expected $1 to be < $2 but was $3))
_assert_greaterthan = $(if $(call test,$2,-gt,$3),$(error "expected $1 to be > $2 but was $3))

assert_filelessthan = $(call _assert_lessthan,"file $1",$2,$(shell echo $(call filesize,$1)))
assert_filegreaterthan = $(call _assert_greaterthan,"file $1",$2,$(shell echo $(call filesize,$1)))

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
BOOTLOADER_BIN=$(FIRMWARE_BUILD)/target/bootloader/platform-$(PLATFORM_ID)-m-lto/bootloader.bin
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

USER_BIN=$(FIRMWARE_BUILD)/target/user-part/platform-$(PLATFORM_ID)-m$(LTO)/tinker.bin
USER_MEM=$(OUT)/user-part.bin
USER_DIR=$(FIRMWARE)/modules/photon/user-part

MFG_TEST_BIN=$(WICED_SDK)/build/$(BUILD_NAME)/binary/$(BUILD_NAME).bin
MFG_TEST_MEM=$(OUT)/mfg_test_pad$(SUFFIX).bin
MFG_TEST_DIR=apps/test/mfg_test

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
	mkdir -p $(TARGET_PARENT)
	mkdir -p $(OUT)
clean:
	rm -rf $(TARGET_PARENT)
	rm -rf $(OUT)
	cd "$(WICED_SDK)"; "./make" clean

bootloader:
	@echo building bootloader to $(BOOTLOADER_MEM)
	rm -f $(BOOTLOADER_MEM)
	$(MAKE) -s -C $(BOOTLOADER_DIR) PLATFORM_ID=$(PLATFORM_ID) MONO_MFG_FIRMWARE_AT_USER_PART=y all
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377"  > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc

# add the prepared dct image into the flash image
dct:
	@echo building DCT to $(DCT_MEM)
	rm -f $(DCT_MEM)
	dd if=/dev/zero ibs=1k count=112 | tr "\000" "\377" > $(DCT_MEM)
#	tr "\000" "\377" < /dev/zero | dd of=$(DCT_MEM) ibs=1k count=112
	dd if=$(DCT_PREP) of=$(DCT_MEM) conv=notrunc
	# inject the version string in to the DCT image
	dd if=/dev/zero bs=1 count=32 of=$(DCT_MEM) seek=9406 conv=notrunc
	echo -n $(VERSION_STRING) | dd bs=1 of=$(DCT_MEM) seek=9406 conv=notrunc


$(MFG_TEST_BIN): user-full
	cd "$(WICED_SDK)"; "./make" $(CMD) $(OPTS)

$(MFG_TEST_MEM): $(MFG_TEST_BIN) user-full
	@echo building WICED test tool to $(MFT_TEST_MEM)
	rm -f $(MFG_TEST_MEM)
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(MFG_TEST_MEM)

	# the application image (tinker) is injected into the gap in the
	# manufacturing test firmware that starts @ 0x080e0000 (factory image location)

	@echo Inserting application image into the manufacturing binary
	$(TOOLCHAIN_PREFIX)readelf -s $(basename $(MFG_TEST_BIN)).elf | grep link_fac_location | cut -d ' ' -f 4 | sed -e 's/^/0x/' > $(MFG_TEST_BIN).fac_location
	$(TOOLCHAIN_PREFIX)readelf -s $(basename $(MFG_TEST_BIN)).elf | grep link_fac_end | cut -d ' ' -f 4| sed -e 's/^/0x/' > $(MFG_TEST_BIN).fac_end
	$(TOOLCHAIN_PREFIX)readelf -s $(basename $(MFG_TEST_BIN)).elf | grep link_module_start | cut -d ' ' -f 4 | sed -e 's/^/0x/' > $(MFG_TEST_BIN).module_start

	cp $(MFG_TEST_BIN) $(MFG_TEST_BIN).crc
	dd if=$(USER_MEM) of=$(MFG_TEST_BIN).crc bs=1 seek=$$(( $$(cat $(MFG_TEST_BIN).fac_location) - $$(cat $(MFG_TEST_BIN).module_start) )) conv=notrunc

	@echo Appending: CRC32 to the Flash Image
	$(CRC) $(MFG_TEST_BIN).crc | cut -c 1-10 | $(XXD) -r -p >> $(MFG_TEST_BIN).crc

	dd if=$(MFG_TEST_BIN).crc of=$(MFG_TEST_MEM) conv=notrunc

mfg_test: $(MFG_TEST_MEM)

user: 
	@echo building factory default modular user app to $(USER_MEM)
	rm -f $(USER_MEM)
	$(MAKE) -s -C $(USER_DIR) APP=tinker PLATFORM_ID=$(PLATFORM_ID)  PRODUCT_ID=$(PRODUCT_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) all
	cp $(USER_BIN) $(USER_MEM)

system-full:
	# The system module is composed of part1 and part2 concatenated together
	@echo building full modular system firmware to $(SYSTEM_MEM)
	rm -f $(SYSTEM_MEM)
	$(MAKE) -s -C $(MODULAR_DIR) COMPILE_LTO=n PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) PRODUCT_ID=$(PRODUCT_ID) all
	# 512k - 4 bytes for the CRC
	dd if=/dev/zero ibs=1 count=524288 | tr "\000" "\377" > $(SYSTEM_MEM)
	dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(SYSTEM_MEM) conv=notrunc
	dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(SYSTEM_MEM) seek=256 conv=notrunc

wl:
	cd "$(WICED_SDK)/$(MFG_TEST_DIR)"; make
	cp $(WICED_SDK)/$(MFG_TEST_DIR)/wl43362A2.exe $(OUT)/wl.exe

user-full: user

combined-full: setup bootloader dct user-full mfg_test system-full $(WL_DEP) checks-full
	@echo Building combined full image to $(COMBINED_MEM)
	rm -f $(COMBINED_MEM)
	cat $(BOOTLOADER_MEM) $(DCT_MEM) $(SYSTEM_MEM) $(MFG_TEST_MEM) > $(COMBINED_MEM)

	# Generate combined.elf from combined.bin
	${TOOLCHAIN_PREFIX}ld -b binary -r -o $(OUT)/temp.elf $(COMBINED_MEM)
	${TOOLCHAIN_PREFIX}objcopy --rename-section .data=.text --set-section-flags .data=alloc,code,load $(OUT)/temp.elf
	${TOOLCHAIN_PREFIX}ld $(OUT)/temp.elf -T ../stm32/combined_bin_to_elf.ld -o $(COMBINED_ELF)
	${TOOLCHAIN_PREFIX}strip -s $(COMBINED_ELF)
	rm -f $(OUT)/temp.elf
	@echo
	@echo
	@echo "Success! Release binaries written to $(OUT)"

st-flash: combined-full
	st-flash write $(COMBINED_MEM) 0x8000000

openocd-flash:
	openocd -f interface/ftdi/particle-ftdi.cfg -f target/stm32f2x.cfg  -c "init; reset halt" -c "stm32f2x unlock 0; reset halt" -c "flash protect 0 0 11 off" -c "program $(COMBINED_MEM) 0x08000000 reset exit"

openocd-flash-stlink:
	openocd -f interface/stlink-v2.cfg -f target/stm32f2x.cfg  -c "init; reset halt" -c "stm32f2x unlock 0; reset halt" -c "flash protect 0 0 11 off" -c "program $(COMBINED_MEM) 0x08000000 reset exit"

checks-common:
	$(call assert_filesize,$(BOOTLOADER_MEM),16384)
	$(call assert_filebyte,$(BOOTLOADER_MEM),400,0$(PLATFORM_ID))
	$(call assert_filesize,$(DCT_MEM),114688)
	$(call assert_filesize,$(MFG_TEST_MEM),393216)
	$(call assert_filebyte,$(MFG_TEST_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(SYSTEM_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(SYSTEM_MEM),400,0$(PLATFORM_ID))

checks-full: checks-common
	$(call assert_filesize,$(SYSTEM_MEM),524288)
	# check that system part2 has platform ID set (256k + 400)
	$(call assert_filebyte,$(SYSTEM_MEM),262544,0$(PLATFORM_ID))
	$(call assert_filelessthan,$(USER_MEM),$(shell echo $$(( $$(cat $(MFG_TEST_BIN).fac_end) - $$(cat $(MFG_TEST_BIN).fac_location) ))))


.PHONY: wl mfg_test clean all bootloader dct mfg_test $(MFG_TEST_BIN) $(MFG_TEST_MEM) prep_dct write_version checks system-full

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


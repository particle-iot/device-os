

ifeq (,$(PLATFORM_ID))
$(error PLATFORM_ID not defined!)
endif

BOOTLOADER_LTO?=n

ifeq ("$(BOOTLOADER_LTO)","y")
BOOTLOADER_LTO_PATH_SUFFIX=-lto
endif
BOOTLOADER_LTO_PATH_SUFFIX?=

export LC_CTYPE=C

# redefine these for your environment
TOOLCHAIN_PREFIX=arm-none-eabi-
ROOT?=../../../..
FIRMWARE=$(ROOT)/firmware
COMMON_BUILD=$(FIRMWARE)/build

include $(COMMON_BUILD)/macros.mk
include $(COMMON_BUILD)/os.mk
include $(COMMON_BUILD)/version.mk

SERVER_PUB_KEY_RSA=cloud_public.der
SERVER_PUB_KEY_ECC=server-public-key.udp.particle.io.der

FIRMWARE_BUILD=$(FIRMWARE)/build
TARGET_PARENT=$(FIRMWARE_BUILD)/target
OUT=$(FIRMWARE_BUILD)/releases/release-$(VERSION_STRING)-p$(PLATFORM_ID)
DCT_MEM=$(OUT)/dct_pad.bin
ERASE_SECTOR=$(OUT)/erase_sector.bin
BOOTLOADER_BIN=$(FIRMWARE_BUILD)/target/bootloader/platform-$(PLATFORM_ID)$(BOOTLOADER_LTO_PATH_SUFFIX)/bootloader.bin
BOOTLOADER_MEM=$(OUT)/bootloader_pad$(SUFFIX).bin
BOOTLOADER_DIR=$(FIRMWARE)/bootloader

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
USER_DIR=$(FIRMWARE)/modules/$(PLATFORM_NAME)/user-part

PRODUCT_ID?=$(PLATFORM_ID)

CRC=crc32
XXD=xxd
OPTS=

setup:
	-mkdir $(TARGET_PARENT)
	-mkdir $(OUT)

clean:
	-rm -rf $(TARGET_PARENT)
	-rm $(BOOTLOADER_MEM)
	-rm $(DCT_MEM)

bootloader:
	@echo building bootloader to $(BOOTLOADER_MEM)
	-rm $(BOOTLOADER_MEM)
	$(MAKE) -C $(BOOTLOADER_DIR) PLATFORM_ID=$(PLATFORM_ID) COMPILE_LTO=n all
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377"  > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc

# add the prepared dct image into the flash image
dct:
	@echo building DCT to $(DCT_MEM)
	-rm $(DCT_MEM)
	dd if=/dev/zero ibs=1k count=112 | tr "\000" "\377" > $(DCT_MEM)
#	tr "\000" "\377" < /dev/zero | dd of=$(DCT_MEM) ibs=1k count=112
	dd if=dct_header.bin of=$(DCT_MEM) conv=notrunc bs=1 
	# offsets are +8 to account for the 8 byte header
	dd if=cloud_public.der of=$(DCT_MEM) conv=notrunc bs=1 seek=2090
	dd if=server-public-key.udp.particle.io.der of=$(DCT_MEM) conv=notrunc bs=1 seek=3306
		
user: 
	@echo building factory default modular user app to $(USER_MEM)
	-rm $(USER_MEM)
	$(MAKE) -C $(USER_DIR) PLATFORM_ID=$(PLATFORM_ID)  PRODUCT_ID=$(PRODUCT_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) DEBUG_BUILD=y all
	dd if=/dev/zero ibs=1 count=128k | tr "\000" "\377" > $(USER_MEM)
	dd if=$(USER_BIN) of=$(USER_MEM) conv=notrunc
	
system-full:
	# The system module is composed of part1 and part2 concatenated together
	@echo building full modular system firmware to $(SYSTEM_MEM)
	-rm $(SYSTEM_MEM)
	$(MAKE) -C $(MODULAR_DIR) COMPILE_LTO=n PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) DEBUG_BUILD=y PRODUCT_ID=$(PRODUCT_ID) all
	dd if=/dev/zero ibs=1 count=384k | tr "\000" "\377" > $(SYSTEM_MEM)
	dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(SYSTEM_MEM) conv=notrunc
	dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(SYSTEM_MEM) seek=128 conv=notrunc
	
user-full: user
	
combined-full: setup bootloader dct user-full system-full user-full checks-full
	@echo Building combined full image to $(COMBINED_MEM)
	-rm $(COMBINED_MEM)
	cat $(BOOTLOADER_MEM) $(DCT_MEM) $(SYSTEM_MEM) $(USER_MEM) $(USER_MEM) $(USER_MEM) $(USER_MEM) > $(COMBINED_MEM)

	# Generate combined.elf from combined.bin
	${TOOLCHAIN_PREFIX}ld -b binary -r -o $(OUT)/temp.elf $(COMBINED_MEM)
	${TOOLCHAIN_PREFIX}objcopy --rename-section .data=.text --set-section-flags .data=alloc,code,load $(OUT)/temp.elf
	${TOOLCHAIN_PREFIX}ld $(OUT)/temp.elf -T ../stm32/combined_bin_to_elf.ld -o $(COMBINED_ELF)
	${TOOLCHAIN_PREFIX}strip -s $(COMBINED_ELF)
	-rm -rf $(OUT)/temp.elf


st-flash: combined-full
	st-flash write $(COMBINED_MEM) 0x8000000

openocd-flash: 
	openocd -f $(OPENOCD_HOME)/interface/ftdi/particle-ftdi.cfg -f $(OPENOCD_HOME)/target/stm32f2x.cfg  -c "init; reset halt" -c "flash protect 0 0 11 off" -c "program $(COMBINED_MEM) 0x08000000 reset exit"

checks-common:
	$(call assert_filesize,$(BOOTLOADER_MEM),16384)
	$(call assert_filebyte,$(BOOTLOADER_MEM),400,0$(PLATFORM_ID))
	$(call assert_filesize,$(DCT_MEM),114688)
	$(call assert_filebyte,$(SYSTEM_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(SYSTEM_MEM),400,0$(PLATFORM_ID))
	$(call assert_filebyte,$(USER_MEM),12,0$(PLATFORM_ID))
	$(call assert_filesize,$(USER_MEM),131072)

checks-full: checks-common
	$(call assert_filesize,$(SYSTEM_MEM),393216)
	$(call assert_filebyte,$(SYSTEM_MEM),131472,0$(PLATFORM_ID))


.PHONY: wl clean all bootloader dct firmware $(MFG_TEST_BIN) $(MFG_TEST_MEM) prep_dct write_version checks system-full

#
## Building the combined Image

PLATFORM_ID?=10

ifeq (,$(PLATFORM_ID))
$(error PLATFORM_ID not defined!)
endif

CERTS_DIR?=.

# redefine these for your environment
TOOLCHAIN_PREFIX=arm-none-eabi-
BASE?=../../../..
FIRMWARE?=$(BASE)/firmware
COMMON_BUILD=$(FIRMWARE)/build

include $(COMMON_BUILD)/macros.mk
include $(COMMON_BUILD)/os.mk
include $(COMMON_BUILD)/version.mk

SERVER_PUB_KEY_RSA=$(CERTS_DIR)/cloud.pub.der
SERVER_PUB_KEY_ECC=$(CERTS_DIR)/server-public-key.udp.particle.io.der
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
USER_DIR=$(FIRMWARE)/modules/electron/user-part

LISTEN_BIN=$(FIRMWARE_BUILD)/target/user-part/platform-$(PLATFORM_ID)-m$(LTO)/listen_mode.bin
LISTEN_MEM=$(OUT)/listen_mode.bin
LISTEN_DIR=$(FIRMWARE)/modules/electron/user-part


PRODUCT_ID?=$(PLATFORM_ID)


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
	-rm $(BOOTLOADER_MEM)
	-rm $(DCT_MEM)

bootloader:
	@echo building bootloader to $(BOOTLOADER_MEM)
	-rm $(BOOTLOADER_MEM)
	$(MAKE) -C $(BOOTLOADER_DIR) PLATFORM_ID=$(PLATFORM_ID) $(CLEAN) all
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

user: 
	@echo building factory default modular user app to $(USER_MEM)
	-rm $(USER_MEM)
	$(MAKE) -C $(USER_DIR) PLATFORM_ID=$(PLATFORM_ID)  PRODUCT_ID=$(PRODUCT_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) $(CLEAN) all
	cp $(USER_BIN) $(USER_MEM)

listen: 
	@echo building listen mode app to $(LISTEN_MEM)
	-rm $(LISTEN_MEM)
	$(MAKE) -C $(LISTEN_DIR) PLATFORM_ID=$(PLATFORM_ID)  PRODUCT_ID=$(PRODUCT_ID) PRODUCT_FIRMWARE_VERSION=$(VERSION) $(CLEAN) all TEST=app/listen_mode
	cp $(LISTEN_BIN) $(LISTEN_MEM)

system-full:
	@echo building full modular system firmware to $(SYSTEM_MEM)
	-rm $(SYSTEM_MEM)
	$(MAKE) -C $(MODULAR_DIR) COMPILE_LTO=n PLATFORM_ID=$(PLATFORM_ID) DEBUG=y PRODUCT_FIRMWARE_VERSION=$(VERSION) PRODUCT_ID=$(PRODUCT_ID) $(CLEAN) all
	# 512k - 4 bytes for the CRC
	dd if=/dev/zero ibs=1k count=256 | tr "\000" "\377" > $(SYSTEM_MEM)
	dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(SYSTEM_MEM) conv=notrunc
	dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(SYSTEM_MEM) seek=128 conv=notrunc
	
user-full: user
listen-full: listen


combined-full: setup bootloader dct user-full system-full user-full listen-full checks-full combined-build
	
combined-build:
	@echo Building combined full image to $(COMBINED_MEM)
	-rm $(COMBINED_MEM)
	
	dd if=/dev/zero ibs=1k count=1024 | tr "\000" "\377" > $(COMBINED_MEM)
	dd if=$(BOOTLOADER_MEM) bs=1k  of=$(COMBINED_MEM) conv=notrunc
	dd if=$(DCT_MEM) bs=1k seek=16 of=$(COMBINED_MEM) conv=notrunc
	dd if=$(SYSTEM_MEM) bs=1k seek=128 of=$(COMBINED_MEM) conv=notrunc
	dd if=$(LISTEN_MEM) bs=1k seek=512 of=$(COMBINED_MEM) conv=notrunc
	# factory default firmware
	dd if=$(USER_MEM) bs=1k seek=640 of=$(COMBINED_MEM) conv=notrunc

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

checks-full: checks-common
	$(call assert_filesize,$(SYSTEM_MEM), 262144)
	$(call assert_filesize,$(COMBINED_MEM), 1048576)
	# check that system part2 has platform ID set (256k + 400)
	$(call assert_filebyte,$(SYSTEM_MEM),262544,0$(PLATFORM_ID))


.PHONY:  clean all bootloader dct listen-mode prep_dct write_version checks system-full

DFU_USB_ID=2b04:d00a
DFU_DCT = dfu-util -d $(DFU_USB_ID) -a 1 --dfuse-address
DFU_FLASH = dfu-util -d $(DFU_USB_ID) -a 0 --dfuse-address
# Run this after doing a factory reset on the combined image and putting the
# device in DFU mode.
# This will create a blank DCT (with pre-generated keys)
# The this script erases the generated keys, with 0xFF
# And writes the server public key to the appropriate place
prep_dct:
	dd if=/dev/zero ibs=15k count=1 | tr "\000" "\377" > $(ERASE_SECTOR)
#	tr "\000" "\377" < /dev/zero | dd of=$(ERASE_SECTOR) ibs=4258 count=1
	$(DFU_DCT) 1 -D $(ERASE_SECTOR)
	$(DFU_DCT) 2082 -D $(SERVER_PUB_KEY_RSA)
	$(DFU_DCT) 3298 -D $(SERVER_PUB_KEY_ECC)
	dfu-util -d $(DFU_USB_ID) -a 0 -s 0x8004000:0x8000 -U $(DCT_PREP)
	#st-flash read $(DCT_PREP) 0x8004000 0x8000
	#$(DFU_FLASH) 0x4000:0x8000 -U $(DCT_PREP)



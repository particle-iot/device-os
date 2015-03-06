# redefine these for your environment
PLATFORM_ID?=6
CORE?=../../../..
WICED_SDK?=$(CORE)/WICED/WICED-SDK-3.1.1/WICED-SDK
SERVER_PUB_KEY=cloud_public.der
FIRMWARE=$(CORE)/firmware
FIRMWARE_BUILD=$(FIRMWARE)/build
TARGET=$(FIRMWARE_BUILD)/target/photon-rc2
OUT=$(WICED_SDK)/build
DCT_MEM=$(OUT)/dct_pad.bin
DCT_PREP=dct_prep.bin
ERASE_SECTOR=$(OUT)/erase_sector.bin
BOOTLOADER_BIN=$(FIRMWARE_BUILD)/target/bootloader/platform-$(PLATFORM_ID)/bootloader.bin
BOOTLOADER_MEM=$(OUT)/bootloader_pad.bin
BOOTLOADER_DIR=$(FIRMWARE)/bootloader

FIRMWARE_BIN=$(FIRMWARE_BUILD)/target/main/platform-$(PLATFORM_ID)/main.bin
FIRMWARE_MEM=$(OUT)/main_pad.bin
FIRMWARE_DIR=$(FIRMWARE)/main
COMBINED_MEM=$(OUT)/combined.bin

MODULAR_DIR=$(FIRMWARE)/modules
SYSTEM_PART1_BIN=$(FIRMWARE_BUILD)/target/system-part1/platform-$(PLATFORM_ID)/system-part1.bin
SYSTEM_PART2_BIN=$(FIRMWARE_BUILD)/target/system-part2/platform-$(PLATFORM_ID)/system-part2.bin
SYSTEM_MEM=$(OUT)/system_pad.bin

USER_BIN=$(FIRMWARE_BUILD)/target/user-part/platform-$(PLATFORM_ID)/user-part.bin
USER_MEM=$(OUT)/user-part.bin
USER_DIR=$(FIRMWARE)/modules/photon/user-part

CMD=test.mfg_test-BCM9WCDUSI09-ThreadX-NetX-SDIO
BUILD_NAME=test_mfg_test-BCM9WCDUSI09-ThreadX-NetX-SDIO
MFG_TEST_BIN=$(WICED_SDK)/build/$(BUILD_NAME)/binary/$(BUILD_NAME).bin
MFG_TEST_MEM=$(OUT)/mfg_test_pad.bin

CRC=crc32
XXD=xxd
OPTS=

all: combined
	mkdir -p $(TARGET)
	cp $(COMBINED_MEM) $(TARGET)
	cp $(SYSTEM_MEM) $(TARGET)
		
clean:
	cd "$(WICED_SDK)"; "$(WICED_SDK)/make" clean
	-rm $(MFG_TEST_BIN)
	-rm $(BOOTLOADER)
	-rm $(DCT)
		
bootloader:
	@echo building $(BOOTLOADER_MEM)
	$(MAKE) -C $(BOOTLOADER_DIR) PLATFORM_ID=$(PLATFORM_ID) all 
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377" > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc

# add the prepared dct image into the flash image
dct: 	
	-rm $(DCT_MEM)
	dd if=/dev/zero ibs=1k count=112 | tr "\000" "\377" > $(DCT_MEM)
	dd if=$(DCT_PREP) of=$(DCT_MEM) conv=notrunc
			
$(MFG_TEST_BIN):
	cd "$(WICED_SDK)"; "./make" $(CMD) $(OPTS)
	@echo Appending: CRC32 to the Flash Image
	cp $@ $@.no_crc	
	$(CRC) $@.no_crc | cut -c 1-10 | $(XXD) -r -p >> $@	

$(MFG_TEST_MEM): $(MFG_TEST_BIN)
	-rm $(MFG_TEST_MEM)
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(MFG_TEST_MEM)
	dd if=$(MFG_TEST_BIN) of=$(MFG_TEST_MEM) conv=notrunc

mfg_test: $(MFG_TEST_MEM)
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377" > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc
			
firmware:
	@echo building $(FIRMWARE_MEM)
	-rm $(FIRMWARE_MEM)
	$(MAKE) -C $(FIRMWARE_DIR) PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_REVISION=$(VERSION) all
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(FIRMWARE_MEM)
	dd if=$(FIRMWARE_BIN) of=$(FIRMWARE_MEM) conv=notrunc	

user:
	@echo building $(USER_MEM)
	-rm $(USER_MEM)
	$(MAKE) -C $(USER_DIR) PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_REVISION=$(VERSION) all
	cp $(USER_BIN) $(USER_MEM)

system:
	# The system module is composed of part1 and part2 concatenated together
	# adjust the module_info end address and the final CRC
	@echo building $(SYSTEM_MEM)
	-rm $(SYSTEM_MEM)
	$(MAKE) -C $(MODULAR_DIR) PLATFORM_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_REVISION=$(VERSION) all
	dd if=/dev/zero ibs=1 count=393212 | tr "\000" "\377" > $(SYSTEM_MEM)
	dd if=$(SYSTEM_PART1_BIN) bs=1k of=$(SYSTEM_MEM) conv=notrunc	
	dd if=$(SYSTEM_PART2_BIN) bs=1k of=$(SYSTEM_MEM) seek=256 conv=notrunc
	# 5FFFC is the maximum length (384k-4 bytes). Place in end address in module_info struct
	echo fcff0708 | $(XXD) -r -p | dd bs=1 of=$(SYSTEM_MEM) seek=392 conv=notrunc
	# change the module function from system-part modular (04) to monolithic (03 since that's what the factory reset is expecting.
	echo 03 | $(XXD) -r -p | dd bs=1 of=$(SYSTEM_MEM) seek=402 conv=notrunc
	$(CRC) $(SYSTEM_MEM) | cut -c 1-10 | $(XXD) -r -p >> $(SYSTEM_MEM)

combined: bootloader dct mfg_test firmware user system
	-rm $(COMBINED_MEM)
	cat $(BOOTLOADER_MEM) $(DCT_MEM) $(MFG_TEST_MEM) $(FIRMWARE_MEM) $(USER_MEM) > $(COMBINED_MEM)

flash: combined
	st-flash write $(COMBINED_MEM) 0x8000000

.PHONY: mfg_test clean all bootloader dct mfg_test firmware $(MFG_TEST_BIN) $(MFG_TEST_MEM) prep_dct
	
		
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
	$(DFU_DCT) 1:4258 -D $(ERASE_SECTOR)
	$(DFU_DCT) 2082 -D $(SERVER_PUB_KEY)
	#st-flash read $(DCT_PREP) 0x8004000 0x8000
	#$(DFU_FLASH) 0x4000:0x8000 -U $(DCT_PREP)
	
# Feb 24 2015 - steps to dct_prep.bin file
# flash the combined image 
# enter dfu mode
# use st-flash GUI tool to erase DCT sectors 0x8004000 and 0x8008000
# use the prep_dct goal to write the cloud public key
# use st-flash GUI tool to save the 


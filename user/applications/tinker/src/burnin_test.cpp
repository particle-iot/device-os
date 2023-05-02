/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "hal_platform.h"
#if HAL_PLATFORM_RTL872X && defined(ENABLE_FQC_FUNCTIONALITY) 
#include "application.h"

// Needed for hal_storage_read
#define PARTICLE_USE_UNSTABLE_API 1 

#include "spark_wiring_logging.h"
#include "spark_wiring_random.h"
#include "spark_wiring_led.h"
#include "random.h"
#include "storage_hal.h"

#include "fqc_test.h"
#include "burnin_test.h"
extern "C" {
#include "core_portme.h"
}

extern uintptr_t platform_km0_part1_flash_start;
extern uintptr_t platform_bootloader_module_info_flash_start;
extern uintptr_t platform_system_part1_flash_start;

namespace particle {

namespace detail {

static const uint32_t softCrc32Tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

} // detail

uint32_t softCrc32(const uint8_t *pBuffer, uint32_t bufferSize, uint32_t const *p_crc) {
    uint32_t crc = (p_crc) ? *p_crc : 0;

    crc = crc ^ ~0U;

    while (bufferSize--)
        crc = detail::softCrc32Tab[(crc ^ *pBuffer++) & 0xFF] ^ (crc >> 8);

    return crc ^ ~0U;
}

// Retained state variables
static retained BurninTest::BurninTestState BurninState;
static retained char LastBurnInTest[16];
static retained uint32_t UptimeMillis;
static retained char BurninErrorMessage[1024];

BurninTest::BurninTest() {
	tests_ = {
		&particle::BurninTest::testGpio,
		&particle::BurninTest::testWifiScan,
		&particle::BurninTest::testBleScan,
		&particle::BurninTest::testSram,
		&particle::BurninTest::testSpiFlash,
		&particle::BurninTest::testCpuLoad,
	};

	test_names_ = {
		"GPIO", 
		"WIFI_SCAN", 
		"BLE_SCAN", 
		"SRAM", 
		"SPI_FLASH", 
		"CPU_LOAD"
	};
}

BurninTest::~BurninTest() {
}

BurninTest* BurninTest::instance() {
    static BurninTest tester;
    return &tester;
}

void BurninTest::setup(bool forceEnable) {
	if (!forceEnable) {
	    hal_pin_t trigger_pin = D7; // PA27 aka SWD
		// Read the trigger pin for a 1khz pulse. If present, enter burnin mode.
		pinMode(trigger_pin, INPUT);
	    uint32_t pulse_width_micros = pulseIn(trigger_pin, HIGH);
	    pinMode(trigger_pin, PIN_MODE_SWD);

	    const uint32_t error_margin_micros = 31;
	    // 1KHZ square wave at 50% duty cycle = 500us pulses
	    const uint32_t expected_pulse_width_micros = 500;

	    if((pulse_width_micros > (expected_pulse_width_micros + error_margin_micros)) ||
	       (pulse_width_micros < (expected_pulse_width_micros - error_margin_micros))) {
			BurninState = BurninTestState::DISABLED;
	    	return;
	    }
	}

    logger_ = std::make_unique<Serial1LogHandler>(115200, LOG_LEVEL_INFO);
    
	// Detect if backup SRAM has a failed test in it (IE state is "TEST FAILED")
	Log.info("BURN IN START: ResetReason: %d State: %d ErrorMessage: %s", System.resetReason(), (int)BurninState, BurninErrorMessage);

	if(BurninState == BurninTestState::IN_PROGRESS) {
		Log.warn("Previous test failed: %s", LastBurnInTest);
		BurninState = BurninTestState::FAILED;
	}
	else if(BurninState == BurninTestState::FAILED) {
		Log.info("Resetting from failed test state: %s", LastBurnInTest);
		BurninState = BurninTestState::IN_PROGRESS;
		UptimeMillis = 0;
	}
	else {
		BurninState = BurninTestState::IN_PROGRESS;
		UptimeMillis = 0;
	}

	RGB.control(true);
    os_thread_create(&led_thread_, "led", OS_THREAD_PRIORITY_DEFAULT, ledLoop, this, OS_THREAD_STACK_SIZE_DEFAULT);
}

static uint32_t printRuntimeInfo(void) {
    runtime_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    HAL_Core_Runtime_Info(&info, NULL);
    Log.info("freeheap: %lu total_heap %lu largest_free_block_heap %lu max_used_heap %lu", 
    	info.freeheap, 
    	info.total_heap, 
    	info.largest_free_block_heap,
    	info.max_used_heap);

    return info.freeheap;
}

void BurninTest::loop() {
	switch(BurninState){
		case BurninTestState::DISABLED:
			return;
		case BurninTestState::IN_PROGRESS:
		{
			UptimeMillis = millis();
			auto startMillis = UptimeMillis;

			// pick random test to run, run it
			auto test_number = random(tests_.size());
			auto test = tests_[test_number];
			strlcpy(LastBurnInTest, test_names_[test_number].c_str(), sizeof(LastBurnInTest));
			Log.info("Running test: %s", LastBurnInTest);
			bool test_passed = (this->*test)();

			if (!test_passed) {
				BurninState = BurninTestState::FAILED;
				Log.error("Elapsed: %lu test failed: %s\n", millis() - startMillis, LastBurnInTest);
			}
			else {
				Log.info("Elapsed: %lu test passed: %s\n", millis() - startMillis, LastBurnInTest);
			}
		}
		break;
		case BurninTestState::FAILED:
		{
			// log failure text every X seconds to UART
			Log.error("FAILED: Uptime: %lu Test: %s ResetReason %d Message: %s", 
				UptimeMillis,
				LastBurnInTest,
				System.resetReason(),
				BurninErrorMessage);
			delay(5000);
		}
		break;
		default:
		break;
	}
}

static int blueBlinksFromRuntime(uint64_t run_time_millis) {
	if(run_time_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(72h).count()){
		return 4;
	} else if(run_time_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(48h).count()){
		return 3;
	} else if(run_time_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(24h).count()){
		return 2;
	} else {
		return 1;
	}
}

static void blinkLed(int blink_time, int red, int green, int blue) {
	RGB.color(red, green, blue);
	delay(blink_time / 2);
	RGB.color(0, 0, 0);
	delay(blink_time / 2);
}

// Blink LED / signal uptime / failure state
void BurninTest::ledLoop(void * arg) {
	system_tick_t total_runtime_millis = 0;
	int blinks_this_period = 0;

    while (true) {
		// If failed = solid red
		if(BurninState == BurninTestState::FAILED) {
			RGB.color(255, 0, 0);
		}
		// If running = blink led
		else if (BurninState == BurninTestState::IN_PROGRESS) {

			blinkLed(BLINK_PERIOD_MILLIS, 0, 255, 0);
			total_runtime_millis += BLINK_PERIOD_MILLIS;
			blinks_this_period++;

			int blue_blinks = blueBlinksFromRuntime(total_runtime_millis);
			Log.trace("total_runtime_millis: %lu blinks_this_period: %d blue_blinks: %d", total_runtime_millis, blinks_this_period, blue_blinks);

			if (blue_blinks + blinks_this_period == BLINK_COUNT_PER_CYCLE) {
				for (int i = 0; i < blue_blinks; i++) {
					blinkLed(BLINK_PERIOD_MILLIS, 0, 0, 255);
					total_runtime_millis += BLINK_PERIOD_MILLIS;
				}

				blinks_this_period = 0;
			}
		}
	}
}


static JSONValue getValue(const JSONValue& obj, const char* name) {
    JSONObjectIterator it(obj);
    while (it.next()) {
        if (it.name() == name) {
            return it.value();
        }
    }
    return JSONValue();
}

bool BurninTest::callFqcTest(String testName){
	bool testPassed = true;
	char buffer[256] = {};

	JSONBufferWriter writer(buffer, sizeof(buffer)-1);
	writer.beginObject().name(testName).value("").endObject();

	JSONValue gpioTestCommand = JSONValue::parseCopy(buffer);
	FqcTest::instance()->process(gpioTestCommand);

	char * fqcTestRawReply = FqcTest::instance()->reply();
	JSONValue testResult = getValue(JSONValue::parseCopy(fqcTestRawReply), "pass");
	if (!testResult.isValid()) {
		const char* jsonPassString = "\"pass\":true";
		if(!strstr(fqcTestRawReply, jsonPassString)) {
			strlcpy(BurninErrorMessage, fqcTestRawReply, sizeof(BurninErrorMessage));
			testPassed = false;
		}
	}
	else if(testResult.toString() != "true") {
		strlcpy(BurninErrorMessage, fqcTestRawReply, sizeof(BurninErrorMessage));
		testPassed = false;
	}
	
	return testPassed;
}

bool BurninTest::testGpio() {
	return callFqcTest("IO_TEST");
}

bool BurninTest::testWifiScan() {
	return callFqcTest("WIFI_SCAN_NETWORKS");
}

bool BurninTest::testBleScan() {
	const size_t SCAN_RESULT_MAX = 30;
	BleScanResult scanResults[SCAN_RESULT_MAX];
	BLE.on();
	BLE.setScanTimeout(50);
	int count = BLE.scan(scanResults, SCAN_RESULT_MAX);
	if (count < 0) {
		Log.error("BLE scan failed: %s", get_system_error_message(count));
	}
	else {
		Log.info("Found %d beacons", count);

		for (int ii = 0; ii < count; ii++) {
			uint8_t buf[BLE_MAX_ADV_DATA_LEN];
			size_t len = scanResults[ii].advertisingData().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);
			Log.info("Beacon %d: rssi %d Advertising Data Len %u", ii, scanResults[ii].rssi(), len);
			if (len > 0) {
				Log.print("Advertising Data: ");
				Log.dump(buf, len);	
				Log.print("\r\n");
			}
		}
	}

	BLE.off();
	return true;
}

bool BurninTest::testSram() {
	bool test_passed = true;
 	Random rng;
	MemoryChunk* root = nullptr;
    size_t total_memory_allocated = 0;
    size_t chunk_count = 0;

    // Generate a random test pattern
    char * test_pattern = (char *)malloc(CHUNK_DATA_SIZE);
    if(!test_pattern) {
		strlcpy(BurninErrorMessage, "Failed to alloc test pattern chunk", sizeof(BurninErrorMessage));
		printRuntimeInfo();
    	test_passed = false;
    	// return early, no other allocations to free
    	return test_passed; 
    }
 	rng.gen(test_pattern, CHUNK_DATA_SIZE);
   
   	printRuntimeInfo();
    // Exhaust memory, leaving some free space for OS operation
    while(System.freeMemory() > (10 * 1024)) {
		MemoryChunk* b = new MemoryChunk();
		if (!b) {
			break;
		} else {
			total_memory_allocated += sizeof(MemoryChunk);
			chunk_count++;
	    	memcpy(b->data, test_pattern, CHUNK_DATA_SIZE);
			b->next = root;
			root = b;
		}
	}
	printRuntimeInfo();
    Log.info("Allocated %u chunks with total size %u, free memory now %lu", chunk_count, total_memory_allocated, System.freeMemory());

	MemoryChunk* current = root;
	while(test_passed && current) {
		if(memcmp(current->data, test_pattern, CHUNK_DATA_SIZE) != 0) {
			String errorMessage = "Chunk failed to match test pattern. ptr @:0x";
			errorMessage += String(current->data, HEX);
			strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
			test_passed = false;
			break;		
		}
		current = current->next;
	}

    SCOPE_GUARD({
	    // Free all allocated memory
		free(test_pattern);

		current = root;
		while(current) {
			MemoryChunk* block_to_free = current;
			current = current->next;
			delete block_to_free;
		}
	});
	return test_passed;
}

const int SPI_FLASH_BUFFER_SIZE = 4096;
static uint8_t spi_flash_buffer[SPI_FLASH_BUFFER_SIZE] = {};

bool BurninTest::testSpiFlash(){
	bool test_passed = true;

#if HAL_PLATFORM_RTL872X
	// MBR part1, Bootloader, System Parts
	Vector<uint32_t> module_addresses = {(uint32_t)&platform_km0_part1_flash_start, (uint32_t)&platform_bootloader_module_info_flash_start, (uint32_t)&platform_system_part1_flash_start};
#else
	// TODO: Non P2 platforms
	Vector<uint32_t> module_addresses = {};
#endif

	int hal_read_result = 0;

	for (auto & address : module_addresses) {
		module_info_t module = {};
		Log.trace("Module Addr 0x%08lx", address);
		hal_read_result = hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, address, (uint8_t*)&module, sizeof(module));
		if (hal_read_result != sizeof(module)) {
			test_passed = false;
			break;
		}
		
		uint32_t module_start =  (uint32_t)module.module_start_address;
		uint32_t module_end = (uint32_t)module.module_end_address;
		uint32_t module_size = module_end - module_start;

		Log.info("module start: 0x%08lX end: 0x%08lX size: 0x%08lX", module_start, module_end, module_size);

		uint32_t calculated_crc = 0;
		uint32_t bytesRemaining = module_size;
		uint32_t readAddress = module_start;
		while (bytesRemaining) {
			memset(spi_flash_buffer, 0x00, sizeof(spi_flash_buffer));
			// Determine chunk size to read
			uint32_t chunk_size = bytesRemaining >= SPI_FLASH_BUFFER_SIZE ? SPI_FLASH_BUFFER_SIZE : bytesRemaining;
			
			// Read image bytes into buff
			Log.trace("hal_storage_read addr 0x%08lx size 0x%04lx", readAddress, chunk_size);
			hal_read_result = hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, readAddress, spi_flash_buffer, chunk_size);

			if (hal_read_result < 0) {
				test_passed = false;
				break;
			}

			// crc buffer
			calculated_crc = particle::softCrc32(spi_flash_buffer, chunk_size, &calculated_crc);

			readAddress += chunk_size;
			bytesRemaining -= chunk_size;
		}

		if (!test_passed) {
			// If we failed to read module data, dont bother reading the other modules
			break;
		}

		uint32_t module_crc;
		hal_read_result = hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, module_end, (uint8_t *)&module_crc, sizeof(module_crc));
		if (hal_read_result < 0) {
			test_passed = false;
			break;
		}

		uint8_t * reverse_crc = (uint8_t *)&module_crc;
		std::reverse(reverse_crc, reverse_crc + 4);
		Log.info("calculated crc: 0x%08lX module crc: 0x%08lX", calculated_crc, module_crc);

		if (calculated_crc != module_crc) {
			test_passed = false;
			// log module that failed
			String errorMessage = String(module_start, HEX);
			errorMessage += String(" module crc: ");
			errorMessage += String(module_crc, HEX);
			errorMessage += String(" calculated crc: ");
			errorMessage += String(calculated_crc, HEX);
			strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
			break;
		}
	}

	if (!test_passed && hal_read_result < 0) {
		Log.error("hal_storage_read failed %d", hal_read_result);
		String errorMessage = String("hal_storage_read failed ");
		errorMessage += String(hal_read_result);
		strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
	}

	return test_passed;
}

bool BurninTest::testCpuLoad() {
	// 4500 iterations is about enough to meet the 10 second test duration minimum
	coremark_set_iterations(random(4500, 6000));
	coremark_main();
	return true;
}

} // particle
#endif // HAL_PLATFORM_RTL872X

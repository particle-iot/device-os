#include "application.h"

#include "spark_wiring_logging.h"
#include "spark_wiring_random.h"
#include "spark_wiring_led.h"
#include "random.h"

#include "fqc_test.h"
#include "burnin_test.h"

#if HAL_PLATFORM_RTL872X
extern uintptr_t platform_km0_part1_flash_start;
extern uintptr_t platform_bootloader_module_info_flash_start;
extern uintptr_t platform_system_part1_flash_start;
#endif

namespace particle {

// Retained state variables
static retained BurninTest::BurninTestState BurninState;
static retained BurninTest::BurninTestName LastBurnInTest;
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

	test_names_ = {"NONE", "GPIO", "WIFI_SCAN", "BLE_SCAN", "SRAM", "SPI_FLASH", "CPU_LOAD"};
}

BurninTest::~BurninTest() {
}

BurninTest* BurninTest::instance() {
    static BurninTest tester;
    return &tester;
}

void BurninTest::setup(bool forceEnable) {
	uint32_t pulse_width_micros = 0;

	if (!forceEnable) {
	    hal_pin_t trigger_pin = D7; // PA27 aka SWD
		// Read the trigger pin for a 1khz pulse. If present, enter burnin mode.
		pinMode(trigger_pin, INPUT);
	    pulse_width_micros = pulseIn(trigger_pin, HIGH);
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
    Log.info("*** BURN IN START *** pulse_width_micros: %lu ", pulse_width_micros);

	// Detect if backup SRAM has a failed test in it (IE state is "TEST FAILED")
	Log.info("BurninState: %d ErrorMessage: %s", (int)BurninState, BurninErrorMessage);

	if(BurninState == BurninTestState::IN_PROGRESS) {
		Log.warn("Previous test failed: %s", test_names_[(int)LastBurnInTest].c_str());
		BurninState = BurninTestState::FAILED;
	}
	else if(BurninState == BurninTestState::FAILED) {
		Log.info("Resetting from failed test state");
		BurninState = BurninTestState::IN_PROGRESS;
		UptimeMillis = 0;
	}
	else {
		BurninState = BurninTestState::IN_PROGRESS;
		UptimeMillis = 0;
	}

	RGB.control(true);
	start_time_millis_ = System.millis();
	next_blink_millis_ = start_time_millis_ + BLINK_PERIOD_MILLIS;
	next_status_report_millis_ = start_time_millis_ + BLINK_PERIOD_STATUS_REPORT_MILLIS;
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

			// pick random test to run, run it
			auto test = tests_[random(tests_.size())];
			bool test_passed = (this->*test)();

			if (!test_passed) {
				BurninState = BurninTestState::FAILED;
				Log.error("Uptime: %lu test failed: %s\n", UptimeMillis, test_names_[(int)LastBurnInTest].c_str());
			}
			else {
				Log.info("Uptime: %lu test passed: %s\n", UptimeMillis, test_names_[(int)LastBurnInTest].c_str());
			}
		}
		break;
		case BurninTestState::FAILED:
		{
			// log failure text every X seconds to UART
			Log.error("***BURNIN_FAILED***, UptimeMillis, %lu, test, %s, message, %s", 
				UptimeMillis,
				test_names_[(int)LastBurnInTest].c_str(),
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

static void setLed(bool on, bool blue){
	if(!on){
		RGB.color(0, 0, 0);
	}
	else if(blue) {
		RGB.color(0, 0, 255);
	}
	else {
		RGB.color(0, 255, 0);
	}
}

// Blink LED / signal uptime / failure state
void BurninTest::ledLoop(void * arg) {
    BurninTest* self = static_cast<BurninTest*>(arg);

    while (true) {
		static bool led_on = false;
		
		static bool blink_blue = false;
		static int blink_blue_count = 0;

		uint64_t current_millis = System.millis();

		// If failed = solid red
		if(BurninState == BurninTestState::FAILED) {
			RGB.color(255, 0, 0);
		}
		// If running = blink led
		else if (BurninState == BurninTestState::IN_PROGRESS) {
			// Time to blink?
			if (current_millis > self->next_blink_millis_) {
				// Time to report status? 
				if ((current_millis > self->next_status_report_millis_) && (blink_blue_count == 0)) {
					blink_blue = true;
					blink_blue_count = blueBlinksFromRuntime(current_millis - self->start_time_millis_);
				}

				setLed(led_on, blink_blue);
				led_on = !led_on;
				self->next_blink_millis_ = current_millis + BLINK_PERIOD_MILLIS;

				// Count down blue blinks, stop reporting status when done 
				if (blink_blue && !led_on) {
					blink_blue_count -= 1;
					if (blink_blue_count == 0) {
						blink_blue = false;
						self->next_status_report_millis_ = current_millis + BLINK_PERIOD_STATUS_REPORT_MILLIS;
					}
				}
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

	JSONValue testResult = getValue(JSONValue::parseCopy(FqcTest::instance()->reply()), "pass");
	if(!testResult.isValid() || testResult.toString() != "true") {
		strlcpy(BurninErrorMessage, FqcTest::instance()->reply(), sizeof(BurninErrorMessage));
		testPassed = false;
	}
	
	return testPassed;
}

bool BurninTest::testGpio() {
	LastBurnInTest = BurninTestName::GPIO;
	return callFqcTest("IO_TEST");
}

bool BurninTest::testWifiScan() {
	LastBurnInTest = BurninTestName::WIFI_SCAN;
	return callFqcTest("WIFI_SCAN_NETWORKS");
}

bool BurninTest::testBleScan() {
	LastBurnInTest = BurninTestName::BLE_SCAN;

	const size_t SCAN_RESULT_MAX = 30;
	BleScanResult scanResults[SCAN_RESULT_MAX];
	BLE.on();
	BLE.setScanTimeout(50);
	int count = BLE.scan(scanResults, SCAN_RESULT_MAX);
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

	BLE.off();
	return true;
}

bool BurninTest::testSram() {
	LastBurnInTest = BurninTestName::SRAM;

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

bool BurninTest::testSpiFlash(){
	LastBurnInTest = BurninTestName::SPI_FLASH;
	bool test_passed = true;

#if HAL_PLATFORM_RTL872X
	// MBR part1, Bootloader, System Parts
	Vector<uint32_t> module_addresses = {(uint32_t)&platform_km0_part1_flash_start, (uint32_t)&platform_bootloader_module_info_flash_start, (uint32_t)&platform_system_part1_flash_start};
#else
	// TODO: Non P2 platforms
	Vector<uint32_t> module_addresses = {};
#endif

	for (auto & address : module_addresses) {
		module_info_t *module_header = (module_info_t*)(address);
		uint32_t module_start =  (uint32_t)module_header->module_start_address;
		uint32_t module_end = (uint32_t)module_header->module_end_address;
		uint32_t module_size = module_end - module_start;
		Log.info("module start: 0x%08lX end: 0x%08lX size: 0x%08lX", module_start, module_end, module_size);

		uint32_t calculated_crc = HAL_Core_Compute_CRC32((uint8_t *)module_header->module_start_address, module_size);
		uint32_t module_crc = *(uint32_t *)(module_header->module_end_address);
		uint8_t * reverse_crc = (uint8_t *)&module_crc;
		std::reverse(reverse_crc, reverse_crc + 4);
		Log.info("calculated crc: 0x%08lX module crc: 0x%08lX", calculated_crc, module_crc);

		if(calculated_crc != module_crc) {
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

	return test_passed;
}

bool BurninTest::testCpuLoad() {
	LastBurnInTest = BurninTestName::CPU_LOAD;
	// TODO: Coremark test? Matrix multiplication, long list traversal, state machines, etc. 
	return true;
}

}
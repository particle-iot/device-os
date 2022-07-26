#pragma once

#include "spark_wiring_vector.h"
#include "debug_output_handler.h"

namespace particle {

class BurninTest {
public:
    BurninTest();
    ~BurninTest();

    static BurninTest* instance();

    void setup(bool forceEnable = false);
    void loop();

	enum class BurninTestState : uint32_t {
		NONE,
		DISABLED,
		IN_PROGRESS,
		FAILED,
	};

private:

    static const uint32_t CHUNK_DATA_SIZE = 508;
    struct MemoryChunk {
    	char data[CHUNK_DATA_SIZE];
		MemoryChunk * next;
    };

	static void ledLoop(void * arg);
    os_thread_t led_thread_ = nullptr;

	static const uint32_t BLINK_PERIOD_MILLIS = 1000;
	static const uint32_t BLINK_COUNT_PER_CYCLE = 10;

	typedef bool (BurninTest::*burnin_test_function)();
	Vector<burnin_test_function> tests_;
	Vector<String> test_names_;
    std::unique_ptr<Serial1LogHandler> logger_;

	bool callFqcTest(String testName);

	bool testGpio();
	bool testWifiScan();
	bool testBleScan();
	bool testSram();
	bool testSpiFlash();
	bool testCpuLoad();
};

}

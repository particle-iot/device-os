#pragma once

#include "spark_wiring_vector.h"

namespace particle {


class BurninTest {
public:
    BurninTest();
    ~BurninTest();

    void setup();
    void loop();

private:

    static const uint32_t CHUNK_DATA_SIZE = 508;
    struct memory_chunk {
    	char data[CHUNK_DATA_SIZE];
		memory_chunk * next;
    };

	enum BurninTestState {
		DISABLED,
		IN_PROGRESS,
		FAILED,
	};

	enum BurninTestName {
		NONE,
		GPIO,
		WIFI_SCAN,
		BLE_SCAN,
		SRAM,
		SPI_FLASH,
		CPU_LOAD
	};

	bool enabled;

	static void led_loop(void * arg);
    os_thread_t led_thread_ = nullptr;

	static const uint32_t BLINK_PERIOD_MILLIS = 500;
	static const uint32_t BLINK_PERIOD_STATUS_REPORT_MILLIS =  (10 * 2 * BLINK_PERIOD_MILLIS);
	uint64_t start_time_millis;
	uint64_t next_blink_millis;
	uint64_t next_status_report_millis;

	

	typedef bool (BurninTest::*burnin_test_function)();
	Vector<burnin_test_function> tests;

	bool test_gpio();
	bool test_wifi_scan();
	bool test_ble_scan();
	bool test_sram();
	bool test_spi_flash();
	bool test_cpu_load();

};

extern BurninTest Burnin;

}
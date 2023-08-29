## 5.5.0

### FEATURES

- Asset OTA [#2668](https://github.com/particle-iot/device-os/pull/2668)
- [M SoM] Platform support [#2681](https://github.com/particle-iot/device-os/pull/2681)
- [rtl872x] Experimental WPA3 support [#2673](https://github.com/particle-iot/device-os/pull/2673)
- [WiFi] Hidden SSID support [#2673](https://github.com/particle-iot/device-os/pull/2673)
- [rtl872x] GPIO drive strength configuration support [#2680](https://github.com/particle-iot/device-os/pull/2680)

### ENHANCEMENTS

- [WiFi] WiFI interface power state notifications [#2669](https://github.com/particle-iot/device-os/pull/2669)
- [rtl872x] Improve `SPI1` (P2 / Tracker M) and `SPI` (M SoM) behavior at 50MHz by changing RXD sample delay [#2677](https://github.com/particle-iot/device-os/pull/2677)
- [rtl872x] Improve USB Serial TX performance [#2672](https://github.com/particle-iot/device-os/pull/2672)
- [Cellular] Enables UPSV=1 low power mode for R510 when idle for >=9.2s [#2674](https://github.com/particle-iot/device-os/pull/2674)
- [rtl872x] Fix logic level overshoot on SPI, I2C, PWM pins [#2680](https://github.com/particle-iot/device-os/pull/2680)
- [rtl872x] Revert the QSPI flash speed to 80MHz [#2684](https://github.com/particle-iot/device-os/pull/2684)
- Update the key used for validating the ServerMoved signature [#2688](https://github.com/particle-iot/device-os/pull/2688)
- [rtl872x] remove the delay in UART flush() [#2686](https://github.com/particle-iot/device-os/pull/2686)

### BUGFIXES

- [rtl872x] Fix incorrect I2C read timeout [#2671](https://github.com/particle-iot/device-os/pull/2671)
- [rtl872x] Do not initialize RTC after waking up from HIBERNATE sleep [#2667](https://github.com/particle-iot/device-os/pull/2667)
- [rtl872x] Disable pull resistors when pin is configure as `OUTPUT` [#2666](https://github.com/particle-iot/device-os/pull/2666)
- [rtl872x] Disable SWDIO/SWDCLK pins when going into sleep [#2666](https://github.com/particle-iot/device-os/pull/2666)
- [rtl872x] Disable brown-out detector [#2679](https://github.com/particle-iot/device-os/pull/2679)
- [rtl872x] Avoid glitch on I2C pins when reconfiguring I2C peripheral [#2682](https://github.com/particle-iot/device-os/pull/2682)
- [msom] Fix ethernet cs, reset, interrupt GPIO pins for M2 eval [#2690](https://github.com/particle-iot/device-os/pull/2690)

### INTERNAL

- [WiFi] Resolve `wiring/sleep20` test failures [#2669](https://github.com/particle-iot/device-os/pull/2669)
- [nrf52840] Add factory reset test (`ota/factory_reset`) [#2662](https://github.com/particle-iot/device-os/pull/2662)

## 5.5.0-rc.1

### FEATURES

- Asset OTA [#2668](https://github.com/particle-iot/device-os/pull/2668)
- [M SoM] Platform support [#2681](https://github.com/particle-iot/device-os/pull/2681)
- [rtl872x] Experimental WPA3 support [#2673](https://github.com/particle-iot/device-os/pull/2673)
- [WiFi] Hidden SSID support [#2673](https://github.com/particle-iot/device-os/pull/2673)
- [rtl872x] GPIO drive strength configuration support [#2680](https://github.com/particle-iot/device-os/pull/2680)

### ENHANCEMENTS

- [WiFi] WiFI interface power state notifications [#2669](https://github.com/particle-iot/device-os/pull/2669)
- [rtl872x] Improve `SPI1` (P2 / Tracker M) and `SPI` (M SoM) behavior at 50MHz by changing RXD sample delay [#2677](https://github.com/particle-iot/device-os/pull/2677)
- [rtl872x] Improve USB Serial TX performance [#2672](https://github.com/particle-iot/device-os/pull/2672)
- [Cellular] Enables UPSV=1 low power mode for R510 when idle for >=9.2s [#2674](https://github.com/particle-iot/device-os/pull/2674)
- [rtl872x] Fix logic level overshoot on SPI, I2C, PWM pins [#2680](https://github.com/particle-iot/device-os/pull/2680)
- [rtl872x] Revert the QSPI flash speed to 80MHz [#2684](https://github.com/particle-iot/device-os/pull/2684)

### BUGFIXES

- [rtl872x] Fix incorrect I2C read timeout [#2671](https://github.com/particle-iot/device-os/pull/2671)
- [rtl872x] Do not initialize RTC after waking up from HIBERNATE sleep [#2667](https://github.com/particle-iot/device-os/pull/2667)
- [rtl872x] Disable pull resistors when pin is configure as `OUTPUT` [#2666](https://github.com/particle-iot/device-os/pull/2666)
- [rtl872x] Disable SWDIO/SWDCLK pins when going into sleep [#2666](https://github.com/particle-iot/device-os/pull/2666)
- [rtl872x] Disable brown-out detector [#2679](https://github.com/particle-iot/device-os/pull/2679)
- [rtl872x] Avoid glitch on I2C pins when reconfiguring I2C peripheral [#2682](https://github.com/particle-iot/device-os/pull/2682)

### INTERNAL

- [WiFi] Resolve `wiring/sleep20` test failures [#2669](https://github.com/particle-iot/device-os/pull/2669)
- [nrf52840] Add factory reset test (`ota/factory_reset`) [#2662](https://github.com/particle-iot/device-os/pull/2662)

## 5.4.1

### ENHANCEMENTS
- [rtl872x] dynamically enable Wi-Fi stack on demand [#2664](https://github.com/particle-iot/device-os/pull/2664)

### BUGFIXES
- [rtl872x] linker: stop relying on .dynalib + .psram_text being contiguous and properly and similarly aligned within LMA and VMA, just copy them separately [#2665](https://github.com/particle-iot/device-os/pull/2665)
- [rtl872x] fix BLE race condition [#2664](https://github.com/particle-iot/device-os/pull/2664)

## 5.4.0

### ENHANCEMENTS
- [rtl872x] USB HID Mouse/Keyboard support [#2659](https://github.com/particle-iot/device-os/pull/2659)

### BUGFIXES
- Fix/nanopb string max size [#2657](https://github.com/particle-iot/device-os/pull/2657)
- Fixes clean target for applications with large number of files + P2 PSRAM size calculation issues [#2661](https://github.com/particle-iot/device-os/pull/2661)

### INTERNAL
- [rtl872x] Allow KM4 SDK Bootloader images to boot [#2656](https://github.com/particle-iot/device-os/pull/2656)
- [rtl872x] Combined MFG firmware [#2658](https://github.com/particle-iot/device-os/pull/2658)

## 5.3.2

### ENHANCEMENTS
- [rtl872x] SPI and GPIO HAL changes to support Neopixel [#2654](https://github.com/particle-iot/device-os/pull/2654)
- [rtl872x] Implement WiFi.selectAntenna [#2651](https://github.com/particle-iot/device-os/pull/2651)

### BUGFIXES
- [nRF52] UART sleep/wakeup [#2652](https://github.com/particle-iot/device-os/pull/2652)
- [rtl872x] Fix Wifi stack issues [#2649](https://github.com/particle-iot/device-os/pull/2649)
- [rtl872x] BLE scanning panic [#2650](https://github.com/particle-iot/device-os/pull/2650)

### INTERNAL
- [rtl872x] Fix burnin GPIO and SPI Flash tests for photon 2 [#2653](https://github.com/particle-iot/device-os/pull/2653)

## 5.3.1

### ENHANCEMENTS
- [rtl872x] Adds Wiring API System.backupRamSync() to manually backup `retained` variables [#2633](https://github.com/particle-iot/device-os/pull/2633)

### BUGFIXES
- [rtl872x] Fix problems waking from sleep mode [#2647](https://github.com/particle-iot/device-os/pull/2647)
- Fixes inconsistent BLE state issues [#2629](https://github.com/particle-iot/device-os/pull/2629)
- [rtl872x] Fixes pmic shared interrupt and usb detection [#2630](https://github.com/particle-iot/device-os/pull/2630)
- [rtl872x] Fixes D7 configuration when exiting hibernate mode [#2631](https://github.com/particle-iot/device-os/pull/2631)
- [rtl872x][bootloader] Fixes USB serial port not being connectable on AMD based Windows [#2625](https://github.com/particle-iot/device-os/pull/2625)[#2638](https://github.com/particle-iot/device-os/pull/2638)
- [rtl872x] Fixes `retained` variables not being saved, now periodically saved every 10s [#2633](https://github.com/particle-iot/device-os/pull/2633)[#2642](https://github.com/particle-iot/device-os/pull/2642)[#2644](https://github.com/particle-iot/device-os/pull/2644)
- [rtl872x] Fixes assertion failure waiting for connection events from the stack when `BLE.connect()` called [#2636](https://github.com/particle-iot/device-os/pull/2636)
- [nRF52] Fixes watchdog timer reload value accuracy [#2635](https://github.com/particle-iot/device-os/pull/2635)
- [rtl872x] hal: Fixes BLE notifying multiple times [#2637](https://github.com/particle-iot/device-os/pull/2637)
- [r510] Enables PS (packet switched) only mode for R510 modems (Boron/BSoM/ESoMX) [#2639](https://github.com/particle-iot/device-os/pull/2639)[#2645](https://github.com/particle-iot/device-os/pull/2645)
- [rtl872x] hal: Fixes I2C failing to read/write from/to slave device [#2634](https://github.com/particle-iot/device-os/pull/2634)
- [p2] fixes SPI speed settings, and improves SPI DMA timing for larger transfers [#2641](https://github.com/particle-iot/device-os/pull/2641)
- [rtl872x] hal: uart may deadlock on initialization [#2643](https://github.com/particle-iot/device-os/pull/2643)
- [rtl872x] Dont enable usart RX/TX pullups[#2646](https://github.com/particle-iot/device-os/pull/2646)

## 5.3.0

### FEATURES
- Static IP configuration support [#2621](https://github.com/particle-iot/device-os/pull/2621)
- Hardware watchdog [#2595](https://github.com/particle-iot/device-os/pull/2595)[#2617](https://github.com/particle-iot/device-os/pull/2617)[#2620](https://github.com/particle-iot/device-os/pull/2620)
- Ethernet GPIO config [#2616](https://github.com/particle-iot/device-os/pull/2616)
- [wiring][gen3] Allow gen3 to select internal ADC reference source [#2619](https://github.com/particle-iot/device-os/pull/2619)

### ENHANCEMENTS
- [rtl872x] Support IO wakeup sources through IO expander.[#2604](https://github.com/particle-iot/device-os/pull/2604)[#2608](https://github.com/particle-iot/device-os/pull/2608)[#2614](https://github.com/particle-iot/device-os/pull/2614)

### BUGFIXES
- [rtl872x] enable factory reset feature [#2612](https://github.com/particle-iot/device-os/pull/2612)
- [rtl872x] Dcache fixes for exflash HAL [#2623](https://github.com/particle-iot/device-os/pull/2623)
- [rtl872x] fix overflow error in HAL_Delay_Microseconds [#2606](https://github.com/particle-iot/device-os/pull/2606)
- [nRF52] BLE plus RTC sleep causes hardfault [#2615](https://github.com/particle-iot/device-os/pull/2615)
- [rtl872x] hal: fix heap allocation issue in interrupt hal and postpone the mode button initialization. [#2624](https://github.com/particle-iot/device-os/pull/2624)
- [rtl872x] bootloader: fix destination address flash page alignment in case of compressed modules [#2628](https://github.com/particle-iot/device-os/pull/2628)

### INTERNAL
- [photon2] Update FQC test with photon 2 pinout [#2610](https://github.com/particle-iot/device-os/pull/2610)
- [test] mailbox support and support for resets within tests [#2611](https://github.com/particle-iot/device-os/pull/2611)
- [bootloader] remove nanopb dependency[#2607](https://github.com/particle-iot/device-os/pull/2607)
- Server key rotation [#2570](https://github.com/particle-iot/device-os/pull/2570)
- Fix GCC platform build [#2613](https://github.com/particle-iot/device-os/pull/2613)[#2618](https://github.com/particle-iot/device-os/pull/2618)
- Ensure thread07 test executes as intended [#2622](https://github.com/particle-iot/device-os/pull/2622) 
- [rtl872x] Fix watchdog tests; system/application thread stack size increase [#2626](https://github.com/particle-iot/device-os/pull/2626)
- [test] turn off NCP before testing wiring/watchdog [#2627](https://github.com/particle-iot/device-os/pull/2627)

## 5.2.0

### FEATURES
- Initial support for BG95-M6 modem [#2555](https://github.com/particle-iot/device-os/pull/2555)

### ENHANCEMENTS
- [TrackerM] Collect cellular properties with system info [#2602](https://github.com/particle-iot/device-os/pull/2602)
- [rtl872x] fixes System.ticks() [#2600](https://github.com/particle-iot/device-os/pull/2600)
- System setup and BLE threading improvements [#2587](https://github.com/particle-iot/device-os/pull/2587)
- [rtl872x] [freertos] [experimental] multi-step priority disinheritance [#2581](https://github.com/particle-iot/device-os/pull/2581)
- [rtl872x] sleep improvements [#2586](https://github.com/particle-iot/device-os/pull/2586)
- [rtl872x] fixes pinResetFast clearing too many pins and improves speed [#2582](https://github.com/particle-iot/device-os/pull/2582)
- [rtl872x] prebootloader: enable BOR with lowest available thresholds [#2569](https://github.com/particle-iot/device-os/pull/2569)

### BUGFIXES
- [rtl872x] Fix USART/DMA deadlock [#2603](https://github.com/particle-iot/device-os/pull/2603)
- [rtl872x] Free memory from rtl sdk in SystemISRTaskQueue [#2599](https://github.com/particle-iot/device-os/pull/2599)
- [rtl872x] exflash: revert dcache invalidate calls after writes/erasures [#2598](https://github.com/particle-iot/device-os/pull/2598)
- [rtl872x] Fixes a deadlock when requiring to enable RSIP [#2596](https://github.com/particle-iot/device-os/pull/2596)
- [gen3] Use OTP Feature flag to change ADC reference source [#2597](https://github.com/particle-iot/device-os/pull/2597)
- [rtl872x] hal: remove an assert in read()/peek() and instead adjust read/peek size [#2594](https://github.com/particle-iot/device-os/pull/2594)
- [boron]Use the internal ADC reference on BRN404X [#2588](https://github.com/particle-iot/device-os/pull/2588)
- [p2] Fixes the conflict between Flash API and XIP [#2561](https://github.com/particle-iot/device-os/pull/2561)
- [Quectel] Account for "eMTC" type while obtaining signal values [#2589](https://github.com/particle-iot/device-os/pull/2589)
- [Boron / B SoM] R410 PPP crash in network phase workaround [#2571](https://github.com/particle-iot/device-os/pull/2571)
- [Cellular] R410 initialization SIM failure workaround [#2573](https://github.com/particle-iot/device-os/pull/2573)
- [gen3] [p2] Fix i2c hal deadlock [#2572](https://github.com/particle-iot/device-os/pull/2572)
- [TrackerM] Prevent connection over wifi when configured in SCAN_ONLY mode [#2567](https://github.com/particle-iot/device-os/pull/2567)
- [p2] WiFi bugfixes [#2562](https://github.com/particle-iot/device-os/pull/2562)
- [tracker/trackerm] Fix acquireWireBuffer for platforms where system initializes I2C before user app [2551](https://github.com/particle-iot/device-os/pull/2551)
- [rtl872x] BLE Central connection failure when peer disconnects [#2552](https://github.com/particle-iot/device-os/pull/2552)

### INTERNAL
- [test] Use compatible pins for PWM tests on TrackerM [#2592](https://github.com/particle-iot/device-os/pull/2592)
- [test] bump fastpin max limit to 10% [#2591](https://github.com/particle-iot/device-os/pull/2591)
- [test] Add thresholds for trackerM for slo tests [#2590](https://github.com/particle-iot/device-os/pull/2590)
- [test] Modify pins for trackerM spix tests [#2585](https://github.com/particle-iot/device-os/pull/2585)
- [trackerm] TrackerM EVT v0.0.3 pinmap update [#2580](https://github.com/particle-iot/device-os/pull/2580)
- Allow setting the ICCID of a virtual device [#2583](https://github.com/particle-iot/device-os/pull/2583)
- Protobuf defs refactor / fixes submessage encoding after nanopb 0.4.5 upgrade [#2578](https://github.com/particle-iot/device-os/pull/2578)
- Update nanopb to 0.4.5 [#2563](https://github.com/particle-iot/device-os/pull/2563)
- Fix no_fixture_i2c for esomx,boron,bsom platforms [#2559](https://github.com/particle-iot/device-os/pull/2559)
- [CI] fixes [#2564](https://github.com/particle-iot/device-os/pull/2564)
- Move no_fixture_i2c to correct dir and symlink to integration/wiring [#2558](https://github.com/particle-iot/device-os/pull/2558)
- [trackerm] TrackerM EVT v0.0.2 pinmap update [#2550](https://github.com/particle-iot/device-os/pull/2550)
- Increase timeouts for internal CI builds on windows [#2545](https://github.com/particle-iot/device-os/pull/2545)

## 5.1.0

### FEATURES
- [rtl8721x][p2] supports BLE GATT client, BLE central role and pairing APIs [#2542](https://github.com/particle-iot/device-os/pull/2542)

### ENHANCEMENTS
- [rtl872x] Improve ADC accuracy [#2546](https://github.com/particle-iot/device-os/pull/2546)
- [rtl8721x] update rtl872x.tcl script for latest openocd version [#2525](https://github.com/particle-iot/device-os/pull/2525)
- [hal] wifi: add generic 'world' country code as not every country code is exposed through API [#2539](https://github.com/particle-iot/device-os/pull/2539)
- [rtl8721x] Increase power management thread stack size for Tracker M [#2535](https://github.com/particle-iot/device-os/pull/2535)
- [trackerM]Fix MCP23S17 driver to allow mirrored (shared) interrupts [#2533](https://github.com/particle-iot/device-os/pull/2533)

### BUGFIXES
- [rtl872x] usb: make sure CDC line coding struct is packed [#2541](https://github.com/particle-iot/device-os/pull/2541)
- [rtl872x] hal: fix fetching of MAC addresses [#2537](https://github.com/particle-iot/device-os/pull/2537)
- [tests] Fix listening mode tests on trackerM [#2534](https://github.com/particle-iot/device-os/pull/2534)
- [wiring] ApplicationWatchdog: fixes potential 2x timeout required to fire [#2536](https://github.com/particle-iot/device-os/pull/2536)
- [p2][gen3]Fix BLE control request channel sending malformed packets [#2538](https://github.com/particle-iot/device-os/pull/2538)
- [tests/trackerm] Add workaround for TrackerM ota tests [#2544](https://github.com/particle-iot/device-os/pull/2544)

### INTERNAL
- Use new `prtcl` compile/clean commands for internal CI builds [#2543](https://github.com/particle-iot/device-os/pull/2543)
- Improve COAP message logging [#2498](https://github.com/particle-iot/device-os/pull/2498)
- HAL wiring api calls to access exflash read/write functions for OTP flash page [#2540](https://github.com/particle-iot/device-os/pull/2540)

## 5.0.1

### FEATURES
- Added APIs for hardware configuration for reading OTP format [#2526](https://github.com/particle-iot/device-os/pull/2526)
- [trackerM] Update demux driver and pinmap to support more latest hardware

### BUGFIXES
- Secures DCT initialization from getting interrupted between creating DCT file and filling it with 0xff to default state [#2530](https://github.com/particle-iot/device-os/pull/2530)
- [trackerM] Fix hal dynalib tables [#2523](https://github.com/particle-iot/device-os/pull/2523)

### INTERNAL
- [trackerM] Fix interrupt based tests [#2527](https://github.com/particle-iot/device-os/pull/2527)

## 5.0.0

### BREAKING CHANGE

- [esomx] Remove undefined pins from esomx pinmap [#2505](https://github.com/particle-iot/device-os/pull/2466)

### FEATURES
- P2 Support [#2466](https://github.com/particle-iot/device-os/pull/2466)
- Tracker M Support [#2492](https://github.com/particle-iot/device-os/pull/2492) [#2518](https://github.com/particle-iot/device-os/pull/2518) [#2522](https://github.com/particle-iot/device-os/pull/2522)
- [p2] Allow setting WiFi country code and channel plan [#2485](https://github.com/particle-iot/device-os/pull/2485) [#2473](https://github.com/particle-iot/device-os/pull/2473)
- [rtl872x] Implement HAL event group for UART [#2493](https://github.com/particle-iot/device-os/pull/2493)
- [rtl872x] Implement APIs to set/get BLE address [#2508](https://github.com/particle-iot/device-os/pull/2508)

### ENHANCEMENTS
- [p2][wifi] Add more complete WiFi network configuration information for P2 [#2474](https://github.com/particle-iot/device-os/pull/2474)
- [p2][photon2] Add VBAT_MEAS pin and charging indication pin for Photon2 [#2482](https://github.com/particle-iot/device-os/pull/2482)
- [ethernet][p2] Allow ethernet detection on P2 [#2478](https://github.com/particle-iot/device-os/pull/2478)
- Register without waiting for CSD connections [#2404](https://github.com/particle-iot/device-os/pull/2404)

### BUGFIXES
- [p2] usart: flush DMA FIFO data when required [#2477](https://github.com/particle-iot/device-os/pull/2477)
- [gen3] port newlib stdin/stdout/stderr memory leak workaround [#2467](https://github.com/particle-iot/device-os/pull/2467)
- [p2] Flash supports page write and other fixes [#2470](https://github.com/particle-iot/device-os/pull/2470)
- [esomx] map ADC channels to the correct pins [#2495](https://github.com/particle-iot/device-os/pull/2495)
- [rtl872x] Fix millis() rollover [#2501](https://github.com/particle-iot/device-os/pull/2501)
- [p2][gen3] fixes for attachInterrupt() / noInterrupts() / interrupts() APIs [#2503](https://github.com/particle-iot/device-os/pull/2503)
- [rtl872x] hal: fix issue that uart rx dma may hang up. [#2502](https://github.com/particle-iot/device-os/pull/2502)
- [rtl872x] hal: implement i2c sleep and fix the reset APIs [#2497](https://github.com/particle-iot/device-os/pull/2497)
- [p2] attachInterrupt() should not configure input pullup/pulldown/nopull [#2507](https://github.com/particle-iot/device-os/pull/2507)
- [p2] PWM HAL fixes [#2511](https://github.com/particle-iot/device-os/pull/2511)
- Disable direct logging in listening mode [#2512](https://github.com/particle-iot/device-os/pull/2512)
- Ensure task yield and system thread pump in system_delay_pump [#2519](https://github.com/particle-iot/device-os/pull/2519) [#2521](https://github.com/particle-iot/device-os/pull/2521)

### INTERNAL
- [p2] deprecate tinker-fqc app [#2479](https://github.com/particle-iot/device-os/pull/2479) [#2480](https://github.com/particle-iot/device-os/pull/2480) [#2481](https://github.com/particle-iot/device-os/pull/2481)
- Update devtools .bundleignore for P2 SDK files [#2475](https://github.com/particle-iot/device-os/pull/2475)
- Fix GCC platform on some versions of GCC; Fix wiring/api tests for P2; Fix building of wiring/api tests on CI [#2483](https://github.com/particle-iot/device-os/pull/2483)
- [p2] Fix DCache alignment [#2476](https://github.com/particle-iot/device-os/pull/2476)
- [p2] fixes INTERRUPTS_01_isisr_willpreempt_servicedirqn test [#2486](https://github.com/particle-iot/device-os/pull/2486)
- Remove redundant toolchain overrides [#2489](https://github.com/particle-iot/device-os/pull/2489)
- Enable OTA updates on the GCC platform [#2464](https://github.com/particle-iot/device-os/pull/2464)
- Allow overriding the platform ID on the GCC platform [#2462](https://github.com/particle-iot/device-os/pull/2462)
- [workbench] Update workbench buildscripts to 1.11.0 [#2465](https://github.com/particle-iot/device-os/pull/2465)
- [tests] Change PWM pins to better match e-series pinout for E SoM X. Fix other tests [#2463](https://github.com/particle-iot/device-os/pull/2463)
- [tests] Fix broken tone and servo tests for p2 platform, also EEPROM_03 for all. [#2469](https://github.com/particle-iot/device-os/pull/2469)
- [workbench] Stop ignoring device os scripts directory [#2471](https://github.com/particle-iot/device-os/pull/2471)
- [p2] Simplify burn in LED blink, work around System.millis() reset [#2491](https://github.com/particle-iot/device-os/pull/2491)
- [tests][esomx] disable RGB pwm channel(s) when running pwm tests [#2494](https://github.com/particle-iot/device-os/pull/2494)
- [rtl872x] Poll micros instead of rom DelayUs function. Tweak 1ms test [#2499](https://github.com/particle-iot/device-os/pull/2499)
- [ci] branch pattern changes for test-build-system [#2509](https://github.com/particle-iot/device-os/pull/2509)
- [p2] PWM fixture tests [#2511](https://github.com/particle-iot/device-os/pull/2511)
- [test] fixes race condition with wiring/no_fixture TIME_17 [#2513](https://github.com/particle-iot/device-os/pull/2513)
- [services] device tree stubs [#2515](https://github.com/particle-iot/device-os/pull/2515)
- [tests] integration: bump binary-version-reader and device-constants dependency for P2-related module manipulation fixes [#2516](https://github.com/particle-iot/device-os/pull/2516)

## 5.0.0-alpha.2

> ## :warning: Please note this is an internal release, do not use on production devices!

### FEATURES
- [p2] Allow setting WiFi country code and channel plan [#2485](https://github.com/particle-iot/device-os/pull/2485) [#2473](https://github.com/particle-iot/device-os/pull/2473)

### ENHANCEMENTS
- [p2][wifi] Add more complete WiFi network configuration information for P2 [#2474](https://github.com/particle-iot/device-os/pull/2474)
- [p2][photon2] Add VBAT_MEAS pin and charging indication pin for Photon2 [#2482](https://github.com/particle-iot/device-os/pull/2482)
- [ethernet][p2] Allow ethernet detection on P2 [#2478](https://github.com/particle-iot/device-os/pull/2478)

### BUGFIXES
- [p2] usart: flush DMA FIFO data when required [#2477](https://github.com/particle-iot/device-os/pull/2477)

### INTERNAL
- [p2] deprecate tinker-fqc app [#2479](https://github.com/particle-iot/device-os/pull/2479) [#2480](https://github.com/particle-iot/device-os/pull/2480) [#2481](https://github.com/particle-iot/device-os/pull/2481)
- Update devtools .bundleignore for P2 SDK files [#2475](https://github.com/particle-iot/device-os/pull/2475)
- Fix GCC platform on some versions of GCC; Fix wiring/api tests for P2; Fix building of wiring/api tests on CI [#2483](https://github.com/particle-iot/device-os/pull/2483)
- [p2] Fix DCache alignment [#2476](https://github.com/particle-iot/device-os/pull/2476) 
- [p2] fixes INTERRUPTS_01_isisr_willpreempt_servicedirqn test [#2486](https://github.com/particle-iot/device-os/pull/2486)
- Remove redundant toolchain overrides [#2489](https://github.com/particle-iot/device-os/pull/2489)

## 5.0.0-alpha.1

> ## :warning: Please note this is an internal release, do not use on production devices!

### FEATURES
- P2 Support [#2466](https://github.com/particle-iot/device-os/pull/2466)

### ENHANCEMENTS
- Register without waiting for CSD connections [#2404](https://github.com/particle-iot/device-os/pull/2404)

### BUGFIXES
- [gen3] port newlib stdin/stdout/stderr memory leak workaround [#2467](https://github.com/particle-iot/device-os/pull/2467)
- [p2] Flash supports page write and other fixes [#2470](https://github.com/particle-iot/device-os/pull/2470)

### INTERNAL
- Enable OTA updates on the GCC platform [#2464](https://github.com/particle-iot/device-os/pull/2464)
- Allow overriding the platform ID on the GCC platform [#2462](https://github.com/particle-iot/device-os/pull/2462)
- [workbench] Update workbench buildscripts to 1.11.0 [#2465](https://github.com/particle-iot/device-os/pull/2465)
- [tests] Change PWM pins to better match e-series pinout for E SoM X. Fix other tests [#2463](https://github.com/particle-iot/device-os/pull/2463)
- [tests] Fix broken tone and servo tests for p2 platform, also EEPROM_03 for all. [#2469](https://github.com/particle-iot/device-os/pull/2469)
- [workbench] Stop ignoring device os scripts directory [#2471](https://github.com/particle-iot/device-os/pull/2471)

## 4.1.0

### FEATURES

- Hardware watchdog [#2595](https://github.com/particle-iot/device-os/pull/2595) [#2626](https://github.com/particle-iot/device-os/pull/2626)
- Server key rotation [#2570](https://github.com/particle-iot/device-os/pull/2570)

### BUGFIXES

- Wi-Fi/cellular network manager bugfixes [#2621](https://github.com/particle-iot/device-os/pull/2621)
- [nRF52] UART sleep/wakeup [#2652](https://github.com/particle-iot/device-os/pull/2652)
- [nRF52] watchdog timeout is not accurate [#2635](https://github.com/particle-iot/device-os/pull/2635)
- [nRF52] BLE plus RTC sleep causes hardfault [#2615](https://github.com/particle-iot/device-os/pull/2615)
- Fix inconsistent BLE state issue [#2629](https://github.com/particle-iot/device-os/pull/2629)
- [wiring][gen3] Allow gen3 to select internal ADC reference source [#2619](https://github.com/particle-iot/device-os/pull/2619)
- System setup and BLE threading improvements [#2587](https://github.com/particle-iot/device-os/pull/2587)
- [quectel] Account for "eMTC" type while obtaining signal values [#2589](https://github.com/particle-iot/device-os/pull/2589)
- Fix i2c hal deadlock [#2572](https://github.com/particle-iot/device-os/pull/2572)
- [r510] enable PS (packet switched) Only mode for R510 modems (Boron/BSoM/ESoMX) [#2640](https://github.com/particle-iot/device-os/pull/2640) [See TAN012](https://docs.particle.io/reference/technical-advisory-notices/tan012/)

### INTERNAL

- [test] turn off NCP before testing wiring/watchdog [#2627](https://github.com/particle-iot/device-os/pull/2627)
- [test] Remove manual wakeup. Hibernate + watchdog on platforms that support it [#2620](https://github.com/particle-iot/device-os/pull/2620)
- [test] add more watchdog test cases [#2617](https://github.com/particle-iot/device-os/pull/2617)
- [test] mailbox support and support for resets within tests [#2611](https://github.com/particle-iot/device-os/pull/2611)
- [test] Move `no_fixture_i2c` to correct dir and symlink to `integration/wiring` [#2558](https://github.com/particle-iot/device-os/pull/2558)
- [test] Fix listening mode tests [#2534](https://github.com/particle-iot/device-os/pull/2534)
- [test] Ensure thread07 test executes as intended [#2622](https://github.com/particle-iot/device-os/pull/2622)
- Protobuf defs refactor / fixes submessage encoding after nanopb 0.4.5 upgrade [#2578](https://github.com/particle-iot/device-os/pull/2578)
- Update nanopb to 0.4.5 [#2563](https://github.com/particle-iot/device-os/pull/2563)

## 4.0.2

### BUGFIXES
- [gen3] Use OTP Feature flag to change ADC reference source [#2597](https://github.com/particle-iot/device-os/pull/2597)
- [boron]Use the internal ADC reference on some Gen 3 platforms [#2588](https://github.com/particle-iot/device-os/pull/2588)

## 4.0.1

### BUGFIXES
- [wiring] ApplicationWatchdog: fixes potential 2x timeout required to fire [#2536](https://github.com/particle-iot/device-os/pull/2536)
- [gen3]Fix BLE control request channel sending malformed packets [#2538](https://github.com/particle-iot/device-os/pull/2538)
- Fix issue with platform_ncp_get_info(0) for quectel platforms [#2532](https://github.com/particle-iot/device-os/pull/2532/)
- Secures DCT initialization from getting interrupted between creating DCT file and filling it with 0xff to default state [#2530](https://github.com/particle-iot/device-os/pull/2530)
- [Boron / B SoM] R410 PPP crash in network phase workaround [#2571](https://github.com/particle-iot/device-os/pull/2571)
- [Cellular] R410 initialization SIM failure workaround [#2573](https://github.com/particle-iot/device-os/pull/2573)

### INTERNAL
- Use new `prtcl` compile/clean commands for internal CI builds [#2543](https://github.com/particle-iot/device-os/pull/2543)
- Increase timeouts for internal CI builds on windows [#2545](https://github.com/particle-iot/device-os/pull/2545)
- HAL wiring api calls to access exflash read/write functions for OTP flash page [#2540](https://github.com/particle-iot/device-os/pull/2540)
- [hal] wifi: add generic 'world' country code as not every country code is exposed through API [#2539](https://github.com/particle-iot/device-os/pull/2539)

## 4.0.0

### DEPRECATION

- [deprecation][gen2] supply secure [#2442](https://github.com/particle-iot/device-os/pull/2442)
- [deprecation] Adds warning to some deprecated API's that will be removed in Device OS 5.x [#2445](https://github.com/particle-iot/device-os/pull/2445)
- [deprecation] PRODUCT_ID macro [#2446](https://github.com/particle-iot/device-os/pull/2446)
- [deprecation] remove setup_done flag and add deprecation notice [#2447](https://github.com/particle-iot/device-os/pull/2447)

### FEATURES

- [ota] new API System.updateStatus() [#2344](https://github.com/particle-iot/device-os/pull/2344)
- [esomx] Adds support for new platform esomx [#2443](https://github.com/particle-iot/device-os/pull/2443) [#2459](https://github.com/particle-iot/device-os/pull/2459) [#2505](https://github.com/particle-iot/device-os/pull/2505) [#2495](https://github.com/particle-iot/device-os/pull/2495)
- [gen3] Determine flash part at runtime [#2456](https://github.com/particle-iot/device-os/pull/2456)
- [gen3][quectel] Adds support for BG95-M1, BG95-MF, BG77, and EG91-NAX [#2458](https://github.com/particle-iot/device-os/pull/2458)
- `System.hardwareInfo()` API [#2526](https://github.com/particle-iot/device-os/pull/2526) [#2529](https://github.com/particle-iot/device-os/pull/2529)

### ENHANCEMENTS

- [gen3] wifi: add dhcp dns info to wifi config [#2440](https://github.com/particle-iot/device-os/pull/2440)
- [ota] Additional state for firmware update checks [#2344](https://github.com/particle-iot/device-os/pull/2344)
- Use a custom content type with CID packets when resuming the session [#2441](https://github.com/particle-iot/device-os/pull/2441)

### BUGFIXES

- [gen3] fixes hardfault during low level USB peripheral initialization under an atomic section [#2448](https://github.com/particle-iot/device-os/pull/2448)
- [gen3] Device unable to enter listening mode with button press [#2451](https://github.com/particle-iot/device-os/pull/2451)
- Return relevant error from control request to enter listening mode [#2419](https://github.com/particle-iot/device-os/pull/2419)
- [gen3] hal: fix power leak on Boron [#2452](https://github.com/particle-iot/device-os/pull/2452)
- [gen3] port newlib stdin/stdout/stderr memory leak workaround [#2467](https://github.com/particle-iot/device-os/pull/2467)
- [gen3] fixes `interrupts()` API: should not clear pending interrupts [#2504](https://github.com/particle-iot/device-os/pull/2504)
- Ensure that RTOS context switch is performed in tight `delay(1)` loops [#2519](https://github.com/particle-iot/device-os/pull/2519) [#2520](https://github.com/particle-iot/device-os/pull/2520) [#2524](https://github.com/particle-iot/device-os/pull/2524)

### INTERNAL

- [ci] minor update sc-101315/device-os-manifest [#2449](https://github.com/particle-iot/device-os/pull/2449)
- [ci] chore/ci-less-frequent-cross-platform-build-checks [#2434](https://github.com/particle-iot/device-os/pull/2434)
- [docs] Update dependencies for ARM GCC 10.2.1 [#2431](https://github.com/particle-iot/device-os/pull/2431)
- [gen3] suppress certain reviewed GCC warnings [sc-100940] [#2420](https://github.com/particle-iot/device-os/pull/2420)
- [ci] feature/sc-100324/ci-build-cross-platform [#2418](https://github.com/particle-iot/device-os/pull/2418)
- [test] Wi-Fi resolve test improvements [#2454](https://github.com/particle-iot/device-os/pull/2454)
- [workbench] update-device-os-workbench-manifest-json [#2457](https://github.com/particle-iot/device-os/pull/2457)
- [ci] test-build-system-tune-timeouts [#2455](https://github.com/particle-iot/device-os/pull/2455)
- Allow overriding the platform ID on the GCC platform [#2462](https://github.com/particle-iot/device-os/pull/2462)
- Enable OTA updates on the GCC platform [#2464](https://github.com/particle-iot/device-os/pull/2464)
- Fix GCC platform on some versions of GCC; Fix building of wiring/api tests on CI [#2483](https://github.com/particle-iot/device-os/pull/2483)
- [workbench] Update workbench buildscripts to 1.11.0 [#2465](https://github.com/particle-iot/device-os/pull/2465)
- [workbench] Stop ignoring device os scripts directory [#2471](https://github.com/particle-iot/device-os/pull/2471)
- [workbench] Remove redundant toolchain overrides [#2489](https://github.com/particle-iot/device-os/pull/2489)
- [wifi] Add stubs to allow setting common country code and channel plan in P2 [#2473](https://github.com/particle-iot/device-os/pull/2473) [#2485](https://github.com/particle-iot/device-os/pull/2485)
- [tests] Change PWM pins to better match e-series pinout for E SoM X. Fix other tests [#2463](https://github.com/particle-iot/device-os/pull/2463)
- [tests] Misc changes to tests from 5.x [#2466](https://github.com/particle-iot/device-os/pull/2466)
- [tests][esomx] disable RGB pwm channel(s) when running pwm tests [#2494](https://github.com/particle-iot/device-os/pull/2494)
- Streamline `.bundleignore` between 5.x and 4.x codebases [#2496](https://github.com/particle-iot/device-os/pull/2496)
- branch pattern changes for `test-build-system` [#2510](https://github.com/particle-iot/device-os/pull/2510)
- [test] fixes race condition with `wiring/no_fixture` `TIME_17` [#2514](https://github.com/particle-iot/device-os/pull/2514)

## 4.0.0-beta.1

> ## :warning: Please note this is an internal release, do not use on production devices!

### BUGFIXES
- [gen3] port newlib stdin/stdout/stderr memory leak workaround [#2467](https://github.com/particle-iot/device-os/pull/2467)
- [esomx] map ADC channels to the correct pins [#2495](https://github.com/particle-iot/device-os/pull/2495)

### INTERNAL
- Allow overriding the platform ID on the GCC platform [#2462](https://github.com/particle-iot/device-os/pull/2462)
- Enable OTA updates on the GCC platform [#2464](https://github.com/particle-iot/device-os/pull/2464)
- Fix GCC platform on some versions of GCC; Fix building of wiring/api tests on CI [#2483](https://github.com/particle-iot/device-os/pull/2483)
- [workbench] Update workbench buildscripts to 1.11.0 [#2465](https://github.com/particle-iot/device-os/pull/2465)
- [workbench] Stop ignoring device os scripts directory [#2471](https://github.com/particle-iot/device-os/pull/2471)
- [workbench] Remove redundant toolchain overrides [#2489](https://github.com/particle-iot/device-os/pull/2489)
- [wifi] Add stubs to allow setting common country code and channel plan in P2 [#2473](https://github.com/particle-iot/device-os/pull/2473) [#2485](https://github.com/particle-iot/device-os/pull/2485)
- [tests] Change PWM pins to better match e-series pinout for E SoM X. Fix other tests [#2463](https://github.com/particle-iot/device-os/pull/2463)
- [tests] Misc changes to tests from 5.x [#2466](https://github.com/particle-iot/device-os/pull/2466)
- [tests][esomx] disable RGB pwm channel(s) when running pwm tests [#2494](https://github.com/particle-iot/device-os/pull/2494)

## 4.0.0-alpha.2

> ## :warning: Please note this is an internal release, do not use on production devices!

### FEATURES
- [esomx] Adds support for new platform esomx [#2443](https://github.com/particle-iot/device-os/pull/2443) [#2459](https://github.com/particle-iot/device-os/pull/2459)
- [E404X] Determine flash part at runtime [#2456](https://github.com/particle-iot/device-os/pull/2456)
- [gen3][quectel] Adds support for BG95-M1, BG95-MF, BG77, and EG91-NAX [#2458](https://github.com/particle-iot/device-os/pull/2458)

### BUGFIXES
- [gen3] hal: fix power leak on Boron [#2452](https://github.com/particle-iot/device-os/pull/2452)

### INTERNAL

- [workbench] update-device-os-workbench-manifest-json [#2457](https://github.com/particle-iot/device-os/pull/2457)
- [ci] test-build-system-tune-timeouts [#2455](https://github.com/particle-iot/device-os/pull/2455)

## 4.0.0-alpha.1

> ## :warning: Please note this is an internal release, do not use on production devices!

### DEPRECATION

- [deprecation][gen2] supply secure [#2442](https://github.com/particle-iot/device-os/pull/2442)
- [deprecation] Adds warning to some deprecated API's that will be removed in Device OS 5.x [#2445](https://github.com/particle-iot/device-os/pull/2445)
- [deprecation] PRODUCT_ID macro [#2446](https://github.com/particle-iot/device-os/pull/2446)
- [deprecation] remove setup_done flag and add deprecation notice [#2447](https://github.com/particle-iot/device-os/pull/2447)

### FEATURES

- [ota] new API System.updateStatus() [#2344](https://github.com/particle-iot/device-os/pull/2344)

### ENHANCEMENTS

- [gen3] wifi: add dhcp dns info to wifi config [#2440](https://github.com/particle-iot/device-os/pull/2440)
- [ota] Additional state for firmware update checks [#2344](https://github.com/particle-iot/device-os/pull/2344)
- Use a custom content type with CID packets when resuming the session [#2441](https://github.com/particle-iot/device-os/pull/2441)

### BUGFIXES

- [gen3] fixes hardfault during low level USB peripheral initialization under an atomic section [#2448](https://github.com/particle-iot/device-os/pull/2448)
- [gen3] Device unable to enter listening mode with button press [#2451](https://github.com/particle-iot/device-os/pull/2451)
- Return relevant error from control request to enter listening mode [#2419](https://github.com/particle-iot/device-os/pull/2419)


### INTERNAL

- [ci] minor update sc-101315/device-os-manifest [#2449](https://github.com/particle-iot/device-os/pull/2449)
- [ci] chore/ci-less-frequent-cross-platform-build-checks [#2434](https://github.com/particle-iot/device-os/pull/2434)
- [docs] Update dependencies for ARM GCC 10.2.1 [#2431](https://github.com/particle-iot/device-os/pull/2431)
- [gen3] suppress certain reviewed GCC warnings [sc-100940] [#2420](https://github.com/particle-iot/device-os/pull/2420)
- [ci] feature/sc-100324/ci-build-cross-platform [#2418](https://github.com/particle-iot/device-os/pull/2418)
- [test] Wi-Fi resolve test improvements [#2454](https://github.com/particle-iot/device-os/pull/2454)

## 3.3.1

### BUGFIXES

- [Boron / B SoM] R410 PPP crash in network phase workaround [#2571](https://github.com/particle-iot/device-os/pull/2571)
- [Cellular] R410 initialization SIM failure workaround [#2573](https://github.com/particle-iot/device-os/pull/2573)

## 3.3.0

### FEATURES

- [Gen3] BLE Provisioning Mode [#2382](https://github.com/particle-iot/device-os/pull/2382) [#2379](https://github.com/particle-iot/device-os/pull/2379) [#2405](https://github.com/particle-iot/device-os/pull/2405)
- System.off() API support for unsubscribing from system events/handlers [#2390](https://github.com/particle-iot/device-os/pull/2390)
- CoAP Blockwise transfer for Describe messages [#2377](https://github.com/particle-iot/device-os/pull/2377) [#2417](https://github.com/particle-iot/device-os/pull/2417)

### ENHANCEMENTS

- Enables stack overflow detection in all builds [#2392](https://github.com/particle-iot/device-os/pull/2392)
- Adds LOG_C_VARG() logging macro to support vargs from user apps [#2393](https://github.com/particle-iot/device-os/pull/2393)
- Adds os_thread_dump* introspection functions to wrap the FreeRTOS functions [#2394](https://github.com/particle-iot/device-os/pull/2394)
- [ci] Migration to CircleCI [#2395](https://github.com/particle-iot/device-os/pull/2395)
- Adds ability to override the panic handler in user applications [#2384](https://github.com/particle-iot/device-os/pull/2384)
- [gen3] SystemPowerConfiguration::socBitPrecision(uint8_t bits)) API added [#2401](https://github.com/particle-iot/device-os/pull/2401)

### BUGFIXES

- Thread/interrupt safety for system_event_t subscriptions [#2390](https://github.com/particle-iot/device-os/pull/2390)
- [gen3] BLE: introduce wiring APIs to change ATT MTU [#2398](https://github.com/particle-iot/device-os/pull/2398)
- [gen3] Dummy setup code should not return error [#2411](https://github.com/particle-iot/device-os/pull/2411)

### INTERNAL

- Removes WiFiTesting mode and Setup over Serial1 [#2386](https://github.com/particle-iot/device-os/pull/2386)
- [gen3] Add GCC feature 'build-id' into the system part1, monolithic and user apps [#2391](https://github.com/particle-iot/device-os/pull/2391) [#2400](https://github.com/particle-iot/device-os/pull/2400)
- Moves legacy makefile-based unit tests to CMake [#2396](https://github.com/particle-iot/device-os/pull/2396)
- Makes system_version.h compliant with C; Coalesce FreeRTOS task list internals into single location [#2399](https://github.com/particle-iot/device-os/pull/2399)
- Improves reliability of a few tests [#2406](https://github.com/particle-iot/device-os/pull/2406)
- [test] fixes integration slo/connect_time [#2412](https://github.com/particle-iot/device-os/pull/2412)
- [ci] fixes gcovr report generation [#2409](https://github.com/particle-iot/device-os/pull/2409)
- Updates C++ standard version to C++17 (gnu variant) [#2414](https://github.com/particle-iot/device-os/pull/2414)
- [test] improves wiring/no_fixture_long_running NETWORK_XX test timing [#2415](https://github.com/particle-iot/device-os/pull/2415)
- [test] improves wiring/no_fixture LISTENING & NETWORK test timing [#2416](https://github.com/particle-iot/device-os/pull/2416)

## 3.3.0-rc.1

### FEATURES

- [Gen3] BLE Provisioning Mode [#2382](https://github.com/particle-iot/device-os/pull/2382) [#2379](https://github.com/particle-iot/device-os/pull/2379) [#2405](https://github.com/particle-iot/device-os/pull/2405)
- System.off() API support for unsubscribing from system events/handlers [#2390](https://github.com/particle-iot/device-os/pull/2390)

### ENHANCEMENTS

- Enables stack overflow detection in all builds [#2392](https://github.com/particle-iot/device-os/pull/2392)
- Adds LOG_C_VARG() logging macro to support vargs from user apps [#2393](https://github.com/particle-iot/device-os/pull/2393)
- Adds os_thread_dump* introspection functions to wrap the FreeRTOS functions [#2394](https://github.com/particle-iot/device-os/pull/2394)
- [ci] Migration to CircleCI [#2395](https://github.com/particle-iot/device-os/pull/2395)
- Adds ability to override the panic handler in user applications [#2384](https://github.com/particle-iot/device-os/pull/2384)
- [gen3] SystemPowerConfiguration::socBitPrecision(uint8_t bits)) API added [#2401](https://github.com/particle-iot/device-os/pull/2401)

### BUGFIXES

- Thread/interrupt safety for system_event_t subscriptions [#2390](https://github.com/particle-iot/device-os/pull/2390)

### INTERNAL

- Removes WiFiTesting mode and Setup over Serial1 [#2386](https://github.com/particle-iot/device-os/pull/2386)
- [gen3] Add GCC feature 'build-id' into the system part1, monolithic and user apps [#2391](https://github.com/particle-iot/device-os/pull/2391) [#2400](https://github.com/particle-iot/device-os/pull/2400)
- Moves legacy makefile-based unit tests to CMake [#2396](https://github.com/particle-iot/device-os/pull/2396)
- Makes system_version.h compliant with C; Coalesce FreeRTOS task list internals into single location [#2399](https://github.com/particle-iot/device-os/pull/2399)
- Improves reliability of a few tests [#2406](https://github.com/particle-iot/device-os/pull/2406)

## 3.2.1-p2.4

### BUGFIXES
- Fix overlapping OTA/system-part1/user app sections [#385](https://github.com/particle-iot/firmware-private/pull/385)
- Fix exflash_hal for bootloader usage; remove reentrant libc functions from ROM imports to avoid interference with newlib [#383](https://github.com/particle-iot/firmware-private/pull/383)
- Connecting to unsecured wifi would cause bus fault [#377](https://github.com/particle-iot/firmware-private/pull/377)
- [test] minor fix for P2 [#375](https://github.com/particle-iot/firmware-private/pull/375)
- [rtl872x] hal: i2c thread safe [#361](https://github.com/particle-iot/firmware-private/pull/361)

### ENHANCEMENTS
- [rtl872x] hal: support accessing the OTP built in the external flash [#379](https://github.com/particle-iot/firmware-private/pull/379)
- [rtl872x] hal: add ADC calibration. [#380](https://github.com/particle-iot/firmware-private/pull/380)
- [rtl872x] hal: supports Serial3. [#362](https://github.com/particle-iot/firmware-private/pull/362)

### INTERNAL
- Add new openocd version [#366](https://github.com/particle-iot/firmware-private/pull/366)

## 3.2.0

### BUGFIXES

- Fix hardfault handler to SOS again [#2381](https://github.com/particle-iot/device-os/pull/2381)
- [Gen 3] Fix LittleFS truncate causing file corruption [#268](https://github.com/littlefs-project/littlefs/issues/268) [#2385](https://github.com/particle-iot/device-os/pull/2385)
- [Gen 3] HAL - Only skip modules that fail integrity check from module info [#2387](https://github.com/particle-iot/device-os/pull/2387)

### INTERNAL

- Fix compilation errors on recent Linux distros with GCC 11 and updated system header files [#2383](https://github.com/particle-iot/device-os/pull/2383)

## 3.2.0-rc.1

### FEATURES

- [wiring] `acquireSerialXBuffer()` API support for UART [#2375](https://github.com/particle-iot/device-os/pull/2375)
- [Boron / B SoM ] Support for SARA R510 [#2359](https://github.com/particle-iot/device-os/pull/2359) [#2365](https://github.com/particle-iot/device-os/pull/2365)
- [Electron] Optional feature to use HSE/LSI as RTC clock source instead of LSE (external 32KHz XTAL) [#2354](https://github.com/particle-iot/device-os/pull/2354)
- [Gen 3] BLE: Add ability to set and scan extended advertisement size (when using Coded PHY) [#2331](https://github.com/particle-iot/device-os/pull/2331)

### ENHANCEMENTS

- Enable GCC `-Wextra` when building Device OS to enable additional diagnostics provided by GCC. Fix issues uncovered [#2340](https://github.com/particle-iot/device-os/pull/2340)
- Refactor system describe/info JSON generation to reduce size and remove invalid modules from it [#2347](https://github.com/particle-iot/device-os/pull/2347) [#2349](https://github.com/particle-iot/device-os/pull/2349)
- [Boron / B SoM / R510] Add additional modem responsiveness check on warm boot to avoid triggering R510-specific initialization issue [#2373](https://github.com/particle-iot/device-os/pull/2373)

### BUGFIXES

- [Gen 3] BLE: fix unintialized `scan_phys` in default scan parameters, preventing scanning from working unless `BLE.setScanPhy()` is manually set [#2338](https://github.com/particle-iot/device-os/pull/2338) [#2345](https://github.com/particle-iot/device-os/pull/2345)
- [Argon / Tracker] Avoid power leakage through ESP32 `ESPBOOT` pin [#2342](https://github.com/particle-iot/device-os/pull/2342)
- Fix parsing of JSON strings with more than 127 tokens [#2348](https://github.com/particle-iot/device-os/pull/2348)
- Clear module slots in DCT when preparing for an OTA update [#2346](https://github.com/particle-iot/device-os/pull/2346)
- [Gen 3] Increase BLE operation timeout to ensure that BLE events are correctly handled and do not trigger an assertion [#2371](https://github.com/particle-iot/device-os/pull/2371)
- [Argon] Fix occasional WiFI setup issue over BLE [#2372](https://github.com/particle-iot/device-os/pull/2372)
- [Gen 3] Ensure that invalid modules are not presented to the cloud in System Describe message [#2374](https://github.com/particle-iot/device-os/pull/2374)
- [Gen 3] BLE: Fix copy-constructor and assignment operator issues in wiring APIs [#2376](https://github.com/particle-iot/device-os/pull/2376)

### INTERNAL

- [Photon / P1] system part 2 size optimizations [#2349](https://github.com/particle-iot/device-os/pull/2349)
- Fix `gcovr` installation on CI [#2361](https://github.com/particle-iot/device-os/pull/2361) [#2364](https://github.com/particle-iot/device-os/pull/2364)
- [test] Increase cloud connection timeout to 9 min for on-device tests where applicable [#2369](https://github.com/particle-iot/device-os/pull/2369)
- [test] Enable `wiring/ble_central_peripheral` and `wiring/ble_scanner_broadcaster` tests to be run under `device-os-test-runner` [#2376](https://github.com/particle-iot/device-os/pull/2376)

## 3.1.0

### FEATURES

- [Gen 3] 256KB application support [#2322](https://github.com/particle-iot/device-os/pull/2322)
- Support for DTLS connection IDs [#2249](https://github.com/particle-iot/device-os/pull/2249)
- GCC 10 support [#2288](https://github.com/particle-iot/device-os/pull/2288)
- [Gen 3] BLE LESC support [#2262](https://github.com/particle-iot/device-os/pull/2262)
- [Gen 3] BLE 5 PHY_CODED (long range) scanning and advertising support [#2287](https://github.com/particle-iot/device-os/pull/2287) [#2298](https://github.com/particle-iot/device-os/pull/2298) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Tracker] AB1805 native watchdog support [#2316](https://github.com/particle-iot/device-os/pull/2316)
- [Boron / B SoM / Electron] Support for SARA R410 05.12 modem firmware [#2317](https://github.com/particle-iot/device-os/pull/2317) [#2319](https://github.com/particle-iot/device-os/pull/2319) [#2318](https://github.com/particle-iot/device-os/pull/2318)
- Add an API to get the maximum supported size of event data [#2315](https://github.com/particle-iot/device-os/pull/2315)

### ENHANCEMENTS

- [Argon] Cache ESP32 MAC address in persistent storage to improve boot-up times [#2327](https://github.com/particle-iot/device-os/pull/2327)
- [Cellular] Inhibit Cellular URCs before going into sleep to prevent them from triggering wake-up [#2295](https://github.com/particle-iot/device-os/pull/2295) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Gen 3] Remove XIP support for accessing the external flash [#2302](https://github.com/particle-iot/device-os/pull/2302) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Photon / P1] Enable LTO [#2288](https://github.com/particle-iot/device-os/pull/2288)
- Trigger a compiler error when a function returning a value does not do so [#2323](https://github.com/particle-iot/device-os/pull/2323)
- [Gen 3] Fix non-MBR-based bootloader updates [#2327](https://github.com/particle-iot/device-os/pull/2327)

### BUGFIXES

- Do not reset the DTLS session on socket errors [#2335](https://github.com/particle-iot/device-os/pull/2335) [#2337](https://github.com/particle-iot/device-os/pull/2337)
- [Cellular] Prevent ICCID querying errors when not connected to a cellular network by using airplane mode `CFUN=4` [#2328](https://github.com/particle-iot/device-os/pull/2328)
- [Photon / P1] Make sure to close all sockets when deinitializing WICED WLAN connectivity subsystem [#2313](https://github.com/particle-iot/device-os/pull/2313) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Gen 2] Fix unexpected network connection establishment after exiting sleep mode when only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- [Gen 2] Fix unexpected network connection establishment when the modem or WiFi initialization failes, but only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- Correctly restore network interface power state after waking up from `STOP` or `ULTRA_LOW_POWER` sleep modes [#2308](https://github.com/particle-iot/device-os/pull/2308)
- Fix the issue that the Particle.disconnect() doesn't clear the auto-connect flag [#2306](https://github.com/particle-iot/device-os/pull/2306)
- [Electron] Fix unintended modem reset after an ongoing network registartion attempt is actively cancelled by the application [#2307](https://github.com/particle-iot/device-os/pull/2307)
- Fix `Particle.unsubscribe()` not preserving system subscriptions [#2293](https://github.com/particle-iot/device-os/pull/2293)
- Querying the value of an empty string variable causes an error [#2297](https://github.com/particle-iot/device-os/pull/2297)

### INTERNAL

- [ci] Remove build directory after finishing the build job [#2311](https://github.com/particle-iot/device-os/pull/2311)
- [ci] Fix MarkupSafe weirdness [#2314](https://github.com/particle-iot/device-os/pull/2314)
- Add an integration test to validate network/cloud connection time SLOs [#2312](https://github.com/particle-iot/device-os/pull/2312) [#2320](https://github.com/particle-iot/device-os/pull/2320) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- Allow clearing session data and running custom setup code in integration tests [#2280](https://github.com/particle-iot/device-os/pull/2280)
- Add `.bundleignore` for Workbench Device OS source code bundles [#2326](https://github.com/particle-iot/device-os/pull/2326)
- Manage GCC dependencies with `.workbench/manifest.json` [d94f08030](https://github.com/particle-iot/device-os/commit/d94f0803068026d0b2aa0af426ba80c8b62299c7)

## 3.1.0-rc.1

### FEATURES

- [Gen 3] 256KB application support [#2322](https://github.com/particle-iot/device-os/pull/2322)
- Support for DTLS connection IDs [#2249](https://github.com/particle-iot/device-os/pull/2249)
- GCC 10 support [#2288](https://github.com/particle-iot/device-os/pull/2288)
- [Gen 3] BLE LESC support [#2262](https://github.com/particle-iot/device-os/pull/2262)
- [Gen 3] BLE 5 PHY_CODED (long range) scanning and advertising support [#2287](https://github.com/particle-iot/device-os/pull/2287) [#2298](https://github.com/particle-iot/device-os/pull/2298) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Tracker] AB1805 native watchdog support [#2316](https://github.com/particle-iot/device-os/pull/2316)
- [Boron / B SoM / Electron] Support for SARA R410 05.12 modem firmware [#2317](https://github.com/particle-iot/device-os/pull/2317) [#2319](https://github.com/particle-iot/device-os/pull/2319) [#2318](https://github.com/particle-iot/device-os/pull/2318)
- Add an API to get the maximum supported size of event data [#2315](https://github.com/particle-iot/device-os/pull/2315)

### ENHANCEMENTS

- [Argon] Cache ESP32 MAC address in persistent storage to improve boot-up times [#2327](https://github.com/particle-iot/device-os/pull/2327)
- [Cellular] Inhibit Cellular URCs before going into sleep to prevent them from triggering wake-up [#2295](https://github.com/particle-iot/device-os/pull/2295) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Gen 3] Remove XIP support for accessing the external flash [#2302](https://github.com/particle-iot/device-os/pull/2302) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Photon / P1] Enable LTO [#2288](https://github.com/particle-iot/device-os/pull/2288)
- Trigger a compiler error when a function returning a value does not do so [#2323](https://github.com/particle-iot/device-os/pull/2323)
- [Gen 3] Fix non-MBR-based bootloader updates [#2327](https://github.com/particle-iot/device-os/pull/2327)

### BUGFIXES

- [Cellular] Prevent ICCID querying errors when not connected to a cellular network by using airplane mode `CFUN=4` [#2328](https://github.com/particle-iot/device-os/pull/2328)
- [Photon / P1] Make sure to close all sockets when deinitializing WICED WLAN connectivity subsystem [#2313](https://github.com/particle-iot/device-os/pull/2313) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [Gen 2] Fix unexpected network connection establishment after exiting sleep mode when only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- [Gen 2] Fix unexpected network connection establishment when the modem or WiFi initialization failes, but only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- Correctly restore network interface power state after waking up from `STOP` or `ULTRA_LOW_POWER` sleep modes [#2308](https://github.com/particle-iot/device-os/pull/2308)
- Fix the issue that the Particle.disconnect() doesn't clear the auto-connect flag [#2306](https://github.com/particle-iot/device-os/pull/2306)
- [Electron] Fix unintended modem reset after an ongoing network registartion attempt is actively cancelled by the application [#2307](https://github.com/particle-iot/device-os/pull/2307)
- Fix `Particle.unsubscribe()` not preserving system subscriptions [#2293](https://github.com/particle-iot/device-os/pull/2293)
- Querying the value of an empty string variable causes an error [#2297](https://github.com/particle-iot/device-os/pull/2297)

### INTERNAL

- [ci] Remove build directory after finishing the build job [#2311](https://github.com/particle-iot/device-os/pull/2311)
- [ci] Fix MarkupSafe weirdness [#2314](https://github.com/particle-iot/device-os/pull/2314)
- Add an integration test to validate network/cloud connection time SLOs [#2312](https://github.com/particle-iot/device-os/pull/2312) [#2320](https://github.com/particle-iot/device-os/pull/2320) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- Allow clearing session data and running custom setup code in integration tests [#2280](https://github.com/particle-iot/device-os/pull/2280)
- Add `.bundleignore` for Workbench Device OS source code bundles [#2326](https://github.com/particle-iot/device-os/pull/2326)
- Manage GCC dependencies with `.workbench/manifest.json` [d94f08030](https://github.com/particle-iot/device-os/commit/d94f0803068026d0b2aa0af426ba80c8b62299c7)

## 3.0.0

### BREAKING CHANGES

- [Cellular] Remove `rssi` and `qual` from `Cellular.RSSI()` [#2212](https://github.com/particle-iot/device-os/pull/2212)
- [Gen 3] BLE API consistency enhancements [#2222](https://github.com/particle-iot/device-os/pull/2222)

### FEATURES

- [Electron] Proactively attempt to recover from a number of failed cellular registration states [#2301](https://github.com/particle-iot/device-os/pull/2301)
- [Cellular] Battery presence detection when charging is disabled [#2272](https://github.com/particle-iot/device-os/pull/2272)
- Increase the maximum DTLS packet size and payload of the cloud primitives [#2260](https://github.com/particle-iot/device-os/pull/2260)
- [Cellular] Send modem firmware version to the cloud as part of the system describe message [#2265](https://github.com/particle-iot/device-os/pull/2265)
- [Gen 3] OTAv3 protocol [#2199](https://github.com/particle-iot/device-os/pull/2199)
- [Tracker] ESP32 WiFi scanning support [#2250](https://github.com/particle-iot/device-os/pull/2250)
- [Cellular] `SystemPowerFeature::DISABLE_CHARGING` configuration option to enable or disable charging [#2257](https://github.com/particle-iot/device-os/pull/2257)
- `Network.isOn()` and `Network.isOff()` APIs to query the network interface power state [#2205](https://github.com/particle-iot/device-os/pull/2205)
- [Gen 3] BLE legacy pairing [#2237](https://github.com/particle-iot/device-os/pull/2237)
- [Cellular] Query cellular signal while trying to register on a network [#2232](https://github.com/particle-iot/device-os/pull/2232)
- [Tracker] WiFi/GNSS/FuelGauge sleep wake-up sources [#2200](https://github.com/particle-iot/device-os/pull/2200)
- Configure multiple pins as wakeup source at a time [#2228](https://github.com/particle-iot/device-os/pull/2228) [#2231](https://github.com/particle-iot/device-os/pull/2231)

### ENHANCEMENTS

- [Argon] Reduce cloud keep-alive timeout to 25 seconds from 30 seconds [#2304](https://github.com/particle-iot/device-os/pull/2304)
- Improve I2C reset procedure to be less destructive and issue STOP condition as soon as possible [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Cellular] Perform PMIC/FuelGauge/RTC I2C bus reset on boot to avoid accidental writes after a non-graceful reset [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Cellular] System power management improvements [#2272](https://github.com/particle-iot/device-os/pull/2272) [#2290](https://github.com/particle-iot/device-os/pull/2290)
- [Cellular] Update ICCID/IMSI to APN map with a new Kore ICCID prefix [#2276](https://github.com/particle-iot/device-os/pull/2276)
- Disable some of the elliptic-curves not in use to save flash space [#2273](https://github.com/particle-iot/device-os/pull/2273)
- [Cellular] Update LTE signal strength/quality parameters (RSRP/RSRQ) mapping to percentages [#2285](https://github.com/particle-iot/device-os/pull/2285)
- [B5 SoM / Quectel] Improve warm and cold boot behavior [#2300](https://github.com/particle-iot/device-os/pull/2300)
- [Gen 3] Custom logging categories for AT parser and GSM 07.10 multiplexer to differentiate between cellular modem and ESP32 on Tracker platforms [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Tracker] Reduce code size of GSM 07.10 multiplexer implementation making sure that ESP32 and cellular NCP client use the same template variant of it [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Electron] Speed up modem power-on [#2268](https://github.com/particle-iot/device-os/pull/2268)
- [Tracker] Cache ESP32 NCP firmware version in non-volatile memory [#2269](https://github.com/particle-iot/device-os/pull/2269)
- [Gen 3] Network interface management improvements [#2217](https://github.com/particle-iot/device-os/pull/2217)
- [Gen 3] `SPI.transfer()` support for constant buffers residing in flash [#2196](https://github.com/particle-iot/device-os/pull/2196)
- [Gen 3] Add characteristic discovery to `BleService` [#2203](https://github.com/particle-iot/device-os/pull/2203)
- [Gen 3] BLE Scanned/Connected/Disconnected/Data Received callbacks in C++ style [#2224](https://github.com/particle-iot/device-os/pull/2224)
- [Gen 3] BLE scanning filter [#2223](https://github.com/particle-iot/device-os/pull/2223)
- [Electron] Build system parts with LTO enabled [#2235](https://github.com/particle-iot/device-os/pull/2235)
- Add more operators for `BleAddress`, `BleUuid` and `IPAddress`[#2216](https://github.com/particle-iot/device-os/pull/2216)
- Upate MbedTLS to 2.22.0 [#2117](https://github.com/particle-iot/device-os/pull/2117)
- [Tracker] ESP32 NCP firmware updated to version 0.0.7
- Use `PARTICLE_` prefix for LED defines in order not to pollute global namespace [#2247](https://github.com/particle-iot/device-os/pull/2247)
- [Gen 3] Ethernet FeatherWing power state management [#2258](https://github.com/particle-iot/device-os/pull/2258)
- [Cellular] Changes how signal strength and quality percentages are calculated to provide a more accurate representation of signal conditions [#2236](https://github.com/particle-iot/device-os/pull/2236)

### BUGFIXES

- [Gen 3] Add workaround for Nordic nRF52840 anomaly 219 (TWIM: I2C timing spec is violated at 400 kHz) [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Gen 3] Fix micros/millis/unixtime becoming non-monotonic [2a4fcb82b](https://github.com/particle-iot/device-os/commit/2a4fcb82b0968300b8a0227f665ffe94203f9f38) [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Gen 3] Use `PIN_INVALID` when initializing SPI peripheral to avoid overriding the pin mode of the default CS pin on reinitialization [#2275](https://github.com/particle-iot/device-os/pull/2275)
- [Argon / Tracker] Make sure that ESP32 NCP power state is correctly initialized on boot [#2279](https://github.com/particle-iot/device-os/pull/2279)
- [Electron] Increase `AT+COPS` timeout to 5 minutes [#2281](https://github.com/particle-iot/device-os/pull/2281)
- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
- [B5 SoM / Tracker] Fix warm boot sometimes requiring modem reset [#2289](https://github.com/particle-iot/device-os/pull/2289)
- [Boron / B SoM] Fix external SIM getting stuck in initialization [#2263](https://github.com/particle-iot/device-os/pull/2263)
- [BLE] Return `false` in `BlePeerDevice::getCharacteristicByDescription()` if expected characteristic was not found [#2266](https://github.com/particle-iot/device-os/pull/2266)
- [Gen 3] Fix UART DMA RX transfer size issues causing DMA writes outside of the RX buffer [#2264](https://github.com/particle-iot/device-os/pull/2264)
- [Gen 3] Fix `ChannelStream::waitEvent()` timeout calculation [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Gen 3] Fix warm boot feature regression introduced in 3.0.0-beta.1 [#2269](https://github.com/particle-iot/device-os/pull/2269)
- [Gen 3] Disconnect from the server on OTAv3 update errors [#2270](https://github.com/particle-iot/device-os/pull/2270)
- [Gen 2] Fix D0 alternate-function being unconditionally reset when calling `Serial1.end()` [#2256](https://github.com/particle-iot/device-os/pull/2256)
- [Gen 3] Fix an issue with `BLE.scan()` deadlocking [#2220](https://github.com/particle-iot/device-os/pull/2220)

### INTERNAL

- Startup SLO automated tests [#2277](https://github.com/particle-iot/device-os/pull/2277) [#2274](https://github.com/particle-iot/device-os/pull/2274)

## 3.0.0-rc.2

### FEATURES

- [Cellular] Battery presence detection when charging is disabled [#2272](https://github.com/particle-iot/device-os/pull/2272)
- Increase the maximum DTLS packet size and payload of the cloud primitives [#2260](https://github.com/particle-iot/device-os/pull/2260)

### ENHANCEMENTS

- [Cellular] Update ICCID/IMSI to APN map with a new Kore ICCID prefix [#2276](https://github.com/particle-iot/device-os/pull/2276)
- Disable some of the elliptic-curves not in use to save flash space [#2273](https://github.com/particle-iot/device-os/pull/2273)
- [Cellular] Update LTE signal strength/quality parameters (RSRP/RSRQ) mapping to percentages [#2285](https://github.com/particle-iot/device-os/pull/2285)
- [Cellular] System power management improvements [#2272](https://github.com/particle-iot/device-os/pull/2272)
- [B5 SoM / Quectel] Improve warm and cold boot behavior [#2300](https://github.com/particle-iot/device-os/pull/2300)

### BUGFIXES

- [Gen 3] Use `PIN_INVALID` when initializing SPI peripheral to avoid overriding the pin mode of the default CS pin on reinitialization [#2275](https://github.com/particle-iot/device-os/pull/2275)
- [Argon / Tracker] Make sure that ESP32 NCP power state is correctly initialized on boot [#2279](https://github.com/particle-iot/device-os/pull/2279)
- [Electron] Increase `AT+COPS` timeout to 5 minutes [#2281](https://github.com/particle-iot/device-os/pull/2281)
- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
- [B5 SoM / Tracker] Fix warm boot sometimes requiring modem reset [#2289](https://github.com/particle-iot/device-os/pull/2289)
- [Gen 3] Fix micros/millis/unixtime becoming non-monotonic when RTC overflow event occurs [2a4fcb82b](https://github.com/particle-iot/device-os/commit/2a4fcb82b0968300b8a0227f665ffe94203f9f38)

### INTERNAL

- Startup SLO automated tests [#2277](https://github.com/particle-iot/device-os/pull/2277) [#2274](https://github.com/particle-iot/device-os/pull/2274)

## 3.0.0-rc.1

### FEATURES

- [Cellular] Send modem firmware version to the cloud as part of the system describe message [#2265](https://github.com/particle-iot/device-os/pull/2265)

### ENHANCEMENTS

- [Gen 3] Custom logging categories for AT parser and GSM 07.10 multiplexer to differentiate between cellular modem and ESP32 on Tracker platforms [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Tracker] Reduce code size of GSM 07.10 multiplexer implementation making sure that ESP32 and cellular NCP client use the same template variant of it [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Electron] Speed up modem power-on [#2268](https://github.com/particle-iot/device-os/pull/2268)
- [Tracker] Cache ESP32 NCP firmware version in non-volatile memory [#2269](https://github.com/particle-iot/device-os/pull/2269)

### BUGFIXES

- [Boron / B SoM] Fix external SIM getting stuck in initialization [#2263](https://github.com/particle-iot/device-os/pull/2263)
- [BLE] Return `false` in `BlePeerDevice::getCharacteristicByDescription()` if expected characteristic was not found [#2266](https://github.com/particle-iot/device-os/pull/2266)
- [Gen 3] Fix UART DMA RX transfer size issues causing DMA writes outside of the RX buffer [#2264](https://github.com/particle-iot/device-os/pull/2264)
- [Gen 3] Fix `ChannelStream::waitEvent()` timeout calculation [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Gen 3] Fix warm boot feature regression introduced in 3.0.0-beta.1 [#2269](https://github.com/particle-iot/device-os/pull/2269)
- [Gen 3] Disconnect from the server on OTAv3 update errors [#2270](https://github.com/particle-iot/device-os/pull/2270)

## 3.0.0-beta.1

### BREAKING CHANGES

- [Cellular] Remove `rssi` and `qual` from `Cellular.RSSI()` [#2212](https://github.com/particle-iot/device-os/pull/2212)

### FEATURES

- [Gen 3] OTAv3 protocol [#2199](https://github.com/particle-iot/device-os/pull/2199)
- [Tracker] ESP32 WiFi scanning support [#2250](https://github.com/particle-iot/device-os/pull/2250)
- [Cellular] `SystemPowerFeature::DISABLE_CHARGING` configuration option to enable or disable charging [#2257](https://github.com/particle-iot/device-os/pull/2257)
- `Network.isOn()` and `Network.isOff()` APIs to query the network interface power state [#2205](https://github.com/particle-iot/device-os/pull/2205)
- [Gen 3] BLE legacy pairing [#2237](https://github.com/particle-iot/device-os/pull/2237)
- [Cellular] Query cellular signal while trying to register on a network [#2232](https://github.com/particle-iot/device-os/pull/2232)
- [Tracker] WiFi/GNSS/FuelGauge sleep wake-up sources [#2200](https://github.com/particle-iot/device-os/pull/2200)
- Configure multiple pins as wakeup source at a time [#2228](https://github.com/particle-iot/device-os/pull/2228) [#2231](https://github.com/particle-iot/device-os/pull/2231)

### ENHANCEMENTS

- [Gen 3] Network interface management improvements [#2217](https://github.com/particle-iot/device-os/pull/2217)
- [Gen 3] `SPI.transfer()` support for constant buffers residing in flash [#2196](https://github.com/particle-iot/device-os/pull/2196)
- [Gen 3] Add characteristic discovery to `BleService` [#2203](https://github.com/particle-iot/device-os/pull/2203)
- [Gen 3] BLE Scanned/Connected/Disconnected/Data Received callbacks in C++ style [#2224](https://github.com/particle-iot/device-os/pull/2224)
- [Gen 3] BLE scanning filter [#2223](https://github.com/particle-iot/device-os/pull/2223)
- [Gen 3] BLE API consistency enhancements [#2222](https://github.com/particle-iot/device-os/pull/2222)
- [Electron] Build system parts with LTO enabled [#2235](https://github.com/particle-iot/device-os/pull/2235)
- Add more operators for `BleAddress`, `BleUuid` and `IPAddress`[#2216](https://github.com/particle-iot/device-os/pull/2216)
- Upate MbedTLS to 2.22.0 [#2117](https://github.com/particle-iot/device-os/pull/2117)
- [Tracker] ESP32 NCP firmware updated to version 0.0.7
- Use `PARTICLE_` prefix for LED defines in order not to pollute global namespace [#2247](https://github.com/particle-iot/device-os/pull/2247)
- [Gen 3] Ethernet FeatherWing power state management [#2258](https://github.com/particle-iot/device-os/pull/2258)
- [Cellular] Changes how signal strength and quality percentages are calculated to provide a more accurate representation of signal conditions [#2236](https://github.com/particle-iot/device-os/pull/2236)

### BUGFIXES

- [Gen 2] Fix D0 alternate-function being unconditionally reset when calling `Serial1.end()` [#2256](https://github.com/particle-iot/device-os/pull/2256)
- [Gen 3] Fix an issue with `BLE.scan()` deadlocking [#2220](https://github.com/particle-iot/device-os/pull/2220)

## 2.3.1

### BUGFIXES

- [Cellular] R410 initialization SIM failure workaround [#2573](https://github.com/particle-iot/device-os/pull/2573)
- [Boron / B SoM] R410 PPP crash in network phase workaround [#2571](https://github.com/particle-iot/device-os/pull/2571)

## 2.3.0

### FEATURES

- [Boron / B SoM ] Support for SARA R510 [#2359](https://github.com/particle-iot/device-os/pull/2359) [#2365](https://github.com/particle-iot/device-os/pull/2365)
- [Electron] Optional feature to use HSE/LSI as RTC clock source instead of LSE (external 32KHz XTAL) [#2354](https://github.com/particle-iot/device-os/pull/2354)

### ENHANCEMENTS

- [Boron / B SoM / R510] Add additional modem responsiveness check on warm boot to avoid triggering R510-specific initialization issue [#2373](https://github.com/particle-iot/device-os/pull/2373)

### INTERNAL

- Fix gcov installation [#2361](https://github.com/particle-iot/device-os/pull/2361)

## 2.3.0-rc.1

### FEATURES

- [Boron / B SoM ] Support for SARA R510 [#2359](https://github.com/particle-iot/device-os/pull/2359) [#2365](https://github.com/particle-iot/device-os/pull/2365)
- [Electron] Optional feature to use HSE/LSI as RTC clock source instead of LSE (external 32KHz XTAL) [#2354](https://github.com/particle-iot/device-os/pull/2354)

### INTERNAL

- Fix gcov installation [#2361](https://github.com/particle-iot/device-os/pull/2361)

## 2.2.0

### FEATURES

- [Boron / B SoM / Electron] Support for SARA R410 05.12 modem firmware [#2317](https://github.com/particle-iot/device-os/pull/2317) [#2319](https://github.com/particle-iot/device-os/pull/2319) [#2318](https://github.com/particle-iot/device-os/pull/2318)

### ENHANCEMENTS

- [Gen 3] Remove XIP support for accessing the external flash [#2302](https://github.com/particle-iot/device-os/pull/2302) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- Trigger a compiler error when a function returning a value does not do so [#2323](https://github.com/particle-iot/device-os/pull/2323)
- [Argon] Cache ESP32 MAC address in persistent storage to improve boot-up times [#2327](https://github.com/particle-iot/device-os/pull/2327)
- [Cellular] Add `CellularSignal::isValid()` and `CellularSignal::operator bool()` APIs [#2212](https://github.com/particle-iot/device-os/pull/2212)
- Refactor system describe/info JSON generation to reduce size and remove invalid modules from it [#2347](https://github.com/particle-iot/device-os/pull/2347) [#2349](https://github.com/particle-iot/device-os/pull/2349)

### BUGFIXES

- [Gen 2] Fix unexpected network connection establishment after exiting sleep mode when only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- [Gen 2] Fix unexpected network connection establishment when the modem or WiFi initialization failes, but only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- [Photon / P1] Make sure to close all sockets when deinitializing WICED WLAN connectivity subsystem [#2313](https://github.com/particle-iot/device-os/pull/2313) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [B5 SoM / Tracker] Fixes external flash DFU definition on in bootloader to use 4KB sectors [399b8a0](https://github.com/particle-iot/device-os/pull/2321/commits/399b8a085898101adcf396ef03ef7bb9bbbf479c) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- Fix non-MBR-based bootloader updates [#2327]((https://github.com/particle-iot/device-os/pull/2327))
- Clear module slots in DCT when preparing for an OTA update [#2346](https://github.com/particle-iot/device-os/pull/2346)
- Do not reset the DTLS session on socket errors [#2335](https://github.com/particle-iot/device-os/pull/2335) [#2337](https://github.com/particle-iot/device-os/pull/2337)
- [Argon / Tracker] Avoid power leakage through ESP32 `ESPBOOT` pin [#2342](https://github.com/particle-iot/device-os/pull/2342)
- Fix parsing of JSON strings with more than 127 tokens [#2348](https://github.com/particle-iot/device-os/pull/2348)

### INTERNAL

- Add an integration test to validate network/cloud connection time SLOs [#2312](https://github.com/particle-iot/device-os/pull/2312) [#2320](https://github.com/particle-iot/device-os/pull/2320) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [ci] Fix MarkupSafe weirdness [#2317](https://github.com/particle-iot/device-os/pull/2317)
- Add `.bundleignore` for Workbench Device OS source code bundles [#2326](https://github.com/particle-iot/device-os/pull/2326)
- Manage GCC dependencies with `.workbench/manifest.json` [d94f08030](https://github.com/particle-iot/device-os/commit/d94f0803068026d0b2aa0af426ba80c8b62299c7)
- [CI] Generate public Codefresh URL in Slack notifications [#2333](https://github.com/particle-iot/device-os/pull/2333)
- [Photon / P1] system part 2 size optimizations [#2349](https://github.com/particle-iot/device-os/pull/2349)
- [Electron] Increase `MBEDTLS_SSL_MAX_CONTENT_LEN` to 900 [#2349](https://github.com/particle-iot/device-os/pull/2349)


## 2.2.0-rc.2

### ENHANCEMENTS

- Refactor system describe/info JSON generation to reduce size and remove invalid modules from it [#2347](https://github.com/particle-iot/device-os/pull/2347) [#2349](https://github.com/particle-iot/device-os/pull/2349)

### BUGFIXES

- Clear module slots in DCT when preparing for an OTA update [#2346](https://github.com/particle-iot/device-os/pull/2346)
- Do not reset the DTLS session on socket errors [#2335](https://github.com/particle-iot/device-os/pull/2335) [#2337](https://github.com/particle-iot/device-os/pull/2337)
- [Argon / Tracker] Avoid power leakage through ESP32 `ESPBOOT` pin [#2342](https://github.com/particle-iot/device-os/pull/2342)
- Fix parsing of JSON strings with more than 127 tokens [#2348](https://github.com/particle-iot/device-os/pull/2348)

### INTERNAL

- [CI] Generate public Codefresh URL in Slack notifications [#2333](https://github.com/particle-iot/device-os/pull/2333)
- [Photon / P1] system part 2 size optimizations [#2349](https://github.com/particle-iot/device-os/pull/2349)
- [Electron] Increase `MBEDTLS_SSL_MAX_CONTENT_LEN` to 900 [#2349](https://github.com/particle-iot/device-os/pull/2349)

## 2.2.0-rc.1

### FEATURES

- [Boron / B SoM / Electron] Support for SARA R410 05.12 modem firmware [#2317](https://github.com/particle-iot/device-os/pull/2317) [#2319](https://github.com/particle-iot/device-os/pull/2319) [#2318](https://github.com/particle-iot/device-os/pull/2318)

### ENHANCEMENTS

- [Gen 3] Remove XIP support for accessing the external flash [#2302](https://github.com/particle-iot/device-os/pull/2302) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- Trigger a compiler error when a function returning a value does not do so [#2323](https://github.com/particle-iot/device-os/pull/2323)
- [Argon] Cache ESP32 MAC address in persistent storage to improve boot-up times [#2327](https://github.com/particle-iot/device-os/pull/2327)
- [Cellular] Add `CellularSignal::isValid()` and `CellularSignal::operator bool()` APIs [#2212](https://github.com/particle-iot/device-os/pull/2212)

### BUGFIXES

- [Gen 2] Fix unexpected network connection establishment after exiting sleep mode when only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- [Gen 2] Fix unexpected network connection establishment when the modem or WiFi initialization failes, but only `Network.on()` was called [#2309](https://github.com/particle-iot/device-os/pull/2309)
- [Photon / P1] Make sure to close all sockets when deinitializing WICED WLAN connectivity subsystem [#2313](https://github.com/particle-iot/device-os/pull/2313) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [B5 SoM / Tracker] Fixes external flash DFU definition on in bootloader to use 4KB sectors [399b8a0](https://github.com/particle-iot/device-os/pull/2321/commits/399b8a085898101adcf396ef03ef7bb9bbbf479c) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- Fix non-MBR-based bootloader updates [#2327]((https://github.com/particle-iot/device-os/pull/2327))

### INTERNAL

- Add an integration test to validate network/cloud connection time SLOs [#2312](https://github.com/particle-iot/device-os/pull/2312) [#2320](https://github.com/particle-iot/device-os/pull/2320) [#2321](https://github.com/particle-iot/device-os/pull/2321)
- [ci] Fix MarkupSafe weirdness [#2317](https://github.com/particle-iot/device-os/pull/2317)
- Add `.bundleignore` for Workbench Device OS source code bundles [#2326](https://github.com/particle-iot/device-os/pull/2326)
- Manage GCC dependencies with `.workbench/manifest.json` [d94f08030](https://github.com/particle-iot/device-os/commit/d94f0803068026d0b2aa0af426ba80c8b62299c7)

## 2.1.0

### FEATURES

- [Cellular] Send modem firmware version to the cloud as part of the system describe message [#2265](https://github.com/particle-iot/device-os/pull/2265)
- `Network.isOn()` and `Network.isOff()` APIs to query the network interface power state [#2205](https://github.com/particle-iot/device-os/pull/2205)
- [Electron] Proactively attempt to recover from a number of failed cellular registration states [#2301](https://github.com/particle-iot/device-os/pull/2301)

### ENHANCEMENTS

- [Cellular] Update ICCID/IMSI to APN map with a new Kore ICCID prefix [#2276](https://github.com/particle-iot/device-os/pull/2276)
- [B5 SoM / Quectel] Improve warm and cold boot behavior [#2300](https://github.com/particle-iot/device-os/pull/2300)
- Update Workbench dependencies [#2299](https://github.com/particle-iot/device-os/pull/2299)
- Improve I2C reset procedure to be less destructive and issue STOP condition as soon as possible [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Cellular] Perform PMIC/FuelGauge/RTC I2C bus reset on boot to avoid accidental writes after a non-graceful reset [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Argon] Reduce cloud keep-alive timeout to 25 seconds from 30 seconds [#2304](https://github.com/particle-iot/device-os/pull/2304)
- [Electron] Build system parts with LTO enabled [#2235](https://github.com/particle-iot/device-os/pull/2235)
- Upate MbedTLS to 2.22.0 [#2117](https://github.com/particle-iot/device-os/pull/2117)
- [Gen 3] Ethernet FeatherWing power state management [#2258](https://github.com/particle-iot/device-os/pull/2258)

### BUGFIXES

- [Electron] Fix unintended modem reset after an ongoing network registartion attempt is actively cancelled by the application [#2307](https://github.com/particle-iot/device-os/pull/2307)
- Correctly restore network interface power state after waking up from `STOP` or `ULTRA_LOW_POWER` sleep modes [#2308](https://github.com/particle-iot/device-os/pull/2308)
- [Gen 2] Fix D0 alternate-function being unconditionally reset when calling `Serial1.end()` [#2256](https://github.com/particle-iot/device-os/pull/2256)
- [Boron / B SoM] Fix external SIM getting stuck in initialization [#2263](https://github.com/particle-iot/device-os/pull/2263)
- [BLE] Return `false` in `BlePeerDevice::getCharacteristicByDescription()` if expected characteristic was not found [#2266](https://github.com/particle-iot/device-os/pull/2266)
- [Gen 3] Fix UART DMA RX transfer size issues causing DMA writes outside of the RX buffer [#2264](https://github.com/particle-iot/device-os/pull/2264)
- [Gen 3] Fix `ChannelStream::waitEvent()` timeout calculation [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Gen 3] Use `PIN_INVALID` when initializing SPI peripheral to avoid overriding the pin mode of the default CS pin on reinitialization [#2275](https://github.com/particle-iot/device-os/pull/2275)
- [Electron] Increase `AT+COPS` timeout to 5 minutes [#2281](https://github.com/particle-iot/device-os/pull/2281)
- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
- [B5 SoM / Tracker] Fix warm boot sometimes requiring modem reset [#2289](https://github.com/particle-iot/device-os/pull/2289)
- Fix `Particle.unsubscribe()` not preserving system subscriptions [#2293](https://github.com/particle-iot/device-os/pull/2293)
- Querying the value of an empty string variable causes an error [#2297](https://github.com/particle-iot/device-os/pull/2297)
- [Gen 3] Add workaround for Nordic nRF52840 anomaly 219 (TWIM: I2C timing spec is violated at 400 kHz) [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Gen 3] Fix micros/millis/unixtime becoming non-monotonic [2a4fcb82b](https://github.com/particle-iot/device-os/commit/2a4fcb82b0968300b8a0227f665ffe94203f9f38) [#2303](https://github.com/particle-iot/device-os/pull/2303)
- Fix the issue that the Particle.disconnect() doesn't clear the auto-connect flag [#2306](https://github.com/particle-iot/device-os/pull/2306)

### INTERNAL

- [ci] Remove build directory after finishing the build job [#2311](https://github.com/particle-iot/device-os/pull/2311)
- Startup SLO automated tests [#2277](https://github.com/particle-iot/device-os/pull/2277) [#2274](https://github.com/particle-iot/device-os/pull/2274)
- Allow clearing session data and running custom setup code in integration tests [#2280](https://github.com/particle-iot/device-os/pull/2280)

## 2.1.0-rc.1

### FEATURES

- [Cellular] Send modem firmware version to the cloud as part of the system describe message [#2265](https://github.com/particle-iot/device-os/pull/2265)
- `Network.isOn()` and `Network.isOff()` APIs to query the network interface power state [#2205](https://github.com/particle-iot/device-os/pull/2205)
- [Electron] Proactively attempt to recover from a number of failed cellular registration states [#2301](https://github.com/particle-iot/device-os/pull/2301)

### ENHANCEMENTS

- [Cellular] Update ICCID/IMSI to APN map with a new Kore ICCID prefix [#2276](https://github.com/particle-iot/device-os/pull/2276)
- [B5 SoM / Quectel] Improve warm and cold boot behavior [#2300](https://github.com/particle-iot/device-os/pull/2300)
- Update Workbench dependencies [#2299](https://github.com/particle-iot/device-os/pull/2299)
- Improve I2C reset procedure to be less destructive and issue STOP condition as soon as possible [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Cellular] Perform PMIC/FuelGauge/RTC I2C bus reset on boot to avoid accidental writes after a non-graceful reset [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Argon] Reduce cloud keep-alive timeout to 25 seconds from 30 seconds [#2304](https://github.com/particle-iot/device-os/pull/2304)
- [Electron] Build system parts with LTO enabled [#2235](https://github.com/particle-iot/device-os/pull/2235)
- Upate MbedTLS to 2.22.0 [#2117](https://github.com/particle-iot/device-os/pull/2117)
- [Gen 3] Ethernet FeatherWing power state management [#2258](https://github.com/particle-iot/device-os/pull/2258)

### BUGFIXES

- [Gen 2] Fix D0 alternate-function being unconditionally reset when calling `Serial1.end()` [#2256](https://github.com/particle-iot/device-os/pull/2256)
- [Boron / B SoM] Fix external SIM getting stuck in initialization [#2263](https://github.com/particle-iot/device-os/pull/2263)
- [BLE] Return `false` in `BlePeerDevice::getCharacteristicByDescription()` if expected characteristic was not found [#2266](https://github.com/particle-iot/device-os/pull/2266)
- [Gen 3] Fix UART DMA RX transfer size issues causing DMA writes outside of the RX buffer [#2264](https://github.com/particle-iot/device-os/pull/2264)
- [Gen 3] Fix `ChannelStream::waitEvent()` timeout calculation [#2267](https://github.com/particle-iot/device-os/pull/2267)
- [Gen 3] Use `PIN_INVALID` when initializing SPI peripheral to avoid overriding the pin mode of the default CS pin on reinitialization [#2275](https://github.com/particle-iot/device-os/pull/2275)
- [Electron] Increase `AT+COPS` timeout to 5 minutes [#2281](https://github.com/particle-iot/device-os/pull/2281)
- [Electron] Fix Sleep 2.0 APIs taking up to 10 minutes to power-off the cellular modem while it's attempting network registration [#2284](https://github.com/particle-iot/device-os/pull/2284)
- [B5 SoM / Tracker] Fix warm boot sometimes requiring modem reset [#2289](https://github.com/particle-iot/device-os/pull/2289)
- Fix `Particle.unsubscribe()` not preserving system subscriptions [#2293](https://github.com/particle-iot/device-os/pull/2293)
- Querying the value of an empty string variable causes an error [#2297](https://github.com/particle-iot/device-os/pull/2297)
- [Gen 3] Add workaround for Nordic nRF52840 anomaly 219 (TWIM: I2C timing spec is violated at 400 kHz) [#2303](https://github.com/particle-iot/device-os/pull/2303)
- [Gen 3] Fix micros/millis/unixtime becoming non-monotonic [2a4fcb82b](https://github.com/particle-iot/device-os/commit/2a4fcb82b0968300b8a0227f665ffe94203f9f38) [#2303](https://github.com/particle-iot/device-os/pull/2303)
- Fix the issue that the Particle.disconnect() doesn't clear the auto-connect flag [#2306](https://github.com/particle-iot/device-os/pull/2306)

### INTERNAL

- Startup SLO automated tests [#2277](https://github.com/particle-iot/device-os/pull/2277) [#2274](https://github.com/particle-iot/device-os/pull/2274)
- Allow clearing session data and running custom setup code in integration tests [#2280](https://github.com/particle-iot/device-os/pull/2280)

## 2.0.1

### ENHANCEMENTS

- [Gen 3] Remove incompatibility that required intermediate update through Device OS 1.1.0 [#2254](https://github.com/particle-iot/device-os/pull/2254)

### BUGFIXES

- [Gen 3] Fix processing of modules with `MODULE_DROP_MODULE_INFO` flag in the bootloader, when scheduled to be flashed by Device OS versions before 2.0.0-rc.1 [#2246](https://github.com/particle-iot/device-os/pull/2246)
- Allow `SystemSleepNetworkFlag::INACTIVE_STANDBY` to be specified when entering `HIBERNATE` sleep mode to keep the network coprocessor in its current state [#2248](https://github.com/particle-iot/device-os/pull/2248)
- Add inline optimization to speed up fast pin API calls [#2251](https://github.com/particle-iot/device-os/pull/2251)

## 2.0.0

### BREAKING CHANGES

- Mesh support removed. 2.x+ DeviceOS releases no longer have Mesh capabilities [#2068](https://github.com/particle-iot/device-os/pull/2068)
- Xenon platform support removed. 2.x+ DeviceOS releases no longer support Xenons [#2068](https://github.com/particle-iot/device-os/pull/2068)
- Minimum ARM GCC version required increased to 9.2.1 [#2123](https://github.com/particle-iot/device-os/pull/2123)
- `SPISettings` class is always available even if compiling without Arduino compatibility [#2138](https://github.com/particle-iot/device-os/pull/2138)
- Add deprecation notices for some of the renamed HAL APIs (with appropriate replacements) [#2148](https://github.com/particle-iot/device-os/pull/2148)

### DEPRECATION

- [Cellular] Mark `CellularSignal::rssi` and `CellularSignal::qual` as deprecated [#2182](https://github.com/particle-iot/device-os/pull/2182)

### FEATURES

- [Cellular] Read IMSI when multi-IMSI SIM performs the switch [#2174](https://github.com/particle-iot/device-os/pull/2174) [#2179](https://github.com/particle-iot/device-os/pull/2179)
- Allow UDP server public key to be set in DCT programmatically [#2178](https://github.com/particle-iot/device-os/pull/2178)
- [Gen 2] Wake-up by analog value [#2172](https://github.com/particle-iot/device-os/pull/2172)
- [Gen 2] Wake-up by USART [#2173](https://github.com/particle-iot/device-os/pull/2173)
- [Gen 3] Add `ftruncate()` and `truncate()` APIs [#2195](https://github.com/particle-iot/device-os/pull/2195)
- Expose functions to fetch serial / mobile secret from OTP-area [#2190](https://github.com/particle-iot/device-os/pull/2190)
- [Electron] Wake-up by cellular [#2186](https://github.com/particle-iot/device-os/pull/2186)
- [Gen 3] Wake up from STOP/ULP/Hibernate modes by analog pin [#2163](https://github.com/particle-iot/device-os/pull/2163)
- [Gen 3] Wake up from STOP/ULP modes by WiFi/Cellular and UART [#2162](https://github.com/particle-iot/device-os/pull/2162)
- [Gen 3] `pinSetDriveStrength()` API [#2157](https://github.com/particle-iot/device-os/pull/2157)
- Add `os_queue_peek()` and make sure that queues and semaphores can be accessed from ISRs [#2156](https://github.com/particle-iot/device-os/pull/2156) [#2074](https://github.com/particle-iot/device-os/pull/2074)
- [Gen 3 / Cellular] Proactively attempt to recover from a number of failed cellular registration states [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Reset cellular modem if failing to establish PPP session for over 5 minutes [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 2] Blocking UDP socket reads with a timeout [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Power-loss resistant bootloader updates through MBR [#2151](https://github.com/particle-iot/device-os/pull/2151)
- Ultra low power sleep mode [#2149](https://github.com/particle-iot/device-os/pull/2149) [#2133](https://github.com/particle-iot/device-os/pull/2133) [#2132](https://github.com/particle-iot/device-os/pull/2132) [#2129](https://github.com/particle-iot/device-os/pull/2129) [#2130](https://github.com/particle-iot/device-os/pull/2130) [#2125](https://github.com/particle-iot/device-os/pull/2125) [#2136](https://github.com/particle-iot/device-os/pull/2136)
- Additional APN settings based on ICCIDs [#2144](https://github.com/particle-iot/device-os/pull/2144)
- NTP-based internet test [#2118](https://github.com/particle-iot/device-os/pull/2118)
- [Gen 3] Warm bootup of cellular modems and cellular connectivity resumption [#2102](https://github.com/particle-iot/device-os/pull/2102) [#2146](https://github.com/particle-iot/device-os/pull/2146)
- Support for compressed / combined binaries in OTA updates [#2097](https://github.com/particle-iot/device-os/pull/2097)
- ARM GCC 9 support [#2103](https://github.com/particle-iot/device-os/pull/2103)
- Device-initiated describe messages [#2024](https://github.com/particle-iot/device-os/pull/2024)
- Notify the cloud about planned disconnections [#1899](https://github.com/particle-iot/device-os/pull/1899)

### ENHANCEMENTS

- [Gen 3] Enable Network Diagnostics/Vitals [#2230](https://github.com/particle-iot/device-os/pull/2230)
- Remove warning for publish and subscribe scope deprecation [#2209](https://github.com/particle-iot/device-os/pull/2209)
- Export some common standard C library functions through dynalib [#2225](https://github.com/particle-iot/device-os/pull/2225)
- [Electron] Monitor cellular modem for brown-outs and resets/crashes and perform reinitialization when required [#2219](https://github.com/particle-iot/device-os/pull/2219)
- Clear OTA slots after updating firmware modules to improve reliability of OTA updates [#2176](https://github.com/particle-iot/device-os/pull/2176)
- [Cellular] Replace `AT+COPS=2` with `AT+CFUN=0` or `AT+CFUN=4` to prevent longer registration times [#2177](https://github.com/particle-iot/device-os/pull/2177)
- [Cellular] IMSI-based operator lookup, operator-specific enhancements [#2185](https://github.com/particle-iot/device-os/pull/2185)
- [Electron] Recovery mechanics for cases when the modem becomes unresponsive [#2198](https://github.com/particle-iot/device-os/pull/2198)
- Add `printf` attributes to appopriate wiring functions to generate `-Wformat` warnings [#2201](https://github.com/particle-iot/device-os/pull/2201)
- Change `Time::now()` return type to 32-bit `time32_t` to reduce potential issues with `printf` formatting of 64-bit `time_t` [#2201](https://github.com/particle-iot/device-os/pull/2201)
- [Gen 3] Workaround when unable to obtain DNS servers from remote PPP peer [#2165](https://github.com/particle-iot/device-os/pull/2165)
- System power manager blocks access to FuelGauge for 500ms in its own thread, instead of system when waking up from STOP/ULP sleep mode [#2159](https://github.com/particle-iot/device-os/pull/2159)
- [wiring] Pin operations are not dependent on wiring C++ peripheral object initialization (e.g. `SPI`, `Wire` etc) [#2157](https://github.com/particle-iot/device-os/pull/2157)
- [Gen 3] Default SPI pin drive strength changed to high [#2157](https://github.com/particle-iot/device-os/pull/2157)
- [Gen 3] Restore original `BASEPRI` when exiting FreeRTOS critical section [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Do not use packet buffers from pool in TX path [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Reliable data mode entry when attempting to establish PPP connection [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Changes the implementation of Nordic SDK critical sections to use `BASEPRI` [#2101](https://github.com/particle-iot/device-os/pull/2101)
- Allow resetting the device and disconnecting from the cloud via low-level USB vendor requests [#2142](https://github.com/particle-iot/device-os/pull/2142)
- [Cellular] When resuming cellular connection, do not run `COPS=0` again to avoid triggering PLMN reselection [#2139](https://github.com/particle-iot/device-os/pull/2139)
- Reduce runtime RAM usage by sharing newlib `_impure_ptr` between modules [#2126](https://github.com/particle-iot/device-os/pull/2126)
- [Electron] Use `snprintf()` instead of `sprintf()` [#2122](https://github.com/particle-iot/device-os/pull/2122)
- [Gen 3] System thread wakeup on cloud data [#2113](https://github.com/particle-iot/device-os/pull/2113)
- [Argon] Hide unsupported WiFi wiring APIs [#2120](https://github.com/particle-iot/device-os/pull/2120)
- [B5 SoM / Tracker] Disable 2G fallback for BG96-based devices [#2112](https://github.com/particle-iot/device-os/pull/2112)
- [wiring] Changes default I2C timeouts when communicating with FuelGauge and PMIC to more manageable values [#2096](https://github.com/particle-iot/device-os/pull/2096)
- [wiring] Propagate low-level I2C errors in `FuelGauge` methods [#2094](https://github.com/particle-iot/device-os/pull/2094)
- [Gen 3] Network stack enhancements [#2079](https://github.com/particle-iot/device-os/pull/2079)
- Send describe messages as confirmable CoAP messages [#2024](https://github.com/particle-iot/device-os/pull/2024)
- [Argon] OTA adjustments [#2045](https://github.com/particle-iot/device-os/pull/2045)
- Remove support for unused control requests [#2064](https://github.com/particle-iot/device-os/pull/2064)
- RTC HAL refactoring to increase time-keeping precision [#2123](https://github.com/particle-iot/device-os/pull/2123)
- Y2k38 `time_t` size change adjustments [#2123](https://github.com/particle-iot/device-os/pull/2123)
- [wiring] Refactor wiring `Time` class to use reentrant versions of libc time functions [#2123](https://github.com/particle-iot/device-os/pull/2123)

### BUGFIXES

- [Gen 2] Fix an issue in Sleep 2.0 API with STOP and ULP sleep modes potentially blocking user application on wake-up [#2238](https://github.com/particle-iot/device-os/pull/2238)
- [Electron] Fix a regression in `+C*REG` URC parsing [#2239](https://github.com/particle-iot/device-os/pull/2239)
- [Gen 3] Fix a potential crash when using sleep with multiple pins specified as wake-up sources together with BLE [#2227](https://github.com/particle-iot/device-os/pull/2227)
- [Electron] 2G Electrons with uBlox G350 should use `AT+CPWROFF` exclusively for powering off the modem [#2229](https://github.com/particle-iot/device-os/pull/2229)
- [Cellular] Set `INPUT_PULLUP` pin mode for FuelGauge and PMIC interrupt pins [#2207](https://github.com/particle-iot/device-os/pull/2207)
- Disconnect from the cloud before going into sleep [#2206](https://github.com/particle-iot/device-os/pull/2206)
- Use `always_inline` attribute for deprecated HAL API [#2204](https://github.com/particle-iot/device-os/pull/2204)
- [Gen 3] Set SPI `MOSI` drive strength as high by default [#2214](https://github.com/particle-iot/device-os/pull/2214) [nrf5_sdk#12](https://github.com/particle-iot/nrf5_sdk/pull/12)
- [Gen 3] Fix LED behavior in case of network loss before cloud connection is established [#2210](https://github.com/particle-iot/device-os/pull/2210)
- [Gen 3 / Cellular] Allow Software Default MNO profile to be used when chosen by SIM ICCID-based selection [#2211](https://github.com/particle-iot/device-os/pull/2211)
- [Cellular] Allow Software Default MNO profile to be used with certain version of SARA R410 modem firmware, where MNO profile 100 is not supported [#2213](https://github.com/particle-iot/device-os/pull/2213)
- [Boron / B SoM] Workaround for SARA R410 PPP ConfReq behavior [#2208](https://github.com/particle-iot/device-os/pull/2208) [lwip#8](https://github.com/particle-iot/lwip/pull/8)
- Fix `CellularSignal` deprecation messages [#2221](https://github.com/particle-iot/device-os/pull/2221)
- [Cellular] Fix PMIC reducing current on warm boot and causing modem brownouts [#2215](https://github.com/particle-iot/device-os/pull/2215)
- [Electron] Workaround for modem HAL relying on system networking code to re-attempt initialization [#2218](https://github.com/particle-iot/device-os/pull/2218)
- [Electron] Fix modem log timestamps starting with a high number on boot [#2169](https://github.com/particle-iot/device-os/pull/2169)
- [Cellular] Make sure 2G fallback stays disabled on Quectel BG96-based platforms [#2175](https://github.com/particle-iot/device-os/pull/2175)
- [Gen 3] Fix USART wake-up source configuration in Ultra Low Power mode, causing immediate sleep mode exit [#2180](https://github.com/particle-iot/device-os/pull/2180)
- [Gen 3] Fix tone generation behavior with zero duration (infinite) [#2183](https://github.com/particle-iot/device-os/pull/2183)
- Exclude printable objects from `Print` overload taking integral and unsigned integer convertible types [#2181](https://github.com/particle-iot/device-os/pull/2181)
- [Boron / B SoM] Fix warm bootup on uBlox SARA R4-based devices [#2188](https://github.com/particle-iot/device-os/pull/2188)
- [Gen 2] Support repeated-START between WRITE and READ operations in I2C Slave mode [#2184](https://github.com/particle-iot/device-os/pull/2184) [#2193](https://github.com/particle-iot/device-os/pull/2193)
- Fastpin functions should not depend on the object initialization order [#2194](https://github.com/particle-iot/device-os/pull/2194)
- [Electron] Fix modem power leakage when the modem is in an unknown state when going into a sleep mode [#2197](https://github.com/particle-iot/device-os/pull/2197)
- [BLE] Fix issue with `.serviceUUID()` not returning UUID when there is an array [#2202](https://github.com/particle-iot/device-os/pull/2202)
- [Gen 3] Default SPI pin drive strength changed to high [7f2e8a711bd14abd1e094679f1cc6d26742cb6c9](https://github.com/particle-iot/device-os/commit/7f2e8a711bd14abd1e094679f1cc6d26742cb6c9)
- [wiring] `Servo` object should deinit its pin when destructed [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Fix an issue with `loop()` not being executed in `SEMI_AUTOMATIC` modem when network interfaces are down [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Fix cycle counter synchronization when processing RTC overflow events [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Electron] Increase default AT command timeouts [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Move PWM-capable pins from the PWM peripheral shared with RGB pins when possible [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Call into LwIP PPP code to indicate `PPP_IP` protocol is finished [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Fix BLE event processing while in STOP/ULP sleep mode [#2155](https://github.com/particle-iot/device-os/pull/2155)
- [Gen 3] `Cellular.command()` should check NCP state before attempting to execute command [#2110](https://github.com/particle-iot/device-os/pull/2110) [#2153](https://github.com/particle-iot/device-os/pull/2153)
- [Gen 2] Fix RTC thread-safety issues when accessing RTC peripheral [#2154](https://github.com/particle-iot/device-os/pull/2154)
- Workaround for Gen 3 devices not connecting to the cloud in non-automatic threaded mode [#2152](https://github.com/particle-iot/device-os/pull/2152)
- Enable PMIC buck converter on boot by default [#2147](https://github.com/particle-iot/device-os/pull/2147)
- [Gen 3] Reliably turn off the cellular modem when going into sleep mode to reduce current consumption [#2110](https://github.com/particle-iot/device-os/pull/2110)
- [Gen 3] Fixes the behavior when the USB host puts the device into suspended state [#2101](https://github.com/particle-iot/device-os/pull/2101)
- [Gen 3] Fixes potential deadlock in SPI HAL [#2101](https://github.com/particle-iot/device-os/pull/2101) [#2091](https://github.com/particle-iot/device-os/pull/2091)
- [Gen 3] Filter out non-vendor requests in USB control request handler [#2101](https://github.com/particle-iot/device-os/pull/2101)
- [Gen 3] SOF-based USB Serial port state detection [#2101](https://github.com/particle-iot/device-os/pull/2101)
- [Gen 3] Fixes an issue with devices not waking up by RTC from STOP sleep mode [#2134](https://github.com/particle-iot/device-os/pull/2134)
- [Gen 3] Fix `rename()` filesystem call [#2141](https://github.com/particle-iot/device-os/pull/2141)
- [Gen 3] Treat failure to open data channel as critical error [#2139](https://github.com/particle-iot/device-os/pull/2139)
- [Photon] Fix WPA Enterprise X509 certificate parsing [#2126](https://github.com/particle-iot/device-os/commit/428940879f5fd1d5a3bdd658260ee45a9f7aca90)
- Use newlib-nano headers when compiling [#2126](https://github.com/particle-iot/device-os/pull/2126)
- [Gen 2] Reset the device after applying an update while in listening mode [#2127](https://github.com/particle-iot/device-os/pull/2127)
- [Electron] Process `CEREG: 0` URCs on R4-based devices to detect loss of cellular connectivity [#2119](https://github.com/particle-iot/device-os/pull/2119)
- [Cellular] Fixes the issue that FuelGauge doesn't work as expected after being woken up [#2116](https://github.com/particle-iot/device-os/pull/2116)
- [Electron] Fixes buffer overrun in modem hal [#2115](https://github.com/particle-iot/device-os/pull/2115)
- [WiFi] `WiFiCredentials::setSecurity()` should be taking wiring security type (e.g. `WPA2` instead of `WLAN_SEC_WPA2`) [#2098](https://github.com/particle-iot/device-os/pull/2098)
- [Boron] Fixes SARA R4 power on sequence where the default attempt should be made with runtime baudrate [#2107](https://github.com/particle-iot/device-os/pull/2107)
- [Gen 2] Fixes an issue with I2C bus pins driven low if building with JTAG/SWD enabled [#2080](https://github.com/particle-iot/device-os/pull/2080)
- [Boron] Fixes an issue with SARA R4 modems on LTE Borons becoming unresponsive when sending substantial amount of network data continuously [#2100](https://github.com/particle-iot/device-os/pull/2100)
- Fix session resumption in `AUTOMATIC` system mode [#2024](https://github.com/particle-iot/device-os/pull/2024)

### INTERNAL

- Run on-device tests under the DeviceOS test runner [#2140](https://github.com/particle-iot/device-os/pull/2140) [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Remove old deprecated platforms [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Addresses multiple issues in on-device no-fixture tests [#2150](https://github.com/particle-iot/device-os/pull/2150)

## 2.0.0-rc.4

### ENHANCEMENTS

- [Gen 3] Enable Network Diagnostics/Vitals [#2230](https://github.com/particle-iot/device-os/pull/2230)

### BUGFIXES

- [Gen 3] Fix a potential crash when using sleep with multiple pins specified as wake-up sources together with BLE [#2227](https://github.com/particle-iot/device-os/pull/2227)
- [Electron] 2G Electrons with uBlox G350 should use `AT+CPWROFF` exclusively for powering off the modem [#2229](https://github.com/particle-iot/device-os/pull/2229)

## 2.0.0-rc.3

### BUGFIXES

- [Cellular] Set `INPUT_PULLUP` pin mode for FuelGauge and PMIC interrupt pins [#2207](https://github.com/particle-iot/device-os/pull/2207)
- Disconnect from the cloud before going into sleep [#2206](https://github.com/particle-iot/device-os/pull/2206)
- Use `always_inline` attribute for deprecated HAL API [#2204](https://github.com/particle-iot/device-os/pull/2204)
- [Gen 3] Set SPI `MOSI` drive strength as high by default [#2214](https://github.com/particle-iot/device-os/pull/2214) [nrf5_sdk#12](https://github.com/particle-iot/nrf5_sdk/pull/12)
- [Gen 3] Fix LED behavior in case of network loss before cloud connection is established [#2210](https://github.com/particle-iot/device-os/pull/2210)
- [Gen 3 / Cellular] Allow Software Default MNO profile to be used when chosen by SIM ICCID-based selection [#2211](https://github.com/particle-iot/device-os/pull/2211)
- [Cellular] Allow Software Default MNO profile to be used with certain version of SARA R410 modem firmware, where MNO profile 100 is not supported [#2213](https://github.com/particle-iot/device-os/pull/2213)
- [Boron / B SoM] Workaround for SARA R410 PPP ConfReq behavior [#2208](https://github.com/particle-iot/device-os/pull/2208) [lwip#8](https://github.com/particle-iot/lwip/pull/8)
- Fix `CellularSignal` deprecation messages [#2221](https://github.com/particle-iot/device-os/pull/2221)
- [Cellular] Fix PMIC reducing current on warm boot and causing modem brownouts [#2215](https://github.com/particle-iot/device-os/pull/2215)
- [Electron] Workaround for modem HAL relying on system networking code to re-attempt initialization [#2218](https://github.com/particle-iot/device-os/pull/2218)

### ENHANCEMENTS

- Remove warning for publish and subscribe scope deprecation [#2209](https://github.com/particle-iot/device-os/pull/2209)
- Export some common standard C library functions through dynalib [#2225](https://github.com/particle-iot/device-os/pull/2225)
- [Electron] Monitor cellular modem for brown-outs and resets/crashes and perform reinitialization when required [#2219](https://github.com/particle-iot/device-os/pull/2219)

## 2.0.0-rc.2

### DEPRECATION

- [Cellular] Mark `CellularSignal::rssi` and `CellularSignal::qual` as deprecated [#2182](https://github.com/particle-iot/device-os/pull/2182)

### FEATURES

- [Cellular] Read IMSI when multi-IMSI SIM performs the switch [#2174](https://github.com/particle-iot/device-os/pull/2174) [#2179](https://github.com/particle-iot/device-os/pull/2179)
- Allow UDP server public key to be set in DCT programmatically [#2178](https://github.com/particle-iot/device-os/pull/2178)
- [Gen 2] Wake-up by analog value [#2172](https://github.com/particle-iot/device-os/pull/2172)
- [Gen 2] Wake-up by USART [#2173](https://github.com/particle-iot/device-os/pull/2173)
- [Gen 3] Add `ftruncate()` and `truncate()` APIs [#2195](https://github.com/particle-iot/device-os/pull/2195)
- Expose functions to fetch serial / mobile secret from OTP-area [#2190](https://github.com/particle-iot/device-os/pull/2190)
- [Electron] Wake-up by cellular [#2186](https://github.com/particle-iot/device-os/pull/2186)

### ENHANCEMENTS

- Clear OTA slots after updating firmware modules to improve reliability of OTA updates [#2176](https://github.com/particle-iot/device-os/pull/2176)
- [Cellular] Replace `AT+COPS=2` with `AT+CFUN=0` or `AT+CFUN=4` to prevent longer registration times [#2177](https://github.com/particle-iot/device-os/pull/2177)
- [Cellular] IMSI-based operator lookup, operator-specific enhancements [#2185](https://github.com/particle-iot/device-os/pull/2185)
- [Electron] Recovery mechanics for cases when the modem becomes unresponsive [#2198](https://github.com/particle-iot/device-os/pull/2198)
- Add `printf` attributes to appopriate wiring functions to generate `-Wformat` warnings [#2201](https://github.com/particle-iot/device-os/pull/2201)
- Change `Time::now()` return type to 32-bit `time32_t` to reduce potential issues with `printf` formatting of 64-bit `time_t` [#2201](https://github.com/particle-iot/device-os/pull/2201)

### BUGFIXES

- [Electron] Fix modem log timestamps starting with a high number on boot [#2169](https://github.com/particle-iot/device-os/pull/2169)
- [Cellular] Make sure 2G fallback stays disabled on Quectel BG96-based platforms [#2175](https://github.com/particle-iot/device-os/pull/2175)
- [Gen 3] Fix USART wake-up source configuration in Ultra Low Power mode, causing immediate sleep mode exit [#2180](https://github.com/particle-iot/device-os/pull/2180)
- [Gen 3] Fix tone generation behavior with zero duration (infinite) [#2183](https://github.com/particle-iot/device-os/pull/2183)
- Exclude printable objects from `Print` overload taking integral and unsigned integer convertible types [#2181](https://github.com/particle-iot/device-os/pull/2181)
- [Boron / B SoM] Fix warm bootup on uBlox SARA R4-based devices [#2188](https://github.com/particle-iot/device-os/pull/2188)
- [Gen 2] Support repeated-START between WRITE and READ operations in I2C Slave mode [#2184](https://github.com/particle-iot/device-os/pull/2184) [#2193](https://github.com/particle-iot/device-os/pull/2193)
- Fastpin functions should not depend on the object initialization order [#2194](https://github.com/particle-iot/device-os/pull/2194)
- [Electron] Fix modem power leakage when the modem is in an unknown state when going into a sleep mode [#2197](https://github.com/particle-iot/device-os/pull/2197)
- [BLE] Fix issue with `.serviceUUID()` not returning UUID when there is an array [#2202](https://github.com/particle-iot/device-os/pull/2202)
- [Gen 3] Default SPI pin drive strength changed to high [7f2e8a711bd14abd1e094679f1cc6d26742cb6c9](https://github.com/particle-iot/device-os/commit/7f2e8a711bd14abd1e094679f1cc6d26742cb6c9)

## 2.0.0-rc.1

### BREAKING CHANGES

- Mesh support removed. 2.x+ DeviceOS releases no longer have Mesh capabilities [#2068](https://github.com/particle-iot/device-os/pull/2068)
- Xenon platform support removed. 2.x+ DeviceOS releases no longer support Xenons [#2068](https://github.com/particle-iot/device-os/pull/2068)
- Minimum ARM GCC version required increased to 9.2.1 [#2123](https://github.com/particle-iot/device-os/pull/2123)
- `SPISettings` class is always available even if compiling without Arduino compatibility [#2138](https://github.com/particle-iot/device-os/pull/2138)
- Add deprecation notices for some of the renamed HAL APIs (with appropriate replacements) [#2148](https://github.com/particle-iot/device-os/pull/2148)

### FEATURES

- [Gen 3] Wake up from STOP/ULP/Hibernate modes by analog pin [#2163](https://github.com/particle-iot/device-os/pull/2163)
- [Gen 3] Wake up from STOP/ULP modes by WiFi/Cellular and UART [#2162](https://github.com/particle-iot/device-os/pull/2162)
- [Gen 3] `pinSetDriveStrength()` API [#2157](https://github.com/particle-iot/device-os/pull/2157)
- Add `os_queue_peek()` and make sure that queues and semaphores can be accessed from ISRs [#2156](https://github.com/particle-iot/device-os/pull/2156) [#2074](https://github.com/particle-iot/device-os/pull/2074)
- [Gen 3 / Cellular] Proactively attempt to recover from a number of failed cellular registration states [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Reset cellular modem if failing to establish PPP session for over 5 minutes [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 2] Blocking UDP socket reads with a timeout [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Power-loss resistant bootloader updates through MBR [#2151](https://github.com/particle-iot/device-os/pull/2151)
- Ultra low power sleep mode [#2149](https://github.com/particle-iot/device-os/pull/2149) [#2133](https://github.com/particle-iot/device-os/pull/2133) [#2132](https://github.com/particle-iot/device-os/pull/2132) [#2129](https://github.com/particle-iot/device-os/pull/2129) [#2130](https://github.com/particle-iot/device-os/pull/2130) [#2125](https://github.com/particle-iot/device-os/pull/2125) [#2136](https://github.com/particle-iot/device-os/pull/2136)
- Additional APN settings based on ICCIDs [#2144](https://github.com/particle-iot/device-os/pull/2144)
- NTP-based internet test [#2118](https://github.com/particle-iot/device-os/pull/2118)
- [Gen 3] Warm bootup of cellular modems and cellular connectivity resumption [#2102](https://github.com/particle-iot/device-os/pull/2102) [#2146](https://github.com/particle-iot/device-os/pull/2146)
- Support for compressed / combined binaries in OTA updates [#2097](https://github.com/particle-iot/device-os/pull/2097)
- ARM GCC 9 support [#2103](https://github.com/particle-iot/device-os/pull/2103)
- Device-initiated describe messages [#2024](https://github.com/particle-iot/device-os/pull/2024)
- Notify the cloud about planned disconnections [#1899](https://github.com/particle-iot/device-os/pull/1899)

### ENHANCEMENTS

- [Gen 3] Workaround when unable to obtain DNS servers from remote PPP peer [#2165](https://github.com/particle-iot/device-os/pull/2165)
- System power manager blocks access to FuelGauge for 500ms in its own thread, instead of system when waking up from STOP/ULP sleep mode [#2159](https://github.com/particle-iot/device-os/pull/2159)
- [wiring] Pin operations are not dependent on wiring C++ peripheral object initialization (e.g. `SPI`, `Wire` etc) [#2157](https://github.com/particle-iot/device-os/pull/2157)
- [Gen 3] Default SPI pin drive strength changed to high [#2157](https://github.com/particle-iot/device-os/pull/2157)
- [Gen 3] Restore original `BASEPRI` when exiting FreeRTOS critical section [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Do not use packet buffers from pool in TX path [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Reliable data mode entry when attempting to establish PPP connection [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Changes the implementation of Nordic SDK critical sections to use `BASEPRI` [#2101](https://github.com/particle-iot/device-os/pull/2101)
- Allow resetting the device and disconnecting from the cloud via low-level USB vendor requests [#2142](https://github.com/particle-iot/device-os/pull/2142)
- [Cellular] When resuming cellular connection, do not run `COPS=0` again to avoid triggering PLMN reselection [#2139](https://github.com/particle-iot/device-os/pull/2139)
- Reduce runtime RAM usage by sharing newlib `_impure_ptr` between modules [#2126](https://github.com/particle-iot/device-os/pull/2126)
- [Electron] Use `snprintf()` instead of `sprintf()` [#2122](https://github.com/particle-iot/device-os/pull/2122)
- [Gen 3] System thread wakeup on cloud data [#2113](https://github.com/particle-iot/device-os/pull/2113)
- [Argon] Hide unsupported WiFi wiring APIs [#2120](https://github.com/particle-iot/device-os/pull/2120)
- [B5 SoM / Tracker] Disable 2G fallback for BG96-based devices [#2112](https://github.com/particle-iot/device-os/pull/2112)
- [wiring] Changes default I2C timeouts when communicating with FuelGauge and PMIC to more manageable values [#2096](https://github.com/particle-iot/device-os/pull/2096)
- [wiring] Propagate low-level I2C errors in `FuelGauge` methods [#2094](https://github.com/particle-iot/device-os/pull/2094)
- [Gen 3] Network stack enhancements [#2079](https://github.com/particle-iot/device-os/pull/2079)
- Send describe messages as confirmable CoAP messages [#2024](https://github.com/particle-iot/device-os/pull/2024)
- [Argon] OTA adjustments [#2045](https://github.com/particle-iot/device-os/pull/2045)
- Remove support for unused control requests [#2064](https://github.com/particle-iot/device-os/pull/2064)
- RTC HAL refactoring to increase time-keeping precision [#2123](https://github.com/particle-iot/device-os/pull/2123)
- Y2k38 `time_t` size change adjustments [#2123](https://github.com/particle-iot/device-os/pull/2123)
- [wiring] Refactor wiring `Time` class to use reentrant versions of libc time functions [#2123](https://github.com/particle-iot/device-os/pull/2123)

### BUGFIXES

- [wiring] `Servo` object should deinit its pin when destructed [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Fix an issue with `loop()` not being executed in `SEMI_AUTOMATIC` modem when network interfaces are down [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Fix cycle counter synchronization when processing RTC overflow events [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Electron] Increase default AT command timeouts [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Move PWM-capable pins from the PWM peripheral shared with RGB pins when possible [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Call into LwIP PPP code to indicate `PPP_IP` protocol is finished [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Fix BLE event processing while in STOP/ULP sleep mode [#2155](https://github.com/particle-iot/device-os/pull/2155)
- [Gen 3] `Cellular.command()` should check NCP state before attempting to execute command [#2110](https://github.com/particle-iot/device-os/pull/2110) [#2153](https://github.com/particle-iot/device-os/pull/2153)
- [Gen 2] Fix RTC thread-safety issues when accessing RTC peripheral [#2154](https://github.com/particle-iot/device-os/pull/2154)
- Workaround for Gen 3 devices not connecting to the cloud in non-automatic threaded mode [#2152](https://github.com/particle-iot/device-os/pull/2152)
- Enable PMIC buck converter on boot by default [#2147](https://github.com/particle-iot/device-os/pull/2147)
- [Gen 3] Reliably turn off the cellular modem when going into sleep mode to reduce current consumption [#2110](https://github.com/particle-iot/device-os/pull/2110)
- [Gen 3] Fixes the behavior when the USB host puts the device into suspended state [#2101](https://github.com/particle-iot/device-os/pull/2101)
- [Gen 3] Fixes potential deadlock in SPI HAL [#2101](https://github.com/particle-iot/device-os/pull/2101) [#2091](https://github.com/particle-iot/device-os/pull/2091)
- [Gen 3] Filter out non-vendor requests in USB control request handler [#2101](https://github.com/particle-iot/device-os/pull/2101)
- [Gen 3] SOF-based USB Serial port state detection [#2101](https://github.com/particle-iot/device-os/pull/2101)
- [Gen 3] Fixes an issue with devices not waking up by RTC from STOP sleep mode [#2134](https://github.com/particle-iot/device-os/pull/2134)
- [Gen 3] Fix `rename()` filesystem call [#2141](https://github.com/particle-iot/device-os/pull/2141)
- [Gen 3] Treat failure to open data channel as critical error [#2139](https://github.com/particle-iot/device-os/pull/2139)
- [Photon] Fix WPA Enterprise X509 certificate parsing [#2126](https://github.com/particle-iot/device-os/commit/428940879f5fd1d5a3bdd658260ee45a9f7aca90)
- Use newlib-nano headers when compiling [#2126](https://github.com/particle-iot/device-os/pull/2126)
- [Gen 2] Reset the device after applying an update while in listening mode [#2127](https://github.com/particle-iot/device-os/pull/2127)
- [Electron] Process `CEREG: 0` URCs on R4-based devices to detect loss of cellular connectivity [#2119](https://github.com/particle-iot/device-os/pull/2119)
- [Cellular] Fixes the issue that FuelGauge doesn't work as expected after being woken up [#2116](https://github.com/particle-iot/device-os/pull/2116)
- [Electron] Fixes buffer overrun in modem hal [#2115](https://github.com/particle-iot/device-os/pull/2115)
- [WiFi] `WiFiCredentials::setSecurity()` should be taking wiring security type (e.g. `WPA2` instead of `WLAN_SEC_WPA2`) [#2098](https://github.com/particle-iot/device-os/pull/2098)
- [Boron] Fixes SARA R4 power on sequence where the default attempt should be made with runtime baudrate [#2107](https://github.com/particle-iot/device-os/pull/2107)
- [Gen 2] Fixes an issue with I2C bus pins driven low if building with JTAG/SWD enabled [#2080](https://github.com/particle-iot/device-os/pull/2080)
- [Boron] Fixes an issue with SARA R4 modems on LTE Borons becoming unresponsive when sending substantial amount of network data continuously [#2100](https://github.com/particle-iot/device-os/pull/2100)
- Fix session resumption in `AUTOMATIC` system mode [#2024](https://github.com/particle-iot/device-os/pull/2024)

### INTERNAL

- Run on-device tests under the DeviceOS test runner [#2140](https://github.com/particle-iot/device-os/pull/2140) [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Remove old deprecated platforms [#2150](https://github.com/particle-iot/device-os/pull/2150)
- Addresses multiple issues in on-device no-fixture tests [#2150](https://github.com/particle-iot/device-os/pull/2150)

## 1.5.4-rc.2

### BUGFIXES

- Configure PMIC and FuelGauge interrupt pins as `INPUT_PULLUP` [#2207](https://github.com/particle-iot/device-os/pull/2207)
- [Gen 3 / Cellular] Do not use packet buffers from pool in TX path [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3 / Cellular] Call into LwIP PPP code to indicate `PPP_IP` protocol is finished [#2150](https://github.com/particle-iot/device-os/pull/2150)
- [Gen 3] Workaround when unable to obtain DNS servers from remote PPP peer [#2165](https://github.com/particle-iot/device-os/pull/2165)
- [Cellular] When resuming cellular connection, do not run `COPS=0` again to avoid triggering PLMN reselection [#2139](https://github.com/particle-iot/device-os/pull/2139)


## 1.5.4-rc.1

### BUGFIXES

- [Electron] Fixes buffer overrun in modem HAL [#2115](https://github.com/particle-iot/device-os/pull/2115)
- [Electron] Process 'CEREG: 0' URCs on R410M [#2119](https://github.com/particle-iot/device-os/pull/2119)
- [Photon] Fix WPA Enterprise X509 certificate parsing [428940879f5fd1d5a3bdd658260ee45a9f7aca90](https://github.com/particle-iot/device-os/commit/428940879f5fd1d5a3bdd658260ee45a9f7aca90)

## 1.5.3-tracker.1

### FEATURES

- [Gen 3] Add API to read OTP hardware version and model information
- [Asset Tracker] Alternate I2C interface on D8/D9

### ENHANCEMENTS

- [Asset Tracker] Improve external RTC accuracy

### BUGFIXES

- [Asset Tracker] Hardware flow control should not be disable for revision 0 devices
- [Asset Tracker] Suspend program/erase opertion when QSPI flash init gives timeout error

## 1.5.2

### BUGFIXES

- [Boron / B SoM] LTE-M1 (SARA R4) radio unresponsive when sending certain amount of data within a period of time [#2100](https://github.com/particle-iot/device-os/pull/2100) [#2105](https://github.com/particle-iot/device-os/pull/2105)
- [B5 SoM] Suspend program/erase opertion when QSPI flash init gives timeout error

### ENHANCEMENTS

- [Boron/B SoM] Forward compatibility with 460800 baudrate on SARA R4-based devices to support >= 2.x downgrades [#2104](https://github.com/particle-iot/device-os/pull/2104)
- [Boron/B SoM] Enables hardware flow control on SARA R4-based devices with appropriate modem firmware version [#2079](https://github.com/particle-iot/device-os/pull/2079) [cc7adb7c46ea82383a5f948ea1dc898270a27a3c](https://github.com/particle-iot/device-os/commit/cc7adb7c46ea82383a5f948ea1dc898270a27a3c)

## 1.5.1

### ENHANCEMENTS

- [Argon] Increase the size of the ARP queue [#2075](https://github.com/particle-iot/device-os/pull/2075)
- [Cellular] Increase registration timeout to 10 minutes [#2072](https://github.com/particle-iot/device-os/pull/2072)
- [Electron / Boron] Make sure that SARA R4-based LTE devices report LTE Cat M1 access technology [#2083](https://github.com/particle-iot/device-os/pull/2083)
- [B5 SoM] Enable hardware flow control on Quectel modems earlier in the initialization process [#2069](https://github.com/particle-iot/device-os/pull/2069) [#2071](https://github.com/particle-iot/device-os/pull/2071)
- [Vitals] Add CoAP transmitted, retransmitted counter diagnostics [#2043](https://github.com/particle-iot/device-os/pull/2043)
- [Vitals] Add CoAP latency diagnostic [#2050](https://github.com/particle-iot/device-os/pull/2050)
- [Cellular] Increase registration timeout to 10 minutes [#2072](https://github.com/particle-iot/device-os/pull/2072)
- Remove locking in some of the SPI and I2C APIs. SPI perofrmance tests [#2088](https://github.com/particle-iot/device-os/pull/2088)
- `Particle.process()` no-op when called from custom threads [#2085](https://github.com/particle-iot/device-os/pull/2085)
- [Electron] Minor log message changes when the modem is not responsive [#2087](https://github.com/particle-iot/device-os/pull/2087)

### BUGFIXES

- [wiring] Resolves an incompatibility around usage of `SPIClass` [#2095](https://github.com/particle-iot/device-os/pull/2095)
- [Cellular] Fixes an issue with CGI (Cellular Global Identity) not available on some devices [#2067](https://github.com/particle-iot/device-os/pull/2067)
- [Electron] Compatibility CellularSignal `rssi` and `qual` values generation for LTE devices using `AT+UCGED` [#2070](https://github.com/particle-iot/device-os/pull/2070)
- [B5 SoM / Asset Tracker] Zero RSRP/RSRQ values from `AT+QCSQ` treated as errors [#2078](https://github.com/particle-iot/device-os/pull/2078)
- [wiring] BLE: use `delete` for objects allocated with `new` [#2081](https://github.com/particle-iot/device-os/pull/2081)
- [Gen 3] I2C HAL recovers the I2C bus on transmission errors [#2084](https://github.com/particle-iot/device-os/pull/2084)
- [Gen 3] Devices should be able to enter STANDBY / Hibernate sleep mode without specifying any wake-up sources [#2086](https://github.com/particle-iot/device-os/pull/2086)
- [Gen 3] Fix an issue with `platform_user_ram.ld` generation when the application path contains 'data' [#2090](https://github.com/particle-iot/device-os/pull/2090)
- [wiring] Fix `SPI` macros polluting global namespace [#2089](https://github.com/particle-iot/device-os/pull/2089)
- [system] `firmware_update_failed` and `firmware_update_complete` are mixed up [71a8eb56f8d34efbda46f5d547c92e32a42a2148](https://github.com/particle-iot/device-os/commit/71a8eb56f8d34efbda46f5d547c92e32a42a2148)
- [system] Do not propagate non-critical network errors to communication layer [ba6c9d863ed79cd036848329c3b89b3c0e84dbd0](https://github.com/particle-iot/device-os/commit/ba6c9d863ed79cd036848329c3b89b3c0e84dbd0)
- [Gen 3] Fix a crash in `netif_ext_callback_handler()` when parsing unset event fields [dd01b12ea6166b6b175c46348a2b4c180d779ead](https://github.com/particle-iot/device-os/commit/dd01b12ea6166b6b175c46348a2b4c180d779ead)


## 1.5.1-rc.1

### ENHANCEMENTS

- [Argon] Increase the size of the ARP queue [#2075](https://github.com/particle-iot/device-os/pull/2075)
- [Cellular] Increase registration timeout to 10 minutes [#2072](https://github.com/particle-iot/device-os/pull/2072)
- [Electron / Boron] Make sure that SARA R4-based LTE devices report LTE Cat M1 access technology [#2083](https://github.com/particle-iot/device-os/pull/2083)
- [B5 SoM] Enable hardware flow control on Quectel modems earlier in the initialization process [#2069](https://github.com/particle-iot/device-os/pull/2069) [#2071](https://github.com/particle-iot/device-os/pull/2071)
- [Vitals] Add CoAP transmitted, retransmitted counter diagnostics [#2043](https://github.com/particle-iot/device-os/pull/2043)
- [Vitals] Add CoAP latency diagnostic [#2050](https://github.com/particle-iot/device-os/pull/2050)
- [Cellular] Increase registration timeout to 10 minutes [#2072](https://github.com/particle-iot/device-os/pull/2072)
- Remove locking in some of the SPI and I2C APIs. SPI perofrmance tests [#2088](https://github.com/particle-iot/device-os/pull/2088)
- `Particle.process()` no-op when called from custom threads [#2085](https://github.com/particle-iot/device-os/pull/2085)
- [Electron] Minor log message changes when the modem is not responsive [#2087](https://github.com/particle-iot/device-os/pull/2087)

### BUGFIXES

- [Cellular] Fixes an issue with CGI (Cellular Global Identity) not available on some devices [#2067](https://github.com/particle-iot/device-os/pull/2067)
- [Electron] Compatibility CellularSignal `rssi` and `qual` values generation for LTE devices using `AT+UCGED` [#2070](https://github.com/particle-iot/device-os/pull/2070)
- [B5 SoM / Asset Tracker] Zero RSRP/RSRQ values from `AT+QCSQ` treated as errors [#2078](https://github.com/particle-iot/device-os/pull/2078)
- [wiring] BLE: use `delete` for objects allocated with `new` [#2081](https://github.com/particle-iot/device-os/pull/2081)
- [Gen 3] I2C HAL recovers the I2C bus on transmission errors [#2084](https://github.com/particle-iot/device-os/pull/2084)
- [Gen 3] Devices should be able to enter STANDBY / Hibernate sleep mode without specifying any wake-up sources [#2086](https://github.com/particle-iot/device-os/pull/2086)
- [Gen 3] Fix an issue with `platform_user_ram.ld` generation when the application path contains 'data' [#2090](https://github.com/particle-iot/device-os/pull/2090)
- [wiring] Fix `SPI` macros polluting global namespace [#2089](https://github.com/particle-iot/device-os/pull/2089)
- [system] `firmware_update_failed` and `firmware_update_complete` are mixed up [71a8eb56f8d34efbda46f5d547c92e32a42a2148](https://github.com/particle-iot/device-os/commit/71a8eb56f8d34efbda46f5d547c92e32a42a2148)
- [system] Do not propagate non-critical network errors to communication layer [ba6c9d863ed79cd036848329c3b89b3c0e84dbd0](https://github.com/particle-iot/device-os/commit/ba6c9d863ed79cd036848329c3b89b3c0e84dbd0)
- [Gen 3] Fix a crash in `netif_ext_callback_handler()` when parsing unset event fields [dd01b12ea6166b6b175c46348a2b4c180d779ead](https://github.com/particle-iot/device-os/commit/dd01b12ea6166b6b175c46348a2b4c180d779ead)

## 1.5.1-tracker.3

### BUGFIXES

- [Asset Tracker] Use `EXTERNAL_FLASH_SIZE` in address check used before erasing OTA region #287

## 1.5.1-tracker.2

### FEATURES

- [Asset Tracker] Persistent feature flag (`FEATURE_FLAG_LED_OVERRIDDEN`) for taking over control of the RGB LED from the system #285

### ENHANCEMENTS

- [Asset Tracker] Increase SPI clock speed for IO expander #281
- [Argon] Increase the size of the ARP queue [#2075](https://github.com/particle-iot/device-os/pull/2075)
- [Cellular] Increase registration timeout to 10 minutes [#2072](https://github.com/particle-iot/device-os/pull/2072)
- [Electron / Boron] Make sure that SARA R4-based LTE devices report LTE Cat M1 access technology [#2083](https://github.com/particle-iot/device-os/pull/2083)

### BUGFIXES

- [Asset Tracker] Update pinmap #282
- [Asset Tracker] Fix `analogRead()` not functioning #283
- [Asset Tracker] Fix OTA section and filesystem overlap #286
- [Cellular] Fixes an issue with CGI (Cellular Global Identity) not available on some devices [#2067](https://github.com/particle-iot/device-os/pull/2067)
- [Electron] Compatibility CellularSignal `rssi` and `qual` values generation for LTE devices using `AT+UCGED` [#2070](https://github.com/particle-iot/device-os/pull/2070)
- [B5 SoM / Asset Tracker] Zero RSRP/RSRQ values from `AT+QCSQ` treated as errors [#2078](https://github.com/particle-iot/device-os/pull/2078)
- [wiring] BLE: use `delete` for objects allocated with `new` [#2081](https://github.com/particle-iot/device-os/pull/2081)
- [Gen 3] I2C HAL recovers the I2C bus on transmission errors [#2084](https://github.com/particle-iot/device-os/pull/2084)
- [Gen 3] Devices should be able to enter STANDBY / Hibernate sleep mode without specifying any wake-up sources [#2086](https://github.com/particle-iot/device-os/pull/2086)

### INTERNAL

- [Asset Tracker] Add `SPI1` stress tests #284

## 1.5.1-tracker.1

### FEATURES

- Platform support for Asset Tracker SoM (`tracker` platform id 26)

## 1.5.0

### DEPRECATION

- Spark Core End-Of-Life. 1.5.x+ releases no longer support Spark Core [#2003](https://github.com/particle-iot/device-os/pull/2003)

### FEATURES

- Introduce configurable I2C buffer [#2022](https://github.com/particle-iot/device-os/pull/2022)
- Support for > 255 I2C buffers and transfers, configurable timeouts [#2035](https://github.com/particle-iot/device-os/pull/2035)
- Thread-safe cloud variables [#1998](https://github.com/particle-iot/device-os/pull/1998)
- Sleep 2.0 [#1986](https://github.com/particle-iot/device-os/pull/1986)
- Enables C++14 chrono string literals for wiring APIs [#1709](https://github.com/particle-iot/device-os/pull/1709)
- [Gen 3] Implements persistent antenna selection (`Mesh.selectAntenna()`) [#1933](https://github.com/particle-iot/device-os/pull/1933)
- GCC 8 support [#1971](https://github.com/particle-iot/device-os/pull/1971)
- [B5 SoM] B5 SoM platform support
- [Boron/Electron/B5 SoM] Reverts default PMIC settings changes, adds higher level power configuration API [#1992](https://github.com/particle-iot/device-os/pull/1992)

### ENHANCEMENTS

- Replace AT+CSQ usage on Quectel-based devices with AT+QCSQ [147f654](https://github.com/particle-iot/device-os/commit/147f65472596017176498db4edec2dcc72c67215)
- Replace AT+CSQ/CESQ usage on ublox-based LTE devices with AT+UCGED [#2033](https://github.com/particle-iot/device-os/pull/2033) [#2038](https://github.com/particle-iot/device-os/pull/2038)
- [wiring] Specify floating precision in JSON library [#2054](https://github.com/particle-iot/device-os/pull/2054)
- Expose full concurrent HAL APIs to the application [#2052](https://github.com/particle-iot/device-os/pull/2052)
- Close cloud socket on communication errors on all platforms except for Electron [#2056](https://github.com/particle-iot/device-os/pull/2056)
- [B5 SoM] Enable hardware flow control for Quectel modem during runtime and add support for EG91-EX [#2042](https://github.com/particle-iot/device-os/pull/2042)
- Enables Workbench DeviceOS local development (deviceOS@source) [#1957](https://github.com/particle-iot/device-os/pull/1957)
- [Gen 3/Cellular] Enables logging of modem AT commands and data transmissions by default in modular builds [#1994](https://github.com/particle-iot/device-os/pull/1994)
- [Gen 2] Removes write protection enforcement of system firmware on boot [#2009](https://github.com/particle-iot/device-os/pull/2009)
- [Cellular] Adds `SystemPowerFeature::DISABLE` to disable system power management [#2015](https://github.com/particle-iot/device-os/pull/2015)

### BUGFIXES

- [Boron/B SoM/B5 SoM] Fix Cellular Global Identity generation [#2062](https://github.com/particle-iot/device-os/pull/2062)
- [B5 SoM] Strip padding `F` from ICCID [4aa1746](https://github.com/particle-iot/device-os/commit/4aa1746dd0f8fbffbca4e1ca898e609a738a756d)
- Fix 3G signal strength calculation on ublox-based cellular devices [553af7b](https://github.com/particle-iot/device-os/commit/553af7b89d969517b02c8a3b3ef4875a31f2cf27)
- Only increment unacknowledged counter for requests, not acknowledgements [#2046](https://github.com/particle-iot/device-os/pull/2046)
- Fix YModem transfer regression introduced in 1.5.0-rc.2 [#2051](https://github.com/particle-iot/device-os/pull/2051)
- [wiring] Fixes a memory leakage in `Mutex` and `RecursiveMutex` classes [#2053](https://github.com/particle-iot/device-os/pull/2053)
- [B5 SoM] Fixes a hardfault when Ethernet is enabled [#2057](https://github.com/particle-iot/device-os/pull/2057)
- [P1] Enable power save clock (32KHz) only if not disabled by feature flag [#2058](https://github.com/particle-iot/device-os/pull/2058)
- [wiring] Fixes multiple conflicting candidates when calling `Wire.requestFrom()` [#2059](https://github.com/particle-iot/device-os/pull/2059)
- [Gen 2] Fix `SPI.beginTransaction()` deadlocking [#2060](https://github.com/particle-iot/device-os/pull/2060)
- [Gen 3] Fix a race condition in NCP client when the GSM0710 muxer connection to the NCP is abruptly broken [#2049](https://github.com/particle-iot/device-os/pull/2049)
- [Workbench] Workaround for failing Device OS builds on Windows [#2037](https://github.com/particle-iot/device-os/pull/2037)
- [Gen 2] platform: fix `FLASH_Update()` for unaligned writes [#2036](https://github.com/particle-iot/device-os/pull/2036)
- Fixes the issue with SPI default SS pin being not configured [#2039](https://github.com/particle-iot/device-os/pull/2039)
- [gcc] fixes builds on osx and enables usage of the latest gcc versions [#2041](https://github.com/particle-iot/device-os/pull/2041)
- [Gen 3] Fixes renegotiation of options and adds IPCP packet parsing [#2029](https://github.com/particle-iot/device-os/pull/2029)
- [Gen 3] Fix parsing of large AT command responses [#2017](https://github.com/particle-iot/device-os/pull/2017)
- String::substring() unnecessarily modifies the source buffer [#2026](https://github.com/particle-iot/device-os/pull/2026)
- [B5 SoM] Fix CGI for Quectel modems [#2019](https://github.com/particle-iot/device-os/pull/2019)
- [Gen 2] Fixes socket leak in `UDP.begin()` [#2031](https://github.com/particle-iot/device-os/pull/2031)
- [Gen 3] Fixes `Serial.read()` returning -1 instead of 0 when there's no data to read [#2034](https://github.com/particle-iot/device-os/pull/2034)
- [Gen 3] Fixes BLE UUID conversion issue [#1997](https://github.com/particle-iot/device-os/pull/1997)
- [wiring] Fixes a regression in SPI due to thread-safety additions from [#1879](https://github.com/particle-iot/device-os/pull/1879) [#2023](https://github.com/particle-iot/device-os/pull/2023)
- [Boron/BSoM] Enables advance SETUP/MODE button features (single click - RSSI, double click - soft poweroff) [#1918](https://github.com/particle-iot/device-os/pull/1918)
- Fixes a call to objdump when the compiler is not in PATH [#1961](https://github.com/particle-iot/device-os/pull/1961)
- [gcc] Ensure GPIO pinmap is initialized on first use [#1963](https://github.com/particle-iot/device-os/pull/1963)
- [Gen 3] Fixes too many connection attempts with deactivated SIM [#1969](https://github.com/particle-iot/device-os/pull/1969)
- [Gen 3] Fixes NCP issue with devices stuck in a seemingly connected state [#1980](https://github.com/particle-iot/device-os/pull/1980)
- [Photon/P1] Fixes a hardfault when calling certain `WiFI` class methods with WICED networking stack uninitialized [#1949](https://github.com/particle-iot/device-os/pull/1949)
- [Gen 3] Fixes `.particle/toolchains/gcc-arm/5.3.1/bin/objdump: not found` issues in Workbench [#1996](https://github.com/particle-iot/device-os/pull/1996)
- [B5 SoM] Temporarily disables hardware flow control [#1999](https://github.com/particle-iot/device-os/pull/1999)
- [Gen 3] Fixes retained variables first-time initialization [#2001](https://github.com/particle-iot/device-os/pull/2001)
- [B5 SoM] Removes unsupported `AT+CGDCONT` call with `CHAP:<APN>` [#2006](https://github.com/particle-iot/device-os/pull/2006)
- [Boron] Increases `AT+COPS=0` timeout, lowers baudrate to 115200 [#2008](https://github.com/particle-iot/device-os/pull/2008)
- Prevents expansion of `EXTRA_CFLAGS` variable [#2012](https://github.com/particle-iot/device-os/pull/2012)
- [Gen 3] Workaround for QSPI/XIP nRF52840 hardware anomaly 215 [#2010](https://github.com/particle-iot/device-os/pull/2010)
- Fixes a potential buffer overflow in `Time.format()` [#1973](https://github.com/particle-iot/device-os/pull/1973)
- [Gen 2] Clear WLAN_WD when `disconnect()` is called [#2016](https://github.com/particle-iot/device-os/pull/2016)
- [Gen 3] Fixes concurrency/thread-safety SPI issues with Ethernet Featherwing [#1879](https://github.com/particle-iot/device-os/pull/1879)

### INTERNAL

- [test] Removes A6 pin from PWM tests on B5 SoM [d9c859c](https://github.com/particle-iot/device-os/commit/d9c859c3b26b4d416104930e9c37ab95ec6112fa)
- Fix building of boost on macOS [#2044](https://github.com/particle-iot/device-os/pull/2044)
- [ci] Workaround GitHub rate limiting by using github.com/xxx/yyy/tarball instead of api.github.com
- [test] unit: fixes Print tests failing [#2027](https://github.com/particle-iot/device-os/pull/2027)
- [B5 SoM] Change the prefix of BLE broadcasting name for B5SoM [#2030](https://github.com/particle-iot/device-os/pull/2030)
- update workbench manifest with latest `buildscripts` dependency [#2020](https://github.com/particle-iot/device-os/pull/2020)
- Increase `VitalsPublisher` test coverage [#1920](https://github.com/particle-iot/device-os/pull/1920)
- Adds missing platforms to manifest.json [#1959](https://github.com/particle-iot/device-os/pull/1959)
- Add support for control requests sent by the test runner [#1987](https://github.com/particle-iot/device-os/pull/1987)
- Add support for chunked transfer of USB request/reply data [#1991](https://github.com/particle-iot/device-os/pull/1991)
- Implements `EnumFlags` class for bitwise operations on C++ enum classes [#1978](https://github.com/particle-iot/device-os/pull/1933)
- Improves cellular test coverage [#1930](https://github.com/particle-iot/device-os/pull/193)
- [Gen 3] Frees up some flash space in system-part1 [#2000](https://github.com/particle-iot/device-os/pull/2000)
- [Cellular] Internal function to modify cellular registration timeout [#2007](https://github.com/particle-iot/device-os/pull/2007) [#2014](https://github.com/particle-iot/device-os/pull/2014)
- Removes xsom platform [#2011](https://github.com/particle-iot/device-os/pull/2011)

## 1.5.0-rc.2

### FEATURES

- Introduce configurable I2C buffer [#2022](https://github.com/particle-iot/device-os/pull/2022)
- Support for > 255 I2C buffers and transfers, configurable timeouts [#2035](https://github.com/particle-iot/device-os/pull/2035)
- Thread-safe cloud variables [#1998](https://github.com/particle-iot/device-os/pull/1998)

### ENHANCEMENTS

- [B5 SoM] Enable hardware flow control for Quectel modem during runtime and add support for EG91-EX [#2042](https://github.com/particle-iot/device-os/pull/2042)

### BUGFIXES

- [Workbench] Workaround for failing Device OS builds on Windows [#2037](https://github.com/particle-iot/device-os/pull/2037)
- [Gen 2] platform: fix `FLASH_Update()` for unaligned writes [#2036](https://github.com/particle-iot/device-os/pull/2036)
- Fixes the issue with SPI default SS pin being not configured [#2039](https://github.com/particle-iot/device-os/pull/2039)
- [gcc] fixes builds on osx and enables usage of the latest gcc versions [#2041](https://github.com/particle-iot/device-os/pull/2041)
- [Gen 3] Fixes renegotiation of options and adds IPCP packet parsing [#2029](https://github.com/particle-iot/device-os/pull/2029)
- [Gen 3] Fix parsing of large AT command responses [#2017](https://github.com/particle-iot/device-os/pull/2017)
- String::substring() unnecessarily modifies the source buffer [#2026](https://github.com/particle-iot/device-os/pull/2026)
- [B5 SoM] Fix CGI for Quectel modems [#2019](https://github.com/particle-iot/device-os/pull/2019)
- [Gen 2] Fixes socket leak in `UDP.begin()` [#2031](https://github.com/particle-iot/device-os/pull/2031)
- [Gen 3] Fixes `Serial.read()` returning -1 instead of 0 when there's no data to read [#2034](https://github.com/particle-iot/device-os/pull/2034)
- [Gen 3] Fixes BLE UUID conversion issue [#1997](https://github.com/particle-iot/device-os/pull/1997)
- [wiring] Fixes a regression in SPI due to thread-safety additions from [#1879](https://github.com/particle-iot/device-os/pull/1879) [#2023](https://github.com/particle-iot/device-os/pull/2023)

### INTERNAL

- [test] unit: fixes Print tests failing [#2027](https://github.com/particle-iot/device-os/pull/2027)
- [B5 SoM] Change the prefix of BLE broadcasting name for B5SoM [#2030](https://github.com/particle-iot/device-os/pull/2030)
- update workbench manifest with latest `buildscripts` dependency [#2020](https://github.com/particle-iot/device-os/pull/2020)

## 1.5.0-rc.1

### DEPRECATION

- Spark Core End-Of-Life. 1.5.x+ releases no longer support Spark Core [#2003](https://github.com/particle-iot/device-os/pull/2003)

### FEATURES

- Sleep 2.0 [#1986](https://github.com/particle-iot/device-os/pull/1986)
- Enables C++14 chrono string literals for wiring APIs [#1709](https://github.com/particle-iot/device-os/pull/1709)
- [Gen 3] Implements persistent antenna selection (`Mesh.selectAntenna()`) [#1933](https://github.com/particle-iot/device-os/pull/1933)
- GCC 8 support [#1971](https://github.com/particle-iot/device-os/pull/1971)
- [B5 SoM] B5 SoM platform support
- [Boron/Electron/B5 SoM] Reverts default PMIC settings changes, adds higher level power configuration API [#1992](https://github.com/particle-iot/device-os/pull/1992)

### ENHANCEMENTS

- Enables Workbench DeviceOS local development (deviceOS@source) [#1957](https://github.com/particle-iot/device-os/pull/1957)
- [Gen 3/Cellular] Enables logging of modem AT commands and data transmissions by default in modular builds [#1994](https://github.com/particle-iot/device-os/pull/1994)
- [Gen 2] Removes write protection enforcement of system firmware on boot [#2009](https://github.com/particle-iot/device-os/pull/2009)
- [Cellular] Adds `SystemPowerFeature::DISABLE` to disable system power management [#2015](https://github.com/particle-iot/device-os/pull/2015)

### BUGFIXES

- [Boron/BSoM] Enables advance SETUP/MODE button features (single click - RSSI, double click - soft poweroff) [#1918](https://github.com/particle-iot/device-os/pull/1918)
- Fixes a call to objdump when the compiler is not in PATH [#1961](https://github.com/particle-iot/device-os/pull/1961)
- [gcc] Ensure GPIO pinmap is initialized on first use [#1963](https://github.com/particle-iot/device-os/pull/1963)
- [Gen 3] Fixes too many connection attempts with deactivated SIM [#1969](https://github.com/particle-iot/device-os/pull/1969)
- [Gen 3] Fixes NCP issue with devices stuck in a seemingly connected state [#1980](https://github.com/particle-iot/device-os/pull/1980)
- [Photon/P1] Fixes a hardfault when calling certain `WiFI` class methods with WICED networking stack uninitialized [#1949](https://github.com/particle-iot/device-os/pull/1949)
- [Gen 3] Fixes `.particle/toolchains/gcc-arm/5.3.1/bin/objdump: not found` issues in Workbench [#1996](https://github.com/particle-iot/device-os/pull/1996)
- [B5 SoM] Temporarily disables hardware flow control [#1999](https://github.com/particle-iot/device-os/pull/1999)
- [Gen 3] Fixes retained variables first-time initialization [#2001](https://github.com/particle-iot/device-os/pull/2001)
- [B5 SoM] Removes unsupported `AT+CGDCONT` call with `CHAP:<APN>` [#2006](https://github.com/particle-iot/device-os/pull/2006)
- [Boron] Increases `AT+COPS=0` timeout, lowers baudrate to 115200 [#2008](https://github.com/particle-iot/device-os/pull/2008)
- Prevents expansion of `EXTRA_CFLAGS` variable [#2012](https://github.com/particle-iot/device-os/pull/2012)
- [Gen 3] Workaround for QSPI/XIP nRF52840 hardware anomaly 215 [#2010](https://github.com/particle-iot/device-os/pull/2010)
- Fixes a potential buffer overflow in `Time.format()` [#1973](https://github.com/particle-iot/device-os/pull/1973)
- [Gen 2] Clear WLAN_WD when `disconnect()` is called [#2016](https://github.com/particle-iot/device-os/pull/2016)
- [Gen 3] Fixes concurrency/thread-safety SPI issues with Ethernet Featherwing [#1879](https://github.com/particle-iot/device-os/pull/1879)

### INTERNAL

- Increase `VitalsPublisher` test coverage [#1920](https://github.com/particle-iot/device-os/pull/1920)
- Adds missing platforms to manifest.json [#1959](https://github.com/particle-iot/device-os/pull/1959)
- Add support for control requests sent by the test runner [#1987](https://github.com/particle-iot/device-os/pull/1987)
- Add support for chunked transfer of USB request/reply data [#1991](https://github.com/particle-iot/device-os/pull/1991)
- Implements `EnumFlags` class for bitwise operations on C++ enum classes [#1978](https://github.com/particle-iot/device-os/pull/1933)
- Improves cellular test coverage [#1930](https://github.com/particle-iot/device-os/pull/193)
- [Gen 3] Frees up some flash space in system-part1 [#2000](https://github.com/particle-iot/device-os/pull/2000)
- [Cellular] Internal function to modify cellular registration timeout [#2007](https://github.com/particle-iot/device-os/pull/2007) [#2014](https://github.com/particle-iot/device-os/pull/2014)
- Removes xsom and asom platforms [#2011](https://github.com/particle-iot/device-os/pull/2011)

### DOCUMENTATION

- Documents adding custom CFLAGS during DeviceOS application build [#2004](https://github.com/particle-iot/device-os/pull/2004)

## 1.4.5-b5som.2

### BUGFIXES

- [B5SoM] ncp: lowers the baudrate to 115200 and explicitly enables flow control with AT+IFC=2,2

## 1.4.5-b5som.1

### FEATURES

- [B5SoM] Initial support for the platform.

## 1.4.4

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>**Note:** If your Gen 3 device does not have a Cloud connection, this release will also require manually updating the SoftDevice via CLI. The instructions are available in the release notes and the SoftDevice binaries are available in the Github release.
>
>This release contains v501 bootloaders.

### BUGFIXES

- [Photon/P1] Fixes handling of invalid WiFi access point config entries in DCT [#1976](https://github.com/particle-iot/device-os/pull/1976)


## 1.4.3

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>**Note:** If your Gen 3 device does not have a Cloud connection, this release will also require manually updating the SoftDevice via CLI. The instructions are available in the release notes and the SoftDevice binaries are available in the Github release.
>
>This release contains v501 bootloaders.

### BUGFIXES

- [Photon/P1/Electron] Fixes thread-safety issues with non-reentrant C standard library functions requiring temporary thread-local storage [#1970](https://github.com/particle-iot/device-os/pull/1970)


## 1.4.2

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>**Note:** If your Gen 3 device does not have a Cloud connection, this release will also require manually updating the SoftDevice via CLI. The instructions are available in the release notes and the SoftDevice binaries are available in the Github release.
>
>This release contains v501 bootloaders.

### BUGFIXES

- [Gen 3] Fixes a boot/crash-loop due to a `POWER_CLOCK_IRQn` firing in between the jump into the system firmware from the bootloader [#1948](https://github.com/particle-iot/device-os/pull/1948)

## 1.4.1

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>**Note:** If your Gen 3 device does not have a Cloud connection, this release will also require manually updating the SoftDevice via CLI. The instructions are available in the release notes and the SoftDevice binaries are available in the Github release.
>
>This release contains v500 bootloaders.

### FEATURES

- [Gen 2] Adds support for serial number, NCP type and mobile secret in STM32F2 OTP area [#1927](https://github.com/particle-iot/device-os/pull/1927) [#1931](https://github.com/particle-iot/device-os/pull/1931/files)

### ENHANCEMENTS

- Implements a command to reset all network interfaces of a device remotely [#1943](https://github.com/particle-iot/device-os/pull/1943)
- [Electron] Disables usage of `AT+UDNSRN` on 2G/3G Electrons in favor of a standalone socket-based DNS client [#1940](https://github.com/particle-iot/device-os/pull/1940)
- [Electron, Boron] Increase the PMIC input current limit from 900mA to 1500mA and limit charging current to 896mA to allow 2G/3G devices to function when powered by sufficient power supply through VIN without the battery [#1921](https://github.com/particle-iot/device-os/pull/1921)
- DTLS handshake timeout increased to 24 seconds to allow 3 retransmission attempts [#1914](https://github.com/particle-iot/device-os/pull/1914)
- [Electron] Enables modem logs in non-debug builds [#1941](https://github.com/particle-iot/device-os/pull/1941)

### BUGFIXES

- [Photon / P1] Limits maximum TLS version to TLS1.1 for WPA Enterprise authentication as TLS1.2 seems to be broken in WICED 3.7.0-7 [#1945](https://github.com/particle-iot/device-os/pull/1945)
- [Boron] Fixes `attachInterrupt(D7, ...)` not working due to a constraint introduced previously for cellular devices (Electron) [#1939](https://github.com/particle-iot/device-os/pull/1939) [#1944](https://github.com/particle-iot/device-os/pull/1944)
- Send safe mode event when the session is resumed [#1935](https://github.com/particle-iot/device-os/pull/1935)
- [LTE, u-blox] adds a mitigation to keep DNS Client responsive ch38990 [#1938](https://github.com/particle-iot/device-os/pull/1938)
- [Electron] Fixes RSSI failing due to Power Saving mode active [#1917](https://github.com/particle-iot/device-os/pull/1917) [#1892](https://github.com/particle-iot/device-os/pull/1892)
- [Gen 3] Fixes memory leak when scanning for BLE devices [#1929](https://github.com/particle-iot/device-os/pull/1929) [#1926](https://github.com/particle-iot/device-os/pull/1926)
- [Gen 3] Fixes reporting of discovered BLE peer characteristic descriptors [#1916](https://github.com/particle-iot/device-os/pull/1916)
- [Gen 3] Fixes `BleCharacteristic::setValue()` with default `BLeTxRxType` argument (`BleTxRxType::AUTO`) for characteristics with `WRITE` property [#1915](https://github.com/particle-iot/device-os/pull/1915) [#1913](https://github.com/particle-iot/device-os/issues/1913) [#1924](https://github.com/particle-iot/device-os/issues/1924)


### INTERNAL

- [test] i2c_mcp23017: implements HighPriorityInterruptInterferer for nRF52840-based platforms [#1947](https://github.com/particle-iot/device-os/pull/1947)
- [LTE] Removes log noise by closing only untracked socket handles that are open ch37610 [#1938](https://github.com/particle-iot/device-os/pull/1938)
- [ci] Buildpack builder updated to 0.0.8 to move buildpack builds off of Travis
- [docs] Adds Artifact Versioning and Tagging (1.0.0) artifacts.md [#1703](https://github.com/particle-iot/device-os/pull/1703)


## 1.4.1-rc.1

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>**Note:** If your Gen 3 device does not have a Cloud connection, this release will also require manually updating the SoftDevice via CLI. The instructions are available in the release notes and the SoftDevice binaries are available in the Github release.
>
>This release contains v500 bootloaders.

### FEATURES

- [Gen 2] Adds support for serial number, NCP type and mobile secret in STM32F2 OTP area [#1927](https://github.com/particle-iot/device-os/pull/1927) [#1931](https://github.com/particle-iot/device-os/pull/1931/files)

### ENHANCEMENTS

- [Electron, Boron] Increase the PMIC input current limit from 900mA to 1500mA and limit charging current to 896mA to allow 2G/3G devices to function when powered by sufficient power supply through VIN without the battery [#1921](https://github.com/particle-iot/device-os/pull/1921)
- DTLS handshake timeout increased to 24 seconds to allow 3 retransmission attempts [#1914](https://github.com/particle-iot/device-os/pull/1914)

### BUGFIXES

- [Electron] Fixes RSSI failing due to Power Saving mode active [#1917](https://github.com/particle-iot/device-os/pull/1917) [#1892](https://github.com/particle-iot/device-os/pull/1892)
- [Gen 3] Fixes memory leak when scanning for BLE devices [#1929](https://github.com/particle-iot/device-os/pull/1929) [#1926](https://github.com/particle-iot/device-os/pull/1926)
- [Gen 3] Fixes reporting of discovered BLE peer characteristic descriptors [#1916](https://github.com/particle-iot/device-os/pull/1916)
- [Gen 3] Fixes `BleCharacteristic::setValue()` with default `BLeTxRxType` argument (`BleTxRxType::AUTO`) for characteristics with `WRITE` property [#1915](https://github.com/particle-iot/device-os/pull/1915) [#1913](https://github.com/particle-iot/device-os/issues/1913) [#1924](https://github.com/particle-iot/device-os/issues/1924)

### INTERNAL

- [ci] Buildpack builder updated to unreleased branch `feature/buildpack-runnable-without-travis` to move buildpack builds off of Travis
- [docs] Adds Artifact Versioning and Tagging (1.0.0) artifacts.md [#1703](https://github.com/particle-iot/device-os/pull/1703)

## 1.4.0

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>**Note:** If your Gen 3 device does not have a Cloud connection, this release will also require manually updating the SoftDevice via CLI. The instructions are available in the release notes and the SoftDevice binaries are available in the Github release.
>
>This release contains v500 bootloaders.

### FEATURES

- [Gen 3] Introduces BLE.begin() and BLE.end() wiring APIs [#1890](https://github.com/particle-iot/device-os/pull/1890)
- [Gen 3] Exposes POSIX `select()` and `poll()` from socket HAL dynalib [#1895](https://github.com/particle-iot/device-os/pull/1895)

### ENHANCEMENTS

- [Gen 3] Adds `BleCharacteristic::setValue(..., BleTxRxType)` API to send data with or without acknowledgement [#1901](https://github.com/particle-iot/device-os/pull/1901)
- [Gen 3] Alternately broadcast user's and Particle-specific BLE advertising data when the device is in the Listening mode [#1882](https://github.com/particle-iot/device-os/pull/1882)
- Updates FreeRTOS from [10.0.1](https://github.com/particle-iot/freertos/commit/3feb84fee1840c0a8a3ea50810fb5f3e7527c6ce) to [10.2.1](https://github.com/particle-iot/freertos/commit/c4e1510c832f9467169f2ab7165ec49d3ce7428a). [Changelog](https://gist.github.com/avtolstoy/2f4b147f3a678bef75afde818a8ff77c) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates Catch2 from [2.4.2](https://github.com/particle-iot/catch2/commit/03d122a35c3f5c398c43095a87bc82ed44642516) to [2.9.1](https://github.com/particle-iot/catch2/commit/2f631bb8087a0355d2b23a75a28d936ce237659d). [Changelog](https://gist.github.com/avtolstoy/4b628c894798b4d0b3617860030de788) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates FakeIt from [2.0.5-7-g362271d](https://github.com/particle-iot/fakeit/commit/362271de8f59178aaf12fc0c27de1a814ee5a98d) to [2.0.5-13-g317419c](https://github.com/particle-iot/fakeit/317419c2e2f5a98e023a4d62628eb149fe3d3c3a). [Changelog](https://gist.github.com/avtolstoy/63742d7e03949a651bc74d240f903113) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates LittleFS from [1.6.0](https://github.com/particle-iot/littlefs/commit/9996634a148c68f6135318cce0f69b9debf73469) to [1.7.2](https://github.com/particle-iot/littlefs/commit/ed07f602fbfa5e9bd905829997436c607f10837a). [Changelog](https://gist.github.com/avtolstoy/1af0fb67f6e15d1b7711a49759cbdfb9). [Fork diff](https://github.com/particle-iot/littlefs/compare/7e110b44c0e796dc56e2fe86587762d685653029...ed07f602fbfa5e9bd905829997436c607f10837a) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates LwIP from [2.0.5](https://github.com/particle-iot/lwip/commit/362271de8f59178aaf12fc0c27de1a814ee5a98d) to [2.1.2](https://github.com/particle-iot/lwip/commit/4fe04959e5665dc58cb2552f750d82e257aab87d). [Changelog](https://gist.github.com/avtolstoy/1a5d6bf832451ce4329151eb2995ce0f). [Fork diff](https://github.com/particle-iot/lwip/compare/bd116cd6d9627ebdae41bb061a6e39cbd7909e60...4fe04959e5665dc58cb2552f750d82e257aab87d) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates LwIP Contrib from [2.0.0](https://github.com/particle-iot/lwip-contrib/commit/cce6cd11ffc1e0bddcb5a9c96674d1a7ae73e36f) to [2.1.2](https://github.com/particle-iot/lwip-contrib/commit/35b011d4cf4c4b480f8859c456587a884ec9d287). [Changelog](https://gist.github.com/avtolstoy/4cb3f7ae77ec359bc797528ab1a98412) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates MbedTLS from [2.9.0](https://github.com/particle-iot/mbedtls/commit/48fe5b1030039557c6a6340d22bb473fc8042920) to [2.16.2](https://github.com/particle-iot/mbedtls/commit/4e4e631f48e72448213ac340e172ea8442dc442b). [Changelog](https://gist.github.com/avtolstoy/50db3f7c10c5479f5fd9fa817b7df693). [Fork diff](https://github.com/particle-iot/mbedtls/compare/a8a2d73d794ff28b2079ca9ccbf98bb0e97cb3b3...4e4e631f48e72448213ac340e172ea8442dc442b) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates miniz from [2.0.7](https://github.com/particle-iot/miniz/commit/f07041c88cdbb5a85401a0f49366cac2143871d8) to [2.1.0](https://github.com/particle-iot/miniz/commit/af839bf788cec53c0d72f22e46cf58701498e264). [Changelog](https://gist.github.com/avtolstoy/c22df11f316203fc1857d039802525de). [Fork diff](https://github.com/particle-iot/miniz/compare/be54fea1c66154ad98a01b890a17043858837d26...af839bf788cec53c0d72f22e46cf58701498e264) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates nanopb from [0.3.9](https://github.com/particle-iot/nanopb/commit/2d40a90db76036458cf1150cdf5979e5e5fc77c6) to [0.3.9.3](https://github.com/particle-iot/nanopb/commit/6b91cc53dfb53ff5cd34bd2e057fbfc8ae7f12eb). [Changelog](https://gist.github.com/avtolstoy/96f3078e4076129a6cb62be69f62c287). [Fork diff](https://github.com/particle-iot/nanopb/compare/74171dee87de0c2465fd9cef1d8b296f8cb17746...6b91cc53dfb53ff5cd34bd2e057fbfc8ae7f12eb) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates OpenThread from [20190130](https://github.com/particle-iot/openthread/commit/0eab4ecddd8208741ae5856275e2dc2bee8e0838) to [20190709](https://github.com/particle-iot/openthread/commit/1bb328f93b7feba919e81f63ebe5a9745811239a). [Changelog](https://gist.github.com/avtolstoy/1408ced79f33291f51c2a0e9c9f94fdd). [Fork diff](https://github.com/particle-iot/openthread/compare/f2ea476c3afaa7bf51dc71d4a5713510e2b8c10d...1bb328f93b7feba919e81f63ebe5a9745811239a) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates WizNet Ethernet Driver from [20180515](https://github.com/particle-iot/ioLibrary_Driver/commit/53401b1d55b63641f60577e40e6996da59d71fd5) to [20190529](https://github.com/particle-iot/ioLibrary_Driver/commit/890a73fb8beb1ccbd5a43d1f6aee545bd883fb46). [Changelog](https://gist.github.com/avtolstoy/2c5d13ef00886136aefaaeca51af6a72). [Fork diff](https://github.com/particle-iot/ioLibrary_Driver/compare/b5592d446dbed91704d0fde88e7aff748df1887d...d4e78e46259069d02c3383e7792432e12b9c54c1) [#1864](https://github.com/particle-iot/device-os/pull/1864)

### BUGFIXES

- Fixes an issue in `IPAddress::operator bool()` causing the operator to return `false` for valid IPv6 addresses [#1912](https://github.com/particle-iot/device-os/pull/1912)
- Fixes [#1865](https://github.com/particle-iot/device-os/issues/1865), `TCPClient::connect()` return values [#1909](https://github.com/particle-iot/device-os/pull/1909)
- [Gen 2] [LTE] fixes slow to close TCP sockets on SARA-R410M-02B [ch35609] [#1909](https://github.com/particle-iot/device-os/pull/1909)
- [Gen 3] Subscribes both notification and indication if both properties are present in peer BLE characteristic [#1901](https://github.com/particle-iot/device-os/pull/1901)
- [Gen 3] Restricts `BleCharacteristic` templated `getValue()` and `setValue()` arguments to be standard layout [#1901](https://github.com/particle-iot/device-os/pull/1901)
- [Gen 3] `BleUuid` comparison operators are no longer case-sensitive [#1902](https://github.com/particle-iot/device-os/pull/1902)
- [Gen 3] Fixes `BleCharacteristic` constructor template to accept characteristic and service UUID arguments with different types [#1902](https://github.com/particle-iot/device-os/pull/1902)
- Fixes `SerialLogHandler` interfering with the Serial setup console in listening mode [#1909](https://github.com/particle-iot/device-os/pull/1909)
- Fixes a regression introduced in 1.1.0 where the system layer was always sending its handshake messages even if the session was resumed causing increased data usage [#1905](https://github.com/particle-iot/device-os/pull/1905)
- Properly seeds `rand()` in multiple threads including system thread. Fixes ephemeral port allocation in LwIP on Gen 3 platforms [#1905](https://github.com/particle-iot/device-os/pull/1905)
- Control requests that reset the device (e.g. `particle usb dfu`) no longer cause unnecessary reconnection to the cloud [#1905](https://github.com/particle-iot/device-os/pull/1905)
- Initialize user module in monolithic builds [#1905](https://github.com/particle-iot/device-os/pull/1905)
- [Gen 3] Fixes heap and application static RAM overlap introduced in 1.3.0-rc.1 [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Fixes tinker build errors when building with `LOG_SERIAL=y` [#1898](https://github.com/particle-iot/device-os/pull/1898)
- [Gen 3] Restored default BLE device address is incorrect. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- [Gen 3] Read BLE device name might be contracted. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- [Gen 3] operator& of the BleCharacteristicProperty enum class doesn't work as expected. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- [Gen 3] The length of got advertising and scan response data is not updated. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- Fixes dynalib alignment issue when compiling relatively large applications potentially due to an unconfirmed bug in GCC by moving the dynalib into a separate section (`.dynalib`) [#1894](https://github.com/particle-iot/device-os/pull/1894)
- [Gen 3] Fixes incorrect handling of `MODULE_INFO_FLAG_DROP_MODULE_INFO` in the bootloader [#1897](https://github.com/particle-iot/device-os/pull/1897)
- [Gen 3] Adds a dummy suffix to the NCP and SoftDevice modules' module info with unique SHA to cause the communication layer to detect the change in SYSTEM DESCRIBE after NCP or SoftDevice update [#1897](https://github.com/particle-iot/device-os/pull/1897)

### INTERNAL

- Minor enhancements in `wiring/no_fixture` and `wiring/no_fixture_long_running` tests [#1912](https://github.com/particle-iot/device-os/pull/1912)
- [Git] Fixes whitespace issues in `.gitmodules` [#1910](https://github.com/particle-iot/device-os/pull/1910)
- [Gen 3] Adds `wiring/no_fixture_ble`, `wiring/ble_central_peripheral` and `wiring/ble_scanner_broadcaster` on-device tests [#1901](https://github.com/particle-iot/device-os/pull/1901)
- Update release script to include Electron without DEBUG_BUILD=y and tinker-serial-debugging apps for Gen 3 [#1903](https://github.com/particle-iot/device-os/pull/1903)
- [Photon / P1] Changes to support building combined images for the recent releases [#1887](https://github.com/particle-iot/device-os/pull/1887)
- `wiring/no_fixture` test adjustments for Gen 2 and Gen 3 platforms [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Coveralls test coverage reports generated by Travis [#1896](https://github.com/particle-iot/device-os/pull/1896)

## 1.4.0-rc.1

### FEATURES

- [Gen 3] Introduces BLE.begin() and BLE.end() wiring APIs [#1890](https://github.com/particle-iot/device-os/pull/1890)
- [Gen 3] Exposes POSIX `select()` and `poll()` from socket HAL dynalib [#1895](https://github.com/particle-iot/device-os/pull/1895)

### ENHANCEMENTS

- [Gen 3] Alternately broadcast user's and Particle-specific BLE advertising data when the device is in the Listening mode [#1882](https://github.com/particle-iot/device-os/pull/1882)
- Updates FreeRTOS from [10.0.1](https://github.com/particle-iot/freertos/commit/3feb84fee1840c0a8a3ea50810fb5f3e7527c6ce) to [10.2.1](https://github.com/particle-iot/freertos/commit/c4e1510c832f9467169f2ab7165ec49d3ce7428a). [Changelog](https://gist.github.com/avtolstoy/2f4b147f3a678bef75afde818a8ff77c) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates Catch2 from [2.4.2](https://github.com/particle-iot/catch2/commit/03d122a35c3f5c398c43095a87bc82ed44642516) to [2.9.1](https://github.com/particle-iot/catch2/commit/2f631bb8087a0355d2b23a75a28d936ce237659d). [Changelog](https://gist.github.com/avtolstoy/4b628c894798b4d0b3617860030de788) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates FakeIt from [2.0.5-7-g362271d](https://github.com/particle-iot/fakeit/commit/362271de8f59178aaf12fc0c27de1a814ee5a98d) to [2.0.5-13-g317419c](https://github.com/particle-iot/fakeit/317419c2e2f5a98e023a4d62628eb149fe3d3c3a). [Changelog](https://gist.github.com/avtolstoy/63742d7e03949a651bc74d240f903113) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates LittleFS from [1.6.0](https://github.com/particle-iot/littlefs/commit/9996634a148c68f6135318cce0f69b9debf73469) to [1.7.2](https://github.com/particle-iot/littlefs/commit/ed07f602fbfa5e9bd905829997436c607f10837a). [Changelog](https://gist.github.com/avtolstoy/1af0fb67f6e15d1b7711a49759cbdfb9). [Fork diff](https://github.com/particle-iot/littlefs/compare/7e110b44c0e796dc56e2fe86587762d685653029...ed07f602fbfa5e9bd905829997436c607f10837a) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates LwIP from [2.0.5](https://github.com/particle-iot/lwip/commit/362271de8f59178aaf12fc0c27de1a814ee5a98d) to [2.1.2](https://github.com/particle-iot/lwip/commit/4fe04959e5665dc58cb2552f750d82e257aab87d). [Changelog](https://gist.github.com/avtolstoy/1a5d6bf832451ce4329151eb2995ce0f). [Fork diff](https://github.com/particle-iot/lwip/compare/bd116cd6d9627ebdae41bb061a6e39cbd7909e60...4fe04959e5665dc58cb2552f750d82e257aab87d) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates LwIP Contrib from [2.0.0](https://github.com/particle-iot/lwip-contrib/commit/cce6cd11ffc1e0bddcb5a9c96674d1a7ae73e36f) to [2.1.2](https://github.com/particle-iot/lwip-contrib/commit/35b011d4cf4c4b480f8859c456587a884ec9d287). [Changelog](https://gist.github.com/avtolstoy/4cb3f7ae77ec359bc797528ab1a98412) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates MbedTLS from [2.9.0](https://github.com/particle-iot/mbedtls/commit/48fe5b1030039557c6a6340d22bb473fc8042920) to [2.16.2](https://github.com/particle-iot/mbedtls/commit/4e4e631f48e72448213ac340e172ea8442dc442b). [Changelog](https://gist.github.com/avtolstoy/50db3f7c10c5479f5fd9fa817b7df693). [Fork diff](https://github.com/particle-iot/mbedtls/compare/a8a2d73d794ff28b2079ca9ccbf98bb0e97cb3b3...4e4e631f48e72448213ac340e172ea8442dc442b) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates miniz from [2.0.7](https://github.com/particle-iot/miniz/commit/f07041c88cdbb5a85401a0f49366cac2143871d8) to [2.1.0](https://github.com/particle-iot/miniz/commit/af839bf788cec53c0d72f22e46cf58701498e264). [Changelog](https://gist.github.com/avtolstoy/c22df11f316203fc1857d039802525de). [Fork diff](https://github.com/particle-iot/miniz/compare/be54fea1c66154ad98a01b890a17043858837d26...af839bf788cec53c0d72f22e46cf58701498e264) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates nanopb from [0.3.9](https://github.com/particle-iot/nanopb/commit/2d40a90db76036458cf1150cdf5979e5e5fc77c6) to [0.3.9.3](https://github.com/particle-iot/nanopb/commit/6b91cc53dfb53ff5cd34bd2e057fbfc8ae7f12eb). [Changelog](https://gist.github.com/avtolstoy/96f3078e4076129a6cb62be69f62c287). [Fork diff](https://github.com/particle-iot/nanopb/compare/74171dee87de0c2465fd9cef1d8b296f8cb17746...6b91cc53dfb53ff5cd34bd2e057fbfc8ae7f12eb) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates OpenThread from [20190130](https://github.com/particle-iot/openthread/commit/0eab4ecddd8208741ae5856275e2dc2bee8e0838) to [20190709](https://github.com/particle-iot/openthread/commit/1bb328f93b7feba919e81f63ebe5a9745811239a). [Changelog](https://gist.github.com/avtolstoy/1408ced79f33291f51c2a0e9c9f94fdd). [Fork diff](https://github.com/particle-iot/openthread/compare/f2ea476c3afaa7bf51dc71d4a5713510e2b8c10d...1bb328f93b7feba919e81f63ebe5a9745811239a) [#1864](https://github.com/particle-iot/device-os/pull/1864)
- Updates WizNet Ethernet Driver from [20180515](https://github.com/particle-iot/ioLibrary_Driver/commit/53401b1d55b63641f60577e40e6996da59d71fd5) to [20190529](https://github.com/particle-iot/ioLibrary_Driver/commit/890a73fb8beb1ccbd5a43d1f6aee545bd883fb46). [Changelog](https://gist.github.com/avtolstoy/2c5d13ef00886136aefaaeca51af6a72). [Fork diff](https://github.com/particle-iot/ioLibrary_Driver/compare/b5592d446dbed91704d0fde88e7aff748df1887d...d4e78e46259069d02c3383e7792432e12b9c54c1) [#1864](https://github.com/particle-iot/device-os/pull/1864)

### BUGFIXES

- [Gen 3] Fixes heap and application static RAM overlap introduced in 1.3.0-rc.1 [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Fixes tinker build errors when building with `LOG_SERIAL=y` [#1898](https://github.com/particle-iot/device-os/pull/1898)
- [Gen 3] Restored default BLE device address is incorrect. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- [Gen 3] Read BLE device name might be contracted. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- [Gen 3] operator& of the BleCharacteristicProperty enum class doesn't work as expected. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- [Gen 3] The length of got advertising and scan response data is not updated. [#1891](https://github.com/particle-iot/device-os/pull/1891)
- Fixes dynalib alignment issue when compiling relatively large applications potentially due to an unconfirmed bug in GCC by moving the dynalib into a separate section (`.dynalib`) [#1894](https://github.com/particle-iot/device-os/pull/1894)
- [Gen 3] Fixes incorrect handling of `MODULE_INFO_FLAG_DROP_MODULE_INFO` in the bootloader [#1897](https://github.com/particle-iot/device-os/pull/1897)
- [Gen 3] Adds a dummy suffix to the NCP and SoftDevice modules' module info with unique SHA to cause the communication layer to detect the change in SYSTEM DESCRIBE after NCP or SoftDevice update [#1897](https://github.com/particle-iot/device-os/pull/1897)

### INTERNAL

- [Photon / P1] Changes to support building combined images for the recent releases [#1887](https://github.com/particle-iot/device-os/pull/1887)
- `wiring/no_fixture` test adjustments for Gen 2 and Gen 3 platforms [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Coveralls test coverage reports generated by Travis [#1896](https://github.com/particle-iot/device-os/pull/1896)

## 1.3.1

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).
>
>This release contains v401 bootloaders.

### FEATURES

- [gen 3] Nordic SoftDevice update support [#1816](https://github.com/particle-iot/device-os/pull/1816)
- [gen 3] API for selecting BLE antenna for BLE radio [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for setting/getting BLE device name [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for setting BLE device address [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for manually discovering peer device's BLE services and characteristics [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for fetching discovered peer's services and characteristics [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API in BlePeerDevice for establishing BLE connection without scanning required [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for manually subscribing/unsubscribing peer characteristic's notification [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for disconnecting all on-going BLE connections [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] BLE support [#1740](https://github.com/particle-iot/device-os/pull/1740)
- [gen 3] NFC support [#1606](https://github.com/particle-iot/device-os/pull/1606)

### ENHANCEMENTS

- [Boron/BSoM] power: Increases charging current to 900mA when powered through VIN (VUSB) pin [#1846](https://github.com/particle-iot/device-os/pull/1846)
- Cancel network connection when processing a USB request that resets the device [#1830](https://github.com/particle-iot/device-os/pull/1830)
- Particle.connected() should return true only after handshake messages are acknowledged [#1825](https://github.com/particle-iot/device-os/pull/1825)
- [gen 3] USB state tracking enhancements [#1871](https://github.com/particle-iot/device-os/pull/1871)
- [gen 3] Adds timeouts to I2C HAL operations [#1875](https://github.com/particle-iot/device-os/pull/1875)
- [gen 3] Refactors BLE event dispatching [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Decreases BLE runtime RAM consumption [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Supports up to 23 local characteristics, 20 of them are available for user application [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Supports up to 3 central links [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] More convenient methods provided in BleUuid class [#1847](https://github.com/particle-iot/device-os/pull/1847)
- ensures AT interface is responsive [#1886](https://github.com/particle-iot/device-os/pull/1886)
- [enhancement] Cache cellular diagnostics [#1810](https://github.com/particle-iot/device-os/pull/1810)
- [enhancement] allow the bootloader to be flashed over DFU [#1788](https://github.com/particle-iot/device-os/pull/1788)

### BUG FIXES

- [Gen 3] Fixes heap and application static RAM overlap introduced in 1.3.0-rc.1 [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Fixes tinker build errors when building with `LOG_SERIAL=y` [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Fixes dynalib alignment issue when compiling relatively large applications potentially due to an unconfirmed bug in GCC by moving the dynalib into a separate section (`.dynalib`) [#1894](https://github.com/particle-iot/device-os/pull/1894)
- [Gen 3] Fixes incorrect handling of `MODULE_INFO_FLAG_DROP_MODULE_INFO` in the bootloader [#1897](https://github.com/particle-iot/device-os/pull/1897)
- [Gen 3] Adds a dummy suffix to the NCP and SoftDevice modules' module info with unique SHA to cause the communication layer to detect the change in SYSTEM DESCRIBE after NCP or SoftDevice update [#1897](https://github.com/particle-iot/device-os/pull/1897)
- Fixes a regression introduced in 1.1.0 where the system layer was always sending its handshake messages even if the session was resumed causing increased data usage [#1905](https://github.com/particle-iot/device-os/pull/1905)
- Properly seeds `rand()` in multiple threads including system thread. Fixes ephemeral port allocation in LwIP on Gen 3 platforms [#1905](https://github.com/particle-iot/device-os/pull/1905)
- Control requests that reset the device (e.g. `particle usb dfu`) no longer cause unnecessary reconnection to the cloud [#1905](https://github.com/particle-iot/device-os/pull/1905)
- Initialize user module in monolithic builds [#1905](https://github.com/particle-iot/device-os/pull/1905)
- fixes RSSI regression on G350 (2G) devices [#1841](https://github.com/particle-iot/device-os/pull/1841)
- [electron, gen3] Temporarily increase IDLE task priority whenever a thread exits (calls vTaskDelete) to resolve a memory leak that resulted in device being stuck "blinking green" until reset in some cases. [#1862](https://github.com/particle-iot/device-os/pull/1862)
- [gen 3] Fixes mesh pub/sub socket consuming all packet buffers [#1839](https://github.com/particle-iot/device-os/pull/1839)
- [Photon/P1] Bootloader correctly re-imports the DCT functions from system firmware after its modification through DFU [#1868](https://github.com/particle-iot/device-os/pull/1868)
- [gen 3] `Mesh.off()` disconnects the cloud. Resolves an issue with `loop()` not being executed in `SEMI_AUTOMATIC` mode after `Mesh.off()` [#1857](https://github.com/particle-iot/device-os/pull/1857)
- [Argon] Fixes the issue being unable to reset the device through RST pin by changing the ESPEN mode to `OUTPUT_OPEN_DRAIN` [#1870](https://github.com/particle-iot/device-os/pull/1870)
- [Electron/LTE] devices drop Cloud connection every time user firmware opens and closes a TCP socket [ch34976]() [#1854](https://github.com/particle-iot/device-os/pull/1854)
- [gen 2] Fixes an issue with clock stretching in I2C slave mode with underrun reads with certain I2C masters (e.g. Gen 3 devices) [#1829](https://github.com/particle-iot/device-os/pull/1829)
- Fix to ensure device resets after bootloader update [#1873](https://github.com/particle-iot/device-os/pull/1873)
- Fixes boot issue for Core introduced in 1.2.1-rc.3 [#1873](https://github.com/particle-iot/device-os/pull/1873)
- [gen 3] Resolved a HardFault after USB cable is unplugged under certain conditions [#1871](https://github.com/particle-iot/device-os/pull/1871)
- [gen 3] BLE advertising parameters didn't apply, issue #1874 [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] BLE scanning parameters didn't apply, issue #1859 and #1855 [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Local characteristic notification causes SOS: [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] BLE address order reversed [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gcc] fixes virtual platform exception on startup [#1878](https://github.com/particle-iot/device-os/pull/1878)
- Add two-digit mnc diagnostic flag [#1804](https://github.com/particle-iot/device-os/pull/1807)
- BLE fixes from 1.3.0-alpha.1 [#1817](https://github.com/particle-iot/device-os/pull/1817)
- NFC Context parameter is missing and NFC.update() will remove callback, fixes from 1.3.0-alpha.1 [#1818](https://github.com/particle-iot/device-os/pull/1818)
- [lte] cellular fixes [#1824](https://github.com/particle-iot/device-os/pull/1824)
- fixes #1811 - increases the number of event handlers to 6 [#1822](https://github.com/particle-iot/device-os/pull/1822)
- [gen 3] fixes memory usage diagnostics (reported negative values in safe mode) [#1819](https://github.com/particle-iot/device-os/pull/1819)
- System.disableUpdates() operates asynchronously [#1801](https://github.com/particle-iot/device-os/pull/1801)
- [gen 3] [bsom] Building platform BSOM results flash overflow. [#1802](https://github.com/particle-iot/device-os/pull/1802)
- [gen 3] [hal] fixes early wakeup by RTC from STOP sleep mode [#1803](https://github.com/particle-iot/device-os/pull/1803)

### INTERNAL

- Adjusts on-device tests [#1898](https://github.com/particle-iot/device-os/pull/1898)
- Add coverage to CMake unit-tests [#1860](https://github.com/particle-iot/device-os/pull/1860)
- Pull test implementation out of source file [#1867](https://github.com/particle-iot/device-os/pull/1867)
- [gen 3] Changes WKP pin to A7 for SoM platforms to avoid an overlap with Ethernet chip on EVB ESPEN mode to `OUTPUT_OPEN_DRAIN` [#1837](https://github.com/particle-iot/device-os/pull/1837)
- [hal] Fixes WIFIEN pin mode for ASoM [#1889](https://github.com/particle-iot/device-os/pull/1889)
- Refactor/move catch test [#1869](https://github.com/particle-iot/device-os/pull/1869)
- fixes to communications public interface [#1863](https://github.com/particle-iot/device-os/pull/1863)
- Refactors platform pinmap to be in platform-specific headers [#1838](https://github.com/particle-iot/device-os/pull/1838)
- Improve compatibility with recent versions of GCC [#1806](https://github.com/particle-iot/device-os/pull/1806)
- Set path to Boost libraries [#1872](https://github.com/particle-iot/device-os/pull/1872)
- [docs] Updates `spark_publish_vitals` and build scripts documentation. [#1800](https://github.com/particle-iot/device-os/pull/1800)
- [docs] update the test documentation [#1683](https://github.com/particle-iot/device-os/pull/1683)

## 1.3.1-rc.1

>**Note:** If your Gen 2 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release).  Gen 3 devices should not need an updated bootloader this release.
>
>This release contains v400 bootloaders.

### FEATURES

- [gen 3] Nordic SoftDevice update support [#1816](https://github.com/particle-iot/device-os/pull/1816)
- [gen 3] API for selecting BLE antenna for BLE radio [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for setting/getting BLE device name [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for setting BLE device address [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for manually discovering peer device's BLE services and characteristics [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for fetching discovered peer's services and characteristics [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API in BlePeerDevice for establishing BLE connection without scanning required [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for manually subscribing/unsubscribing peer characteristic's notification [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] API for disconnecting all on-going BLE connections [#1847](https://github.com/particle-iot/device-os/pull/1847)

### ENHANCEMENTS

- [Boron/BSoM] power: Increases charging current to 900mA when powered through VIN (VUSB) pin [#1846](https://github.com/particle-iot/device-os/pull/1846)
- Cancel network connection when processing a USB request that resets the device [#1830](https://github.com/particle-iot/device-os/pull/1830)
- Particle.connected() should return true only after handshake messages are acknowledged [#1825](https://github.com/particle-iot/device-os/pull/1825)
- [gen 3] USB state tracking enhancements [#1871](https://github.com/particle-iot/device-os/pull/1871)
- [gen 3] Adds timeouts to I2C HAL operations [#1875](https://github.com/particle-iot/device-os/pull/1875)
- [gen 3] Refactors BLE event dispatching [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Decreases BLE runtime RAM consumption [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Supports up to 23 local characteristics, 20 of them are available for user application [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Supports up to 3 central links [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] More convenient methods provided in BleUuid class [#1847](https://github.com/particle-iot/device-os/pull/1847)
- ensures AT interface is responsive [#1886](https://github.com/particle-iot/device-os/pull/1886)

### BUG FIXES

- fixes RSSI regression on G350 (2G) devices [#1841](https://github.com/particle-iot/device-os/pull/1841)
- [electron, gen3] Temporarily increase IDLE task priority whenever a thread exits (calls vTaskDelete) to resolve a memory leak that resulted in device being stuck "blinking green" until reset in some cases. [#1862](https://github.com/particle-iot/device-os/pull/1862)
- [gen 3] Fixes mesh pub/sub socket consuming all packet buffers [#1839](https://github.com/particle-iot/device-os/pull/1839)
- [Photon/P1] Bootloader correctly re-imports the DCT functions from system firmware after its modification through DFU [#1868](https://github.com/particle-iot/device-os/pull/1868)
- [gen 3] `Mesh.off()` disconnects the cloud. Resolves an issue with `loop()` not being executed in `SEMI_AUTOMATIC` mode after `Mesh.off()` [#1857](https://github.com/particle-iot/device-os/pull/1857)
- [Argon] Fixes the issue being unable to reset the device through RST pin by changing the ESPEN mode to `OUTPUT_OPEN_DRAIN` [#1870](https://github.com/particle-iot/device-os/pull/1870)
- [Electron/LTE] devices drop Cloud connection every time user firmware opens and closes a TCP socket [ch34976]() [#1854](https://github.com/particle-iot/device-os/pull/1854)
- [gen 2] Fixes an issue with clock stretching in I2C slave mode with underrun reads with certain I2C masters (e.g. Gen 3 devices) [#1829](https://github.com/particle-iot/device-os/pull/1829)
- Fix to ensure device resets after bootloader update [#1873](https://github.com/particle-iot/device-os/pull/1873)
- Fixes boot issue for Core introduced in 1.2.1-rc.3 [#1873](https://github.com/particle-iot/device-os/pull/1873)
- [gen 3] Resolved a HardFault after USB cable is unplugged under certain conditions [#1871](https://github.com/particle-iot/device-os/pull/1871)
- [gen 3] BLE advertising parameters didn't apply, issue #1874 [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] BLE scanning parameters didn't apply, issue #1859 and #1855 [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] Local characteristic notification causes SOS: [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gen 3] BLE address order reversed [#1847](https://github.com/particle-iot/device-os/pull/1847)
- [gcc] fixes virtual platform exception on startup [#1878](https://github.com/particle-iot/device-os/pull/1878)

### INTERNAL

- Add coverage to CMake unit-tests [#1860](https://github.com/particle-iot/device-os/pull/1860)
- Pull test implementation out of source file [#1867](https://github.com/particle-iot/device-os/pull/1867)
- [gen 3] Changes WKP pin to A7 for SoM platforms to avoid an overlap with Ethernet chip on EVB ESPEN mode to `OUTPUT_OPEN_DRAIN` [#1837](https://github.com/particle-iot/device-os/pull/1837)
- [hal] Fixes WIFIEN pin mode for ASoM [#1889](https://github.com/particle-iot/device-os/pull/1889)
- Refactor/move catch test [#1869](https://github.com/particle-iot/device-os/pull/1869)
- fixes to communications public interface [#1863](https://github.com/particle-iot/device-os/pull/1863)
- Refactors platform pinmap to be in platform-specific headers [#1838](https://github.com/particle-iot/device-os/pull/1838)
- Improve compatibility with recent versions of GCC [#1806](https://github.com/particle-iot/device-os/pull/1806)
- Set path to Boost libraries [#1872](https://github.com/particle-iot/device-os/pull/1872)

## 1.3.0-rc.1

>**Note:** If your Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release)
>
>This release contains v311 bootloaders.

### FEATURES

- [gen 3] BLE support [#1740](https://github.com/particle-iot/device-os/pull/1740)
- [gen 3] NFC support [#1606](https://github.com/particle-iot/device-os/pull/1606)

### ENHANCEMENTS

- [enhancement] Cache cellular diagnostics [#1810](https://github.com/particle-iot/device-os/pull/1810)
- [enhancement] allow the bootloader to be flashed over DFU [#1788](https://github.com/particle-iot/device-os/pull/1788)

### BUG FIXES

- Add two-digit mnc diagnostic flag [#1804](https://github.com/particle-iot/device-os/pull/1807)
- BLE fixes from 1.3.0-alpha.1 [#1817](https://github.com/particle-iot/device-os/pull/1817)
- NFC Context parameter is missing and NFC.update() will remove callback, fixes from 1.3.0-alpha.1 [#1818](https://github.com/particle-iot/device-os/pull/1818)
- [lte] cellular fixes [#1824](https://github.com/particle-iot/device-os/pull/1824)
- fixes #1811 - increases the number of event handlers to 6 [#1822](https://github.com/particle-iot/device-os/pull/1822)
- [gen 3] fixes memory usage diagnostics (reported negative values in safe mode) [#1819](https://github.com/particle-iot/device-os/pull/1819)
- System.disableUpdates() operates asynchronously [#1801](https://github.com/particle-iot/device-os/pull/1801)
- [gen 3] [bsom] Building platform BSOM results flash overflow. [#1802](https://github.com/particle-iot/device-os/pull/1802)
- [gen 3] [hal] fixes early wakeup by RTC from STOP sleep mode [#1803](https://github.com/particle-iot/device-os/pull/1803)

### INTERNAL

- [docs] Updates `spark_publish_vitals` and build scripts documentation. [#1800](https://github.com/particle-iot/device-os/pull/1800)
- [docs] update the test documentation [#1683](https://github.com/particle-iot/device-os/pull/1683)

## 1.3.0-alpha.1

>**Note:** If your Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release)
>
>This release contains v311 bootloaders.

### FEATURES

- [gen 3] BLE support [#1740](https://github.com/particle-iot/device-os/pull/1740)
- [gen 3] NFC support [#1606](https://github.com/particle-iot/device-os/pull/1606)

### BUG FIXES

- [gen 3] [bsom] Building platform BSOM results flash overflow. [#1802](https://github.com/particle-iot/device-os/pull/1802)
- [gen3] [hal] fixes early wakeup by RTC from STOP sleep mode [#1803](https://github.com/particle-iot/device-os/pull/1803)

## 1.2.1

>**Note:** If your Gen 2 Photon/P1 or Gen 3 device does not have a Cloud connection, it is recommended to update system firmware via CLI with `particle update`.  Electron bootloaders are still contained in system firmware and will update automatically as needed.
>
>This release contains v311 bootloaders.

### FEATURES

- [Enterprise] Immediate Product Firmware Updates [#1732](https://github.com/particle-iot/device-os/pull/1732)
- On-demand Device Vitals publishing [#1724](https://github.com/particle-iot/device-os/pull/1724)

### ENHANCEMENTS

- Cache cellular diagnostics [#1820](https://github.com/particle-iot/device-os/pull/1820)
- allow the bootloader to be flashed over DFU [#1788](https://github.com/particle-iot/device-os/pull/1788)
- [gen 3] Upgrades Nordic nRF5 SDK to 15.3.0 [#1768](https://github.com/particle-iot/device-os/pull/1768)
- [gen 3] Update error codes in Gen3 parser to facilitate debugging efforts and provide context to system errors [#1766](https://github.com/particle-iot/device-os/pull/1766)
- [gen 3] Remove bootloader machine code string from system-part1 on Gen3 platforms.  Bootloader will require local update with `particle flash --serial bootloader.bin` or OTA update from the Cloud. [#1771](https://github.com/particle-iot/device-os/pull/1771)
- Updates C++ and C standard versions (C++14 and C11) [#1757](https://github.com/particle-iot/device-os/pull/1757)
- [gen 3] Always check ongoing RX DMA transaction when reporting number of bytes available in RX buffer [#1758](https://github.com/particle-iot/device-os/pull/1758)
- Introduce safety checks on heap usage from ISRs [#1761](https://github.com/particle-iot/device-os/pull/1761)
- [gen 3] Mesh network scan enhancements [#1760](https://github.com/particle-iot/device-os/pull/1760)
- expires a session after 3 unsuccessful attempts at connecting to the cloud [#1776](https://github.com/particle-iot/device-os/pull/1776)
- Integrate cellular network vitals data into `DESCRIBE_x` message [#1759](https://github.com/particle-iot/device-os/pull/1759)
- [gen 2] adds Kore Vodafone SIM support & removes Twilio SIM support (still supported through 3rd party API) [ch31955] [#1780](https://github.com/particle-iot/device-os/pull/1780)
- Reserve memory for system-part1 SRAM [#1742](https://github.com/particle-iot/device-os/pull/1742)

### BUGFIXES

- fixes RSSI regression on G350 (2G) devices [#1848](https://github.com/particle-iot/device-os/pull/1848)
- Add two-digit mnc diagnostic flag [#1804](https://github.com/particle-iot/device-os/pull/1804)
- [lte] cellular fixes [#1824](https://github.com/particle-iot/device-os/pull/1824)
- fixes #1811 - increases the number of event handlers to 6 [#1822](https://github.com/particle-iot/device-os/pull/1822)
- [gen 3] fixes memory usage diagnostics (reported negative values in safe mode) [#1819](https://github.com/particle-iot/device-os/pull/1819)
- System.disableUpdates() operates asynchronously [#1801](https://github.com/particle-iot/device-os/pull/1801)
- [gen 3] Fixes radio initialization sequence for SoftDevice S140v6.1.1 [#1794](https://github.com/particle-iot/device-os/pull/1794)
- [gen 2] Fix ABI compatibility issue in cellular HAL regarding `CellularDevice` and `cellular_device_info()` [#1792](https://github.com/particle-iot/device-os/pull/1792)
- [gen 3] [bootloader] fixes SOS 10 when upgrading bootloader first from older system firmware.  External flash sleep refactoring [#1799](https://github.com/particle-iot/device-os/pull/1799)
- [system] network manager: allows to clear interface-specific credentials notwithstanding interface state, except for Mesh [#1773](https://github.com/particle-iot/device-os/pull/1773)
- [gen 3] pinMode fixes, D7 was initialized as OUTPUT mode, `analogWrite()` and `digitalWrite()` were changing pinMode back to default after use [#1777](https://github.com/particle-iot/device-os/pull/1777)
- [gen 3] Mesh network scan fixes [#1760](https://github.com/particle-iot/device-os/pull/1760)
- Wait for confirmable messages when entering the deep sleep mode [#1767](https://github.com/particle-iot/device-os/pull/1767)
- [gen 1] Fixes Spark Core function calls broken in 0.8.0-rc.4 [ch32050] [#1770](https://github.com/particle-iot/device-os/pull/1770)
- [Electron/LTE] disables all eDRX AcT types [ch32051] [#1762](https://github.com/particle-iot/device-os/pull/1762)
- [Electron/LTE] Make sure that the RAT information is actual before calculating signal strength (RSSI) and quality [#1779](https://github.com/particle-iot/device-os/pull/1779)
- intelligent update flags synchronization [#1784](https://github.com/particle-iot/device-os/pull/1784)
- [gen 3] Fixes various issues caused by the gateway reset [#1778](https://github.com/particle-iot/device-os/pull/1778)
- [Photon/P1] Fixes MAC address info not being available in listening mode [#1783](https://github.com/particle-iot/device-os/pull/1783)
- [wiring] Make sure that `Serial` and `SerialX` methods are in sync with the documentation and don't return unexpected values [#1782](https://github.com/particle-iot/device-os/pull/1782)
- [gen3] Fixes a HeapError panic due to malloc() call from an ISR (caused by rand() usage) [#1786](https://github.com/particle-iot/device-os/pull/1786)
- [gen3] Fixes HAL_USB_USART_Send_Data() returning incorrect values [#1787](https://github.com/particle-iot/device-os/pull/1787)

## 1.2.1-rc.3

>**Note:** If your Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release)
>
>This release contains v311 bootloaders.

### ENHANCEMENTS

- Cache cellular diagnostics [#1820](https://github.com/particle-iot/device-os/pull/1820)
- allow the bootloader to be flashed over DFU [#1788](https://github.com/particle-iot/device-os/pull/1788)

### BUG FIXES

- Add two-digit mnc diagnostic flag [#1804](https://github.com/particle-iot/device-os/pull/1804)
- [lte] cellular fixes [#1824](https://github.com/particle-iot/device-os/pull/1824)
- fixes #1811 - increases the number of event handlers to 6 [#1822](https://github.com/particle-iot/device-os/pull/1822)
- [gen 3] fixes memory usage diagnostics (reported negative values in safe mode) [#1819](https://github.com/particle-iot/device-os/pull/1819)
- System.disableUpdates() operates asynchronously [#1801](https://github.com/particle-iot/device-os/pull/1801)

## 1.2.1-rc.2

>**Note:** If your Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release)
>
>This release contains v311 bootloaders (bumped this release due to Gen 3 changes in [#1799](https://github.com/particle-iot/device-os/pull/1799) ). We have also separated BOOTLOADER_DEPENDENCY for Gen 2 and Gen 3. For now, we will leave Gen 2 as is depending on v201 bootloader (so there is nothing to do for Gen 2 (Photon/P1 only) unless you want the latest bootloader, although it is not mandatory), but we have bumped Gen 3 to v311 because we have removed the embedded bootloaders from Gen 3 system firmware in 1.2.0-rc.1. To force the Cloud update for Gen 2 (Photon/P1 only) we will bump the bootloader dependency version to v302 in v1.2.0 default.

### BUG FIXES

- [gen 3] Fixes radio initialization sequence for SoftDevice S140v6.1.1 [#1794](https://github.com/particle-iot/device-os/pull/1794)
- [gen 2] Fix ABI compatibility issue in cellular HAL regarding `CellularDevice` and `cellular_device_info()` [#1792](https://github.com/particle-iot/device-os/pull/1792)
- [gen 3] [bootloader] fixes SOS 10 when upgrading bootloader first from older system firmware.  External flash sleep refactoring [#1799](https://github.com/particle-iot/device-os/pull/1799)

## 1.2.1-rc.1

>**Note:** If your Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release)
>
>This release contains v310 bootloaders (bumped this release due to Gen 3 changes in [#1768](https://github.com/particle-iot/device-os/pull/1768) ). We have also separated BOOTLOADER_DEPENDENCY for Gen 2 and Gen 3. For now, we will leave Gen 2 as is depending on v201 bootloader (so there is nothing to do for Gen 2 (Photon/P1 only) unless you want the latest bootloader, although it is not mandatory), but we have bumped Gen 3 to v302 because also in this release we have removed the embedded bootloaders from Gen 3 system firmware. To force the Cloud update for Gen 2 (Photon/P1 only) we will bump the bootloader dependency version to v302 in v1.2.0 default.

### ENHANCEMENTS

- [gen3] Upgrades Nordic nRF5 SDK to 15.3.0 [#1768](https://github.com/particle-iot/device-os/pull/1768)

## 1.2.0-rc.1

>**Note:** If your Gen 3 device does not have a Cloud connection, it is recommended to update system firmware, and then the bootloader via CLI with `particle flash --serial bootloader.bin` (bootloaders found in Github release)
>
>This release contains v302 bootloaders (bumped this release due to Gen 3 changes in [#1777](https://github.com/particle-iot/device-os/pull/1777) ). We have also separated BOOTLOADER_DEPENDENCY for Gen 2 and Gen 3. For now, we will leave Gen 2 as is depending on v201 bootloader (so there is nothing to do for Gen 2 (Photon/P1 only) unless you want the latest bootloader, although it is not mandatory), but we have bumped Gen 3 to v302 because also in this release we have removed the embedded bootloaders from Gen 3 system firmware. To force the Cloud update for Gen 2 (Photon/P1 only) we will bump the bootloader dependency version to v302 in v1.2.0 default.

### FEATURES

- [Enterprise] Immediate Product Firmware Updates [#1732](https://github.com/particle-iot/device-os/pull/1732)
- On-demand Device Vitals publishing [#1724](https://github.com/particle-iot/device-os/pull/1724)

### ENHANCEMENTS

- [gen 3] Update error codes in Gen3 parser to facilitate debugging efforts and provide context to system errors [#1766](https://github.com/particle-iot/device-os/pull/1766)
- [gen 3] Remove bootloader machine code string from system-part1 on Gen3 platforms.  Bootloader will require local update with `particle flash --serial bootloader.bin` or OTA update from the Cloud. [#1771](https://github.com/particle-iot/device-os/pull/1771)
- Updates C++ and C standard versions (C++14 and C11) [#1757](https://github.com/particle-iot/device-os/pull/1757)
- [gen 3] Always check ongoing RX DMA transaction when reporting number of bytes available in RX buffer [#1758](https://github.com/particle-iot/device-os/pull/1758)
- Introduce safety checks on heap usage from ISRs [#1761](https://github.com/particle-iot/device-os/pull/1761)
- [gen 3] Mesh network scan enhancements [#1760](https://github.com/particle-iot/device-os/pull/1760)
- expires a session after 3 unsuccessful attempts at connecting to the cloud [#1776](https://github.com/particle-iot/device-os/pull/1776)
- Integrate cellular network vitals data into `DESCRIBE_x` message [#1759](https://github.com/particle-iot/device-os/pull/1759)
- [gen 2] adds Kore Vodafone SIM support & removes Twilio SIM support (still supported through 3rd party API) [ch31955] [#1780](https://github.com/particle-iot/device-os/pull/1780)
- Reserve memory for system-part1 SRAM [#1742](https://github.com/particle-iot/device-os/pull/1742)

### BUGFIXES

- [system] network manager: allows to clear interface-specific credentials notwithstanding interface state, except for Mesh [#1773](https://github.com/particle-iot/device-os/pull/1773)
- [gen 3] pinMode fixes, D7 was initialized as OUTPUT mode, `analogWrite()` and `digitalWrite()` were changing pinMode back to default after use [#1777](https://github.com/particle-iot/device-os/pull/1777)
- [gen 3] Mesh network scan fixes [#1760](https://github.com/particle-iot/device-os/pull/1760)
- Wait for confirmable messages when entering the deep sleep mode [#1767](https://github.com/particle-iot/device-os/pull/1767)
- [gen 1] Fixes Spark Core function calls broken in 0.8.0-rc.4 [ch32050] [#1770](https://github.com/particle-iot/device-os/pull/1770)
- [Electron/LTE] disables all eDRX AcT types [ch32051] [#1762](https://github.com/particle-iot/device-os/pull/1762)
- [Electron/LTE] Make sure that the RAT information is actual before calculating signal strength (RSSI) and quality [#1779](https://github.com/particle-iot/device-os/pull/1779)
- intelligent update flags synchronization [#1784](https://github.com/particle-iot/device-os/pull/1784)
- [gen 3] Fixes various issues caused by the gateway reset [#1778](https://github.com/particle-iot/device-os/pull/1778)
- [Photon/P1] Fixes MAC address info not being available in listening mode [#1783](https://github.com/particle-iot/device-os/pull/1783)
- [wiring] Make sure that `Serial` and `SerialX` methods are in sync with the documentation and don't return unexpected values [#1782](https://github.com/particle-iot/device-os/pull/1782)
- [gen3] Fixes a HeapError panic due to malloc() call from an ISR (caused by rand() usage) [#1786](https://github.com/particle-iot/device-os/pull/1786)
- [gen3] Fixes HAL_USB_USART_Send_Data() returning incorrect values [#1787](https://github.com/particle-iot/device-os/pull/1787)

### INTERNAL

- [gen 3] update bootloader dependency to v302 [#1785](https://github.com/particle-iot/device-os/pull/1785)

## 1.2.0-beta.1

### FEATURES

- [Enterprise] Immediate Product Firmware Updates [#1732](https://github.com/particle-iot/device-os/pull/1732)
- [Enterprise] Device Vitals reporting [#1724](https://github.com/particle-iot/device-os/pull/1724)

### INTERNAL

- [test] fixes unit-tests for testing version string with pre-release [#1749](https://github.com/particle-iot/device-os/pull/1749)
- [test] update upgrade-downgrade.sh to 1.2.0-beta.1 [#1749](https://github.com/particle-iot/device-os/pull/1749)

## 1.2.0-alpha.1

### FEATURES

- [Enterprise] Immediate Product Firmware Updates [#1732](https://github.com/particle-iot/device-os/pull/1732)
- [Enterprise] Device Vitals reporting [#1724](https://github.com/particle-iot/device-os/pull/1724)

## 1.1.0

### FEATURES

- [gen 3] Argon, Boron, Xenon platform Device OS `mesh_develop` merged into `develop` [#1700](https://github.com/particle-iot/device-os/pull/1700)
- [gen 3] Adds A SoM, B SoM and X SoM platforms (compile with PLATFORM=asom, PLATFORM=bsom, PLATFORM=xsom) to Device OS [#1662](https://github.com/particle-iot/device-os/pull/1662)
- [som] Runtime power management IC detection [#1733](https://github.com/particle-iot/device-os/pull/1733)

### ENHANCEMENTS

- [photon/p1/electron] mbedTLS updated from v2.4.2 to v2.9.0 [#1700](https://github.com/particle-iot/device-os/pull/1700)
- [electron/LTE] FreeRTOS updated from v8.2.2 to v10.0.1 [#1700](https://github.com/particle-iot/device-os/pull/1700)
- [gen 2] Binary size optimizations [#1720](https://github.com/particle-iot/device-os/pull/1720)
- Updates Tinker for all platforms [#1717](https://github.com/particle-iot/device-os/pull/1717)
- [gen 3] Adds button and RGB LED mirroring support [#1590](https://github.com/particle-iot/device-os/pull/1590)
- [bsom] Disables system power management [#1722](https://github.com/particle-iot/device-os/pull/1722)
- [boron/LTE] Enable Cat M1-only mode and disable eDRX completely [#1723](https://github.com/particle-iot/device-os/pull/1723)
- [gen 3] QSPI flash is put into sleep mode and is deinitialized when entering STANDBY or STOP sleep mode [1725](https://github.com/particle-iot/device-os/pull/1725)
- [gen 3] Parameter check for SPI slave mode with HAL_SPI_INTERFACE1 [#1731](https://github.com/particle-iot/device-os/pull/1731)

### BUGFIXES

- [gen 3] [hal] Fixes USBSerial SOS issue when removing USB cable from battery powered device [#1707](https://github.com/particle-iot/device-os/pull/1707)
- [gen 3] Fixes A5 staying high when using Ethernet feather wing [#1696](https://github.com/particle-iot/device-os/pull/1696)
- [core] Disable system logs for Core to reduce flash space needed to build tests [#1713](https://github.com/particle-iot/device-os/pull/1713)
- [wiring] Fixed a potential (but unlikely due to bounds checking) buffer overflow in time formatting function [#1712](https://github.com/particle-iot/device-os/pull/1712)
- [electron] [G350] fixes Cellular.RSSI() issues due to unknown RAT [#1721](https://github.com/particle-iot/device-os/pull/1721)
- [gen 3] Fixes a deadlock in `system_power_manager` and `i2c_hal` when exiting the sleep mode  [1725](https://github.com/particle-iot/device-os/pull/1725)
- [gen 3] Fixes issues in USB and WCID descriptors preventing Control Interface from working correctly on Windows platforms [#1736](https://github.com/particle-iot/device-os/pull/1736)
- [bootloader] SysTick needs to be disabled in Reset_System() on Gen 2 platforms [#1741](https://github.com/particle-iot/device-os/pull/1741)
- [boron] Workaround for SARA R4 ppp session getting broken and system power manager fix [#1726](https://github.com/particle-iot/device-os/pull/1726)
- Fixes system power manager re-enabling charging every 1s with a battery connected (now every 60s) [#1726](https://github.com/particle-iot/device-os/pull/1726)
- [photon/p1] Fixes 802.11n-only mode regression in 0.7.0 ~ 1.1.0-rc.1 [#1755](https://github.com/particle-iot/device-os/pull/1755)
- [gen 3] Updates embedded bootloader, fixes hardfault after hard reset when sleeping [#1756](https://github.com/particle-iot/device-os/pull/1756)

### INTERNAL

- Update release.sh parameter handling [#1690](https://github.com/particle-iot/device-os/pull/1690)
- Adds missing Device OS release tests [#1698](https://github.com/particle-iot/device-os/pull/1698)
- [gen 3] Fixes TEST=wiring/no_fixture [#1694](https://github.com/particle-iot/device-os/pull/1694)
- [gen 3] Add Gen 3 platforms to Device OS build scripts [#1714](https://github.com/particle-iot/device-os/pull/1714)
- [docs] Fix `brew install gcc-arm-none-eabi-53` formula [#1708](https://github.com/particle-iot/device-os/pull/1708)
- [docs] Fixes recent merge issues with `system-versions.md` [#1715](https://github.com/particle-iot/device-os/pull/1715)
- [ci] Build time optimizations [#1712](https://github.com/particle-iot/device-os/pull/1712)
- [ci] disables shallow submodule checkouts [#1735](https://github.com/particle-iot/device-os/pull/1735)
- [docs] for the check and scope guard macros [#1734](https://github.com/particle-iot/device-os/pull/1734)
- [hal] Correct ADC channel number for SoM [#1739](https://github.com/particle-iot/device-os/pull/1739)
- [photon/p1] crypto: re-enables MD5 for TLS (WPA Enterprise) [#1743](https://github.com/particle-iot/device-os/pull/1743)
- [gen 3] Fix/wiring tests [#1719](https://github.com/particle-iot/device-os/pull/1719)
- Remove the message "External flash is not supported" from Gen 3 builds [#1751](https://github.com/particle-iot/device-os/pull/1751)
- Do not fail the build if PARTICLE_DEVELOP is not defined [#1750](https://github.com/particle-iot/device-os/pull/1750)
- [gen 3] Fixes a build system issue that caused object files to be created outside build directory [#1754](https://github.com/particle-iot/device-os/pull/1754)
- [gen 3] Rename SoM platform names `[ch32184]` [#1774](https://github.com/particle-iot/device-os/pull/1774)

## 1.1.0-rc.2

### BUGFIXES

- [photon/p1] Fixes 802.11n-only mode regression in 0.7.0 ~ 1.1.0-rc.1 [#1755](https://github.com/particle-iot/device-os/pull/1755)
- [gen 3] Updates embedded bootloader, fixes hardfault after hard reset when sleeping [#1756](https://github.com/particle-iot/device-os/pull/1756)

### INTERNAL

- Remove the message "External flash is not supported" from Gen 3 builds [#1751](https://github.com/particle-iot/device-os/pull/1751)
- Do not fail the build if PARTICLE_DEVELOP is not defined [#1750](https://github.com/particle-iot/device-os/pull/1750)
- [gen 3] Fixes a build system issue that caused object files to be created outside build directory [#1754](https://github.com/particle-iot/device-os/pull/1754)

## 1.1.0-rc.1

### FEATURES

- [gen 3] Argon, Boron, Xenon platform Device OS `mesh_develop` merged into `develop` [#1700](https://github.com/particle-iot/device-os/pull/1700)
- [gen 3] Adds A SoM, B SoM and X SoM platforms (compile with PLATFORM=asom, PLATFORM=bsom, PLATFORM=xsom) to Device OS [#1662](https://github.com/particle-iot/device-os/pull/1662)
- [som] Runtime power management IC detection [#1733](https://github.com/particle-iot/device-os/pull/1733)

### ENHANCEMENTS

- [photon/p1/electron] mbedTLS updated from v2.4.2 to v2.9.0 [#1700](https://github.com/particle-iot/device-os/pull/1700)
- [electron/LTE] FreeRTOS updated from v8.2.2 to v10.0.1 [#1700](https://github.com/particle-iot/device-os/pull/1700)
- [gen 2] Binary size optimizations [#1720](https://github.com/particle-iot/device-os/pull/1720)
- Updates Tinker for all platforms [#1717](https://github.com/particle-iot/device-os/pull/1717)
- [gen 3] Adds button and RGB LED mirroring support [#1590](https://github.com/particle-iot/device-os/pull/1590)
- [bsom] Disables system power management [#1722](https://github.com/particle-iot/device-os/pull/1722)
- [boron/LTE] Enable Cat M1-only mode and disable eDRX completely [#1723](https://github.com/particle-iot/device-os/pull/1723)
- [gen 3] QSPI flash is put into sleep mode and is deinitialized when entering STANDBY or STOP sleep mode [1725](https://github.com/particle-iot/device-os/pull/1725)
- [gen 3] Parameter check for SPI slave mode with HAL_SPI_INTERFACE1 [#1731](https://github.com/particle-iot/device-os/pull/1731)

### BUGFIXES

- [gen 3] [hal] Fixes USBSerial SOS issue when removing USB cable from battery powered device [#1707](https://github.com/particle-iot/device-os/pull/1707)
- [gen 3] Fixes A5 staying high when using Ethernet feather wing [#1696](https://github.com/particle-iot/device-os/pull/1696)
- [core] Disable system logs for Core to reduce flash space needed to build tests [#1713](https://github.com/particle-iot/device-os/pull/1713)
- [wiring] Fixed a potential (but unlikely due to bounds checking) buffer overflow in time formatting function [#1712](https://github.com/particle-iot/device-os/pull/1712)
- [electron] [G350] fixes Cellular.RSSI() issues due to unknown RAT [#1721](https://github.com/particle-iot/device-os/pull/1721)
- [gen 3] Fixes a deadlock in `system_power_manager` and `i2c_hal` when exiting the sleep mode  [1725](https://github.com/particle-iot/device-os/pull/1725)
- [gen 3] Fixes issues in USB and WCID descriptors preventing Control Interface from working correctly on Windows platforms [#1736](https://github.com/particle-iot/device-os/pull/1736)
- [bootloader] SysTick needs to be disabled in Reset_System() on Gen 2 platforms [#1741](https://github.com/particle-iot/device-os/pull/1741)
- [boron] Workaround for SARA R4 ppp session getting broken and system power manager fix [#1726](https://github.com/particle-iot/device-os/pull/1726)
- Fixes system power manager re-enabling charging every 1s with a battery connected (now every 60s) [#1726](https://github.com/particle-iot/device-os/pull/1726)

### INTERNAL

- Update release.sh parameter handling [#1690](https://github.com/particle-iot/device-os/pull/1690)
- Adds missing Device OS release tests [#1698](https://github.com/particle-iot/device-os/pull/1698)
- [gen 3] Fixes TEST=wiring/no_fixture [#1694](https://github.com/particle-iot/device-os/pull/1694)
- [gen 3] Add Gen 3 platforms to Device OS build scripts [#1714](https://github.com/particle-iot/device-os/pull/1714)
- [docs] Fix `brew install gcc-arm-none-eabi-53` formula [#1708](https://github.com/particle-iot/device-os/pull/1708)
- [docs] Fixes recent merge issues with `system-versions.md` [#1715](https://github.com/particle-iot/device-os/pull/1715)
- [ci] Build time optimizations [#1712](https://github.com/particle-iot/device-os/pull/1712)
- [ci] disables shallow submodule checkouts [#1735](https://github.com/particle-iot/device-os/pull/1735)
- [docs] for the check and scope guard macros [#1734](https://github.com/particle-iot/device-os/pull/1734)
- [hal] Correct ADC channel number for SoM [#1739](https://github.com/particle-iot/device-os/pull/1739)
- [photon/p1] crypto: re-enables MD5 for TLS (WPA Enterprise) [#1743](https://github.com/particle-iot/device-os/pull/1743)
- [gen 3] Fix/wiring tests [#1719](https://github.com/particle-iot/device-os/pull/1719)

## 1.0.1

### BUGFIXES

- [LTE/Electron] Fixes software Timer()'s halting after millis() overflows (every 49.7 days) [#1688](https://github.com/particle-iot/device-os/pull/1688)
- [LTE/Electron] bug fixes and enhancements (see PR ->) [#1689](https://github.com/particle-iot/device-os/pull/1689)

### INTERNAL

- Fix Travis badge [#1670](https://github.com/particle-iot/device-os/pull/1670)
- Mojave doesn't ship with `wget` [#1674](https://github.com/particle-iot/device-os/pull/1674)
- Bypass git dependency in build [#1664](https://github.com/particle-iot/device-os/pull/1664)
- Refactor release scripts [#1687](https://github.com/particle-iot/device-os/pull/1687)

## 1.0.1-rc.1

### BUGFIXES

- [LTE/Electron] Fixes software Timer()'s halting after millis() overflows (every 49.7 days) [#1688](https://github.com/particle-iot/device-os/pull/1688)
- [LTE/Electron] bug fixes and enhancements (see PR ->) [#1689](https://github.com/particle-iot/device-os/pull/1689)

### INTERNAL

- Fix Travis badge [#1670](https://github.com/particle-iot/device-os/pull/1670)
- Mojave doesn't ship with `wget` [#1674](https://github.com/particle-iot/device-os/pull/1674)
- Bypass git dependency in build [#1664](https://github.com/particle-iot/device-os/pull/1664)
- Refactor release scripts [#1687](https://github.com/particle-iot/device-os/pull/1687)

## 1.0.0

### BREAKING CHANGES

- Beginning with 1.0.0 release, Particle.publish() and Particle.subscribe() methods will require event scope to be specified explicitly. This means using `PRIVATE` or `PUBLIC` for `Particle.publish()` and `MY_DEVICES` or `ALL_DEVICES` for `Particle.subscribe()`.  Please update your apps to include the event scope to avoid compilation errors in firmware `>0.7.0`. Deprecation warnings for this change began with `0.7.0-rc.3` [#1365](https://github.com/spark/firmware/pull/1365)
- [Core/Photon/P1] `WiFi.RSSI()` must be cast to `int8_t` when used inline with Serial.print() to produce correct results.  E.g. `Serial.printlnf("%d", (int8_t) WiFi.RSSI() );` Electron is not affected. [#1423](https://github.com/particle-iot/firmware/pull/1423)

### FEATURES

- Diagnostics service [#1390](https://github.com/spark/firmware/pull/1390)
- Network and Cloud diagnostics [#1424](https://github.com/spark/firmware/pull/1424)
- Diagnostics for unacked messages and rate limited events [#1391](https://github.com/spark/firmware/pull/1391)
- System uptime diagnoatics [#1393](https://github.com/spark/firmware/pull/1393)
- Network Signal Quality/Strength rework and diagnostics [#1423](https://github.com/spark/firmware/pull/1423)
- RAM usage diagnostic sources data [#1411](https://github.com/spark/firmware/pull/1411)
- Battery charge diagnostics [#1395](https://github.com/spark/firmware/pull/1395)
- Battery State diagnostics [#1398](https://github.com/spark/firmware/pull/1398)
- [Electron] Reworked power management [#1412](https://github.com/spark/firmware/pull/1412)
- Low-latency interrupt handlers [#1394](https://github.com/spark/firmware/pull/1394)
- [Electron] adds UPSV handling to cellular_hal [#1480](https://github.com/particle-iot/firmware/pull/1480)
- [Photon/P1] TCPClient: non-blocking, blocking, blocking with timeout writes support [#1485](https://github.com/particle-iot/firmware/pull/1485)
- Network and cloud diagnostics [#1424](https://github.com/particle-iot/firmware/pull/1424)
- Out of heap system event and heap fragmentation detection [#1452](https://github.com/particle-iot/firmware/pull/1452)
- USB request handlers [#1444](https://github.com/particle-iot/firmware/pull/1444)

### ENHANCEMENTS

- [Photon/P1] Moves Wi-Fi tester into application module [#1378](https://github.com/spark/firmware/pull/1378)
- [Photon/P1] Wi-FI firmware compression [#1421](https://github.com/spark/firmware/pull/1421)
- Only remake $(TARGET_BASE).elf el al. if necessary [#1223](https://github.com/particle-iot/firmware/pull/1223)
- Use 'using std::\*\*' instead of define \* std::\* [#1258](https://github.com/particle-iot/firmware/pull/1258)
- Added docs on local build setup [#1374](https://github.com/particle-iot/firmware/pull/1374)
- Firmware update and access to internal flash via USB requests [#1456](https://github.com/particle-iot/firmware/pull/1456)
- Generalize FuelGauge to also use alternative I2C interfaces. [#1443](https://github.com/particle-iot/firmware/pull/1443)
- [Photon/P1] WiFi.dnsServerIP()/WiFi.dhcpServerIP() support [#1386](https://github.com/particle-iot/firmware/pull/1386)
- [Photon/P1] Increase maximum supported number of simultaneously active TCP connections [#1350](https://github.com/particle-iot/firmware/pull/1350)
- Disable WKP pin waking device up from `SLEEP_MODE_DEEP` [#1409](https://github.com/particle-iot/firmware/pull/1409)
- System.sleep(): support for multiple wake up pins [#1405](https://github.com/particle-iot/firmware/pull/1405)
- System.sleep() wake up reason [#1410](https://github.com/particle-iot/firmware/pull/1410)
- Serialize access to the CRC peripheral (STM32F2xx) [#1465](https://github.com/particle-iot/firmware/pull/1465)
- [Photon/P1] Enables support for SHA384/SHA512 certificates for WPA Enterprise [#1501](https://github.com/particle-iot/firmware/pull/1501)
- [Photon/P1] A number of networking-related fixes v2 [#1500](https://github.com/particle-iot/firmware/pull/1500)
- [Electron/Photon/P1] Increase Device OS API argument lengths. More data in Functions, Variables, Publish, Subscribe... oh my! [#1537](https://github.com/particle-iot/firmware/pull/1537)
- [Electron] Adds support for SARA-R410 to the Cellular HAL [#1532](https://github.com/particle-iot/firmware/pull/1532)
- [Electron/Photon/P1] Do not check and lock bootloader sector write protection on every boot [ch17416] [#1578](https://github.com/particle-iot/firmware/pull/1578)
- [Photon/P1] Memory usage optimizations ([#1635](https://github.com/particle-iot/firmware/pull/1635))
- [Electron/Photon/P1] Cache persistent feature flags ([#1640](https://github.com/particle-iot/firmware/pull/1640))

### BUGFIXES

- [Electron] Error handling in the data usage API [#1435](https://github.com/spark/firmware/pull/1435)
- Cloud random seed not working [#1312](https://github.com/spark/firmware/issues/1312)
- Fixed shadowing of write(const unint_8_t`*`, sizte_t) in USBKeyboard [#1372](https://github.com/particle-iot/firmware/pull/1372)
- Fix usage of an incorrect prerequisite name in program-* targets [#1463](https://github.com/particle-iot/firmware/pull/1463)
- [Virtual] Fixes virtual device running with UDP protocol [#1462](https://github.com/particle-iot/firmware/pull/1462)
- [Core] Fixes I2C slave mode [#1309](https://github.com/particle-iot/firmware/pull/1309)
- [Electron] moves some newlib functions into part1 [#1471](https://github.com/particle-iot/firmware/pull/1471)
- [Electron] DCD fixes [#1454](https://github.com/particle-iot/firmware/pull/1454)
- [Electron] connect_cancel() fix [#1464](https://github.com/particle-iot/firmware/pull/1464)
- [Electron] Fix caching of the description CRCs in the backup RAM [#1413](https://github.com/particle-iot/firmware/pull/1413)
- [Electron] Guard cellular_command() with a global lock [#1415](https://github.com/particle-iot/firmware/pull/1415)
- [Electron] Fix heap bounds build for system part1 [#1478](https://github.com/particle-iot/firmware/pull/1478)
- [Photon/P1/Electron] Recursive logging freezes the application thread while the LogHandler is trying to acquire a lock on the resource [#1517](https://github.com/particle-iot/firmware/pull/1517)
- [Photon/Electron] WKP pin needs to be disabled as a wakeup source on boot to allow its normal operation [#1496](https://github.com/particle-iot/firmware/pull/1496)
- [Photon/P1] A number of networking-related fixes v2 [#1500](https://github.com/particle-iot/firmware/pull/1500)
- [Photon/P1] A number of networking-related fixes [#1492](https://github.com/particle-iot/firmware/pull/1492)
- [Electron] Fixes missing URCs for received data during TX or RX socket operations. This caused the modem not to be able to receive further data properly until it re-connected to the Cloud which it would do automatically but usually after a short or longer period of time. [#1530](https://github.com/particle-iot/firmware/pull/1530)
- [Electron] `Particle.keepAlive()` API was broken since v0.6.2-rc.2 firmware on Electron where the System would override an early set User ping interval.  This required a workaround of updating the keepAlive after the System made a connection to the Cloud.  See issue #1482 for workaround. [#1536](https://github.com/particle-iot/firmware/pull/1536)
- [Electron] Bug fixes for SARA-R410 LTE E Series [#1547](https://github.com/particle-iot/firmware/pull/1547)
- [Electron] Disables 30 second ping for Kore SIMs on SARA_R410 (default 23 minute ping re-applied)
- [Electron/LTE] Fast OTA Fixes [#1558](https://github.com/particle-iot/firmware/pull/1558)
- [Electron/LTE] eDRX & Power Saving mode disabled by default [#1567](https://github.com/particle-iot/firmware/pull/1567)
- [Electron/Photon/P1] Fixes recursive semaphore lock timeout [ch21928] [#1577](https://github.com/particle-iot/firmware/pull/1577)
- [Electron/LTE] adds 1 retry for UDP/TCP socket send in case of error [ch18789] [#1576](https://github.com/particle-iot/firmware/pull/1576)
- [Photon/P1] Invalidate sockets when turning WiFi off ([#1639](https://github.com/particle-iot/firmware/pull/1639))
- [Electron] Do not set the sticky skip hello after handshake ([#1624](https://github.com/particle-iot/firmware/pull/1624))
- [Electron] `PMIC::getInputCurrentLimit()` cannot report values higher than 900mA ([#1581](https://github.com/particle-iot/firmware/pull/1581))
- [Electron/LTE] AT+CNUM command causing registration failure on LTE devices ([#1627](https://github.com/particle-iot/firmware/pull/1627))
- [Electron/LTE] Power Manager Watchdog Timer Fix ([#1581](https://github.com/particle-iot/firmware/pull/1581))
- [Electron/Photon/P1] Wait for Wiring Thread to start ([#1528](https://github.com/particle-iot/firmware/pull/1528))
- [Electron/Photon/P1] Do not disable interrupts on every system loop iteration ([#1622](https://github.com/particle-iot/firmware/pull/1622))
- [Electron/Photon/P1] Fixes SOS in 0.8.0-rc.11 and also ensures button and OOM events are handled synchronously. Previously in multi-threaded applications the button handling may have been delayed to run within the application thread.  Now it is always handled immediately and should be noted that it is called from an ISR, so it is not advised to dynamically allocate memory in button event handlers. ([#1600](https://github.com/particle-iot/firmware/pull/1600)) ([#1650](https://github.com/particle-iot/firmware/pull/1650))
- [Electron/LTE] Fixes unique stale socket issue with LTE devices ([#1666](https://github.com/particle-iot/firmware/pull/1666))

### INTERNAL

- Minor refactoring of the USB protocol implementation [#1473](https://github.com/particle-iot/firmware/pull/1473)
- [Electron] Flash size optimizations [#1469](https://github.com/particle-iot/firmware/pull/1469)
- Documents low level USB request completion notifications [#1475](https://github.com/particle-iot/firmware/pull/1475)
- IS_CLAIMED request fixes [#1472](https://github.com/particle-iot/firmware/pull/1472)
- fixes the unit test build [#1474](https://github.com/particle-iot/firmware/pull/1474)
- Fixes some 0.8.0-rc.2 tests [#1476](https://github.com/particle-iot/firmware/pull/1476)
- [Electron] fixes sticker-rig issue with POWER_ON command [#1544](https://github.com/particle-iot/firmware/pull/1544)
- [Electron] Fixes monolithic build [#1543](https://github.com/particle-iot/firmware/pull/1543)

## 0.9.0

### BUGFIXES
- [Gen 3] Fixes system-dynalib incompatibility introduced in 0.9.0-rc.1, causing pre-0.9.0-rc.1 user applications that call certain system-dynalib functions to crash the device [#1692]
- [Gen 3] `WKP` pin is configured as pull-down with rising edge trigger when entering STANDBY sleep mode to keep feature parity with Gen 2 devices (#1691)
- [Boron] PPP thread stack size increased by 1K in order to resolve a very rare stack overflow (#1691)
- [Gen 3] Fixes a crash when attempting to send constant data residing in flash through Ethernet interface (#1691)
- [Gen 3] An attempt to unitialize an SPI interface no longer causes an assertion failure if the interface is not initialized (#1663)
- [Gen 3] Default SPI settings are now recognized correctly (#1663)
- [Gen 3] Fixed a possible race condition during the Timer's uninitialization (#1663)
- [Gen 3] `random()` is now properly seeded on application startup (#1663)
- [Boron] Fixes an issue with IMEI and ICCID not being reported in listening mode serial console with `v` command (#1681)
- [Gen 3] SPI MISO is no longer configured with a pull-down and user-provided CS pin is not reset to `INPUT` state when reconfiguring SPI peripheral (#1671)
- [Argon] Fixes a deadlock when initializing NCP client (#1661)
- [Gen 3] DFU mode no longer requires driver installation on Windows. Fixes incorrect WCID descriptors (#1653)
- [Gen 3] Adds missing Arduino-specific definitions (#1658)
- [Xenon] Makes `Serial2` available in user applications (#1660)
- [Argon] WiFi cipher types are now being correctly reported when scanning or retreiving stored credentials (#1659)

### ENHANCEMENTS
- [Gen 3] `micros()` resolution increased by mixing in `DWT->CYCCNT` (#1682)

### FEATURES
- [Gen 3] STOP sleep mode support (#1682)
- [Gen 3] STANDBY sleep mode support (#1667)
- [Gen 3] USB control requests (#1655)
- [Gen 3] Mesh network diagnostics (#1657)
- [Boron] `Cellular.command()` support (#1651)

### INTERNAL
- [Gen 3] Most of the `wiring/no_fixture` tests now successfully run on Xenon, Argon and Boron (#1663)
- Removed strong dependency on `git` (#1664)
- [Gen 3] OpenThread updated to 20190130 master with the fix for negative clock drift between HFCLK and LFCLK (#1684)
- Submodules now use absolute https URLs (#1699)
- Fixed an assertion failure (SOS 10) with Mesh.subscribe() and threading enabled (#1652)

## 0.8.0-rc.27

### BUGFIXES
- [Argon] Sucessful update of the NCP firmware no longer results in `SYSTEM_ERROR_INVALID_STATE` (#1645)
- [Argon] `m` command in listening mode correctly reports WiFi MAC address (#1638)

### ENHANCEMENTS
- [Gen 3] Added newlib `__assert_func()` implementation that logs the assertion failure and delegates to `AssertionFailure` panic handler (#1636)
- [Gen 3] OpenThread upgraded to 2018/12/17 master (#1643)
- [Gen 3] Added a workaround for RTC / TIMER negative drift issue in Nordic 802.15.4 radio driver (#1643)
- [Gen 3] Normalized (lowered) IRQ priorities to a safe 5-7 range (#1643)
- [Gen 3] `timer_hal` and `rtc_hal` migrated to use a single stable monotonic 64-bit microsecond counter provided by OpenThread platform-specific code using the RTC peripheral (#1643)
- [Gen 3] `HAL_disable_irq()` / `HAL_enable_irq()` implementation changed to use `__set_BASEPRI()` instead of `sd_nvic_critical_region_enter()` / `sd_nvic_critical_region_exit()` to avoid assertion failures in Nordic 802.15.4 radio driver
- [Gen 3] Persistent border router prefix (#1647)
- [Gen 3] Enables USB Serial by default (#1649)

## 0.8.0-rc.26

### BUGFIXES

- [Argon] Escape special characters in SSIDs and passwords (#1604)
- [Gen 3] Network system events are correctly generated (#1585)
- [Gen 3] Correct C++ contructor array alignment in system-part1 (#1594)
- [Gen 3] Fixed a conflict between DHCPv4-assigned and ND6-assigned DNS servers (#1596)
- [Argon / Boron] Fixed a race condition when restarting the GSM07.10 multiplexer causing a memory leak/corruption (#1608)
- [Gen 3] IPv4 `IPAddress` endianness issue fixed (#1610)
- [Boron] Fixed a crash when using `STARTUP()` macro to manage Cellular credentials (#1613)
- [Gen 3] Embedded user part update procedure fixed in for hybrid builds (#1617)
- [communication] Sticky `SKIP_SESSION_RESUME_HELLO` no longer set immediately after session resume (#1623)
- [Gen 3] OpenThread locking fixes (#1625)
- [Gen 3] NetworkManager initiail state initialized in `network_setup()`
- [Gen 3] IPv4 `IPAddress` endianness issue fixed (#1610)
- [Gen 3] Fixes an assertion failure in LwIP DHCP code when receiving an offer with > 2 DNS servers (#1618)
- [Argon / Xenon] `Wire1` enabled (#1633)

### ENHANCEMENTS

- [Boron] 3G Borons no longer incur 10 second power-on delay when cold booting (#1584)
- [Gen 3] Build time significantly improved (#1587)
- [Gen 3] Hardware-accelerated SHA-1 (#1593)
- [Gen 3] Newlib 3.0 compatibility (#1599)
- [Argon / Boron] AT parser immediately interrupted when GSM07.10 multiplexer exits asynchronously (e.g. terminated by the peer or due to keepalive timeout) (#1608)
- [Gen 3] NAT64 initial base source port randomized on boot (#1609)
- [Gen 3] LwIP optimizations (#1610)
- [Gen 3] DHCP hostname option enabled (defaults to DeviceID) (#1595)
- [Argon] WiFi passwords are not included in the logging output (#1619)
- [Gen 3] Power failure comparator always configured with 2.8V threshold (#1621)
- [Gen 3] Default mesh transmit power setting changed from 0dBm to 8dBm (#1629)
- [Gen 3] BLE MTU and data length changed to default minimum values, while still allowing upgrade by the peer up to the maximums available on nRF52840 (#1634)

### FEATURES
- [Gen 3] SPI slave mode (#1588)
- [Gen 3] I2C slave mode (#1591)
- [Gen 3] Servo HAL (#1589)
- [Gen 3] Implement a control request to retrieve the module info in the protobuf format (#1614)

### INTERNAL

- [Gen 3] Run unit tests as part of a CI build (#1604).

## 0.8.0-rc.25

### FEATURES

- [Argon] Enables serial setup console `WiFiSetupConsole` in listening mode to manage WiFi credentials (#268)

### BUGFIXES

- [system] Fix memory usage diagnostics (#262)
- [openthread] Work around network name issues (#267)
- [ble] Avoid disabling interrupts when processing control requests (#264)
- [system] Power manager should not immediately go into `NOT_CHARGING` state from `DISCONNECTED` before `DEFAULT_WATCHDOG_TIMEOUT` passes (#263)
- [system] Battery state of charge should not be reported in `DISCONNECTED` state (#263)
- [ifapi] DHCPv4 client shouldn't start on ppp interfaces through `if_set_xflags()` (#263)
- [Boron] ppp (cellular) netif should not be default (#263)
- [Mesh] Fix memory usage diagnostics in modular builds (#262)

### ENHANCEMENTS

- [hal] Define `retained` and `retained_system` macros (#265)
- [system] Use ephemeral ports when connecting to the cloud over IPv4 (#269)
- [system] Re-request permission to become Border Router from the cloud every 5 minutes if previously denied using a separate timer (#266)
- [openthread] When syncing LwIP -> OT multicast subscriptions, immediately join the group on LwIP side as well, if not joined already (#263)

## 0.8.0-rc.24

### BUGFIXES

- [Mesh] Retry system commands on cloud connection errors (#261)

## 0.8.0-rc.23

### BREAKING CHANGES

- [Mesh] Added versioning to mesh pub/sub protocol. Makes the format incompatible with previous releases (#250)

### FEATURES

- [Boron] PMIC and FuelGauge wiring APIs enabled (#257)
- [Boron] Enabled system power manager. (LTE) Borons should now work correctly without the battery attached (#257)
- [Argon, Boron] Ongoing connection attempt can be cancelled from a higher priority context (#258)
- [Mesh] Require the cloud to confirm the BR functionality

### BUGFIXES

- [Mesh] Ethernet wiring object can now be correctly used (#259)
- [Mesh] Increased `MEMP_NUM_NETBUF` for all the platforms to allow to queue up more packet buffers
- [Mesh] Hybrid module reports itself as modular instead of monolithic in system module info (#255)
- [Mesh] Multicast subscriptions created on LwIP were getting lost when Thread interface was going down (#250)
- [system] Claim code shouldn't be published if it's not initialized in DCT
- [Mesh] hal: D7 and RGB LED pins should be in the same PWM group

## 0.8.0-rc.22

### BUGFIXES

- [Argon, Boron] AT parser: Ignore response lines that don't contain an URC when waiting for a command echo
- [Mesh] Use an unused backup register for the `STARTUP_LISTEN_MODE` flag
- [Boron] Fixed SARA U201/R410 power on/off and reset procedures.
- [Mesh] Fixed backup and stack sections overlap in linker files
- [Boron] Added missing `UBVINT` pin definition

### FEATURES

- [Mesh] `HAL_Pin_Configure()` implemented allowing to configure the pin and set it to a certain state without a glitch

## 0.8.0-rc.21

## BUGFIXES

- [Argon] Shorten describe message so it fits when sent to the cloud [#252]
- [Boron] Remove NCP module from describe since it is not updatable [#252]

## 0.8.0-rc.20

### FEATURES

- [Mesh] `pinResetFast()` / `pinSetFast()` / `pinReadFast()` implementation (#244)
- [Mesh] Clear Mesh credentials if Network Joined / Updated request is rejected cloudside with 4xx error code
- [communication] Added `FORCE_PING` protocol command to proactively ping the cloud
- [Mesh] Ping the cloud whenever the network interface state / configuration change
- [Mesh] Send the border router CoAP message to the cloud when the device becomes a border router (#248)

### BUGFIXES

- [Boron] Increased LTE Boron USART polling rate
- [Boron] Increased maximum PPP LCP echo failures to 10
- [Boron] Increased maximum GSM07.10 missed keepalives to 5 on LTE Boron
- [Mesh] NetworkManager: fixes credentials removal and interface state syncup in connected state
- [Mesh] Removes extra comma from JSON module info in hybrid builds (#246)
- [Argon, Boron] Fix echo handling in the AT command parser
- [communication] When resuming a session without sending HELLO message, an error from `ping()` needs to be propagated
- [Mesh] Added a proposed workaround to radio driver from https://devzone.nordicsemi.com/f/nordic-q-a/38460/thread-dynamic-multiprotocol---assertion-at-radioreceive

### ENHANCEMENTS

- [Argon] Changes expected NCP firmware module function from monolithic (`0x03`) to NCP monolithic (`0x07`), adds NCP module info to system module info (#245)
- [Mesh] Move system flags to backup RAM (#216)
- [Mesh] Implement `System.dfu()` and `System.enterSafeMode()` (#209)
- [Mesh] Implement querying of the last reset info (#209)
- [Mesh] Ignore ALOC addresses when synchronizing IP address information between OpenThread and LwIP
- [Mesh] Enable / disable Border Router functionality on system thread instead of LwIP thread
- [Mesh] `otPlatAssertFail()` implemented
- [Boron] Reset the modem if it can't register in the network for 5 minutes
- [Mesh] Use `select()` instead of blocking `recvfrom()` in the DNS64 service
- [system] Disable interrupts in `ISRTaskQueue::process()` only if there is at least on entry in the queue


## 0.8.0-rc.19

### BUGFIXES

- [Mesh] Fixes a bug introduced in rc.18 causing Mesh devices to be stuck in fast blinking green (link-local only communication) despite having a border router within the network.
- [Mesh] `__errno()` is now exported from system-part1 in rt dynalib
- [Mesh] Fixes a bug in IPv4 mapped IPv6 address conversion to IPAddress in TCPClient/TCPServer/UDP
- [Mesh] Correct Mesh backup server address
- [Mesh] Mesh pubsub thread-safety
- [Mesh] Use network ID instead of XPAN ID to identify networks

### FEATURES

- [Mesh] Mesh.localIP() introduced
- [Wiring] UDP: blocking reads
- [Mesh] Full factory reset
- [Mesh] Control requests to enable/disable Ethernet shield detection

## 0.8.0-rc.18

### FEATURES

- [Mesh] Wiring API for networking

### BUGFIXES

- [Mesh] Various bugfixes and stability improvements

## 0.8.0-rc.17

### FEATURES

- [Boron] Initial support for 3G and LTE networking

## 0.8.0-rc.16

### FEATURES

- [Argon] Initial support for WiFi networking

## 0.8.0-rc.15

### FEATURES

- [Mesh] Modular firmware support with factory reset
- [Mesh] I2C Master driver support
- [Mesh] PWM Driver
- [Mesh] Tone Driver
- [Mesh] USB CDC Driver
- [Mesh] USART DMA Driver
- [Mesh] RTC Driver
- [Mesh] FastPin implementation
- [Mesh] Reset reason
- [Mesh] UDP in wiring with multicast support
- [Mesh] Mesh-local publish/subscribe
- [Mesh] EEPROM emulation
- [Mesh] Hybrid firmware for mono->modular upgrade w/ tinker app embedded.

### ENHANCEMENTS

- [Mesh] Timeouts for joiner and comissioner
- [Mesh] Multiple attempts to join network
- [Mesh] Control requests for managing the cloud connection

### BUGFIXES

- [Mesh] I2C support for sending 0-byte transfer
- [Mesh] Stop comissioner role synchronously
- [Mesh] Increase the minimum BLE interval to 30ms

### INTERNAL CHANGES

- Bootloader included in Device OS

## 0.8.0-rc.14

### BUGFIXES

- [Electron/LTE] Fixes unique stale socket issue with LTE devices ([#1666](https://github.com/particle-iot/firmware/pull/1666))

## 0.8.0-rc.12

### ENHANCEMENTS

- [Electron/Photon/P1] Cache persistent feature flags ([#1640](https://github.com/particle-iot/firmware/pull/1640))
- [Photon/P1] Memory usage optimizations ([#1635](https://github.com/particle-iot/firmware/pull/1635))

### BUGFIXES

- [Electron/Photon/P1] Fixes SOS in 0.8.0-rc.11 and also ensures button and OOM events are handled synchronously. Previously in multi-threaded applications the button handling may have been delayed to run within the application thread.  Now it is always handled immediately and should be noted that it is called from an ISR, so it is not advised to dynamically allocate memory in button event handlers. ([#1600](https://github.com/particle-iot/firmware/pull/1600)) ([#1650](https://github.com/particle-iot/firmware/pull/1650))
- [Electron/Photon/P1] Do not disable interrupts on every system loop iteration ([#1622](https://github.com/particle-iot/firmware/pull/1622))
- [Electron/Photon/P1] Wait for Wiring Thread to start ([#1528](https://github.com/particle-iot/firmware/pull/1528))
- [Electron/LTE] Power Manager Watchdog Timer Fix ([#1581](https://github.com/particle-iot/firmware/pull/1581))
- [Electron/LTE] AT+CNUM command causing registration failure on LTE devices ([#1627](https://github.com/particle-iot/firmware/pull/1627))
- [Electron] `PMIC::getInputCurrentLimit()` cannot report values higher than 900mA ([#1581](https://github.com/particle-iot/firmware/pull/1581))
- [Electron] Do not set the sticky skip hello after handshake ([#1624](https://github.com/particle-iot/firmware/pull/1624))
- [Photon/P1] Invalidate sockets when turning WiFi off ([#1639](https://github.com/particle-iot/firmware/pull/1639))

## 0.8.0-rc.11

### ENHANCEMENTS

- [Electron/Photon/P1] Do not check and lock bootloader sector write protection on every boot [ch17416] [#1578](https://github.com/particle-iot/firmware/pull/1578)

### BUGFIXES

- [Electron/LTE] adds 1 retry for UDP/TCP socket send in case of error [ch18789] [#1576](https://github.com/particle-iot/firmware/pull/1576)
- [Electron/Photon/P1] Fixes recursive semaphore lock timeout [ch21928] [#1577](https://github.com/particle-iot/firmware/pull/1577)

## 0.8.0-rc.10

### BUGFIXES

- [Electron/LTE] eDRX & Power Saving mode disabled by default [#1567](https://github.com/particle-iot/firmware/pull/1567)

## 0.8.0-rc.9

### BUGFIXES

- [Electron/LTE] Fast OTA Fixes [#1558](https://github.com/particle-iot/firmware/pull/1558)

## 0.8.0-rc.8

**Note:** This is primarily a MFG. release for SARA-R410 LTE modules. The changes do touch code used on other Electron based platforms, but no other features or fixes are relevant for U260, U270, U201, or G350 modems.  This code has been tested on-device for all mentioned modem types with passing results.  Please let us know if you find any issues.

### BUGFIXES

- [Electron] Disables 30 second ping for Kore SIMs on SARA_R410 (default 23 minute ping re-applied)

## 0.8.0-rc.7

**Note:** This is primarily a MFG. release for SARA-R410 LTE modules. The changes do touch code used on other Electron based platforms, but no other features or fixes are relevant for U260, U270, U201, or G350 modems.  This code has been tested on-device for all mentioned modem types with passing results.  Please let us know if you find any issues.

### BUGFIXES

- [Electron] Bug fixes for SARA-R410 LTE E Series [#1547](https://github.com/particle-iot/firmware/pull/1547)

## 0.8.0-rc.6

**Note:** This is primarily a MFG. release for SARA-R410 LTE modules. The changes do touch code used on other Electron based platforms, but no other features or fixes are relevant for U260, U270, U201, or G350 modems.  This code has been tested on-device for all mentioned modem types with passing results.  Please let us know if you find any issues.

### INTERNAL

- [Electron] Fixes monolithic build [#1543](https://github.com/particle-iot/firmware/pull/1543)
- [Electron] fixes sticker-rig issue with POWER_ON command [#1544](https://github.com/particle-iot/firmware/pull/1544)

## 0.8.0-rc.5

**Note:** This is primarily a MFG. release for SARA-R410 LTE modules. The changes do touch code used on other Electron based platforms, but no other features or fixes are relevant for U260, U270, U201, or G350 modems.  This code has been tested on-device for all mentioned modem types with passing results.  Please let us know if you find any issues.

### ENHANCEMENTS

- [Electron] Adds support for SARA-R410 to the Cellular HAL [#1532](https://github.com/particle-iot/firmware/pull/1532)

## 0.8.0-rc.4

### ENHANCEMENTS

- [Electron/Photon/P1] Increase Device OS API argument lengths. More data in Functions, Variables, Publish, Subscribe... oh my! [#1537](https://github.com/particle-iot/firmware/pull/1537)

### BUGFIXES

- [Electron] `Particle.keepAlive()` API was broken since v0.6.2-rc.2 firmware on Electron where the System would override an early set User ping interval.  This required a workaround of updating the keepAlive after the System made a connection to the Cloud.  See issue #1482 for workaround. [#1536](https://github.com/particle-iot/firmware/pull/1536)
- [Electron] Fixes missing URCs for received data during TX or RX socket operations. This caused the modem not to be able to receive further data properly until it re-connected to the Cloud which it would do automatically but usually after a short or longer period of time. [#1530](https://github.com/particle-iot/firmware/pull/1530)

## 0.8.0-rc.3

### ENHANCEMENTS

- [Photon/P1] A number of networking-related fixes v2 [#1500](https://github.com/particle-iot/firmware/pull/1500)
- [Photon/P1] Enables support for SHA384/SHA512 certificates for WPA Enterprise [#1501](https://github.com/particle-iot/firmware/pull/1501)

### BUGFIXES

- [Photon/P1] A number of networking-related fixes [#1492](https://github.com/particle-iot/firmware/pull/1492)
- [Photon/P1] A number of networking-related fixes v2 [#1500](https://github.com/particle-iot/firmware/pull/1500)
- [Photon/Electron] WKP pin needs to be disabled as a wakeup source on boot to allow its normal operation [#1496](https://github.com/particle-iot/firmware/pull/1496)
- [Photon/P1/Electron] Recursive logging freezes the application thread while the LogHandler is trying to acquire a lock on the resource [#1517](https://github.com/particle-iot/firmware/pull/1517)

## 0.8.0-rc.2

### FEATURES

- USB request handlers [#1444](https://github.com/particle-iot/firmware/pull/1444)
- Out of heap system event and heap fragmentation detection [#1452](https://github.com/particle-iot/firmware/pull/1452)
- Network and cloud diagnostics [#1424](https://github.com/particle-iot/firmware/pull/1424)
- [Photon/P1] TCPClient: non-blocking, blocking, blocking with timeout writes support [#1485](https://github.com/particle-iot/firmware/pull/1485)
- [Electron] adds UPSV handling to cellular_hal [#1480](https://github.com/particle-iot/firmware/pull/1480)

### ENHANCEMENTS

- Serialize access to the CRC peripheral (STM32F2xx) [#1465](https://github.com/particle-iot/firmware/pull/1465)
- System.sleep() wake up reason [#1410](https://github.com/particle-iot/firmware/pull/1410)
- System.sleep(): support for multiple wake up pins [#1405](https://github.com/particle-iot/firmware/pull/1405)
- Disable WKP pin waking device up from `SLEEP_MODE_DEEP` [#1409](https://github.com/particle-iot/firmware/pull/1409)
- [Photon/P1] Increase maximum supported number of simultaneously active TCP connections [#1350](https://github.com/particle-iot/firmware/pull/1350)
- [Photon/P1] WiFi.dnsServerIP()/WiFi.dhcpServerIP() support [#1386](https://github.com/particle-iot/firmware/pull/1386)
- Generalize FuelGauge to also use alternative I2C interfaces. [#1443](https://github.com/particle-iot/firmware/pull/1443)
- Firmware update and access to internal flash via USB requests [#1456](https://github.com/particle-iot/firmware/pull/1456)
- Added docs on local build setup [#1374](https://github.com/particle-iot/firmware/pull/1374)
- Use 'using std::**' instead of define * std::* [#1258](https://github.com/particle-iot/firmware/pull/1258)
- Only remake $(TARGET_BASE).elf el al. if necessary [#1223](https://github.com/particle-iot/firmware/pull/1223)

### BUGFIXES

- [Electron] Fix heap bounds build for system part1 [#1478](https://github.com/particle-iot/firmware/pull/1478)
- [Electron] Guard cellular_command() with a global lock [#1415](https://github.com/particle-iot/firmware/pull/1415)
- [Electron] Fix caching of the description CRCs in the backup RAM [#1413](https://github.com/particle-iot/firmware/pull/1413)
- [Electron] connect_cancel() fix [#1464](https://github.com/particle-iot/firmware/pull/1464)
- [Electron] DCD fixes [#1454](https://github.com/particle-iot/firmware/pull/1454)
- [Electron] moves some newlib functions into part1 [#1471](https://github.com/particle-iot/firmware/pull/1471)
- [Core] Fixes I2C slave mode [#1309](https://github.com/particle-iot/firmware/pull/1309)
- [Virtual] Fixes virtual device running with UDP protocol [#1462](https://github.com/particle-iot/firmware/pull/1462)
- Fix usage of an incorrect prerequisite name in program-* targets [#1463](https://github.com/particle-iot/firmware/pull/1463)
- Fixed shadowing of write(const unint_8_t`*`, sizte_t) in USBKeyboard [#1372](https://github.com/particle-iot/firmware/pull/1372)

### INTERNAL

- Fixes some 0.8.0-rc.2 tests [#1476](https://github.com/particle-iot/firmware/pull/1476)
- fixes the unit test build [#1474](https://github.com/particle-iot/firmware/pull/1474)
- IS_CLAIMED request fixes [#1472](https://github.com/particle-iot/firmware/pull/1472)
- Documents low level USB request completion notifications [#1475](https://github.com/particle-iot/firmware/pull/1475)
- [Electron] Flash size optimizations [#1469](https://github.com/particle-iot/firmware/pull/1469)
- Minor refactoring of the USB protocol implementation [#1473](https://github.com/particle-iot/firmware/pull/1473)

## 0.8.0-rc.1

## FEATURES

- Low-latency interrupt handlers [#1394](https://github.com/spark/firmware/pull/1394)
- [Electron] Reworked power management [#1412](https://github.com/spark/firmware/pull/1412)
- Battery State diagnostics [#1398](https://github.com/spark/firmware/pull/1398)
- Battery charge diagnostics [#1395](https://github.com/spark/firmware/pull/1395)
- RAM usage diagnostic sources data [#1411](https://github.com/spark/firmware/pull/1411)
- Network Signal Quality/Strength rework and diagnostics [#1423](https://github.com/spark/firmware/pull/1423)
- System uptime diagnoatics [#1393](https://github.com/spark/firmware/pull/1393)
- Diagnostics for unacked messages and rate limited events [#1391](https://github.com/spark/firmware/pull/1391)
- Network and Cloud diagnostics [#1424](https://github.com/spark/firmware/pull/1424)
- Diagnostics service [#1390](https://github.com/spark/firmware/pull/1390)

## ENHANCEMENTS

- [Photon/P1] Wi-FI firmware compression [#1421](https://github.com/spark/firmware/pull/1421)
- [Photon/P1] Moves Wi-Fi tester into application module [#1378](https://github.com/spark/firmware/pull/1378)

## BUGFIXES

- Cloud random seed not working [#1312](https://github.com/spark/firmware/issues/1312)
- [Electron] Error handling in the data usage API [#1435](https://github.com/spark/firmware/pull/1435)

## 0.7.0 (see additional changelog 0.7.0-rc.1 ~ 0.7.0-rc.7)

### BUGFIX

- [Photon/Electron] WKP pin needs to be disabled as a wakeup source on boot to allow its normal operation [#1496](https://github.com/particle-iot/firmware/pull/1496)

## 0.7.0-rc.7

### BUGFIX

- [Photon] Regression with SoftAP and URL-encoded form query [#1432](https://github.com/spark/firmware/issues/1432)
- Particle.connect() hard blocking since 0.6.1-rc.1 [#1399](https://github.com/spark/firmware/issues/1399)
- [Electron] Cellular resolve does not return 0 / false when it receives bad DNS resolution related to bad cell service [#1304](https://github.com/spark/firmware/issues/1304)
- [Core] Use the device ID as the USB serial number [#1367](https://github.com/spark/firmware/issues/1367)
- [Electron] Fix heap bounds for system part 1 [#1478](https://github.com/particle-iot/firmware/pull/1478)
- [Electron] connect_cancel() fix [#1464](https://github.com/particle-iot/firmware/pull/1464)
- Fixed shadowing of `write(const unint_8_t*, sizte_t)` in USBKeyboard [#1372](https://github.com/particle-iot/firmware/pull/1372)

## 0.7.0-rc.6

### BUGFIX

- [Electron] Add dependency in system-part-1 on 0.6.4 system-part-3 to prevent upgrades from 0.6.3 or earlier to avoid incompatibilties
with these releases.

## 0.7.0-rc.5

### BUGFIX

 - The device ID is output in lowercase in DFU mode. [#1432](https://github.com/spark/firmware/issues/1432)
 - increase the DTLS buffer from 768 to 800 bytes, so that the system describe message is sent.
 - remove rigid dependency check in bootloader that was causing DCT functions to not load in 0.8.0-rc.1 [#1436](https://github.com/spark/firmware/pull/1436)

## 0.7.0-rc.4

### ENHANCEMENTS

 - USART Half-duplex enhancements [#1308](https://github.com/spark/firmware/pull/1380)

### BUGFIX

 - KRACK WPA2 security bugfix [#1420](https://github.com/spark/firmware/pull/1420)
 - Monolithic build linker error [#1370](https://github.com/spark/firmware/pull/1370)
 - 4-digit serial numbers had additional characters [#1380](https://github.com/spark/firmware/pull/1380)


## 0.7.0-rc.3

### DEPRECATED API

[`[PR #1365]`](https://github.com/spark/firmware/pull/1365) Beginning with 0.8.0 release, `Particle.publish()` and `Particle.subscribe()` methods will require event scope to be specified explicitly. Please update your apps now to include the event scope to avoid compilation errors in >=0.8.0.

### BUGFIX

[`[PR #1362]`](https://github.com/spark/firmware/pull/1362) [`[Fixes #1360]`](https://github.com/spark/firmware/issues/1360) Fixed SoftAP HTTP usage hard faulting in 0.7.0-rc.1 and 0.7.0-rc.2


## 0.7.0-rc.2

### ENHANCEMENTS

- [`[PR #1357]`](https://github.com/spark/firmware/pull/1357) Expands the device code from 4 digits to 6 digits for Photon/P1/Electron platforms

### BUGFIX

- [`[PR #1346]`](https://github.com/spark/firmware/pull/1346) [`[Fixes #1344]`](https://github.com/spark/firmware/issues/1344) `[Photon/P1]` When using `SYSTEM_THREAD(ENABLED)` the TCPServer and WPA Enterprise connections were broken.
- [`[PR #1354]`](https://github.com/spark/firmware/pull/1354) [`[Fixes #1062]`](https://github.com/spark/firmware/issues/1062) A call to `WiFi.scan()` when Wi-Fi module is off or not ready was resulting in a hard fault.
- [`[PR #1357]`](https://github.com/spark/firmware/pull/1357) [`[Fixes #1348]`](https://github.com/spark/firmware/issues/1348) SoftAP SSID was not respecting the string's null terminator, 2 char SSID would appear as 4.
- [`[PR #1355]`](https://github.com/spark/firmware/pull/1355) When using WPA Enterprise access point and constantly reconnecting to it, heap was becoming fragmented which resulted in inability to connect to the access point anymore. Also reduced overall heap usage.

### INTERNAL

- [`[PR #1342]`](https://github.com/spark/firmware/pull/1342) Removed the `firmware-docs` subtree from the `firmware` repo.  Docs updates are made directly to `docs` repo again.
- [`[PR #1352]`](https://github.com/spark/firmware/pull/1352) Added test for `RGB.onChange()` handler leak
- [`[PR #1358]`](https://github.com/spark/firmware/pull/1358) Updates minimal ARM gcc version required to 5.3.1
- [`[PR #1359]`](https://github.com/spark/firmware/pull/1359) Fixes build with `PLATFORM=gcc` on OSX with clang's gcc wrapper


## 0.7.0-rc.1

### FEATURES

- [`[PR #1245]`](https://github.com/spark/firmware/pull/1245) `Particle.publish()` now able to use Future API
- [`[PR #1289]`](https://github.com/spark/firmware/pull/1289) [`[Implements #914]`](https://github.com/spark/firmware/issues/914) `[Photon/P1]` WPA/WPA2 Enterprise support added (PEAP/MSCHAPv2 and EAP-TLS)! [Photon/P1] Automatic cipher/security detection when configuring WiFi settings over Serial.

### ENHANCEMENTS

- [`[PR #1242]`](https://github.com/spark/firmware/pull/1242) `[Photon/P1/Electron]` DFU transfer speeds increased! v100 bootloader is now 41% faster than v7 and 60% faster than the latest v11.
- [`[PR #1236]`](https://github.com/spark/firmware/pull/1236) [`[Fixes #1201]`](https://github.com/spark/firmware/issues/1201) [`[Fixes #1194]`](https://github.com/spark/firmware/issues/1194) Added type-safe wrapper for enum-based flags for `Particle.publish()` which enables logical OR'ed flag combinations `PRIVATE | WITH_ACK`
- [`[PR #1247]`](https://github.com/spark/firmware/pull/1247) Adds error checking to `WiFi.setCredentials()`, will return `true` if credentials has been stored successfully, or `false` otherwise.
- [`[PR #1248]`](https://github.com/spark/firmware/pull/1248) Added an overload to `map()` function that takes `double` arguments.
- [`[PR #1296]`](https://github.com/spark/firmware/pull/1296) `[Photon/P1]` Added support for setting a custom DNS hostname, default is device ID.
- [`[PR #1260]`](https://github.com/spark/firmware/pull/1260) [`[Implements #1067]`](https://github.com/spark/firmware/issues/1067) Adds ability to interrupt the blinking cyan cloud connection with the SETUP/MODE button.
- [`[PR #1271]`](https://github.com/spark/firmware/pull/1271) [`[Implements #1180]`](https://github.com/spark/firmware/issues/1180) `[Photon/P1]` Constrains `WiFi.RSSI()` to -1dBm max.
- [`[PR #1270]`](https://github.com/spark/firmware/pull/1270) Removes `spark/device/ota_result` event and instead sends OTA'd module info as a payload in UpdateDone message, or as an ACK to UpdateDone.
- [`[PR #1300]`](https://github.com/spark/firmware/pull/1300) Restores public server key and server address if missing
- [`[PR #1325]`](https://github.com/spark/firmware/pull/1325) Use backup registers instead of DCT to store system flags to avoid chance of a DCT corruption.
- [`[PR #1306]`](https://github.com/spark/firmware/pull/1306) Bootloader module dependency and integrity checks have been added to system-part2.  If they fail, the device is forced into safe mode and a new bootloader will be OTA transferred to the device.
- [`[PR #1329]`](https://github.com/spark/firmware/pull/1329) Adds a verification and retry scheme to the bootloader flashing routine.
- [`[PR #1330]`](https://github.com/spark/firmware/pull/1330) `[Electron]` Added CRC checking to the Electron DCD implementation so that write errors are detected. Added a critical section around flash operations and around DCD operations to make them thread safe.
- [`[PR #1307]`](https://github.com/spark/firmware/pull/1307) `[Photon/P1]` New version of WICED adds CRC checking to the DCT implementation so that write errors are detected. Added a critical section around flash operations and around DCT operations to make them thread safe.
- [`[PR #1269]`](https://github.com/spark/firmware/pull/1269) [`[Closes #1165]`](https://github.com/spark/firmware/issues/1165) Cloud connection can be closed gracefully allowing confirmable messages to reach the cloud before the connection is terminated

### BUGFIX

- [`[PR #1246]`](https://github.com/spark/firmware/pull/1246) Fixes possible corruption of event data in multi-threaded firmware
- [`[PR #1234]`](https://github.com/spark/firmware/pull/1234) [`[Fixes #1139]`](https://github.com/spark/firmware/issues/1139) `[Electron]` `spark/hardware/max_binary` event was sent in error, adding 69 more bytes of data to handshake (full or session resume). Also fixes other preprocessor errors.
- [`[PR #1236]`](https://github.com/spark/firmware/pull/1236) [`[Fixes #1201]`](https://github.com/spark/firmware/issues/1201) [`[Fixes #1194]`](https://github.com/spark/firmware/issues/1194) Sanitized `Particle.publish()` overloads.
- [`[PR #1237]`](https://github.com/spark/firmware/pull/1237) Fixes potential memory leak and race condition issues in `RGB.onChange()` function.
- [`[PR #1247]`](https://github.com/spark/firmware/pull/1247) Previously no null pointer checks on password argument of `WiFi.setCredentials()`.
- [`[PR #1248]`](https://github.com/spark/firmware/pull/1248) [`[Fixes #1193]`](https://github.com/spark/firmware/issues/1193) Fixes divide by zero on incorrect parameters of `map()` function.
- [`[PR #1254]`](https://github.com/spark/firmware/pull/1254) [`[Fixes #1241]`](https://github.com/spark/firmware/issues/1241) `WiFi.connecting()` was returning `false` while DHCP is resolving, will now remain `true`.
- [`[PR #1296]`](https://github.com/spark/firmware/pull/1296) [`[Fixes #1251]`](https://github.com/spark/firmware/issues/1251) `[Photon/P1]` Default Wi-Fi DNS hostname changed to device ID, to avoid spaces in name which may cause issues.
- [`[PR #1255]`](https://github.com/spark/firmware/pull/1255) [`[Fixes #1136]`](https://github.com/spark/firmware/issues/1136) `[Core]` Interrupts were disabled by default.
- [`[PR #1259]`](https://github.com/spark/firmware/pull/1259) [`[Fixes #1176]`](https://github.com/spark/firmware/issues/1176)  Makes `System.sleep(mode, seconds)` a synchronous operation in multithreaded firmware. This ensures the device is in a well-defined state before entering sleep mode.
- [`[PR #1315]`](https://github.com/spark/firmware/pull/1315) Fixes Particle Publish flag implicit conversion issue. e.g. `Particle.publish("event", "data", NO_ACK);` was previously changing event's TTL instead disabling acknowledgement of the event)
- [`[PR #1316]`](https://github.com/spark/firmware/pull/1316) Fixes LED indication when network credentials are cleared by holding the SETUP button for >10 seconds.
- [`[PR #1270]`](https://github.com/spark/firmware/pull/1270) [`[Fixes #1240]`](https://github.com/spark/firmware/issues/1240) TCP Firmware will not ACK every chunk in Fast OTA mode now.
- [`[PR #1302]`](https://github.com/spark/firmware/pull/1302) [`[Fixes #1282]`](https://github.com/spark/firmware/issues/1282) `[Electron]` `Wire1` was not working correctly.
- [`[PR #1326]`](https://github.com/spark/firmware/pull/1326) Renamed `system_error` enum to `system_error_t` to avoid conflicts with `std::system_error` class.
- [`[PR #1286]`](https://github.com/spark/firmware/pull/1286) Improves stability of TCP server implementation: 1) Update server's list of clients on a client destruction (thanks @tlangmo!), 2) TCPClient now closes underlying socket on destruction.
- [`[PR #1327]`](https://github.com/spark/firmware/pull/1327) [`[Fixes #1098]`](https://github.com/spark/firmware/issues/1098) [Photon/P1] Previously, when entering Sleep-stop mode: `System.sleep(D1, RISING, 60);` while in the process of making a Wi-Fi connection resulted in some parts of the radio still being initialized, consuming about 10-15mA more than normal.
- [`[PR #1336]`](https://github.com/spark/firmware/pull/1336) Fixes an issue with Serial when receiving consecutive multiple 64-byte transmissions from Host
- [`[PR #1337]`](https://github.com/spark/firmware/pull/1337) Fixed system attempting to enter listening mode every 1ms when the SETUP button is pressed.
- [`[PR #1289]`](https://github.com/spark/firmware/pull/1289) Fixes a stack overlap with system-part2 static RAM on Photon/P1
- [`[PR #1289]`](https://github.com/spark/firmware/pull/1289) Fixes a memory leak when Thread is terminated
- [`[PR #1289]`](https://github.com/spark/firmware/pull/1289) Fixes a deadlock in SoftAP, when connection is terminated prematurely
- [`[PR #1340]`](https://github.com/spark/firmware/pull/1340) `[Electron]` Fixes the monolithic build

### INTERNAL

- [`[PR #1313]`](https://github.com/spark/firmware/pull/1313) Compilation fixes for GCC platform
- [`[PR #1323]`](https://github.com/spark/firmware/pull/1323) USB vendor requests should be executed on system thread instead of being processed in ISR.
- [`[PR #1338]`](https://github.com/spark/firmware/pull/1338) Do not read or write feature flags from an ISR

## 0.6.4

### BUGFIXES

- Downgrade bootloader functionality in 0.6.3 would enter an infinite loop after flashing system part 1 0.7.0-rc.X using OTA/serial. `particle flash --usb`/DFU was not affected.

## 0.6.3

### ENHANCEMENTS

- Downgrade bootloader when downgrading from 0.7.0 or newer. [#1416](https://github.com/spark/firmware/pull/1416)

### BUGFIXES

- [KRACK WPA2 security fix](https://github.com/spark/firmware/pull/1419)

## 0.6.2 (same as 0.6.2-rc.2)

### FEATURES

- [[PR #1311]](https://github.com/spark/firmware/pull/1311) `[Implements CH1537] [Electron]` Added support for Twilio SIMs by default in system firmware.

### BUG FIX

- [[PR #1310]](https://github.com/spark/firmware/pull/1310) Fixes a error when `<algorithm>` has already been included before the `math.h` header. Now we only include `math.h` when Arduino compatibility is requested. (math.h was not included in 0.6.0).

## 0.6.2-rc.1

### ENHANCEMENT / BUG FIX

- [[PR #1283]](https://github.com/spark/firmware/pull/1283) [[Implements #1278]](https://github.com/spark/firmware/issues/1278) Restores 0.6.0-style Arduino compatibility by default, full Arduino compatibility when including Arduino.h

## 0.6.1 (same as 0.6.1-rc.2)

### FEATURES

- [[PR #1225]](https://github.com/spark/firmware/pull/1225) [Photon/P1/Electron] Added support for custom LED colors in bootloader v11 (Safe Mode, DFU Mode, Firmware Reset).
- [[PR #1227]](https://github.com/spark/firmware/pull/1227) [[Implements #961]](https://github.com/spark/firmware/issues/961) [Electron] Added new API for hostname IP address lookup `IPAddress ip = Cellular.resolve(hostname)`

### ENHANCEMENTS

- [[PR #1216]](https://github.com/spark/firmware/pull/1216) Improved Arduino Compatibility (now supported by default, added PARTICLE_NO_ARDUINO_COMPATIBILITY=y command line option for disabling)
- [[PR #1217]](https://github.com/spark/firmware/pull/1217) Added Windows, Mac command, & Unix/Linux meta USB keyboard scancode definitions.
- [[PR #1224]](https://github.com/spark/firmware/pull/1224) Allow the compiler to garbage collect USBKeyboard and USBMouse implementations if they are not used in user code, saving flash space.
- [[PR #1225]](https://github.com/spark/firmware/pull/1225) [Photon/P1/Electron] Combined `LEDStatus` and `LEDCustomStatus` into a single class -> `LEDStatus`

### BUG FIXES

- [[PR #1221]](https://github.com/spark/firmware/pull/1221) [[Fixes #1220]](https://github.com/spark/firmware/issues/1220) [Electron] TIM8 PWM pins (B0, B1) did not work correctly in bootloader with `RGB.mirrorTo()`
- [[PR #1222]](https://github.com/spark/firmware/pull/1222) Fixed bug in `String(const char* str, int len)` constructor when the string is longer than the specified length.
- [[PR #1225]](https://github.com/spark/firmware/pull/1225) [Photon/P1/Electron] Fixed LED indication shown during device key generation (blinking white) introduced in 0.6.1-rc.1
- [[PR #1226]](https://github.com/spark/firmware/pull/1226) [[Fixes #1181]](https://github.com/spark/firmware/issues/1181) [Photon/P1/Core] Process TCP `DESCRIBE` properly and return only one response, SYSTEM, APPLICATION, or COMBINED (ALL) describe message.  Was sending separate SYSTEM and APPLICATION previously.
- [[PR #1230]](https://github.com/spark/firmware/pull/1230) Safe Mode event was being published unconditionally introduced in 0.6.1-rc.1
- [[PR #1231]](https://github.com/spark/firmware/pull/1231) [Electron] fixes double newline parser issue on G350 introduced in 428835a 0.6.1-rc.1



## 0.6.1-rc.1

### FEATURES

- [[PR #1190]](https://github.com/spark/firmware/pull/1190) [[Implements #1114]](https://github.com/spark/firmware/issues/1114) Added ability to mirror MODE/SETUP button to any GPIO, available from time of boot, active high or low.
- [[PR #1182]](https://github.com/spark/firmware/pull/1182) [[Fixes #687]](https://github.com/spark/firmware/issues/687) [[Docs]](https://prerelease-docs.particle.io/reference/firmware/electron/#setlistentimeout-) Added `WiFi.set|getListenTimeout()` | `Cellular.set|getListenTimeout()` to override the automatic new Listening Mode timeout (Wi-Fi = no timeout by default, Cellular = 5 minute timeout by default).
- [[PR #1154]](https://github.com/spark/firmware/pull/1154) Added `low_battery` system event, which is generated when low battery condition is detected. This is when the battery falls below the SoC threshold (default 10%, max settable 32%).  The event can only be generated again if the system goes from a non-charging to charing state after the event is generated. The event doesn't carry any data.
- [[PR #1144]](https://github.com/spark/firmware/pull/1144) Added tracking of ACKs for published events (see `WITH_ACK` flag for `Particle.publish()`)
- [[PR #1135]](https://github.com/spark/firmware/pull/1135) [[Fixes #1116]](https://github.com/spark/firmware/issues/1116) [[Fixes #965]](https://github.com/spark/firmware/issues/965) New Time API's! `Time.isValid()` | `Particle.syncTimePending()` | `Particle.syncTimeDone()` | `Particle.timeSyncedLast()`
- [[PR #1127]](https://github.com/spark/firmware/pull/1127) [[PR #1213]](https://github.com/spark/firmware/pull/1213) Added support for runtime logging configuration, which allows to enable logging on already running system via USB control requests. Disabled by default to save flash memory space. (note: this feature is not fully baked with tool support)
- [[PR #1120]](https://github.com/spark/firmware/pull/1120) [[Implements #1059]](https://github.com/spark/firmware/issues/1059) [P1] Added extra spare pin to P1 (P1S6) with GPIO and PWM support.
- [[PR #1204]](https://github.com/spark/firmware/pull/1204) [[Implements #1113]](https://github.com/spark/firmware/issues/1113) RGB LED pins can be mirrored to other PWM capable pins via `RGB.mirrorTo()`. Common Anode/Cathode LED and Bootloader compatible. See PR for usage.
- [[PR #1205]](https://github.com/spark/firmware/pull/1205) [[Closes #569]](https://github.com/spark/firmware/issues/569) [[Closes #976]](https://github.com/spark/firmware/issues/976) [[Closes #1111]](https://github.com/spark/firmware/issues/1111) By implementing a centralized LED service and theme "engine" for system LED signaling, giving users the ability to apply custom LED colors and patterns for system events.

### ENHANCEMENTS

- [[PR #1191]](https://github.com/spark/firmware/pull/1191) Added more Arduino Library compatibility
- [[PR #1188]](https://github.com/spark/firmware/pull/1188) [[Implements #1152]](https://github.com/spark/firmware/issues/1152) Added SPI API's: `SPISettings` | `SPI.beginTransaction()` | `SPI.endTransaction()`
- [[PR #1169]](https://github.com/spark/firmware/pull/1169) Updated system communication logging with new logging API
- [[PR #1160]](https://github.com/spark/firmware/pull/1160) [Electron] Modem USART paused via HW_FLOW_CONTROL (RTS) before going into sleep with SLEEP_NETWORK_STANDBY. Receives and buffers small messages while system sleeping.
- [[PR #1159]](https://github.com/spark/firmware/pull/1159) [[Closes #1085]](https://github.com/spark/firmware/issues/1085) [[Closes #1054]](https://github.com/spark/firmware/issues/1054) Added support for GCC 5.4.x
- [[PR #1151]](https://github.com/spark/firmware/pull/1151) [[Closes #977]](https://github.com/spark/firmware/issues/977) Added System events for cloud/network connection state changes
- [[PR #1122]](https://github.com/spark/firmware/pull/1122) Attach to host even if Serial, USBSerial1 and Keyboard/Mouse are disabled, so that "Control Interface" that receives vendor requests is still accessible.
- [[PR #1097]](https://github.com/spark/firmware/pull/1097) [[Implements #1032]](https://github.com/spark/firmware/issues/1032) When flashing (OTA/YModem) an invalid firmware binary (that the device ignores) it will post an event describing why the binary was not applied.
- [[PR #1203]](https://github.com/spark/firmware/pull/1203) [[PR #1212]](https://github.com/spark/firmware/pull/1212) Automatic bootloader updates have returned to the Electron.  v9 bootloader has been added to firmware release >=0.6.1-rc.1 for Photon/P1/Electron.  After updating your system firmware, a new v9 bootloader will be applied to your device if required.  v9 includes support for SETUP/MODE button and RGB LED mirroring at the bootloader level of operation.  Also included are updates to USB DFU mode so that Windows users do not need to install separate drivers via Zadig.  Bootloader GREEN and WHITE LED flashing speeds (Firmware Reset modes) are faster now as well (you won't see these unless you have loaded user firmware to the Backup location).
- [[PR #1125]](https://github.com/spark/firmware/pull/1125) Breaks on-going network connection when Sleep stop mode is called, thereby speeding up the time to entering sleep when using SYSTEM_THREAD(ENABLED).

### BUGFIX

- [[PR #1186]](https://github.com/spark/firmware/pull/1186) Fixed issue where USB `Serial` might deadlock when interrupts are disabled while using `DEBUG_BUILD=y`
- [[PR #1179]](https://github.com/spark/firmware/pull/1179) [[Fixes #1178]](https://github.com/spark/firmware/issues/1178) [[Fixes #1060]](https://github.com/spark/firmware/issues/1160) [Electron] Bootloader build was failing, fixed and added to CI.
- [[PR #1158]](https://github.com/spark/firmware/pull/1158) [[Fixes #1133]](https://github.com/spark/firmware/issues/1133) [Electron] Before sleeping, now waits for server sent confirmable messages to be acknowledged, in addition to previous behavior of device generated confirmable messages being acknowledged.  Reduces data usage.
- [[PR #1156]](https://github.com/spark/firmware/pull/1156) [[Fixes #1155]](https://github.com/spark/firmware/pull/1156) System.sleep(30) wasn't reapplying power to the network device after set time.
- [[PR #1147]](https://github.com/spark/firmware/pull/1147) [Electron] Fixed approx. -0.1V offset on FuelGauge().getVCell() readings
- [[PR #1145]](https://github.com/spark/firmware/pull/1145) [[Fixes #973]](https://github.com/spark/firmware/issues/973) `Particle.connect()` now blocks `loop()` from running until `Particle.connected()` is `true` in single threaded SEMI_AUTOMATIC mode.
- [[PR #1140]](https://github.com/spark/firmware/pull/1140) [[Fixes #1138]](https://github.com/spark/firmware/issues/1138) [[Fixes #1104]](https://github.com/spark/firmware/issues/1104) [Electron] Fixed modem USART and buffer handling
- [[PR #1130]](https://github.com/spark/firmware/pull/1130) Particle.subscribe() used with same events but changing scope between PUBLIC and PRIVATE or vice versa would potentially result in non-registered subscriptions.  This was also crashing the GCC virtual device with a segfault when subscription checksums were calculated.

### INTERNAL

- [[PR #1196]](https://github.com/spark/firmware/pull/1196) Re-enable GNU extensions for libc globally. Fixes build with ARM GCC 4.9.3 Q1.
- [[PR #1189]](https://github.com/spark/firmware/pull/1189) Typo caused a warning during compilation in wiring/no_fixture Cellular tests.
- [[PR #1184]](https://github.com/spark/firmware/pull/1184) [Electron] moved cellular HAL and its direct dependencies from module 2 to module 3 to free up space (this is system-part3 was reduced in size, while system-part1 was increased)
- [[PR #1167]](https://github.com/spark/firmware/pull/1167) [[Fixes #1036]](https://github.com/spark/firmware/issues/1036) [GCC Virtual Device] workaround for 100% CPU usage problem.
- [[PR #1146]](https://github.com/spark/firmware/pull/1146) [[Closes #1040]](https://github.com/spark/firmware/issues/1040) Added asserts for checking that network calls are run on system thread.
- [[PR #1134]](https://github.com/spark/firmware/pull/1134) [GCC Virtual Device] Error in socket_hal's socket_receive() logic caused random cloud connection errors.


## v0.6.0 (same as v0.6.0-rc.2)

### ENHANCEMENTS

- USB HID enhancements, please see PR: [#1110](https://github.com/spark/firmware/pull/1110) for a list. Closes [#1096](https://github.com/spark/firmware/issues/1096)

### BUGFIX

- Consecutive HID reports were overwriting previous the report before it was delivered to the host. Fixes [#1090](https://github.com/spark/firmware/issues/1090).
- Disabling multiple USB configurations (normal/high power) as this breaks composite driver on Windows. Fixes [#1089](https://github.com/spark/firmware/issues/1089) Serial and USBSerial1 not working at same time on Windows 8.1 Pro.
- Do not run the event loop from delay() when threading is enabled. Fixes [#1055](https://github.com/spark/firmware/issues/1055)
- Cancel current connection attempt before entering the listening mode with WiFi.listen(true) and also WiFi.off(). Fixes [#1013](https://github.com/spark/firmware/issues/1013)

### INTERNAL

- Removed hardcoded server IP that was used when DNS resolution fails. Instead, the cloud connection is failed and the system will have to retry.  This means DNS lookup failure is now consistent with other modes of connection failure.  Addresses #139 Related to #1024


## v0.6.0-rc.1

### BREAKING CHANGES
- `UDP.flush()` and `TCP.flush()`  now conform to the `Stream.flush()` behavior from Arduino 1.0 Wiring. The current (correct) behavior is to wait
  until all data has been transmitted. Previous behavior discarded data in the buffer. [#469](https://github.com/spark/firmware/issues/469)

### FEATURES
- [Logging](https://docs.staging.particle.io/reference/firmware/electron/#logging) library for flexible system and application logging. [Docs](https://docs.staging.particle.io/reference/firmware/electron/#logging)
- [Electron] Reduced data consumption connecting to the cloud with deep sleep. ([See the Docs](https://docs.staging.particle.io/reference/firmware/electron/#optimizing-cellular-data-use-with-cloud-connectivity-on-the-electron) for how to gain the full data reduction.) [#953](https://github.com/spark/firmware/pull/953)
- Can set Claim Code via the Serial interface (for use by the CLI.) [#602](https://github.com/spark/firmware/issues/602)
- Device ID available via dfu-util. [#949](https://github.com/spark/firmware/pull/949)
- [Electron] Firmware Reset now available. [#975](https://github.com/spark/firmware/pull/975) and  [Docs](https://docs.particle.io/guide/getting-started/modes/electron/#firmware-reset)
- System Reset Reason [#403](https://github.com/spark/firmware/issues/403) [Docs](https://docs.staging.particle.io/reference/firmware/electron/#reset-reason)
- [Photon/Electron/P1] Composite USB device driver with HID Mouse & Keyboard implementation for STM32F2 [#902](https://github.com/spark/firmware/pull/902) and [#528](https://github.com/spark/firmware/issues/528)
- Exposes Device ID and Bootloader Version through USB descriptors while in DFU mode, Microsoft WCID support [#1001](https://github.com/spark/firmware/pull/1001)
- USB vendor-specific setup request handling [#1010](https://github.com/spark/firmware/pull/1010)
- [Electron] now allows OTA bootloader updates [#1002](https://github.com/spark/firmware/pull/1002)
- Added Daylight Saving Time support [#1058](https://github.com/spark/firmware/pull/1058) per proposed [#211](https://github.com/spark/firmware/issues/211) [Docs](https://docs.staging.particle.io/reference/firmware/electron/#local-)

### ENHANCEMENTS

- Local build warns if crc32 is not present. [#941](https://github.com/spark/firmware/issues/941)
- [Photon/Core] MAC address is available immediately after `WiFi.on()` [#879](https://github.com/spark/firmware/issues/879)
- [virtual device] support for TCP Server [#1000](https://github.com/spark/firmware/pull/1000)
- [virtual device] support for EEPROM emulation [#1004](https://github.com/spark/firmware/pull/1004)
- Low-level RTOS queues exposed in HAL [#1018](https://github.com/spark/firmware/pull/1018)
- USART LIN bus support. [#930](https://github.com/spark/firmware/pull/930) [Docs](https://docs.staging.particle.io/reference/firmware/electron/#begin--1)
- USART added support for 7E1, 7E2, 7O1, 7O2 modes. [#997](https://github.com/spark/firmware/pull/997) [Docs](https://docs.staging.particle.io/reference/firmware/electron/#begin--1)
- Configurable resolution for analogWrite (PWM and DAC) [#991](https://github.com/spark/firmware/pull/991) [analogWrite() Docs](https://docs.staging.particle.io/reference/firmware/electron/#analogwrite-pwm-) | [analogWriteResolution() Docs](https://docs.staging.particle.io/reference/firmware/electron/#analogwriteresolution-pwm-and-dac-) |  [analogWriteMaxFrequency() Docs](https://docs.staging.particle.io/reference/firmware/electron/#analogwritemaxfrequency-pwm-)
- [System flag](https://docs.particle.io/reference/firmware/core/#system-flags) `SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS` to control if the device resets the network when it cannot connect to the cloud. [#946](https://github.com/spark/firmware/pull/946)
- [Photon] 1KB system backup memory added (same size as Electron) reducing user backup memory to 3KB (3068 bytes) [#1046](https://github.com/spark/firmware/pull/1046)
- Automatically adds vendored libraries from the `lib` directory for extended application projects [#1053](https://github.com/spark/firmware/pull/1053)
- Extended spi_master_slave tests with SPI_MODE0/1/2/3 and MSBFIRST/LSBFIRST testing [#1056](https://github.com/spark/firmware/pull/1056)
- [Electron] System parts reordered from 3,1,2 to 1,2,3 to preserve logical flashing order for OTA/YModem when upgrading. [#1065](https://github.com/spark/firmware/pull/1065)

### BUGFIXES

- SoftAP mode persisting when setup complete if Wi-Fi was off. [#971](https://github.com/spark/firmware/issues/971)
- Free memory allocated for previous system interrupt handler [#951](https://github.com/spark/firmware/pull/951) fixes [#927](https://github.com/spark/firmware/issues/927)
- Fixes to I2C Slave mode implementation with clock stretching enabled [#931](https://github.com/spark/firmware/pull/931)
- `millis()`/`micros()` are now atomic to ensure monotonic values. Fixes [#916](https://github.com/spark/firmware/issues/916), [#925](https://github.com/spark/firmware/issues/925) and [#1042](https://github.com/spark/firmware/issues/1042)
- availableForWrite() was reporting bytes available instead of bytes available for write [#1020](https://github.com/spark/firmware/pull/1020) and [#1017](https://github.com/spark/firmware/issues/1017)
- `digitalRead()` interferes with `analogRead()` [#993](https://github.com/spark/firmware/issues/993)
- USART 9-bit receiving. [#968](https://github.com/spark/firmware/issues/968)
- Fix soft AP suffix broken by the addition of device id in DCT [#1030](https://github.com/spark/firmware/pull/1030)
- WKP pin should not be enabled as a wakeup source unconditionally for STOP mode [#948](https://github.com/spark/firmware/pull/948) and [#938](https://github.com/spark/firmware/issues/938)
- General I2C Improvements and MCP23017 tests [#1047](https://github.com/spark/firmware/pull/1047)
- Rebuilt Wiced_Network_LwIP_FreeRTOS.a WWD_for_SDIO_FreeRTOS.a on OSX [#1057](https://github.com/spark/firmware/pull/1057) fixes Local build stalling on object dump [#1049](https://github.com/spark/firmware/issues/1049)
- Validates that module dependencies would still be satisfied after the module from the "ota_module" location is flashed (via OTA or YMODEM flashing) [#1063](https://github.com/spark/firmware/pull/1063)
- System.sleep SLEEP_MODE_DEEP timing accuracy and sleep STOP mode retains user interrupt handler after resuming [#1051](https://github.com/spark/firmware/pull/1051) fixes [#1043](https://github.com/spark/firmware/issues/1043) and [#1029](https://github.com/spark/firmware/issues/1029)

### INTERNAL

- [Electron] Use floating point arithmetic in PWM to save about 1KB of flash space [#1027](https://github.com/spark/firmware/pull/1027)
- Feature/vendorlibraries [#1009](https://github.com/spark/firmware/pull/1009)
- [Electron] Added a 3rd system module to provide room for additional system firmware [#1035](https://github.com/spark/firmware/pull/1035)
- Remove accidental SYSTEM_MODE(MANUAL) from pwm.cpp in wiring/no_fixture [#1052](https://github.com/spark/firmware/pull/1052)

## v0.5.5

### ENHANCEMENTS

- Downgrade bootloader when downgrading from 0.7.0 or newer. [#1417](https://github.com/spark/firmware/pull/1417)

### BUGFIXES

- [KRACK WPA2 security fix](https://github.com/spark/firmware/pull/1418)

## v0.5.4

### ENHANCEMENTS

- [`[PR #1353]`](https://github.com/spark/firmware/pull/1353) Sticker Rig support added for Electron manufacturing.  6-Digit device setup code added.  Firmware reset enabled on Electron.

## v0.5.3 (same as v0.5.3-rc.3)

### ENHANCEMENTS

- Automatically adds vendored libraries from the `lib` directory for extended application projects [#1053](https://github.com/spark/firmware/pull/1053)

### INTERNAL

- Feature/vendorlibraries [#1009](https://github.com/spark/firmware/pull/1009)


## v0.5.3-rc.2

### FEATURE

- DTR/RTS support (open/closed detection: `Serial.isConnected()`). [#1073](https://github.com/spark/firmware/pull/1073)

### ENHANCEMENTS

- [Electron] System firmware is now aware of system-part3 to allow OTA/YModem upgrade from >=0.5.3-rc.2 to >=0.6.0-rc.1

### BUGFIXES

- added HAL_IsISR() which is used to skip calling the background loop from delay(). fixes [#673](https://github.com/spark/firmware/issue/673)
- Fixes an issue of USB Serial erroneously switching to closed state. [#1073](https://github.com/spark/firmware/pull/1073)
- RTC wakeup time now calculated right before entering SLEEP_MODE_DEEP. Fixes [#1043](https://github.com/spark/firmware/issue/1043)
- STOP mode should retain user interrupt handler. Fixes [#1029](https://github.com/spark/firmware/issue/1029)


## v0.5.3-rc.1

### BUGFIXES

- SoftAP mode persisting when setup complete if Wi-Fi was off. [#971](https://github.com/spark/firmware/issues/971)
- Free memory allocated for previous system interrupt handler [#951](https://github.com/spark/firmware/pull/951) fixes [#927](https://github.com/spark/firmware/issues/927)
- availableForWrite() was reporting bytes available instead of bytes available for write [#1020](https://github.com/spark/firmware/pull/1020) and [#1017](https://github.com/spark/firmware/issues/1017)
- `millis()`/`micros()` are now atomic to ensure monotonic values. Fixes [#916](https://github.com/spark/firmware/issues/916), [#925](https://github.com/spark/firmware/issues/925) and [#1042](https://github.com/spark/firmware/issues/1042)
- Fixes to I2C Slave mode implementation with clock stretching enabled [#931](https://github.com/spark/firmware/pull/931)
- General I2C Improvements and MCP23017 tests [#1047](https://github.com/spark/firmware/pull/1047)
- Rebuilt Wiced_Network_LwIP_FreeRTOS.a WWD_for_SDIO_FreeRTOS.a on OSX [#1057](https://github.com/spark/firmware/pull/1057) fixes Local build stalling on object dump [#1049](https://github.com/spark/firmware/issues/1049)
- `digitalRead()` interfered with `analogRead()` [#1006](https://github.com/spark/firmware/pull/1006) fixes [#993](https://github.com/spark/firmware/issues/993)
- Validates that module dependencies would still be satisfied after the module from the "ota_module" location is flashed (via OTA or YMODEM flashing) [#1063](https://github.com/spark/firmware/pull/1063)


## v0.5.2 (same as v0.5.2-rc.1)

### ENHANCEMENTS

- [Photon/P1] Restores the default WICED country to Japan [#1014](https://github.com/spark/firmware/pull/1014)

### BUGFIXES

- .syncTime() and .unsubscribe() called on the system thread. Prevents issues when multiple threads try to send messages through the cloud connection or manage the network state shared memory. [#1041](https://github.com/spark/firmware/pull/1041)


## v0.5.1 (same as v0.5.1-rc.2)

### FEATURES

- [Electron] Added support in HAL for a SMS received callback handler.


## v0.5.1-rc.1

### FEATURES

- Wi-Fi Country Code can be set to configure the available channels and power transmission. [#942](https://github.com/spark/firmware/pull/942)

### ENHANCEMENTS

- ARM GCC 5.3.1 compiler support

### BUGFIXES

- [Photon/P1] Fix a timing-critical bug in WICED that causes system freeze. [#877](https://github.com/spark/firmware/issues/877)
- Tone not available on A7 after stop-mode sleep. [#938](https://github.com/spark/firmware/issues/938)
- Regression in EEPROM emulation size. [#983](https://github.com/spark/firmware/pull/983)
- [Electron] Wrong bitmask is provided for 4208 setting in power management [#987](https://github.com/spark/firmware/pull/987)


## v0.5.0 (same as v0.5.0-rc.2)

### FEATURES

- Added SYSTEM_FLAG_WIFITEST_OVER_SERIAL1 which is disabled by default. Tinker enables this by default so that the Wi-Fi Tester is available during manufacturing.  Also ensures TX/RX pins are not used for Serial1 by default, in case you want to use these as GPIO. [945](https://github.com/spark/firmware/pull/945)

### ENHANCEMENTS

- Timer::isActive() function added [#950](https://github.com/spark/firmware/pull/950)
- mbedtls headers are private to the communications module now, so user applications can include their own version of mbedtls [](https://github.com/spark/firmware/pull/940)

### BUGFIXES

- Soft AP Claim code fix [#956](https://github.com/spark/firmware/pull/956)
- Variable template fix [#952](https://github.com/spark/firmware/pull/952)
- TCPClient on Electron not receiving all of the data for small files [#896](https://github.com/spark/firmware/issues/896)

## v0.5.0-rc.1

### FEATURES

- [Electron] [SYSTEM_THREAD()](https://docs.particle.io/reference/firmware/electron/#system-thread) is supported (in Beta) [#884](https://github.com/spark/firmware/pull/884)
- [Electron] Cellular [Data Usage API](https://docs.particle.io/reference/firmware/electron/#getdatausage-) [#866](https://github.com/spark/firmware/pull/866)
- [Electron] Configurable keep-alive ping [#913](https://github.com/spark/firmware/pull/913)
- [Electron] Cellular [Band Select API](https://docs.particle.io/reference/firmware/electron/#getbandavailable-) [#891](https://github.com/spark/firmware/pull/891)
- [Electron] Cellular [Local IP API](https://docs.particle.io/reference/firmware/electron/#localip-) [#850](https://github.com/spark/firmware/pull/850)
- [Photon/Electron] Stack overflow detection with SOS code [13-blinks](https://docs.particle.io/guide/getting-started/modes/photon/#red-flash-sos)
- [Photon/Electron] [SPI Slave support](https://docs.particle.io/reference/firmware/photon/#begin-spi_mode-uint16_t-) [#882](https://github.com/spark/firmware/pull/882)
- Atomic writes in [EEPROM emulation](https://docs.particle.io/reference/firmware/electron/#eeprom) [#871](https://github.com/spark/firmware/pull/871)
- Software Watchdog [#860](https://github.com/spark/firmware/pull/860)
- [Serial.availableForWrite()](https://docs.particle.io/reference/firmware/photon/#availableforwrite-) and [Serial.blockOnOverrun()](https://docs.particle.io/reference/firmware/photon/#blockonoverrun-) [#798](https://github.com/spark/firmware/issues/798)
- [Photon] SoftAP HTTP server can serve application pages. [#906](https://github.com/spark/firmware/pull/906)

### ENHANCEMENTS

- Compiler error with variable/function names that are too long. [#883](https://github.com/spark/firmware/pull/883)
- DFU writes are verified [#870](https://github.com/spark/firmware/pull/870)
- [Electron] [NO_ACK flag](https://docs.particle.io/reference/firmware/electron/#particle-publish-) on `Particle.publish()` disables acknoweldgements reducing data use [#862](https://github.com/spark/firmware/pull/862)
- [Electron] Allow session to resume when IP changes. [#848](https://github.com/spark/firmware/pull/848)
- [Electron] Ensure published events are received by the cloud before sleeping. [#909](https://github.com/spark/firmware/pull/909)
- [Electron] [SLEEP_NETWORK_STANDBY on System.sleep()](https://docs.particle.io/reference/firmware/electron/#sleep-sleep-) [#845](https://github.com/spark/firmware/pull/845)
- Serial baudrate to select ymodem mode includes listening mode [#912](https://github.com/spark/firmware/pull/912)
- Wi-Fi connection process forced to timeout after 60 seconds if unsuccessful [#898](https://github.com/spark/firmware/pull/898)
- Added write-verify-retry-fail logic to DFU writes [#870](https://github.com/spark/firmware/pull/870)
- Support for USART (Serial1/2/4/5) [data bits, parity and stop bits](https://docs.particle.io/reference/firmware/electron/#begin-) [#757](https://github.com/spark/firmware/pull/757)

### BUGFIXES

- targets `program-cloud`, `program-dfu` can be used without requiring `all` and will built the firmware correctly. [#899](https://github.com/spark/firmware/issues/899)
- [Electron] Free socket when the socket is closed remotely [#885](https://github.com/spark/firmware/pull/885)
- Extended CAN filters [#857](https://github.com/spark/firmware/pull/857)
- I2C does not ensure a stop condition completes correctly in endTransmission [#856](https://github.com/spark/firmware/pull/856)
- DAC1/2 possible problem with `digitalWrite()` after `analogWrite()` [#855](https://github.com/spark/firmware/pull/855)
- Servo HAL: Do not disable timer if some of its channels are still in use [#839](https://github.com/spark/firmware/pull/839)
- USB driver fixes and Serial.available() not returning values greater than 1 [#812](https://github.com/spark/firmware/pull/812) [#669](https://github.com/spark/firmware/issues/669) [#846](https://github.com/spark/firmware/issues/846) [#923](https://github.com/spark/firmware/issues/923)
- SOS During `WiFi.scan()` [#651](https://github.com/spark/firmware/issues/651)


### INTERNALS

- dynalib: compile-time check for certain types of ABI breaking changes [#895](https://github.com/spark/firmware/pull/895)


## v0.4.9

### FEATURES

- Support for CAN Bus [#790](https://github.com/spark/firmware/pull/790)
- [blockOnOverrun()]((https://docs.particle.io/reference/firmware/photon/#blockonoverrun-)) on hardware serial to allow applications to disable the default flow control.
- [availableForWrite()]((https://docs.particle.io/reference/firmware/photon/#availableforwrite-)) on hardware serial to allow applications to implement flow control. [#798](https://github.com/spark/firmware/issues/798)
- [WiFi.BSSID()](https://docs.particle.io/reference/firmware/photon/#wifi-bssid-) to retrieve the 6-byte MAC address of the connected AP. [#816](https://github.com/spark/firmware/pull/816)
- [attachInterrupt()](https://docs.particle.io/reference/firmware/photon/#attachinterrupt-) configurable interrupt priority [#806](https://github.com/spark/firmware/issues/806)
- [Time.local()](https://docs.particle.io/reference/firmware/photon/#local-) retrieves the current time in the configured timezone. [#783](https://github.com/spark/firmware/issues/783)
- [photon] [WiFi.getCredentials()](https://docs.particle.io/reference/firmware/photon/#wifi-getcredentials-) lists configured credentials.  [#759](https://github.com/spark/firmware/issues/759)
- variable frequency PWM via [analogWrite(pin,value,hz)](https://docs.particle.io/reference/firmware/photon/#analogwrite-) [#756](https://github.com/spark/firmware/pull/756)
- [ATOMIC_BLOCK()](https://docs.particle.io/reference/firmware/photon/#atomic_block-) and [SINGLE_THREADED_BLOCK()](https://docs.particle.io/reference/firmware/photon/#single_threaded_block-) declarations for atomicity and thread scheduling control. [#758](https://github.com/spark/firmware/issues/758)
- [API](https://docs.particle.io/reference/firmware/photon/#synchronizing-access-to-shared-system-resources) for Guarding resources for use between threads.
- System events for individual button clicks and a run of button clicks. [#818](https://github.com/spark/firmware/issues/818)

### ENHANCEMENTS

- [System.freeMemory()](https://docs.particle.io/reference/firmware/photon/#system-freememory-) shows an accurate value for free memory rather than the highwater mark for the heap.
- [threading] Entering listening mode does not block the system thread. [#788](https://github.com/spark/firmware/issues/788)
- [threading] System times out waiting for unresponsive application when attempting to reset. [#763](https://github.com/spark/firmware/issues/763)
- [threading] `Particle.publish()` doesn't block when in listening mode.  [#761](https://github.com/spark/firmware/issues/761)
- [threading]. `delay()`/`Particle.process()` pumps application events.
- Serial, Serial1, SPI and EEPROM global objects guaranteed to be initialized before use. (Prevents White breathing LED if Serial used in a global instance constructor.)
- [Software Timers]((https://docs.particle.io/reference/firmware/photon/#software-timers) have an option for one-shot timers, and support class member function callbacks.

### BUGFIXES

- RSA key generation would sometimes produce invalid keys. [#779](https://github.com/spark/firmware/pull/779)
- Static IP configuration was not being used.
- Interrupt on WKP with class method as an ISR [#819](https://github.com/spark/firmware/issues/819)
- Memory leak configuring WiFi credentials via SoftAP (TCP/HTTP)
- SPI DMA transfer callback invoked too early [#791](https://github.com/spark/firmware/issues/791)
- Unset `wiced_result_t` for tcp clients in `socket_send`.  [#773](https://github.com/spark/firmware/issues/773)
- Update bootloader to support `System.enterSafeMode()`. [#751](https://github.com/spark/firmware/issues/751)
- [threading] `WiFi.listen(false)` remains in listen mode. [#743](https://github.com/spark/firmware/issues/743)
- Factory Reset doesn't clear WiFi credentials until `network.connect()`. [#736](https://github.com/spark/firmware/issues/736)
- Comparison between `IPAddress` objects does not always work.  [#715](https://github.com/spark/firmware/issues/715)
- P1 dfu-util 0.8 does not read/write from External Flash. [#706](https://github.com/spark/firmware/issues/706)
- DFU errors writing to flash silently ignored. [#813](https://github.com/spark/firmware/pull/813)
- [threading] heap allocation not thread-safe. [#826](https://github.com/spark/firmware/issues/826)
- System crash when button interrupt occurs during i2c transmission. [#709](https://github.com/spark/firmware/issues/709)
- [photon] 'analogWrite()` to DAC pins.  [#671](https://github.com/spark/firmware/issues/671)
- [photon] `analogWrite()` to DAC pins requires `pinMode()` each time. [#662](https://github.com/spark/firmware/issues/662)
- [photon] `System.sleep(pin,edge)` doesn't wake.  [#655](https://github.com/spark/firmware/issues/655)


## v0.4.8-rc.1

### FEATURE

- factory firmware uses full modular firmware [#749](https://github.com/spark/firmware/pull/749). No need for UpdateZero.

- `Timer.changePeriod()` [#720](https://github.com/spark/firmware/pull/720)

### BUGFIXES

- [photon] hang when UDP::stop() is called [#742](https://github.com/spark/firmware/issues/742)
- [photon] I2C hangs with no pullup resistors [#713](https://github.com/spark/firmware/issues/713)

## v0.4.7

### FEATURES

 - [Software Timers](https://docs.particle.io/reference/firmware/photon/#software-timers)
 - [pulseIn(pin, value)](https://docs.particle.io/reference/firmware/photon/#pulsein-) now available for all devices.
 - [WiFi.dnsServerIP()](https://docs.particle.io/reference/firmware/core/#wifi-dnsserverip-) and [WiFi.dhcpServerIP()](https://docs.particle.io/reference/firmware/core/#wifi-dhcpserverip-)
 - [serialEvent()](https://docs.particle.io/reference/firmware/core/#serialevent-)
 - [GCC virtual device](https://github.com/spark/firmware/tree/develop/hal/src/gcc#device-configuration)
 - [System.version()](https://docs.particle.io/reference/firmware/photon/#system-version-) to retrieve the version of system firmware [#688](https://github.com/spark/firmware/issues/688)
 - Firmware control of [OTA updates](https://docs.particle.io/reference/firmware/core/#ota-updates) can happen [#375](https://github.com/spark/firmware/issues/375)

### ENHANCEMENTS

 - [multithreading] Application thread continues to run in listening mode
 - [multithreading] `Particle.process()` called from the application thread pumps application messages [#659](https://github.com/spark/firmware/issues/659)
 - `Particle.variable()` supports `String`s [#657](https://github.com/spark/firmware/issues/657)
 - Simplified [Particle.variable()](https://docs.particle.io/reference/firmware/photon/#variables) API - variable type parameter is optional, and variables are passed by reference so  `&`'s are not required.
 - I2C will generate STOP and SW Reset immediately if Slave Acknowledge failure is detected (must use pull-up resistors), instead of taking 100ms. [commit](https://github.com/spark/firmware/commit/893aa523990752a2afcda4ffa5bba3f66e047014)

### BUGFIXES

 - TCPClient unstable [#672](https://github.com/spark/firmware/issues/672)
 - Photon frequently SOS's immediately following cloud re-connect [#663](https://github.com/spark/firmware/issues/663)
 - `String.toLower()` has no affect on string. [#665](https://github.com/spark/firmware/issues/665)
 - SOS due to WICED socket handlers being called when socket is disposed. [#663](https://github.com/spark/firmware/issues/663) [#672](https://github.com/spark/firmware/issues/672)
 - Application constructors executed after RTOS startup so that HAL_Delay_Milliseconds() can be called. This may mean that `STARTUP()` code executes just a little later than before, but
    can safely use all public APIs.
 - Ensure bootloader region is write protected.
 - White breathing LED on exiting listening mode. [#682](https://github.com/spark/firmware/issues/682)
 - WICED not resolving DNS names with 4 parts (it was trying to decode as an IP address.)
 - SoftAP via HTTP would fail on Safari due to request sent as multiple TCP packets. Fixed WICED HTTP server.  [#680](https://github.com/spark/firmware/issues/680)
 - Retained variables are not persisting, even without reset or deep sleep. [#661](https://github.com/spark/firmware/issues/661)
 - Backup RAM enabled for monolithic builds [#667](https://github.com/spark/firmware/issues/667)
 - Pure virtual call on creation of low priority std::thread [#652](https://github.com/spark/firmware/issues/652)


## v0.4.6

### FEATURES
 - [photon] separate [System Thread](https://docs.particle.io/reference/firmware/photon/#system-thread)
 - [core] Hooks to support FreeRTOS (optional library)
 - Variables stored in [Backup RAM](https://docs.particle.io/reference/firmware/photon/#backup-ram)
 - [printf/printlnf](https://docs.particle.io/reference/firmware/core/#printf-) on `Print` classes - `Serial`, `Serial1`, `TCP`, `UDP`
 - `String.format` for printf-style formatting of to as `String`.
 - [Wire.end()](https://docs.particle.io/reference/firmware/photon/#end-) to release the I2C pins. [#597](https://github.com/spark/firmware/issues/597)
 - [Wire.reset()](https://docs.particle.io/reference/firmware/photon/#reset-) to reset the I2C bus. Thanks @pomplesiegel [#598](https://github.com/spark/firmware/issues/598)
 - [System.ticks()](https://docs.particle.io/reference/firmware/core/#system-cycle-counter) to retrieve the current MCU cycle counter for precise timing.
 - [System.enterSafeMode()](https://docs.particle.io/reference/firmware/core/#system-entersafemode-) to restart the device in safe mode.

### ENHANCEMENTS

 - [photon] `WiFi.selectAntenna()` setting is persistent, so the last selected antenna is used when the
device is in safe mode. [#618]
 - Detect when the cloud hasn't been serviced for 15s and disconnect, so device LED state accurately
reflects the connection state when the application loop has stalled. [#626](https://github.com/spark/firmwarwe/issues/626)
 - Compile-time checks for `Particle.variable()` [#619](https://github.com/spark/firmwarwe/issues/619)
- [photon] Increased retry count when connecting to WiFi. [#620](https://github.com/spark/firmware/issues/620)
- Setup button events [#611](https://github.com/spark/firmware/pull/611)

### BUGFIXES

 - `UDP.receivePacket()` would fail if `UDP.setBuffer()` hadn't been called first. Thanks @r2jitu.
 - [photon] Default SS pin for SPI1 now set to D5. [#623](https://github.com/spark/firmware/issues/623)
 - [photon] Long delay entering listening mode. [#566](https://github.com/spark/firmware/issues/566)
 - [photon] Solid green LED when WiFi network cannot be connected to due to invalid key. (The LED now blinks.)
 - [photon] Storing more than 2 Wi-Fi credentials would sometimes give unpredictable results.
 - [photon] TX/RX pins did not work after entering listening mode. [#632](https://github.com/spark/firmware/issues/632)
 - [photon] Improvements to I2C for MCP23017 / Adafruit RGBLCDShield. [#626](https://github.com/spark/firmware/pull/626)


## v0.4.5

### FEATURES
- `SPI.setClockDividerReference`, `SPI.setClockSpeed` to set clock speed in a more portable manner. [#454](https://github.com/spark/firmware/issues/454)
- `WiFi.scan` function to retrieve details of local access points. [#567](https://github.com/spark/firmware/pull/567)
- `UDP.sendPacket`/`UDP.receivePacket` to send/receive a packet directly to an application-supplied buffer. [#452](https://github.com/spark/firmware/pull/452)
- Static IP Support [photon] - [#451](https://github.com/spark/firmware/pull/451)
- [photon] UDP multicast support via `UDP.joinMulticast`/`UDP.leaveMulticast`. Many thanks @stevie67!
- `waitFor(WiFi.ready)` syntax to make it easier to wait for system events. [#415](https://github.com/spark/firmware/issues/415)
- Flexible time output with `Time.format()` [#572](https://github.com/spark/firmware/issues/572)

### ENHANCEMENTS

- [Recipes and Tips](docs/build.md#recipes-and-tips) section in the build documentation.
- `Particle.function`, `Particle.subscribe` and `attachInterrupt` can take a C++ method and instance pointer. [#534](https://github.com/spark/firmware/pull/534) Thanks to @monkbroc!
- `UDP.setBuffer` to set the buffer a UDP instance uses for `read`/`write`. [#224](https://github.com/spark/firmware/pull/224) and [#452](https://github.com/spark/firmware/pull/452)
- `WiFi.setCredentials()` can take a Cipher type to allow full specification of an AP's credentials. [#574](https://github.com/spark/firmware/pull/574)
- TCPClient (from TCPServer) reports remote IP address. [#551](https://github.com/spark/firmware/pull/551)
- Configurable format in `Time.timeStr()`, including ISO 8601. [#455](https://github.com/spark/firmware/issues/455)
- `Servo.trim(adjust)` to allow small adjustments to the stationary point. [#120](https://github.com/spark/firmware/issues/120)
- Time set from the cloud accounts for network latency. [#581](https://github.com/spark/firmware/issues/581)
- `String(Printable)` constructor so any `Printable` can be converted to a string. [example](https://community.particle.io/t/convert-ipaddress-to-string-for-use-with-spark-publish/14885/4?u=mdma)
- Fluent API on `String` - many methods return `*this` so method calls can be chained.
- Small values passed to `delay(1)` result in more accurate delays. [#260](https://github.com/spark/firmware/issues/260)
- Bootloader does not show factory reset modes if a factory reset image is not available. [#557](https://github.com/spark/firmware/issues/557)

### BUGFIXES

- Listening mode re-enters listening mode after credentials are given. [#558](https://github.com/spark/firmware/pull/558)
- String function dtoa() has problems with larger numbers. [#563](https://github.com/spark/firmware/pull/563)
- System doesn't set color of RGB LED when `RGB.control(true)` is called. [#362](https://github.com/spark/firmware/pull/362), [#472](https://github.com/spark/firmware/pull/472) and [#544](https://github.com/spark/firmware/pull/544)
- WiFi.SSID() may not return previous network when switching. [#560](https://github.com/spark/firmware/pull/560)
- [photon] System.sleep(5) not turning Wi-Fi back on after 5 seconds. [#480](https://github.com/spark/firmware/pull/480)
- regression: floating point support in sprintf not compiled in. [#576](https://github.com/spark/firmware/issues/576)
- [photon] SPI1 default clock speed was 7.5MHz, changed to 15MHz, same as for `SPI`.
- TCPClient::connected() doesn't detect when the socket is closed [#542](https://github.com/spark/firmware/issues/542)
- dfu-util: error during downlod get_status msg removed when using :leave option [#599](https://github.com/spark/firmware/issues/599)
- [Core] A0 could not be used as an output [#595](https://github.com/spark/firmware/issues/595)
- Reinstate CFOD handling on the Photon.

## v0.4.4

### FEATURES
 - logging output [documentation](docs/debugging.md)
 - pressing 'v' in SoftAP mode displays the system version. FIRM-128
 - P1: API (compatible with Core) to access the 1MByte external flash. [#498](https://github.com/spark/firmware/pull/498)
 - Arduino compatibility macros for PROGMEM and more.
 - `RGB.onChange` handler receives notification of the current LED color when it changes. Can be used to match an external LED to the onboard led. [#518](https://github.com/spark/firmware/pull/518) Thanks to @monkbroc!
 - Serial2 available on P1 and Photon (note: this also requires above RGB.onChange handler and two resistors would need to be removed on the Photon)
 - `Spark.connected()` et al. is now `Particle.connected()`. The former `Spark` library is still available but is deprecated.
 - `System.freeMemory()` API to determine the amount of available RAM.
 - `STARTUP()` macro to define blocks of code that execute at startup.

### ENHANCEMENTS
 - Retrieve the LED brightness via `RGB.brightness()`
 - More prominent color change on the RGB LED when there is a cloud connection error.
 - System.sleep() - 2nd parameter changed to `InterruptMode` from uint16_t to
 ensure the correct types are used. [#499](https://github.com/spark/firmware/pull/499)
 - Less aggressive exponential backoff when the re-establishing the cloud connection. [FIRM-177]
 - I2C Wire.endTransmission() returns unique values and [I2C docs updated](https://docs.particle.io/reference/firmware/photon/#endtransmission-)
 - Generate I2C STOP after slave addr NACK, I2C software reset all timeouts -  [commit](https://github.com/spark/firmware/commit/53914d809cc17a3802b879fbb4fddcaa7d264680)
 - Improved I2C Master receive method and implemented error handler - [commit](https://github.com/spark/firmware/commit/1bc00ea480ef1fcdbd8ef9ba3df12b121183aeae) -  [commit](https://github.com/spark/firmware/commit/5359f19985756182ff6511217cbcb588b3341a87)
 - `WiFi.selectAntenna()` default antenna is now INTERNAL. Can be called at startup (before WiFi is initialized to select the desired antenna.


### BUGFIXES

 - [Regression] System connects WiFi when Spark.connect() is called after WiFi.on() [#484](https://github.com/spark/firmware/issues/484)
 - [Debug build](https://github.com/spark/firmware/blob/develop/docs/debugging.md) now working.
 - PWM issue fixed - 500Hz output on all channels [#492](https://github.com/spark/firmware/issues/492)
 - Tone issue fixed on D2,D3,RX,TX [#483](https://github.com/spark/firmware/issues/483)
 - SOS when registering more than 2 subscription handlers, and allow 4 subscription handlers to be successfully registered. [#531](https://github.com/spark/firmware/issues/531)
 - SOS on TCPClient.connect() when DNS resolution failed or when connection fails [#490](https://github.com/spark/firmware/issues/490)
 - `TCPClient::stop()` does not work on first connection [#536](https://github.com/spark/firmware/issues/536)
 - `TCPClient::connect()` does not close an existing socket. [#538](https://github.com/spark/firmware/issues/538)
 - TX/RX PWM randomly inverted [#545](https://github.com/spark/firmware/issues/545)
 - UDP.begin/write return values [#552](https://github.com/spark/firmware/issues/552)

## v0.4.3

### FEATURES
 - Half-duplex mode on Serial1 via `Serial1.halfdupliex()`. Thanks to @prices.
 - `WiFi.connect(WIFI_CONNECT_SKIP_LISTEN)` allows application firmware to skip listen mode when there is no credentials.
 - System events

### ENHANCEMENTS
 - I2C methods now use `micros()` for timeouts rather than `millis()`, so I2C functions can be used in an interrupt handler. [#460](https://github.com/spark/firmware/issues/460)
 - `WiFi.listen(false)` to programmatically exit WiFi listening mode.
 - make is verbose by default. To silence, add `-s` to the command line.
 - `WiFi.connect(WIFI_CONNECT_SKIP_LISTEN)` starts connection but does not enter listening mode if no credentials are found.
 - Setup/Mode button now starts listening mode when WiFi is off.
 - `WiFi.listen(false)` can be used to exit listening mode (from an interrupt.)
 - LED flashes high-speed green when requesting an IP address via DHCP.

### BUGFIXES

 - [Photon/TCPServer] - `TCPClient.connected()` was not returning `false` when the socket was asynchronously disconnected.
 - Fix time being reset on wakeup. (removed WICED RTC init code that resets to default preset time in platform_mcu_powersave_init() within photon-wiced repo.) [#440](https://github.com/spark/firmware/issues/440)
 - `TCPClient.connected()` was not returning `false` when the socket was disconnected from the other end.
 - `strdup()` was returning garbage [#457](https://github.com/spark/firmware/issues/457)
 - `attachInterrupt()` should work on all interrupt pins now except D0 & A5. Please note there are shared lines as per the following issue comment : [#443] (https://github.com/spark/firmware/issues/443#issuecomment-114389744)
 - I2C bus lockup when no slave devices are present by issuing a STOP condition after sLave send address fails.
 - `spark/` events not propagated to application handlers. [#481](https://github.com/spark/firmware/issues/481)
 - `sprintf` calls not linking correctly. [#471](https://github.com/spark/firmware/issues/471)
 - Photon/P1 sometimes did not start without hitting reset after a cold boot.
 - Disable LTO compile for user firmware since it causes linking problems (see `sprintf` above.)

## v0.4.2

### FEATURES
 - EEPROM storage of custom data types via `EEPROM.put()` and `EEPROM.get()'
 - When the device is in safe mode, the LED breathes magenta
 - `attachSystemInterrupt()` allows hooking key system interrupts in user code.
 - [DMA-driven SPI master](https://github.com/spark/docs/pull/49)
 - `UDP.sendPacket()` method avoids buffering data when the user can supply the entire buffer at once.
 - [Photon] SoftAP setup can be done over HTTP
 - platform-neutral fast pin access [449](https://github.com/spark/firmware/pull/449)
 - [P1] Serial2 support

### ENHANCEMENTS

 - [Photon] The system firmware updates the bootloader to latest version
 - [Photon] The system write protects the bootloader region.
 - UDP uses dynamically allocated buffers
 - `PRODUCT_ID` and `PRODUCT_VERSION` place these details at a known place in the firmware image
 - DFU mode and serial firmware update can be triggered by setting the line rate.

### BUGFIXES

 - `Serial1.end()` [hangs the system](https://community.particle.io/t/changing-serial-baud-rate-inside-setup-code-causes-core-freezing-afterwards/10314/6)
 - Malformed CoAP acknowledgement message in cloud protocol.
 - `SPARK_WLAN_Loop()` was not linked. (Workaround was to use `Spark.process()`)
 - UDP doesn't send anything to the device until `UDP.write()` [#407](https://github.com/spark/firmware/issues/407)
 - Divide by zero now caught and causes a SOS.
 - Floating-point support for `sprintf()` reinstated
 - Fixed WICED DCT becoming unmodifiable
 - Fix UDP.parsePacket() not receiving any data on the Photon [#468](https://github.com/spark/firmware/issues/468)

## v0.4.1

### ENHANCEMENTS

- Signed Photon USB Driver for use with Windows 8.1


### BUGFIXES

 - `Spark.syncTime()` was not linked. [#426](https://github.com/spark/firmware/issues/426)
 - Wire.setSpeed(CLOCK_SPEED_100KHZ) was not linked. [#432](https://github.com/spark/firmware/issues/432)
 - WiFi.selectAntenna() was not linked.

## v0.4.0

### NEW PLATFORMS
- PHOTON!!!!


### ENHANCEMENTS

 - `loop()` iteration rate increased by 1000 times - from 200 Hz to over 200 kHz!
 - Compiler: Removed all warnings from the compile (and made warnings as errors) so compiler output is minimal.
 - Debugging: SWD Support, thanks to Elco Jacobs. [#337](https://github.com/spark/core-firmware/pull/337)
 - `Spark.publish()` returns a success value - [#388](https://github.com/spark/firmware/issues/388)
 - `Spark.process()` as the public API for running the system loop. [#347](https://github.com/spark/firmware/issues/347)
 - Sleep no longer resets (on the Photon) [#283](https://github.com/spark/firmware/issues/283)
 - Support for application code outside of the firmware repo. [#374](https://github.com/spark/firmware/issues/374)
 - MAC Address available in setup via 'm' key. [#352](https://github.com/spark/firmware/issues/352)
 - SoftAP setup on the Photon
 - `Spark.sleep()` changed to `System.sleep()` and similarly for `deviceID()` [#390](https://github.com/spark/firmware/issues/390)
 - Listening mode uses existing serial connection if already opened. [#384](https://github.com/spark/firmware/issues/384)
 - `Spark.publish("event", PRIVATE)` shorthand - [#376](https://github.com/spark/firmware/issues/376)
 - Improved integrity checks for firmware images
 - Added additional safe/recovery mode in bootloader (> 6.5 sec : restore factory code without clearing wifi credentials)
 - Enabled CRC verification in bootloader before restoring/copying the firmware image from factory reset, ota downloaded area etc.
 - Added 'program-serial' to build target to enter serial ymodem protocol for flashing user firmware (Testing pending...)
 - Cloud string variables can be re-defined [#241](https://github.com/spark/firmware/issues/241)
 - Removed hard-coded limit on number of functions and variables [#111](https://github.com/spark/firmware/issues/111)
 - Parameterized function callbacks, lambda support for functions [#311](https://github.com/spark/firmware/issues/313)
 - C++ STL headers supported
- Can duplicate the onboard RGB LED color in firmware. [#302](https://github.com/spark/firmware/issues/302)
- `WiFi.selectAntenna()` - select between internal (chip) and external (u.FL) antenna on Photon: [#394](https://github.com/spark/firmware/issues/394)
- `WiFi.resolve()` to look up an IP address from a domain name. [#91](https://github.com/spark/firmware/issues/91)
- `System.dfu()` to reboot the core in dfu mode, until next reset or next DFU update is received.


### BUGFIXES

- SOS calling `Spark.publish()` in `SEMI_AUTOMATIC`/`MANUAL` mode
- Subscriptions maintained when cloud disconnected. [#278](https://github.com/spark/firmware/issues/278)
- Fix for events with composite names. [#382](https://github.com/spark/firmware/issues/382)
- `WiFi.ready()` returning true after `WiFi.off()` in manual mode. [#378](https://github.com/spark/firmware/issues/378)
- `Serial.peek()` implemented. [#387](https://github.com/spark/firmware/issues/387)
- Mode button not working in semi-automatic or manual mode. [#343](https://github.com/spark/firmware/issues/343)
- `Time.timeStr()` had a newline at the end. [#336](https://github.com/spark/firmware/issues/336)
- `WiFi.RSSI()` caused panic in some cases. [#377](https://github.com/spark/firmware/issues/377)
- `Spark.publish()` caused SOS when cloud disconnected. [#322](https://github.com/spark/firmware/issues/332)
- `TCPClient.flush()` discards data in the socket layer also. [#416](https://github.com/spark/firmware/issues/416)


### UNDER THE HOOD

 - Platform: hardware dependencies are factored out from wiring into a hardware abstraction layer
 - Repo: all 3 spark repos (core-common-lib, core-communication-lib, core-firmware) are combined into this repo.
 - Modularization: factored common-lib into `platform`, `services` and `hal` modules.
 - Modularization: factored core-firmware into `wiring`, `system`, 'main' and `user` modules.
 - Modularization: user code compiled as a separate library in the 'user' module
 - Build system: fancy new build system - [build/readme.md](build/readme.md)
 - Modularization: modules folder containing dynamically linked modules for the Photon



## v0.3.4

### FEATURES

- Local Build: Specify custom toolchain with `GCC_PREFIX` environment variable ([firmware](https://github.com/spark/firmware/pull/328), [core-common-lib](https://github.com/spark/core-common-lib/pull/39), [core-communication-lib](https://github.com/spark/core-communication-lib/pull/29))

### ENHANCEMENTS

- Wiring: More efficient and reliable `print(String)` (fix issue [#281](https://github.com/spark/firmware/issues/281)) [#305](https://github.com/spark/firmware/pull/305)
- DFU: Add DFU suffix to .bin file [#323](https://github.com/spark/firmware/pull/323)

### BUGFIXES

- I2C: Use I2C polling mode by default [#322](https://github.com/spark/firmware/pull/322)
- Listening Mode: Fix hard fault when Wi-Fi is off [#320](https://github.com/spark/firmware/pull/320)
- LED Interaction: Fix breathing blue that should be blinking green [#315](https://github.com/spark/firmware/pull/315)


## v0.3.3

### FEATURES

 - Cloud: [Secure random seed](https://github.com/spark/core-communication-lib/pull/25). When the spark does a handshake with the cloud, it receives a random number that is set as a seed for `rand()`
 - Wiring: Arduino-compatible `random()` and `randomSeed()` functions. [#289](https://github.com/spark/core-firmware/pull/289)
 - Wiring: Arduino-compatible functions like `isAlpha()` and `toLowerCase()`. [#293](https://github.com/spark/core-firmware/pull/293)

### ENHANCEMENTS

 - Wire: added missing Slave mode using DMA/Interrupts and updated Master mode using DMA. New APIs `Wire.setSpeed()` and `Wire.strechClock()`. [#284](https://github.com/spark/core-firmware/issues/284)
 - Sleep: `Spark.sleep()` supports wakeup on pin change. [#265](https://github.com/spark/core-firmware/issues/265)

### BUGFIXES

 - RGB: calling `RGB.brightness()` doesn't change the LED brightness immediately [#261](https://github.com/spark/core-firmware/issues/261)
 - Wiring: `pinMode()` `INPUT` and `OUTPUT` constants had reversed values compared to Arduino. [#282](https://github.com/spark/core-firmware/issues/282)
 - Wiring: compiler error using `HEX` with `String`. [#210](https://github.com/spark/core-firmware/pull/210)
 - System Mode: MANUAL mode breaks OTA update [#294](https://github.com/spark/core-firmware/issues/294)

## pre v0.3.3 versions

See https://github.com/spark/core-firmware/releases

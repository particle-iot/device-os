
## v0.6.0-rc.1

### FEATURES

- [Photon] Wi-Fi Country Code [#942](https://github.com/spark/firmware/pull/942)

### ENHANCEMENTS

- Local build support for ARM GCC 5.3.1. [#963](https://github.com/spark/firmware/issues/963)
- Local build warns if crc32 is not present. [#941](https://github.com/spark/firmware/issues/941)

### BUGFIXES

- SoftAP mode persisting when setup complete if Wi-Fi was off. [#971](https://github.com/spark/firmware/issues/971) 
- Free memory allocated for previous system interrupt handler [#927](https://github.com/spark/firmware/issues/927)
- Fixes to I2C Slave mode implementation with clock stretching enabled [#931](https://github.com/spark/firmware/pull/931)

## v0.5.1

### BUGFIXES

- Fixes bit mask provided for PMIC::setChargeVoltage(4208) option. [#987](https://github.com/spark/firmware/pull/987)
- Revert EEPROM capacity to 2048 instead of 2047 Photon / 128 Core [#983](https://github.com/spark/firmware/pull/983)

## v0.5.0 (same as v0.5.0-rc.2)

## v0.5.0-rc.2

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

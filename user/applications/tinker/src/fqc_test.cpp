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
#include "spark_wiring.h"
#include "spark_wiring_ble.h"
#include "spark_wiring_logging.h"
#include "spark_wiring_json.h"

#include "fqc_test.h"
#include "burnin_test.h"

namespace particle {

using namespace spark;

// Helper functions
namespace {
	JSONValue getValue(const JSONValue& obj, const char* name) {
	    JSONObjectIterator it(obj);
	    while (it.next()) {
	        if (it.name() == name) {
	            return it.value();
	        }
	    }
	    return JSONValue();
	}

    JSONValue get(const JSONValue& obj, const char* name) {
        return getValue(obj, name);
    }

    bool has(const JSONValue& obj, const char* name) {
        return getValue(obj, name).isValid();
    }
}

FqcTest::FqcTest() :
        writer((char *)json_response_buffer, sizeof(json_response_buffer)),
        tcpClient(),
        inited_(true) {
	memset(json_response_buffer, 0x00, sizeof(json_response_buffer));
}

FqcTest::~FqcTest() {
}

FqcTest* FqcTest::instance() {
    static FqcTest handler;
    return &handler;
}

char * FqcTest::reply() {
	return writer.buffer();    	
}

size_t FqcTest::replySize() {
	return writer.dataSize();    	
}

void FqcTest::initWriter(){
	memset(json_response_buffer, 0x00, sizeof(json_response_buffer));
	writer = JSONBufferWriter((char *)json_response_buffer, sizeof(json_response_buffer));
}

bool FqcTest::process(JSONValue test){
	initWriter();

    if (has(test, "WRITE_SERIAL1")) {
		return writeSerial1(test);
    } else if (has(test, "USE_ANTENNA")) { 
        return useAntenna(test);
    } else if (has(test, "BLE_SCAN")) {
        return bleScan(test);
    } else if (has(test, "IO_TEST")) {
        return ioTest(test);
    } else if (has(test, "WIFI_NETCAT")) {
        return wifiNetcat(test);
    } else if (has(test, "WIFI_SCAN_NETWORKS")) { 
        return wifiScanNetworks(test);
    } else if (has(test, "BURNIN_TEST")) { 
        BurninTest::instance()->setup(true);
        return true;
    }

    return false;
}

bool FqcTest::passResponse(bool success) {
    writer.beginObject();
    writer.name("pass").value(success);
    writer.endObject();
    return true;
}

bool FqcTest::tcpErrorResponse(int tcpError) {
    writer.beginObject();
    writer.name("pass").value(false);
    writer.name("errorCode").value(tcpError);
    String errorMessage;
    if(tcpError == SYSTEM_ERROR_NETWORK){
        errorMessage = "Wifi not ready";
    }
    else if(tcpError == SYSTEM_ERROR_IO) { 
        errorMessage = "TCP connection failed";
    }
    else if(tcpError == SYSTEM_ERROR_TIMEOUT) {
        errorMessage = "TCP connection did not return any data";
    }
    else if(tcpError == SYSTEM_ERROR_BAD_DATA) { 
        errorMessage = "TCP Echo returned incorrect data";
    }
    else if(tcpError == SYSTEM_ERROR_INVALID_ARGUMENT) { 
        errorMessage = "Netcat command parameters malformed";
    }

    writer.name("message").value(errorMessage);
    writer.endObject();
    return true;
}

void FqcTest::parseIpAndPort(JSONValue parameters) {
    auto ipAddr = String(getValue(parameters, "ip").toString());
    auto port = getValue(parameters, "port").toInt();

    Log.info("IP is: %s, port is: %d", ipAddr.c_str(), port);

    uint8_t server[4] = {};
    char * ipOctet = strtok((char *)ipAddr.c_str(), ".");
    int i = 0;
    while(ipOctet){
        server[i++] = atoi(ipOctet);
        ipOctet = strtok(NULL, ".");
    }

    // Store FQC station port/ip
    this->tcpPort = port;
    memcpy(this->tcpServer, server, sizeof(server));
}

int FqcTest::sendTCPMessage(const char * tx_data, char * rx_data_buffer, int rx_data_buffer_length, int response_poll_ms) {
    int bytesRead = 0;

    if(!WiFi.ready()) {
        Log.error("WiFi not ready");
        return SYSTEM_ERROR_NETWORK;
    }
    
    if (!tcpClient.connected()) {
        if (tcpClient.connect(this->tcpServer, this->tcpPort)) {
            Log.info("TCP client connected to %s", tcpClient.remoteIP().toString().c_str());
        }
        else {
            Log.error("TCP client connection failed");
            return SYSTEM_ERROR_IO;
        }
    }

    if (tcpClient.connected()) {
        Log.info("Sending message: %s len %d", tx_data, strlen(tx_data));
        tcpClient.write((uint8_t *)tx_data, strlen(tx_data));

        // Poll for response
        int delay_period_ms = 500;
        for(int i = 0; i < (response_poll_ms / delay_period_ms) && !tcpClient.available(); i++){
            delay(delay_period_ms);
        }
        
        while(tcpClient.available() && (bytesRead < rx_data_buffer_length)) {
           rx_data_buffer[bytesRead++] = tcpClient.read();
        }

        if(bytesRead) {
            Log.trace("TCP Bytes Read: %d %s", bytesRead, (char *)rx_data_buffer);
            Log.dump(rx_data_buffer, bytesRead);
            Log.print("\r\n");
        }
        else {
            Log.error("No response from server");
            return SYSTEM_ERROR_TIMEOUT;
        }

        tcpClient.stop();
    }

    return bytesRead;
}

bool FqcTest::writeSerial1(JSONValue req) {
    Serial1.begin(115200);
    Serial1.println(get(req, "WRITE_SERIAL1").toString().data());
    return passResponse(true);
}

bool FqcTest::useAntenna(JSONValue req) {
    auto antenna = get(req, "USE_ANTENNA");
    bool success = false;

    if(antenna.isValid()){
        String antennaString(antenna.toString());
        Log.info("Got %s", antennaString.c_str());
        if(antennaString == "internal"){
            BLE.selectAntenna(BleAntennaType::INTERNAL);
            success = true;
        }
        else if(antennaString == "external"){
            BLE.selectAntenna(BleAntennaType::EXTERNAL);
            success = true;
        }
    }

    return passResponse(success);
}

bool FqcTest::bleScan(JSONValue req) {
    //{"BLE_SCAN":{"ip":"192.168.0.102","port":80}}
    auto parameters = get(req, "BLE_SCAN");
    parseIpAndPort(parameters);

    // Enable BLE, advertise with random UUID and name equal to unique characters of device ID
    BLE.on();
    const BleUuid serviceUuid("3270f6dd-f953-482c-ab5b-3be4d63a5ddc");
    String deviceId = System.deviceID();
    String deviceIdShort = deviceId.remove(0, deviceId.length() - 6);

    BleAdvertisingData data;
    data.appendServiceUUID(serviceUuid);
    data.appendLocalName(deviceIdShort.c_str());
    BLE.advertise(&data);

    char rxBuffer[32] = {};
    String scanMessage = "BLE_SCAN:" + deviceIdShort;
    int tcpResponse = sendTCPMessage(scanMessage.c_str(), rxBuffer, sizeof(rxBuffer), 30000);

    BLE.off();

    if(tcpResponse > 0) {
        return passResponse((String(rxBuffer) == "OK"));
    } else {
        return tcpErrorResponse(tcpResponse);
    }
}

// Skip 6 and 7 since they are SWC+SWD (PB3, PA27)
// Skip 8 and 9 since they are Uart TX+RX (PA7, PA8)
// The pin order denotes how they are jumpered together. IE pin A5 (PB4) and pin S6 (PB31) are jumpered together.
// The pin order largely reflects which pins are physically next to each other on the P2 jig and can be easily jumpered together. 
// Two sets of pins on the P2 jig are not adjacent and require a hookup wire:
// - S4 (PA0) + WKP/D10 (PA15)
// - A5 (PB4) + S6 (PB31)
#if PLATFORM_ID == PLATFORM_P2
static Vector<uint16_t> p2_gpio_test_pins = { A5, S6, WKP, S4, D0, D1, S0, S1, S2, A1, S3, D2, D4, D5, D3, A0, A2, S5 };
static Vector<uint16_t> photon2_gpio_test_pins = {A0, D10, A1, A2, A5, D5, S4, D4, S3, D3, SCK, D2, MOSI, SCL, MISO, SDA };
#else 
static Vector<uint16_t> p2_gpio_test_pins = { };
static Vector<uint16_t> photon2_gpio_test_pins = { };
#endif

static Vector<uint16_t> gpio_test_pins = {};

static bool assertAllPinsLow(uint16_t exceptPinA, uint16_t exceptPinB, uint16_t * errorPin) {
    for(auto pin : gpio_test_pins) {
        if(pin == exceptPinA || pin == exceptPinB){
            continue;
        }
        if(digitalRead(pin)){
            *errorPin = pin;
            return false;
        }
    }
    return true;
}

static void configureAllPinsInput(void) {
    for(auto pin : gpio_test_pins) {
        pinMode(pin, INPUT_PULLDOWN);
    }
}

const String pinNumberToPinName(uint16_t pinNumber) {
#if HAL_PLATFORM_RTL872X
    hal_pin_info_t pinInfo = hal_pin_map()[pinNumber];

    String portName = pinInfo.gpio_port == RTL_PORT_A ? "PA" : "PB";
    portName += pinInfo.gpio_pin;
    return portName;
#else
    return String(pinNumber);
#endif
}

static bool assertPinStates(uint16_t outputPin, uint16_t inputPin, bool expectedState, uint16_t * errorPin){
    bool inputPinCorrect = (digitalRead(inputPin) == expectedState);
    bool otherPinsLow = assertAllPinsLow(outputPin, inputPin, errorPin);

    Log.trace("Out:%u In:%u expectedState:%u actual state: %ld otherPinsLow:%u errorPin:%u", outputPin, inputPin, expectedState, digitalRead(inputPin), otherPinsLow, *errorPin);

    if(!inputPinCorrect){
        Log.warn("pin %u could not drive pin %u %s", outputPin, inputPin, expectedState ? "HIGH" : "LOW");
        *errorPin = outputPin;
    }
    else if(!otherPinsLow) {
        Log.warn("pin %u erroneously high. possible short", *errorPin);
    }
    else {
        return true;
    }
    return false;
}

bool FqcTest::ioTest(JSONValue req) {
    const String GPIOTestModes[] {
        "A_out_B_in_high",
        "A_out_B_in_low",
        "B_out_A_in_high",
        "B_out_A_in_low",
    };

    const uint16_t NO_GPIO_ERROR = 0xFFFF;
    String failedTest = "";

    uint16_t errorPin = NO_GPIO_ERROR;
    uint16_t pinA = 0;
    uint16_t pinB = 0;

    // Pick which set of pins to use based on hardware variant
    uint32_t model, variant, efuseReadAttempts = 0;
    int result = hal_get_device_hw_model(&model, &variant, nullptr);
    while (result != SYSTEM_ERROR_NONE && efuseReadAttempts < 5) {
        Log.warn("Failed to read logical efuse: %d attempt: %lu", result, efuseReadAttempts);
        result = hal_get_device_hw_model(&model, &variant, nullptr);
        efuseReadAttempts++;
    }

    if (result != SYSTEM_ERROR_NONE) {
        Log.error("Could not read logical efuse");
        writer.beginObject();
        writer.name("pass").value(false);
        writer.name("message").value("could not read logical efuse");
        writer.endObject();
        return true;
    } else {
        Log.info("Hardware Model: 0x%X Variant: %lu", (unsigned int)model, variant);    
    }

    #if PLATFORM_ID == PLATFORM_P2
    gpio_test_pins = variant == PLATFORM_P2_PHOTON_2 ? photon2_gpio_test_pins : p2_gpio_test_pins;
    #endif

    for(int i = 0; i < gpio_test_pins.size(); i+=2){
        pinA = gpio_test_pins[i];
        pinB = gpio_test_pins[i+1];

        configureAllPinsInput();
        pinMode(pinA, OUTPUT);

        digitalWrite(pinA, HIGH);
        if(!assertPinStates(pinA, pinB, true, &errorPin)){
            failedTest = GPIOTestModes[0];
            break;
        }

        digitalWrite(pinA, LOW);
        if(!assertPinStates(pinA, pinB, false, &errorPin)){
            failedTest = GPIOTestModes[1];
            break;
        }
        
        configureAllPinsInput();
        pinMode(pinB, OUTPUT);

        digitalWrite(pinB, HIGH);
        if(!assertPinStates(pinB, pinA, true, &errorPin)){
            failedTest = GPIOTestModes[2];
            break;
        }

        digitalWrite(pinB, LOW);
        if(!assertPinStates(pinB, pinA, false, &errorPin)){
            failedTest = GPIOTestModes[3];
            break;
        }
    }

    if(errorPin == NO_GPIO_ERROR){ 
        passResponse(true);
    }
    else {
        writer.beginObject();
        writer.name("pass").value(false);
        writer.name("pinA").value(pinNumberToPinName(pinA));
        writer.name("pinB").value(pinNumberToPinName(pinB));
        writer.name("errorPin").value(pinNumberToPinName(errorPin));
        writer.name("message").value(failedTest);
        writer.endObject();
    }

    return true;
}

bool FqcTest::wifiNetcat(JSONValue req) {
    //Log.info("WIFI_NETCAT handler");
    // {
    //   "WIFI_NETCAT": {
    //     "ip": "192.168.0.102",
    //     "port": 80,
    //     "message": "hello",
    //     "expected_response": "HELLO"
    //   }
    // }
    auto parameters = get(req, "WIFI_NETCAT");
    int errorCode = SYSTEM_ERROR_NONE;
    bool success = false;

    if(parameters.isValid()) {
        parseIpAndPort(parameters);

        auto message = getValue(parameters, "message").toString().data();
        auto expectedResponse = getValue(parameters, "expected_response").toString();

        Log.info("message is: \"%s\" expected response: \"%s\"", message, expectedResponse.data());

        uint8_t rxBuffer[256] = {};
        int bytesSent = sendTCPMessage(message, (char *)rxBuffer, sizeof(rxBuffer));
        if(bytesSent > 0) {
            if(String((char *)rxBuffer, bytesSent) == expectedResponse){
                success = true;
            } else {
                Log.error("TCP server did not return expected response");
                errorCode = SYSTEM_ERROR_BAD_DATA;
            }
        }
        else {
            // 'bytesSent' in this case is an error code from sendTCPMessage
            errorCode = bytesSent;
        }
    }
    else {
        Log.error("WIFI_NETCAT parameters malformed");
        errorCode = SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if(errorCode < 0) {
        tcpErrorResponse(errorCode);
    }
    else {
        passResponse(success);
    }
    return true;
}

static void wifi_scan_callback(WiFiAccessPoint* wap, void* cookie) {
    WiFiAccessPoint& ap = *wap;
    auto networks = (Vector<WiFiAccessPoint>*)cookie;
    networks->append(ap);
}

bool FqcTest::wifiScanNetworks(JSONValue req) {
    // Scan for all APs
    Vector<WiFiAccessPoint> networks;

    WiFi.on();

    int result_count = WiFi.scan(wifi_scan_callback, &networks);
    if (result_count > 0) {
        Log.info("Found %d networks total", result_count);
    }
    else {
        Log.warn("Wifi scan failed: %s", get_system_error_message(result_count));
    }

    // Serialize to JSON and return this list over USB, like the regular WIFI scan does
    writer.beginObject();
    writer.name("pass").value(true);
    writer.name("networks").beginArray();
    for(WiFiAccessPoint& network: networks){
        writer.beginObject();
        writer.name("ssid").value(String(network.ssid, network.ssidLength));

        char bssidStr[32] = {};
        sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", network.bssid[0], network.bssid[1], network.bssid[2], network.bssid[3], network.bssid[4], network.bssid[5]);
        writer.name("bssid").value(String(bssidStr));

        writer.name("security").value((int)network.security);
        writer.name("channel").value(network.channel);
        writer.name("rssi").value(network.rssi);
        writer.endObject();
    }
    writer.endArray();
    writer.endObject();
    return true;
}

}
#endif // HAL_PLATFORM_RTL872X


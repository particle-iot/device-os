#include "request_handler.h"

#include "spark_wiring.h"
#include "spark_wiring_ble.h"
#include "spark_wiring_logging.h"
#include "spark_wiring_system.h"
#include "spark_wiring_json.h"
#include "spark_wiring_wifi.h"

#include "delay_hal.h"
#include "check.h"

namespace particle {

namespace {

using namespace spark; // For JSON classes

JSONValue getValue(const JSONValue& obj, const char* name) {
    JSONObjectIterator it(obj);
    while (it.next()) {
        if (it.name() == name) {
            return it.value();
        }
    }
    return JSONValue();
}

} // namespace

// Wrapper for a control request handle
class RequestHandler::Request {
public:
    Request() :
            req_(nullptr) {
    }

    ~Request() {
        destroy();
    }

    int init(ctrl_request* req) {
        if (req->request_size > 0) {
            // Parse request
            auto d = JSONValue::parse(req->request_data, req->request_size);
            CHECK_TRUE(d.isObject(), SYSTEM_ERROR_BAD_DATA);
            data_ = std::move(d);
        }
        req_ = req;
        return 0;
    }

    void destroy() {
        data_ = JSONValue();
        if (req_) {
            // Having a pending request at this point is an internal error
            system_ctrl_set_result(req_, SYSTEM_ERROR_INTERNAL, nullptr, nullptr, nullptr);
            req_ = nullptr;
        }
    }

    template<typename EncodeFn>
    int reply(EncodeFn fn) {
        CHECK_TRUE(req_, SYSTEM_ERROR_INVALID_STATE);
        // Calculate the size of the reply data
        JSONBufferWriter writer(nullptr, 0);
        fn(writer);
        const size_t size = writer.dataSize();
        CHECK_TRUE(size > 0, SYSTEM_ERROR_INTERNAL);
        CHECK(system_ctrl_alloc_reply_data(req_, size, nullptr));
        // Serialize the reply
        writer = JSONBufferWriter(req_->reply_data, req_->reply_size);
        fn(writer);
        CHECK_TRUE(writer.dataSize() == size, SYSTEM_ERROR_INTERNAL);
        return 0;
    }

    void done(int result, ctrl_completion_handler_fn fn = nullptr, void* data = nullptr) {
        if (req_) {
            system_ctrl_set_result(req_, result, fn, data, nullptr);
            req_ = nullptr;
            destroy();
        }
    }

    JSONValue get(const char* name) const {
        return getValue(data_, name);
    }

    bool has(const char* name) const {
        return getValue(data_, name).isValid();
    }

    const JSONValue& data() const {
        return data_;
    }

    bool isEmpty() const {
        return !data_.isValid();
    }

private:
    JSONValue data_;
    ctrl_request* req_;
};

RequestHandler::RequestHandler() :
        inited_(false),
        tcpClient() {
}

RequestHandler::~RequestHandler() {
    destroy();
}

int RequestHandler::init() {
    inited_ = true;
    return 0;
}

void RequestHandler::destroy() {
    inited_ = false;
}

void RequestHandler::process(ctrl_request* ctrlReq) {
    Log.info("USB req size %d type %d data size: %d ", ctrlReq->size, ctrlReq->type, ctrlReq->request_size);
    Log.info("%s", String(ctrlReq->request_data, ctrlReq->request_size).c_str());

    const int r = request(ctrlReq);
    if (r < 0) {
        system_ctrl_set_result(ctrlReq, r, nullptr, nullptr, nullptr);
    }
}

int RequestHandler::request(ctrl_request* ctrlReq) {
    //CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    Request req;
    CHECK(req.init(ctrlReq));
    const int r = CHECK(request(&req));
    req.done(r);
    return 0;
}

int RequestHandler::request(Request* req) {
    if (req->has("WRITE_SERIAL1")) {
    	return writeSerial1(req);
    } else if (req->has("USE_ANTENNA")) { 
        return useAntenna(req);
    } else if (req->has("BLE_SCAN")) {
        return bleScan(req);
    } else if (req->has("IO_TEST")) {
        return ioTest(req);
    } else if (req->has("WIFI_NETCAT")) {
        return wifiNetcat(req);
    } else if (req->has("WIFI_SCAN_NETWORKS")) { 
        return wifiScanNetworks(req);
    } else if (req->isEmpty()) { // Ping request
        return SYSTEM_ERROR_NONE;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }    
}

const auto RequestHandler::passResponse(bool success) { 
	return [success](JSONWriter& w){
	    w.beginObject();
	    w.name("pass").value(success);
	    w.endObject();
	};
}

const auto RequestHandler::tcpErrorResponse(int tcpError) {
    return [tcpError](JSONWriter& w){
        w.beginObject();
        w.name("pass").value(false);
        w.name("errorCode").value(tcpError);
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
        w.name("message").value(errorMessage);
        w.endObject();
    };
}

void RequestHandler::parseIpAndPort(JSONValue parameters) {
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

int RequestHandler::sendTCPMessage(const char * tx_data, char * rx_data_buffer, int rx_data_buffer_length, int response_poll_ms) {
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

int RequestHandler::useAntenna(Request* req) {
    auto antenna = req->get("USE_ANTENNA");
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

    req->reply(passResponse(success));
    return 0;
}

int RequestHandler::writeSerial1(Request* req) {
    Serial1.begin(115200);
    Serial1.println(req->get("WRITE_SERIAL1").toString().data());
    req->reply(passResponse(true));
    return 0;
}

int RequestHandler::bleScan(Request* req) {
    //{"BLE_SCAN":{"ip":"192.168.0.102","port":80}}
    auto parameters = req->get("BLE_SCAN");
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
        req->reply(passResponse((String(rxBuffer) == "OK")));
    } else {
        req->reply(tcpErrorResponse(tcpResponse));
    }

    return 0;
}

// Skip 6 and 7 since they are SWC+SWD (PB3, PA27)
// Skip 8 and 9 since they are Uart TX+RX (PA7, PA8)
// The pin order denotes how they are jumpered together. IE pin 14 (PB4) and pin 21 (PB31) are jumpered together.
// The pin order largely reflects which pins are physically next to each other on the P2 jig and can be easily jumpered together. 
// Two sets of pins are not adjacent and require a hookup wire:
// - Pin19 (PA0) + Pin10 (PA15 ie WKP/D10)
// - Pin14 (PB4) + Pin21 (PB31)
static const Vector<uint16_t> p2_digital_pins = {14, 21, 10, 19, 0, 1, 15, 16, 17, 12, 18, 2, 4, 5, 3, 11, 13, 20 };

bool assertAllPinsLow(uint16_t exceptPinA, uint16_t exceptPinB, uint16_t * errorPin) {
    for(auto pin : p2_digital_pins) {
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

void configureAllPinsInput(void) {
    for(auto pin : p2_digital_pins) {
        pinMode(pin, INPUT_PULLDOWN);
    }
}

bool assertPinStates(uint16_t outputPin, uint16_t inputPin, bool expectedState, uint16_t * errorPin){
    bool inputPinCorrect = (digitalRead(inputPin) == expectedState);
    bool otherPinsLow = assertAllPinsLow(outputPin, inputPin, errorPin);

    Log.info("Out:%u In:%u expectedState:%u actual state: %ld otherPinsLow:%u errorPin:%u", outputPin, inputPin, expectedState, digitalRead(inputPin), otherPinsLow, *errorPin);

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

int RequestHandler::ioTest(Request* req) {
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

    for(int i = 0; i < p2_digital_pins.size(); i+=2){
        pinA = p2_digital_pins[i];
        pinB = p2_digital_pins[i+1];

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
        req->reply(passResponse(true));
    }
    else {
        const auto gpioErrorResponse = [pinA, pinB, errorPin, failedTest](JSONWriter& w){
            w.beginObject();
            w.name("pass").value(false);
            w.name("pinA").value(pinA);
            w.name("pinB").value(pinB);
            w.name("errorPin").value(errorPin);
            w.name("message").value(failedTest);
            w.endObject();
        };
        req->reply(gpioErrorResponse);
    }

    return 0;
}

int RequestHandler::wifiNetcat(Request* req) {
    //Log.info("WIFI_NETCAT handler");
    // {
    //   "WIFI_NETCAT": {
    //     "ip": "192.168.0.102",
    //     "port": 80,
    //     "message": "hello",
    //     "expected_response": "HELLO"
    //   }
    // }
    auto parameters = req->get("WIFI_NETCAT");
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
        req->reply(tcpErrorResponse(errorCode));
    }
    else {
        req->reply(passResponse(success));    
    }
    return 0;
}

const auto RequestHandler::wifiScanNetworksResponse(Vector<WiFiAccessPoint> * networks) { 
    return [networks](JSONWriter& w){
        w.beginObject();
        w.name("pass").value(true);
        w.name("networks").beginArray();
        for(WiFiAccessPoint& network: *networks){
            w.beginObject();
            w.name("ssid").value(String(network.ssid, network.ssidLength));

            char bssidStr[32] = {};
            sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", network.bssid[0], network.bssid[1], network.bssid[2], network.bssid[3], network.bssid[4], network.bssid[5]);
            w.name("bssid").value(String(bssidStr));

            w.name("security").value((int)network.security);
            w.name("channel").value(network.channel);
            w.name("rssi").value(network.rssi);
            w.endObject();
        }
        w.endArray();
        w.endObject();
    };
}

void wifi_scan_callback(WiFiAccessPoint* wap, void* cookie) {
    WiFiAccessPoint& ap = *wap;
    auto networks = (Vector<WiFiAccessPoint>*)cookie;
    networks->append(ap);
}

int RequestHandler::wifiScanNetworks(Request* req) {
    // Scan for all APs
    Vector<WiFiAccessPoint> networks;

    int result_count = WiFi.scan(wifi_scan_callback, &networks);
    Log.info("Found %d networks total", result_count);

    // Serialize to JSON and return this list over USB, like the regular WIFI scan does
    req->reply(wifiScanNetworksResponse(&networks));
    return 0;
}


RequestHandler* RequestHandler::instance() {
    static RequestHandler handler;
    return &handler;
}

} // namespace particle


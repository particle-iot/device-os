#include "application.h"
#include "Particle.h"
#include "check.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "hex_to_bytes.h"

#include "FactoryTester.h"
#include "TesterCommandTypes.h"

#if PLATFORM_ID == PLATFORM_P2 
    extern "C" {
    #define BOOLEAN AMBD_SDK_BOOLEAN
    #include "rtl8721d.h"
    #undef BOOLEAN
    }
#else
    #define BIT(x)                  (1 << (x))
#endif

#define MFG_TEST_APP_VERSION "1.0.0"

//********************************* !IMPORTANT! *********************************
// By default, we do not write data to logical/physical efuse (ie OTP memory).
// If we are asked to BURN_DATA, the data will only be written to RAM buffers that mock the efuse implementation.
// The mfg-cli must explicitly configure the firmware to write to efuse by sending the SET_PERMANENT_BURN message
static bool PERMANENT_BURN = false;
//********************************* !IMPORTANT! *********************************

// eFuse data lengths
#define SECURE_BOOT_KEY_LENGTH 32
#define FLASH_ENCRYPTION_KEY_LENGTH 16
#define USER_MTP_DATA_LENGTH 32

// MTP data lengths
#define MOBILE_SECRET_LENGTH        15
#define SERIAL_NUMBER_LENGTH        9  // Bottom 6 characters are WIFI MAC, but not stored in MTP to save space
#define HARDWARE_VERSION_LENGTH     4
#define HARDWARE_MODEL_LENGTH       4

// Logical eFuse addresses
#define EFUSE_SYSTEM_CONFIG 0x0E
#define EFUSE_WIFI_MAC      0x11A
#define EFUSE_USER_MTP      0x160  // TODO: Figure out if how many writes we have

#define EFUSE_WIFI_MAC_LENGTH 6

#define EFUSE_MOBILE_SECRET_OFFSET 0
#define EFUSE_SERIAL_NUMBER_OFFSET (EFUSE_MOBILE_SECRET_OFFSET + MOBILE_SECRET_LENGTH)
#define EFUSE_HARDWARE_VERSION_OFFSET (EFUSE_SERIAL_NUMBER_OFFSET + SERIAL_NUMBER_LENGTH)
#define EFUSE_HARDWARE_MODEL_OFFSET (EFUSE_HARDWARE_VERSION_OFFSET + HARDWARE_VERSION_LENGTH)

// Physical eFuse addresses
#define EFUSE_FLASH_ENCRYPTION_KEY          0x190
#define EFUSE_SECURE_BOOT_KEY               0x1A0
#define EFUSE_PROTECTION_CONFIGURATION_LOW  0x1C0
#define EFUSE_PROTECTION_CONFIGURATION_HIGH 0x1C1

#define EFUSE_FLASH_ENCRYPTION_KEY_LOCK_BITS (~(BIT(5)|BIT(2))) // Clear Bit 2,5 = Read, Write Forbidden
#define EFUSE_SECURE_BOOT_KEY_LOCK_BITS (~(BIT(6))) // Clear Bit 6 = Write Forbidden

#define EFUSE_SUCCESS 1
#define EFUSE_FAILURE 0

// Logical efuse buffer used for BOTH mocking and real efuse read operations
#define LOGICAL_EFUSE_SIZE 1024
static uint8_t logicalEfuseBuffer[LOGICAL_EFUSE_SIZE];

// Physical buffer used ONLY for mocking physical efuse read/writes
#define PHYSICAL_EFUSE_SIZE 512
static uint8_t physicalEfuseBuffer[PHYSICAL_EFUSE_SIZE];

static uint8_t flashEncryptionKey[FLASH_ENCRYPTION_KEY_LENGTH];
static uint8_t flashEncryptionLockBits[1];
static uint8_t secureBootKey[SECURE_BOOT_KEY_LENGTH];
static uint8_t secureBootLockBits[1];
static uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH];
static uint8_t mobileSecret[MOBILE_SECRET_LENGTH];
static uint8_t serialNumber[SERIAL_NUMBER_LENGTH];
static uint8_t hardwareVersion[HARDWARE_VERSION_LENGTH];
static uint8_t hardwareModel[HARDWARE_MODEL_LENGTH];

static const String JSON_KEYS[MFG_TEST_END] = {
    "flash_encryption_key",
    "secure_boot_key",
    "wifi_mac",
    "mobile_secret",
    "serial_number",
    "hw_version",
    "hw_model"
};

struct EfuseData {
    const MfgTestKeyType json_key;
    bool isValid;
    const bool isPhysicaleFuse;
    uint8_t * data;
    const uint32_t length;
    const uint32_t address;
};

static EfuseData efuseFields[EFUSE_DATA_MAX] = {
    {MFG_TEST_FLASH_ENCRYPTION_KEY, false, true,  flashEncryptionKey,      FLASH_ENCRYPTION_KEY_LENGTH, EFUSE_FLASH_ENCRYPTION_KEY},
    {MFG_TEST_FLASH_ENCRYPTION_KEY, false, true,  flashEncryptionLockBits, 1,                           EFUSE_PROTECTION_CONFIGURATION_LOW},
    {MFG_TEST_SECURE_BOOT_KEY,      false, true,  secureBootKey,           SECURE_BOOT_KEY_LENGTH,      EFUSE_SECURE_BOOT_KEY},
    {MFG_TEST_SECURE_BOOT_KEY,      false, true,  secureBootLockBits,      1,                           EFUSE_PROTECTION_CONFIGURATION_LOW},
    {MFG_TEST_WIFI_MAC,             false, false, wifiMAC,                 EFUSE_WIFI_MAC_LENGTH,       EFUSE_WIFI_MAC},
    {MFG_TEST_MOBILE_SECRET,        false, false, mobileSecret,            MOBILE_SECRET_LENGTH,        EFUSE_USER_MTP + EFUSE_MOBILE_SECRET_OFFSET},
    {MFG_TEST_SERIAL_NUMBER,        false, false, serialNumber,            SERIAL_NUMBER_LENGTH,        EFUSE_USER_MTP + EFUSE_SERIAL_NUMBER_OFFSET},
    {MFG_TEST_HW_VERSION,           false, false, hardwareVersion,         HARDWARE_VERSION_LENGTH,     EFUSE_USER_MTP + EFUSE_HARDWARE_VERSION_OFFSET},
    {MFG_TEST_HW_MODEL,             false, false, hardwareModel,           HARDWARE_MODEL_LENGTH,       EFUSE_USER_MTP + EFUSE_HARDWARE_MODEL_OFFSET}
};

// Wrapper for a control request handle
class Request {
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
            String printResponse = String(req_->reply_data, req_->reply_size);
            Log.info("json resp size: %d string: %s", req_->reply_size, printResponse.c_str());
            system_ctrl_set_result(req_, result, fn, data, nullptr);
            req_ = nullptr;
            destroy();
        }
    }

    const JSONValue& data() const {
        return data_;
    }

private:
    JSONValue data_;
    ctrl_request* req_;
};


// Input strings
const String FactoryTester::IS_READY = "IS_READY";
const String FactoryTester::SET_PERMANENT_BURN = "SET_PERMANENT_BURN";
const String FactoryTester::SET_DATA = "SET_DATA";
const String FactoryTester::BURN_DATA = "BURN_DATA";
const String FactoryTester::VALIDATE_BURNED_DATA = "VALIDATE_BURNED_DATA";
const String FactoryTester::GET_DEVICE_ID = "GET_DEVICE_ID";

// Output strings
const String FactoryTester::PASS = "pass";
const String FactoryTester::ERRORS = "errors";

const String FactoryTester::EFUSE_READ_FAILURE = "Failed to read burned data";
const String FactoryTester::EFUSE_WRITE_FAILURE = "Failed to burn data";
const String FactoryTester::DATA_DOES_NOT_MATCH = "Burned data does not match";
const String FactoryTester::NO_VALID_DATA_BUFFERED = "No valid data buffered";
const String FactoryTester::RSIP_ENABLE_FAILURE = "Failed to enable RSIP";
const String FactoryTester::ALREADY_PROVISIONED = "Already provisioned with same data";

const String FactoryTester::FIELD  = "field";
const String FactoryTester::MESSAGE  = "message";
const String FactoryTester::CODE = "code";
const String FactoryTester::VERSION = "version";


void FactoryTester::setup() {   
    memset(logicalEfuseBuffer, 0xFF, sizeof(logicalEfuseBuffer));
    memset(physicalEfuseBuffer, 0xFF, sizeof(physicalEfuseBuffer));
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        memset(efuseFields[i].data, 0x00, efuseFields[i].length);
    }
    Log.info("Provisioning Firmware Ready!");
}

int FactoryTester::processUSBRequest(ctrl_request* request) {
    memset(usb_buffer, 0x00, sizeof(usb_buffer));
    memcpy(usb_buffer, request->request_data, request->request_size);

    Log.info("USB req size %d type %d data size: %d ", request->size, request->type, request->request_size);
    Log.trace("USB data: %s", usb_buffer);

    Vector<int> resultCodes(MFG_TEST_END);
    Vector<String> resultStrings(MFG_TEST_END);
    this->command_response[0] = 0;

    Request response;
    CHECK(response.init(request));

    String command;
    bool allPassed = false;
    bool isSetData = false;
    bool isValidateBurnedData = false;
    bool isSetDataAndProvisioned = false;
    bool isGetDeviceId = false;

    // Pick apart the JSON command
    JSONObjectIterator it(response.data());
    while (it.next()) {
        command = (String)it.name();
        JSONValue firstData(it.value());

        Log.info("Command %s", command.c_str());

        isSetData = (command == SET_DATA);
        isValidateBurnedData = (command == VALIDATE_BURNED_DATA);
        isSetDataAndProvisioned = (isSetData & isProvisioned());

        if(isSetData || isValidateBurnedData) {
            allPassed = true;
            // Invalidate all currently buffered data
            for(int i = 0; i < EFUSE_DATA_MAX; i++){
                efuseFields[i].isValid = false;
                memset(efuseFields[i].data, 0x00, efuseFields[i].length);
            }

            // Process keys in main object
            JSONObjectIterator innerIterator(firstData);
            while (innerIterator.next()) {
                String innerKey(innerIterator.name().data());
                String innerData(innerIterator.value().toString());

                Log.trace("key: %s value: %s", innerKey.c_str(), innerData.c_str());

                // Parse data from each key
                for (int i = 0; i < MFG_TEST_END; i++) {
                    if(innerKey == JSON_KEYS[i]) {
                        int set_data_result = setData((MfgTestKeyType)i, innerData.c_str());
                        resultCodes[i] = set_data_result;
                        resultStrings[i] = this->command_response[0] != 0 ? this->command_response : "";

                        allPassed &= (set_data_result == 0);
                    } 
                }
            }
        }
        else if(command == IS_READY){
            allPassed = true;
        }
        else if(command == BURN_DATA){
            allPassed = burnData(resultCodes, resultStrings);
        }
        else if(command == GET_DEVICE_ID){
            isGetDeviceId = true;
            allPassed = getDeviceId();
        }
        else if(command == SET_PERMANENT_BURN){
            allPassed = true;
            PERMANENT_BURN = true;
            Log.warn("*** ENABLING PERMANENT BURN ***");
        }
        
        if(allPassed && (isValidateBurnedData || isSetDataAndProvisioned)) {
            // Compare the burned data against the data we just buffered to RAM
            allPassed = validateData(resultCodes, resultStrings);
        }
    }

    Log.trace("Command allPassed: %d", allPassed);
    if(!allPassed) {
        for (int i = 0; i < resultCodes.size(); i++){
            if (resultCodes[i] != 0xFF) {
                Log.trace("resultCodes: %d %d", i, resultCodes[i]);
            }
            if (resultStrings[i] != ""){
                Log.trace("resultStrings: %d %s", i, resultStrings[i].c_str());
            }
        }    
    }

    // Response for IS_READY, SET_PERMANENT_BURN, SET_DATA, BURN_DATA, VALIDATE_BURNED_DATA
    auto commandResponse = [allPassed, resultCodes, resultStrings, isSetDataAndProvisioned, command](JSONWriter& w) {
        w.beginObject();
        w.name(PASS).value(allPassed);

        if(!allPassed){
            w.name(ERRORS).beginArray();
            for(int i = 0; i < resultCodes.size(); i++){
                if(resultCodes[i] < 0){
                    int errorCode = resultCodes[i];
                    const char * errorMessage = (resultStrings[i] != "" ? resultStrings[i].c_str() : get_system_error_message(errorCode));

                    w.beginObject();
                    w.name(FIELD).value(JSON_KEYS[i]);
                    w.name(MESSAGE).value(errorMessage);
                    w.name(CODE).value(errorCode);
                    w.endObject();
                }
            }
            w.endArray();
        }
        else if(isSetDataAndProvisioned) {
            w.name(MESSAGE).value(ALREADY_PROVISIONED);
        }
        else if(command == IS_READY) {
            w.name(VERSION).value(MFG_TEST_APP_VERSION);
        }
        w.endObject();
    };

    // Separate response for GET_DEVICE_ID
    char * device_id = this->command_response;
    auto getDeviceIdResponse = [device_id](JSONWriter& w){
        w.beginObject();
        w.name(PASS).value(true);
        w.name("device_id").value(device_id);
        w.endObject();
    };

    // Send response.
    int r;
    if(isGetDeviceId) {
        r = CHECK(response.reply(getDeviceIdResponse));
    }
    else {
        r = CHECK(response.reply(commandResponse));
    }

    response.done(r);
    return r;
}

// Validate and buffer provisioning data RAM
int FactoryTester::setData(MfgTestKeyType command, const char * commandData) {
    int result = -1;

    switch (command) {
        case MFG_TEST_FLASH_ENCRYPTION_KEY:
            result = this->setFlashEncryptionKey(commandData);
            break;   
        case MFG_TEST_SECURE_BOOT_KEY:
            result = this->setSecureBootKey(commandData);
            break;   
        case MFG_TEST_SERIAL_NUMBER:
            result = this->setSerialNumber(commandData);
            break;   
        case MFG_TEST_MOBILE_SECRET:
            result = this->setMobileSecret(commandData);
            break;  
        case MFG_TEST_HW_VERSION:
            result = this->setHardwareVersion(commandData);
            break;  
        case MFG_TEST_HW_MODEL:
            result = this->setHardwareModel(commandData);
            break;
        case MFG_TEST_WIFI_MAC:
            result = this->setWifiMac(commandData);
            break;
        default:
        case MFG_TEST_END:
            break;
    }

    return result;
}

int FactoryTester::writeEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address) {    
    uint8_t readBackBuffer[32] = {};
    uint32_t eFuseWrite = EFUSE_SUCCESS;
    uint32_t i = 0;

    Log.trace("Physical: %d eFuse write %lu bytes @ 0x%X", physical, length, (unsigned int)address);
    Log.dump(data, length);
    Log.print("\r\n");

    if (physical) {
        for (i = 0; (i < length) && (eFuseWrite == EFUSE_SUCCESS); i++) {
            if(PERMANENT_BURN) {
                eFuseWrite = EFUSE_PMAP_WRITE8(0, address + i, data[i], L25EOUTVOLTAGE);    
            }
            else {
                physicalEfuseBuffer[address + i] &= data[i];    
            }
        }
    }
    else {
        if(PERMANENT_BURN) {
            eFuseWrite = EFUSE_LMAP_WRITE(address, length, data);
        }
        else {
            memcpy(logicalEfuseBuffer + address, data, length);
        }
    }

    if(eFuseWrite == EFUSE_SUCCESS){
        if(readEfuse(physical, readBackBuffer, length, address) == 0){
            Log.trace("eFuse readback after write");
            Log.dump(readBackBuffer, length);
            Log.print("\r\n");
        }
        else {
            Log.warn("Failed to readback efuse after write. type %d @ 0x%lX", physical, address);
        }  
    }
    else {
        Log.warn("Failed to write efuse. type %d @ 0x%lX", physical, address);
    }

    return (eFuseWrite == EFUSE_SUCCESS) ? 0 : SYSTEM_ERROR_NOT_ALLOWED;
}

int FactoryTester::readEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address) {    
    uint32_t eFuseRead = EFUSE_SUCCESS;
    uint32_t i = 0;

    if (physical) {
        for (i = 0; (i < length) && (eFuseRead == EFUSE_SUCCESS); i++) {
            if(PERMANENT_BURN) {
                eFuseRead = EFUSE_PMAP_READ8(0, address + i, &data[i], L25EOUTVOLTAGE);
            }
            else {
                data[i] = physicalEfuseBuffer[address + i];
            }
        }
    }
    else {
        if(PERMANENT_BURN) {
            eFuseRead = EFUSE_LMAP_READ(logicalEfuseBuffer);
            if(eFuseRead == EFUSE_SUCCESS) {
                memcpy(data, logicalEfuseBuffer + address, length);
            }
        }
        else {
            memcpy(data, logicalEfuseBuffer + address, length);    
        }
    }

    if(eFuseRead == EFUSE_FAILURE) {
        Log.warn("Efuse read failure. type %d from address 0x%lX", physical, address);
    }

    return (eFuseRead == EFUSE_SUCCESS) ? 0 : SYSTEM_ERROR_NOT_ALLOWED;
}

int FactoryTester::validateCommandData(const char * data, uint8_t * output_bytes, int output_bytes_length) {
    int len = strlen(data);

    // Validate length of data is as expected
    int bytesLength = len / 2;
    if(bytesLength > output_bytes_length) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    else if (bytesLength < output_bytes_length) {
        return SYSTEM_ERROR_NOT_ENOUGH_DATA;
    }

    // Ensure we only have hex characters in the string data.
    for (uint8_t i = 0; i < len; i++) {
        if (!isxdigit(data[i])) {
            Log.trace("Validate Hex Data FAILED: invalid char value 0x%02X at index %d", data[i], i);
            return SYSTEM_ERROR_BAD_DATA;        
        }
    }
    
    // Convert Hex string to binary data.
    if (hexToBytes(data, (char *)output_bytes, output_bytes_length) != (unsigned)output_bytes_length) {
        Log.trace("Validate Hex Data FAILED: hexToBytes failed");
        return SYSTEM_ERROR_BAD_DATA;        
    }

    return 0;
}

int FactoryTester::validateCommandString(const char * data, int expectedLength) {
    int length = strlen(data);

    // Validate length
    if (length > expectedLength){
        return SYSTEM_ERROR_TOO_LARGE;
    }
    else if(length < expectedLength) {
        return SYSTEM_ERROR_NOT_ENOUGH_DATA;
    }

    // Validate string contents
    for (int i = 0; i < length; i++){
        if (!isalnum(data[i])) {
            return SYSTEM_ERROR_BAD_DATA;
        }
    }

    return 0;
}

// Return true if any data is written to efuse
bool FactoryTester::isProvisioned() {
    uint8_t burnedEfuseDataBuffer[32];
    int efuseReadResult;

    for(unsigned i = 0; i < EFUSE_DATA_MAX; i++){
        EfuseData data = efuseFields[i];

        if(i == EFUSE_DATA_WIFI_MAC){
            // Devices likely have MAC already programmed for functional testing.
            // We will overwrite this MAC with our own, so its presence is not an indicator of a Particle provisioned P2
            continue;
        }

        efuseReadResult = readEfuse(data.isPhysicaleFuse, burnedEfuseDataBuffer, data.length, data.address);
        if(efuseReadResult != 0){
            // If any efuse data fails to read, it must be due to lock bits, and therefore is provisioned
            return true;
        }

        for(unsigned i = 0; i < data.length; i++) {
            if(burnedEfuseDataBuffer[i] != 0xFF) {
                return true;
            }
        }
    }

    return false;
}

// This is the critical step where all the data buffered in RAM is burned to the appropriate physical and logical efuse locations
// The general flow is
// 1) Validate that ALL expected efuse data is buffered and validated, return immediately if this is not true
// 2) Iterate over the buffered efuse data fields. Some of the fields need to be handled in special ways
//    -Flash Encryption Key: The endianess of this key needs to be swapped before writing
//    -Logical efuse data (EXCEPT WIFI MAC): In order to minimize writes to the User MTP section of logical efuse,
//     all of the data is concatenated together and written to logical efuse in a single write operation
bool FactoryTester::burnData(Vector<int> &resultCodes, Vector<String> &resultStrings) {
    bool allBufferedDataValid = true;
    int efuseWriteResult;
    uint8_t userMTPBuffer[USER_MTP_DATA_LENGTH] = {};

    // Check that all buffered data is valid, if not return
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        EfuseData data = efuseFields[i];
        if(!data.isValid) {
            resultCodes[data.json_key] = -1;
            resultStrings[data.json_key] = NO_VALID_DATA_BUFFERED;
            allBufferedDataValid = false;
        }
    }

    if(!allBufferedDataValid){
        return false;
    }

    // Write all buffered data to appropriate efuse locations (physical and logical)
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        EfuseData data = efuseFields[i];

        if(data.isPhysicaleFuse || (data.json_key == MFG_TEST_WIFI_MAC)){

            // The flash encryption key byte order should be reversed when written to efuse
            if(i == EFUSE_DATA_FLASH_ENCRYPTION_KEY){
                uint8_t keyReversed[FLASH_ENCRYPTION_KEY_LENGTH] = {};
                for(int j = 0; j < FLASH_ENCRYPTION_KEY_LENGTH; j++){
                    keyReversed[FLASH_ENCRYPTION_KEY_LENGTH - j - 1] = data.data[j];
                }
                memcpy(data.data, keyReversed, FLASH_ENCRYPTION_KEY_LENGTH);
            }

            efuseWriteResult = writeEfuse(data.isPhysicaleFuse, data.data, data.length, data.address);
            if(efuseWriteResult != 0){
                resultCodes[data.json_key] = efuseWriteResult;
                resultStrings[data.json_key] = EFUSE_WRITE_FAILURE;
                return false;
            }
        }
        // Else buffer logical efuse data
        else if(!data.isPhysicaleFuse) {
            memcpy(userMTPBuffer + (data.address - EFUSE_USER_MTP), data.data, data.length);
        }
    }

    // Combine user MTP data fields to be efficient with logical efuse MTP write operations
    // User serial number as standin for whole efuse failure
    efuseWriteResult = writeEfuse(false, userMTPBuffer, USER_MTP_DATA_LENGTH, EFUSE_USER_MTP);
    if(efuseWriteResult != 0){
        resultCodes[MFG_TEST_SERIAL_NUMBER] = efuseWriteResult;
        resultStrings[MFG_TEST_SERIAL_NUMBER] = EFUSE_WRITE_FAILURE;
        return false;
    }

    // Enable RSIP as last step if everything else succeeded
    efuseWriteResult = setFlashEncryption();
    if(efuseWriteResult != 0) {
        resultCodes[MFG_TEST_FLASH_ENCRYPTION_KEY] = -1;
        resultStrings[MFG_TEST_FLASH_ENCRYPTION_KEY] = RSIP_ENABLE_FAILURE;
        return false;   
    }

    return true;
}

// Read data from efuse, compare to ram buffers
// If all data matches, return true, else false
bool FactoryTester::validateData(Vector<int> &resultCodes, Vector<String> &resultStrings) {
    bool allMatch = true;
    uint8_t burnedEfuseDataBuffer[32];
    int efuseReadResult = 0;
    int compareResult;

    // For each efuse item
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        EfuseData data = efuseFields[i];
        Log.trace("validating %d data.address: 0x%lx, data.length %lu", i, data.address, data.length);

        // Check that all buffered data is valid, if not return    
        if(!data.isValid) {
            resultCodes[data.json_key] = -1;
            resultStrings[data.json_key] = NO_VALID_DATA_BUFFERED;
            allMatch = false;
            continue;
        }

        // Flash encryption key is unreadable after data is burned, no way to explicitly validate it, skip it
        if(i == EFUSE_DATA_FLASH_ENCRYPTION_KEY){
            continue;
        }

        // Read the item into a generic buffer, using efuse data parameters
        efuseReadResult = readEfuse(data.isPhysicaleFuse, burnedEfuseDataBuffer, data.length, data.address);
        if(efuseReadResult != 0){
            resultCodes[data.json_key] = -1;
            resultStrings[data.json_key] = EFUSE_READ_FAILURE;
            allMatch = false;
        }

        // Special case for checking lock bits in configuration byte
        if((i == EFUSE_DATA_FLASH_ENCRYPTION_LOCK_BITS) || (i == EFUSE_DATA_SECURE_BOOT_LOCK_BITS)) {
            compareResult = ((burnedEfuseDataBuffer[0] & data.data[0]) != burnedEfuseDataBuffer[0]);
        }
        else {
            // Compare the default buffer to the ram buffer
            compareResult = memcmp(burnedEfuseDataBuffer, data.data, data.length);    
        }
        
        if(compareResult != 0) {
            resultCodes[data.json_key] = -1;
            resultStrings[data.json_key] = DATA_DOES_NOT_MATCH;  
            allMatch = false;

            Log.warn("Burned data does not match buffered data");
            Log.dump(burnedEfuseDataBuffer, data.length);
            Log.print("\r\n");
            Log.dump(data.data, data.length);
            Log.print("\r\n");
        }
    }

    return allMatch;
}

int FactoryTester::setFlashEncryptionKey (const char * key) {
    EfuseData * flashKeyEfuse = &efuseFields[EFUSE_DATA_FLASH_ENCRYPTION_KEY];
    EfuseData * lockBits = &efuseFields[EFUSE_DATA_FLASH_ENCRYPTION_LOCK_BITS];
    flashKeyEfuse->isValid = false; 
    lockBits->isValid = false;

    int result = validateCommandData(key, flashKeyEfuse->data, flashKeyEfuse->length);
    if(result != 0) {
        return result;
    }

    flashKeyEfuse->isValid = true;
    
    uint8_t configurationByte = 0xFF;
    readEfuse(true, &configurationByte, 1, EFUSE_PROTECTION_CONFIGURATION_LOW);
    lockBits->data[0] = configurationByte & EFUSE_FLASH_ENCRYPTION_KEY_LOCK_BITS;
    lockBits->isValid = true;
    return 0;
}

int FactoryTester::setFlashEncryption(void) {
    uint8_t systemConfigByte = 0;
    readEfuse(false, &systemConfigByte, 1, EFUSE_SYSTEM_CONFIG);
    systemConfigByte |= (BIT(2));

    return this->writeEfuse(false, &systemConfigByte, sizeof(systemConfigByte), EFUSE_SYSTEM_CONFIG);
}

int FactoryTester::setSecureBootKey(const char * key) {
    EfuseData * secureBootKeyEfuse = &efuseFields[EFUSE_DATA_SECURE_BOOT_KEY];
    EfuseData * lockBits = &efuseFields[EFUSE_DATA_SECURE_BOOT_LOCK_BITS];

    secureBootKeyEfuse->isValid = false; 
    lockBits->isValid = false;

    int result = validateCommandData(key, secureBootKeyEfuse->data, secureBootKeyEfuse->length);
    if(result != 0) {
        return result;
    }

    secureBootKeyEfuse->isValid = true;

    uint8_t configurationByte = 0xFF;
    readEfuse(true, &configurationByte, 1, EFUSE_PROTECTION_CONFIGURATION_LOW);
    lockBits->data[0] = configurationByte & EFUSE_SECURE_BOOT_KEY_LOCK_BITS;
    lockBits->isValid = true;
    return 0;
}

int FactoryTester::setSerialNumber(const char * serial_number) {
    // NOTE: Only first 9 bytes are stored to USER MTP. The last 6 bytes are the WIFI mac, and are stored elsewhere in logical efuse
    EfuseData * serialNumberData = &efuseFields[EFUSE_DATA_SERIAL_NUMBER];
    int result = validateCommandString(serial_number, SERIAL_NUMBER_LENGTH + EFUSE_WIFI_MAC_LENGTH);

    if (result == 0) {
        memcpy(serialNumberData->data, (uint8_t *)serial_number, serialNumberData->length);
    }

    serialNumberData->isValid = (result == 0);
    return result;
}

int FactoryTester::setMobileSecret(const char * mobile_secret) {
    EfuseData * mobileSecretData = &efuseFields[EFUSE_DATA_MOBILE_SECRET];
    int result = validateCommandString(mobile_secret, mobileSecretData->length);

    if (result == 0){
        memcpy(mobileSecretData->data, (uint8_t *)mobile_secret, mobileSecretData->length);
    }

    mobileSecretData->isValid = (result == 0);
    return result;
}

int FactoryTester::setHardwareVersion(const char * hardware_version) {
    EfuseData * hardwareVersionData = &efuseFields[EFUSE_DATA_HARDWARE_VERSION];
    int result = validateCommandData(hardware_version, hardwareVersionData->data, hardwareVersionData->length);
    hardwareVersionData->isValid = (result == 0);
    return result;
}

int FactoryTester::setHardwareModel(const char * hardware_model) {
    EfuseData * hardwareModelData = &efuseFields[EFUSE_DATA_HARDWARE_MODEL];
    int result = validateCommandData(hardware_model, hardwareModelData->data, hardwareModelData->length);
    hardwareModelData->isValid = (result == 0);
    return result;
}

int FactoryTester::setWifiMac(const char * wifi_mac) {
    EfuseData * wifimacData = &efuseFields[EFUSE_DATA_WIFI_MAC];
    int result = validateCommandData(wifi_mac, wifimacData->data, wifimacData->length);
    wifimacData->isValid = (result == 0);
    return result;
}

int FactoryTester::getDeviceId(void) {
    uint8_t deviceIdBuffer[HAL_DEVICE_ID_SIZE];
    memset(this->command_response, 0x00, sizeof(this->command_response));
    hal_get_device_id(deviceIdBuffer, sizeof(deviceIdBuffer));
    return bytes2hexbuf_lower_case(deviceIdBuffer, sizeof(deviceIdBuffer), this->command_response) != 0x00;
}

int FactoryTester::testCommand(void) {
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        EfuseData data = efuseFields[i];
        Log.trace("i: %d valid: %d", i, data.isValid);
        if(data.isValid){
            Log.dump(data.data, data.length);
            Log.print("\r\n");
        }
    }
    return 0;
}

char * FactoryTester::get_command_response(void) {
    return command_response;
}


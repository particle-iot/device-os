#include "application.h"
#include "Particle.h"
#include "check.h"

#include "FactoryTester.h"
#include "TesterCommandTypes.h"

// Tron is platform 32
#if PLATFORM_ID == PLATFORM_TRON 
    extern "C" {
    #define BOOLEAN AMBD_SDK_BOOLEAN
    #include "rtl8721d.h"
    #undef BOOLEAN
    }
#else
    #define BIT(x)                  (1 << (x))
#endif

#define MOCK_EFUSE 1 // Use our own RAM buffer for efuse instead of wearing out the hardware MTP/OTP

#define SERIAL    Serial1
//#define SERIAL    Serial

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
#define EFUSE_USER_MTP      0x160  // TODO: Figure out if this really is MTP and if so, how many writes we have

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

#define EFUSE_SUCCESS 1
#define EFUSE_FAILURE 0

#define LOGICAL_EFUSE_SIZE 1024
static uint8_t logicalEfuseBuffer[LOGICAL_EFUSE_SIZE];

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

const String JSON_KEYS[MFG_TEST_END] = {
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
            SERIAL.printlnf("resp: %d %s", req_->reply_size, printResponse.c_str()); // DEBUG
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
const String FactoryTester::SET_DATA = "SET_DATA";
const String FactoryTester::BURN_DATA = "BURN_DATA";
const String FactoryTester::VALIDATE_BURNED_DATA = "VALIDATE_BURNED_DATA";

// Output strings
const String FactoryTester::PASS = "\"pass\"";
const String FactoryTester::ERRORS = "\"errors\"";

const String FactoryTester::FIELD  = "\"field\"";
const String FactoryTester::MESSAGE  = "\"message\"";
const String FactoryTester::CODE = "\"code\"";


void FactoryTester::setup() {   
    memset(logicalEfuseBuffer, 0xFF, sizeof(logicalEfuseBuffer));
    memset(physicalEfuseBuffer, 0xFF, sizeof(physicalEfuseBuffer));
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        memset(efuseFields[i].data, 0x00, efuseFields[i].length);
    }
    SERIAL.printlnf("Tron Ready!");
}

int FactoryTester::processUSBRequest(ctrl_request* request) {
    memset(usb_buffer, 0x00, sizeof(usb_buffer));
    memcpy(usb_buffer, request->request_data, request->request_size);
    SERIAL.printlnf("USB request size %d type %d request data length: %d ", request->size, request->type, request->request_size);// DEBUG
    SERIAL.printlnf("data: %s", usb_buffer); // DEBUG

    delay(1000);// DEBUG

    bool allPassed = true;

    static const int keyCount = MFG_TEST_END;
    Vector<int> resultCodes(keyCount);
    Vector<String> resultStrings(keyCount);

    this->command_response[0] = 0;
    Request response;

    CHECK(response.init(request));

    JSONObjectIterator it(response.data());
    while (it.next()) {
        const char * firstKey = it.name().data();
        JSONValue firstData = it.value();
        JSONType firstType = firstData.type();

        SERIAL.printlnf("%s : %s : %d", firstKey, firstData.toString().data(), firstType); // DEBUG

        // Identify main command type
        if(!strcmp(firstKey, SET_DATA) && (firstType == JSON_TYPE_OBJECT)) {
            JSONObjectIterator innerIterator(firstData);
            while (innerIterator.next()) {
                const char * innerKey = innerIterator.name().data();
                const char * innerData = (const char *)innerIterator.value().toString(); // TODO: non string types?

                SERIAL.printlnf("%s : %s", innerKey, innerData); // DEBUG
                delay(50);

                // Parse data from each key
                for (int i = 0; i < keyCount; i++) {
                    if(strcmp(innerKey, JSON_KEYS[i]) == 0){
                        int set_data_result = setData((MfgTestKeyType)i, innerData);
                        resultCodes[i] = set_data_result;
                        resultStrings[i] = this->command_response[0] != 0 ? this->command_response : "";

                        allPassed &= (set_data_result == 0);
                    } 
                }
            }
        }
        else if(!strcmp(firstKey, IS_READY)){
            allPassed = true; // do nothing really
        }
        else if(!strcmp(firstKey, BURN_DATA)){
            allPassed = burnData(resultCodes, resultStrings);
        }
        else if(!strcmp(firstKey, VALIDATE_BURNED_DATA)){
            // TODO: Read data from efuse into local buffers
            // Parse each key, compare to buffer
        }
    }


    SERIAL.printlnf("allPassed: %d", allPassed); // DEBUG

    // DEBUG:
    for (int i = 0; i < resultCodes.size(); i++){
        if (resultCodes[i] != 0xFF) {
            SERIAL.printlnf("resultCodes: %d %d", i, resultCodes[i]); // DEBUG
            delay(50);
        }
        if (resultStrings[i] != ""){
            SERIAL.printlnf("resultStrings: %d %s", i, resultStrings[i].c_str()); // DEBUG
            delay(50);
        }
    }

    // Send json response
    const int r = CHECK(
        response.reply([allPassed, resultCodes, resultStrings, this](JSONWriter& w) {
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
        w.endObject();
    }));

    response.done(r);
    return r;
}

// Validate and buffer provisioning data RAM
int FactoryTester::setData(MfgTestKeyType command, const char * commandData) {

    int result = -1;

    switch (command) {
        case MFG_TEST_FLASH_ENCRYPTION_KEY:
            result = this->SET_FLASH_ENCRYPTION_KEY(commandData);
            break;   
        case MFG_TEST_SECURE_BOOT_KEY:
            result = this->SET_SECURE_BOOT_KEY(commandData);
            break;   
        case MFG_TEST_SERIAL_NUMBER:
            result = this->SET_SERIAL_NUMBER(commandData);
            break;   
        case MFG_TEST_MOBILE_SECRET:
            result = this->SET_MOBILE_SECRET(commandData);
            break;  
        case MFG_TEST_HW_VERSION:
            result = this->SET_HW_VERSION(commandData);
            break;  
        case MFG_TEST_HW_MODEL:
            result = this->SET_HW_MODEL(commandData);
            break;
        case MFG_TEST_WIFI_MAC:
            result = this->SET_WIFI_MAC(commandData);
            break;
        default:
        case MFG_TEST_END:
            break;
    }

    return result;
}

static char hex_char(char c) {
    if ('0' <= c && c <= '9') return (unsigned char)(c - '0');
    if ('A' <= c && c <= 'F') return (unsigned char)(c - 'A' + 10);
    if ('a' <= c && c <= 'f') return (unsigned char)(c - 'a' + 10);
    return 0xFF;
}

static int hex_to_bin(const char *s, char *buff, int length) {
    int result;
    if (!s || !buff || length <= 0) {
        return -1;
    }

    for (result = 0; *s; ++result) {
        unsigned char msn = hex_char(*s++);
        if (msn == 0xFF) {
            return -1;
        }
        
        unsigned char lsn = hex_char(*s++);
        if (lsn == 0xFF) {
            return -1;
        }
        
        unsigned char bin = (msn << 4) + lsn;

        if (length-- <= 0) {
            return -1;
        }
        
        *buff++ = bin;
    }
    return result;
}

static int bin_to_hex(char * dest, int dest_length, const uint8_t * source, int source_length) {
    if(source_length * 2 >= dest_length){
        return -1;
    }

    for(int i = 0, j = 0; j < source_length; i += 2, j++){
        snprintf(&dest[i], 3, "%02X", source[j]);
    }
    return 0;
}

#if PLATFORM_ID != PLATFORM_TRON 
    // Fake efuse functions for non tron platforms
    #define L25EOUTVOLTAGE                      7

    unsigned EFUSE_LMAP_READ(uint8_t * buffer){
        return 0;
    }

    unsigned EFUSE_PMAP_READ8(uint32_t CtrlSetting, uint32_t Addr, uint8_t* Data, uint8_t L25OutVoltage){
        return 0;
    }
#endif

int FactoryTester::writeEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address) {    
    uint32_t eFuseWrite = EFUSE_SUCCESS;
    uint32_t i = 0;

    if (physical) {
        for (i = 0; (i < length) && (eFuseWrite == EFUSE_SUCCESS); i++) {
            #if MOCK_EFUSE
                physicalEfuseBuffer[address + i] = data[i];
            #else
                //eFuseWrite = EFUSE_PMAP_WRITE8(0, address + i, &data[i], L25EOUTVOLTAGE); // TODO: UNCOMMENT OUT FOR ACTUAL EFUSE WRITES
            #endif
            
            // SERIAL.printlnf("physical eFuse write byte 0x%02X @ 0x%X", data[i], (unsigned int)(address + i)); // Debug
        }
    }
    else {
        #if MOCK_EFUSE
            memcpy(logicalEfuseBuffer + address, data, length);
        #else 
            // eFuseWrite = EFUSE_LMAP_WRITE(address, length, data); // TODO: UNCOMMENT OUT FOR ACTUAL EFUSE WRITES         
        #endif

        // // Debug
        // SERIAL.printlnf("logical eFuse write @ 0x%X", (unsigned int)(address));
        // for(unsigned i = 0; i < length; i++){
        //     SERIAL.printf("%02X", data[i]);
        // }
        // SERIAL.println("");
    }

    return (eFuseWrite == EFUSE_SUCCESS) ? 0 : SYSTEM_ERROR_NOT_ALLOWED;
}

int FactoryTester::readEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address) {    
    uint32_t eFuseRead = EFUSE_SUCCESS;
    uint32_t i = 0;

    if (physical) {
        for (i = 0; (i < length) && (eFuseRead == EFUSE_SUCCESS); i++) {
            #if MOCK_EFUSE
                data[i] = physicalEfuseBuffer[address + i];
            #else
                eFuseRead = EFUSE_PMAP_READ8(0, address + i, &data[i], L25EOUTVOLTAGE);
            #endif
            //SERIAL.printlnf("readEfuse 0x%02X @ 0x%X", data[i], (unsigned int)(address + i));
        }
    }
    else {
        #if MOCK_EFUSE
            memcpy(data, logicalEfuseBuffer + address, length);
        #else
            eFuseRead = EFUSE_LMAP_READ(logicalEfuseBuffer);
            if(eFuseRead == EFUSE_SUCCESS) {
                memcpy(data, logicalEfuseBuffer + address, length);
            }
        #endif
    }

    if(eFuseRead == EFUSE_FAILURE)
    {
        SERIAL.println("Error Reading eFuse");
    }

    // 0 = success, 1 = failure
    return !eFuseRead;
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

    // Validate characters in the serial number.
    for (uint8_t i = 0; i < len; i++) {
        if (!isxdigit(data[i])) {
            SERIAL.printlnf("Validate Hex Data FAILED: invalid char value 0x%02X at index %d", data[i], i);
            return SYSTEM_ERROR_BAD_DATA;        
        }
    }
    
    // Convert Hex string to binary data.
    if (hex_to_bin(data, (char *)output_bytes, output_bytes_length) == -1) {
        SERIAL.printlnf("Validate Hex Data FAILED: hex_to_bin failed");
        return SYSTEM_ERROR_BAD_DATA;        
    }

    return 0;
}

int FactoryTester::validateCommandString(const char * data, int expectedLength) {
    int length = strlen(data);

    //SERIAL.printlnf("validate STR length: %d expectedlength %d", length, expectedLength);
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


bool FactoryTester::burnData(Vector<int> &resultCodes, Vector<String> &resultStrings) {
    // TEST CASES
    // 1) Buffered data is not all marked valid   X
    // 2) Individual efuse write fails            X
    // 3) User MTP logical efuse write fails      X
    // 4) Everything works                        X    

    bool allBufferedDataValid = true;
    int efuseWriteResult;
    uint8_t userMTPBuffer[USER_MTP_DATA_LENGTH] = {0x00};

    // Check that all buffered data is valid, if not return
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        EfuseData data = efuseFields[i];
        if(!data.isValid) {
            resultCodes[data.json_key] = -1;
            resultStrings[data.json_key] = "no valid data buffered";
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
            int efuseWriteResult = writeEfuse(data.isPhysicaleFuse, data.data, data.length, data.address);
            if(efuseWriteResult != 0){
                resultCodes[data.json_key] = efuseWriteResult;
                resultStrings[data.json_key] = "eFuse write failure";
                return false;
            }
        }
        // Else buffer logical efuse data
        else {
            memcpy(userMTPBuffer + (data.address - EFUSE_USER_MTP), data.data, data.length);
        }
    }

    // Combine user MTP data fields to be efficient with logical efuse MTP write operations
    // User serial number as standin for whole efuse failure
    efuseWriteResult = writeEfuse(false, userMTPBuffer, USER_MTP_DATA_LENGTH, EFUSE_USER_MTP);
    if(efuseWriteResult != 0){
        resultCodes[MFG_TEST_SERIAL_NUMBER] = efuseWriteResult;
        resultStrings[MFG_TEST_SERIAL_NUMBER] = "User MTP logical eFuse write failure";
        return false;
    }

    return true;
}

int FactoryTester::SET_FLASH_ENCRYPTION_KEY (const char * key) {
    EfuseData * flashKeyEfuse = &efuseFields[EFUSE_DATA_FLASH_ENCRYPTION_KEY];
    EfuseData * lockBits = &efuseFields[EFUSE_DATA_FLASH_ENCRYPTION_LOCK_BITS];
    // TODO: Should good data be kept if given bad data? Or always invalidate buffered data when given new data?
    flashKeyEfuse->isValid = false; 
    lockBits->isValid = false;

    // Check to make sure a key is not already written
    readEfuse(true, flashKeyEfuse->data, flashKeyEfuse->length, flashKeyEfuse->address);
    for (int i = 0; i < FLASH_ENCRYPTION_KEY_LENGTH; i++) {
        if(flashKeyEfuse->data[i] != 0xFF) {
            SERIAL.printlnf("ERROR: Flash Encryption key already written! Index: %d, value 0x%02X", i, flashKeyEfuse->data[i]);
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
    }

    int result = validateCommandData(key, flashKeyEfuse->data, flashKeyEfuse->length);
    if(result != 0) {
        return result;
    }

    flashKeyEfuse->isValid = true;
    
    lockBits->data[0] = 0xDB; // Bit 2,5 = Read, Write Forbidden
    lockBits->isValid = true;
    return 0;
}

int FactoryTester::ENABLE_FLASH_ENCRYPTION(void) {
    uint8_t systemConfigByte = 0;
    readEfuse(false, &systemConfigByte, 1, EFUSE_SYSTEM_CONFIG);
    //SERIAL.printlnf("System Config: %02X", systemConfigByte);

    systemConfigByte |= (BIT(2));
    //SERIAL.printlnf("System Config: %02X", systemConfigByte);

    return this->writeEfuse(false, &systemConfigByte, sizeof(systemConfigByte), EFUSE_SYSTEM_CONFIG);
}

int FactoryTester::SET_SECURE_BOOT_KEY(const char * key) {
    EfuseData * secureBootKeyEfuse = &efuseFields[EFUSE_DATA_SECURE_BOOT_KEY];
    EfuseData * lockBits = &efuseFields[EFUSE_DATA_SECURE_BOOT_LOCK_BITS];

    secureBootKeyEfuse->isValid = false; 
    lockBits->isValid = false;

    // Check to make sure a key is not already written
    readEfuse(true, secureBootKeyEfuse->data, secureBootKeyEfuse->length, secureBootKeyEfuse->address);
    for (int i = 0; i < FLASH_ENCRYPTION_KEY_LENGTH; i++) {
        if(secureBootKeyEfuse->data[i] != 0xFF) {
            SERIAL.printlnf("ERROR: Flash Encryption key already written! Index: %d, value 0x%02X", i, secureBootKeyEfuse->data[i]);
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
    }

    int result = validateCommandData(key, secureBootKeyEfuse->data, secureBootKeyEfuse->length);
    if(result != 0) {
        return result;
    }

    secureBootKeyEfuse->isValid = true;

    lockBits->data[0] = 0xBF; // Bit 6 = Write Forbidden
    lockBits->isValid = true;
    return 0;
}

int FactoryTester::GET_SECURE_BOOT_KEY(void) {
    uint8_t boot_key[SECURE_BOOT_KEY_LENGTH];
    readEfuse(true, boot_key, sizeof(boot_key), EFUSE_SECURE_BOOT_KEY);

    return bin_to_hex(this->command_response, this->cmd_length, boot_key, sizeof(boot_key));
}

int FactoryTester::ENABLE_SECURE_BOOT(void) {
    return SYSTEM_ERROR_NOT_ALLOWED;

    // // SECURE BOOT WILL BE ENABLED AT A FUTURE DATE. EXAMPLE CODE TO DO SO ONLY
    // uint8_t protectionConfigurationByte = 0;
    // readEfuse(true, &protectionConfigurationByte, 1, EFUSE_PROTECTION_CONFIGURATION_HIGH);

    // protectionConfigurationByte &= ~(BIT(5));

    // // Write enable bit to physical eFuse
    // return this->writeEfuse(true, &protectionConfigurationByte, 1, EFUSE_PROTECTION_CONFIGURATION_HIGH);
}

int FactoryTester::GET_SERIAL_NUMBER(void) {
    char fullSerialNumber[SERIAL_NUMBER_LENGTH + (EFUSE_WIFI_MAC_LENGTH * 2) + 1] = {0};
    readEfuse(false, (uint8_t *)fullSerialNumber, SERIAL_NUMBER_LENGTH, EFUSE_USER_MTP + EFUSE_SERIAL_NUMBER_OFFSET);

    uint8_t mac[EFUSE_WIFI_MAC_LENGTH+1] = {0};
    readEfuse(false, mac, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);

    for(int i = 0, j = (EFUSE_WIFI_MAC_LENGTH / 2); j < EFUSE_WIFI_MAC_LENGTH; i += 2, j++){
        snprintf(&fullSerialNumber[SERIAL_NUMBER_LENGTH + i], 3, "%02X", mac[j]);
    }

    strcpy(this->command_response, fullSerialNumber);
    return 0;
}

int FactoryTester::SET_SERIAL_NUMBER(const char * serial_number) {
    // TODO: Consider taking 15 chars as input and writing to MAC address as well
    // OR: Only requiring 9 chars as input, and wifi mac explicitly sent elsewhere
    EfuseData * serialNumberData = &efuseFields[EFUSE_DATA_SERIAL_NUMBER];
    int result = validateCommandString(serial_number, SERIAL_NUMBER_LENGTH + EFUSE_WIFI_MAC_LENGTH);

    if (result == 0) {
        memcpy(serialNumberData->data, (uint8_t *)serial_number, serialNumberData->length);
    }

    serialNumberData->isValid = (result == 0);
    return result;
}

int FactoryTester::GET_MOBILE_SECRET(void) {
    char mobileSecret[MOBILE_SECRET_LENGTH+1] = {0};
    readEfuse(false, (uint8_t *)mobileSecret, MOBILE_SECRET_LENGTH, EFUSE_USER_MTP + EFUSE_MOBILE_SECRET_OFFSET);
    strcpy(this->command_response, mobileSecret);
    return 0;
}

int FactoryTester::SET_MOBILE_SECRET(const char * mobile_secret) {
    EfuseData * mobileSecretData = &efuseFields[EFUSE_DATA_MOBILE_SECRET];
    int result = validateCommandString(mobile_secret, mobileSecretData->length);

    if (result == 0){
        memcpy(mobileSecretData->data, (uint8_t *)mobile_secret, mobileSecretData->length);
    }

    mobileSecretData->isValid = (result == 0);
    return result;
}

int FactoryTester::GET_HW_VERSION(void) {
    uint8_t hardwareData[HARDWARE_VERSION_LENGTH] = {0};
    readEfuse(false, hardwareData, HARDWARE_VERSION_LENGTH, EFUSE_USER_MTP + EFUSE_HARDWARE_VERSION_OFFSET);

    return bin_to_hex(this->command_response, this->cmd_length, hardwareData, sizeof(hardwareData));
}

int FactoryTester::SET_HW_VERSION(const char * hardware_version) {
    EfuseData * hardwareVersionData = &efuseFields[EFUSE_DATA_HARDWARE_VERSION];
    int result = validateCommandData(hardware_version, hardwareVersionData->data, hardwareVersionData->length);
    hardwareVersionData->isValid = (result == 0);
    return result;
}

int FactoryTester::GET_HW_MODEL(void) {
    uint8_t hardwareModel[HARDWARE_MODEL_LENGTH] = {0};
    readEfuse(false, hardwareModel, HARDWARE_MODEL_LENGTH, EFUSE_USER_MTP + EFUSE_HARDWARE_MODEL_OFFSET);

    return bin_to_hex(this->command_response, this->cmd_length, hardwareModel, sizeof(hardwareModel));
}

int FactoryTester::SET_HW_MODEL(const char * hardware_model) {
    EfuseData * hardwareModelData = &efuseFields[EFUSE_DATA_HARDWARE_MODEL];
    int result = validateCommandData(hardware_model, hardwareModelData->data, hardwareModelData->length);
    hardwareModelData->isValid = (result == 0);
    return result;
}

int FactoryTester::GET_WIFI_MAC(void) {
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH] = {0};
    readEfuse(false, wifiMAC, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);

    return bin_to_hex(this->command_response, this->cmd_length, wifiMAC, sizeof(wifiMAC));
}

int FactoryTester::SET_WIFI_MAC(const char * wifi_mac) {
    EfuseData * wifimacData = &efuseFields[EFUSE_DATA_WIFI_MAC];
    int result = validateCommandData(wifi_mac, wifimacData->data, wifimacData->length);
    wifimacData->isValid = (result == 0);
    return result;
}

int FactoryTester::GET_DEVICE_ID(void) {
    // TODO: Make this use real device ID HAL
    // 0A10ACED202194944A0DFF0A
    const char * deviceIDPrefix = "0A10ACED2021";
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH] = {0};
    readEfuse(false, wifiMAC, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);

    strcpy(this->command_response, deviceIDPrefix);

    return bin_to_hex(this->command_response + strlen(deviceIDPrefix),
                      this->cmd_length - strlen(deviceIDPrefix),
                      wifiMAC, 
                      sizeof(wifiMAC));
}

int FactoryTester::TEST_COMMAND(void) {
    for(int i = 0; i < EFUSE_DATA_MAX; i++){
        SERIAL.printlnf("i: %d valid: %d", i, efuseFields[i].isValid);
        SERIAL.printlnf("%02X%02X%02X%02X", efuseFields[i].data[0], efuseFields[i].data[1], efuseFields[i].data[2], efuseFields[i].data[3]);
        delay(10);
    }

    return 0;
}

char * FactoryTester::get_command_response(void) {
    return command_response;
}


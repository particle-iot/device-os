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
#define HARDWARE_DATA_LENGTH        4
#define HARDWARE_VERSION_LENGTH     4
#define HARDWARE_MODEL_LENGTH       4

// Logical eFuse addresses
#define EFUSE_SYSTEM_CONFIG 0x0E
#define EFUSE_WIFI_MAC      0x11A
#define EFUSE_USER_MTP      0x160  // TODO: Figure out if this really is MTP and if so, how many writes we have

#define EFUSE_WIFI_MAC_LENGTH 6

#define EFUSE_MOBILE_SECRET_OFFSET 0
#define EFUSE_SERIAL_NUMBER_OFFSET (EFUSE_MOBILE_SECRET_OFFSET + MOBILE_SECRET_LENGTH)
#define EFUSE_HARDWARE_DATA_OFFSET (EFUSE_SERIAL_NUMBER_OFFSET + SERIAL_NUMBER_LENGTH)
#define EFUSE_HARDWARE_MODEL_OFFSET (EFUSE_HARDWARE_DATA_OFFSET + HARDWARE_DATA_LENGTH)

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

static unsigned mtp_data_buffered = 0;
static uint8_t userMTPBuffer[USER_MTP_DATA_LENGTH];

struct ProvisioningData {
    uint32_t data_buffered_bits;

    uint8_t flashEncryptionKey[FLASH_ENCRYPTION_KEY_LENGTH];
    uint8_t secureBootKey[SECURE_BOOT_KEY_LENGTH];
    char serialNumber[SERIAL_NUMBER_LENGTH];
    char mobileSecret[MOBILE_SECRET_LENGTH];
    uint8_t hardwareVersion[HARDWARE_VERSION_LENGTH];
    uint8_t hardwareModel[HARDWARE_MODEL_LENGTH];
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH];

    uint8_t userMTPBuffer[USER_MTP_DATA_LENGTH];
};

struct TesterCommand {
    const char *name;
    TesterCommandType type;
};

const TesterCommand commands[] = {
        {"flash_encryption_key",    TesterCommandType::SET_FLASH_ENCRYPTION_KEY},
        {"ENABLE_FLASH_ENCRYPTION",     TesterCommandType::ENABLE_FLASH_ENCRYPTION},
        {"secure_boot_key",         TesterCommandType::SET_SECURE_BOOT_KEY},
        {"GET_SECURE_BOOT_KEY",         TesterCommandType::GET_SECURE_BOOT_KEY},
        {"ENABLE_SECURE_BOOT",          TesterCommandType::ENABLE_SECURE_BOOT},
        {"serial_number",           TesterCommandType::SET_SERIAL_NUMBER},
        {"GET_SERIAL_NUMBER",           TesterCommandType::GET_SERIAL_NUMBER},
        {"mobile_secret",           TesterCommandType::SET_MOBILE_SECRET},
        {"GET_MOBILE_SECRET",           TesterCommandType::GET_MOBILE_SECRET},
        {"hw_version",              TesterCommandType::SET_HW_VERSION},
        {"GET_HW_VERSION",              TesterCommandType::GET_HW_VERSION},
        {"hw_model",                TesterCommandType::SET_HW_MODEL},
        {"GET_HW_MODEL",                TesterCommandType::GET_HW_MODEL},
        {"wifi_mac",                TesterCommandType::SET_WIFI_MAC},
        {"GET_WIFI_MAC",                TesterCommandType::GET_WIFI_MAC},
        {"GET_DEVICE_ID",               TesterCommandType::GET_DEVICE_ID},
        {"IS_READY",                    TesterCommandType::IS_READY},
        {"TEST_COMMAND",                TesterCommandType::TEST_COMMAND},
        // Add commands above this line
        {"",                            TesterCommandType::END}
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
const String FactoryTester::SET_DATA = "SET_DATA";

// Output strings
const String FactoryTester::PASS = "\"pass\"";
const String FactoryTester::RESPONSE  = "\"response\"";
const String FactoryTester::ERROR = "\"error\"";
const String FactoryTester::ERROR_CODE = "\"error_code\"";


void FactoryTester::setup() {   
    memset(logicalEfuseBuffer, 0xFF, sizeof(logicalEfuseBuffer));
    memset(physicalEfuseBuffer, 0xFF, sizeof(physicalEfuseBuffer));
    memset(userMTPBuffer, 0x00, sizeof(userMTPBuffer));
    SERIAL.printlnf("Tron Ready!");
}

int FactoryTester::processUSBRequest(ctrl_request* request) {
    memset(usb_buffer, 0x00, sizeof(usb_buffer));
    memcpy(usb_buffer, request->request_data, request->request_size);
    SERIAL.printlnf("USB request size %d type %d request data length: %d ", request->size, request->type, request->request_size);
    SERIAL.printlnf("data: %s", usb_buffer);

    delay(1000);

    //CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    int result = -1;

    int commandCount = static_cast<unsigned>(TesterCommandType::END);
    int resultCodes[commandCount];
    String resultStrings[commandCount];
    for (int i = 0; i < commandCount; i++){
        resultCodes[i] = 0xFF;
        resultStrings[i] = "";
    }

    this->command_response[0] = 0;
    Request response;

    CHECK(response.init(request));

    JSONObjectIterator it(response.data());
    while (it.next()) {
        const char * firstKey = it.name().data();
        JSONValue firstData = it.value();
        JSONType firstType = firstData.type();

        // if(currentType == JSONType::JSON_TYPE_STRING) {
        //     currentData = it.value().toString().data();
        // }

        SERIAL.printlnf("%s : %s : %d", firstKey, firstData.toString().data(), firstType); // DEBUG

        // Identify main command type
        // Parse main object for SET_DATA
        // Store key and return value for each key sent
        // After iterating complete, look at key/value collection to determine pass/fail.
        // Any failures: pass = false, list keys that failed + error message + error code
        // Send json response

        if(!strcmp(firstKey, SET_DATA) && (firstType == JSON_TYPE_OBJECT)) {
            SERIAL.printlnf("checking SET_DATA"); // DEBUG
            delay(50);
            JSONObjectIterator innerIterator(firstData);
            while (innerIterator.next()) {
                const char * innerKey = innerIterator.name().data();
                const char * innerData = (const char *)innerIterator.value().toString(); // TODO: non string types?
                SERIAL.printlnf("%s : %s", innerKey, innerData); // DEBUG
                delay(50);

                for (int i = 0; commands[i].type != TesterCommandType::END; i++) {
                    if(strcmp(innerKey, commands[i].name) == 0){
                        resultCodes[i] = executeCommand(commands[i].type, innerData);
                        if(this->command_response[0] != 0){
                            resultStrings[i] = this->command_response;
                        }
                    } 
                }
            }
        }
        // TODO: else if other command types
    }

    for (int i = 0; i < commandCount; i++){
        if (resultCodes[i] != 0xFF) {
            SERIAL.printlnf("resultCodes: %d %d", i, resultCodes[i]); // DEBUG
            delay(50);
        }
        if (resultStrings[i] != ""){
            SERIAL.printlnf("resultStrings: %d %s", i, resultStrings[i].c_str()); // DEBUG
            delay(50);
        }
    }

    bool responseMessage = (this->command_response[0] != 0);

    // If error response, and no text specifed, use the text for the error code
    if(result != 0 && !responseMessage){
        strcpy(this->command_response, get_system_error_message(result));
        responseMessage = true;
    }

    if(responseMessage){
        SERIAL.printlnf("RESPONSE VALUE: %d %s", responseMessage, this->command_response); // DEBUG LINE    
    }
    
    const int r = CHECK(
        response.reply([result, responseMessage, this](JSONWriter& w) {
        w.beginObject();
            bool isSuccess = result >= 0;
            w.name(PASS).value(isSuccess);
            if(responseMessage) {
                w.name(isSuccess ? RESPONSE : ERROR).value(this->command_response);
                if(!isSuccess){
                    w.name(ERROR_CODE).value(result);
                }
            }
        w.endObject();
    }));

    response.done(r);
    return r;
}


int FactoryTester::executeCommand(TesterCommandType command, const char * commandData) {

    int result = -1;

    // TODO: Refactor
    switch (command) {
        case TesterCommandType::SET_FLASH_ENCRYPTION_KEY:
            result = this->SET_FLASH_ENCRYPTION_KEY(commandData);
            break;
        case TesterCommandType::ENABLE_FLASH_ENCRYPTION:
            result = this->ENABLE_FLASH_ENCRYPTION();
            break;   
        case TesterCommandType::SET_SECURE_BOOT_KEY:
            result = this->SET_SECURE_BOOT_KEY(commandData);
            break;   
        case TesterCommandType::GET_SECURE_BOOT_KEY:
            result = this->GET_SECURE_BOOT_KEY();
            break;   
        case TesterCommandType::ENABLE_SECURE_BOOT:
            result = this->ENABLE_SECURE_BOOT();
            break;
        case TesterCommandType::GET_SERIAL_NUMBER:
            result = this->GET_SERIAL_NUMBER();
            break;
        case TesterCommandType::SET_SERIAL_NUMBER:
            result = this->SET_SERIAL_NUMBER(commandData);
            break;   
        case TesterCommandType::GET_MOBILE_SECRET:
            result = this->GET_MOBILE_SECRET();
            break;
        case TesterCommandType::SET_MOBILE_SECRET:
            result = this->SET_MOBILE_SECRET(commandData);
            break;  
        case TesterCommandType::GET_HW_VERSION:
            result = this->GET_HW_VERSION();
            break;
        case TesterCommandType::SET_HW_VERSION:
            result = this->SET_HW_VERSION(commandData);
            break;  
        case TesterCommandType::GET_HW_MODEL:
            result = this->GET_HW_MODEL();
            break;
        case TesterCommandType::SET_HW_MODEL:
            result = this->SET_HW_MODEL(commandData);
            break;
        case TesterCommandType::GET_WIFI_MAC:
            result = this->GET_WIFI_MAC();
            break;
        case TesterCommandType::SET_WIFI_MAC:
            result = this->SET_WIFI_MAC(commandData);
            break;
        case TesterCommandType::GET_DEVICE_ID:
            result = this->GET_DEVICE_ID();
            break;
        case TesterCommandType::IS_READY:
            result = this->IS_READY();
            break;
        case TesterCommandType::TEST_COMMAND:
            result = this->TEST_COMMAND();
            break;
        default:
        case TesterCommandType::NONE:
        case TesterCommandType::END:
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
            
            // SERIAL.printlnf("eFuse write byte 0x%02X @ 0x%X", data[i], (unsigned int)(address + i));
        }
    }
    else {
        #if MOCK_EFUSE
            memcpy(logicalEfuseBuffer + address, data, length);
        #else 
            // eFuseWrite = EFUSE_LMAP_WRITE(address, length, data); // TODO: UNCOMMENT OUT FOR ACTUAL EFUSE WRITES         
        #endif
    }

    if(eFuseWrite == EFUSE_FAILURE) {
        if(physical){
            SERIAL.printlnf("Error Writing physical eFuse, only %lu bytes written of %lu to address 0x%X successfully", i-1, length, (unsigned int)address);
        }
        else {
            SERIAL.printlnf("Error Writing logical eFuse address %lu", address);
        }
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

int FactoryTester::bufferMTPData(uint8_t * data, int data_length, TesterMTPTypes mtp_type) {
    int offset = 0;

    switch(mtp_type)
    {
        case TesterMTPTypes::MOBILE_SECRET:
            offset = EFUSE_MOBILE_SECRET_OFFSET;
            break;
        case TesterMTPTypes::SERIAL_NUMBER:
            offset = EFUSE_SERIAL_NUMBER_OFFSET;
            break;
        case TesterMTPTypes::HARDWARE_DATA:
            offset = EFUSE_HARDWARE_DATA_OFFSET;
            break;
        case TesterMTPTypes::HARDWARE_MODEL:
            offset = EFUSE_HARDWARE_MODEL_OFFSET;
            break;
        default:
            return -1;
    }
    
    memcpy(userMTPBuffer + offset, data, data_length);

    mtp_data_buffered |= (static_cast<unsigned>(mtp_type));
    //SERIAL.printlnf("MTP buffered: 0x%X", mtp_data_buffered); // DEBUG

    // Only write data to logical efuse once we have all the components. This is to minimize the number of writes to MTP 
    if( (mtp_data_buffered & ALL_MTP_DATA_BUFFERED) == ALL_MTP_DATA_BUFFERED) {
        mtp_data_buffered = 0;
        //SERIAL.println("writing MTP data"); // DEBUG
        return writeEfuse(false, userMTPBuffer, sizeof(userMTPBuffer), EFUSE_USER_MTP); // TODO: Return true/false on efuse write
    }
    return 0;
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

int FactoryTester::SET_FLASH_ENCRYPTION_KEY (const char * key) {
    // TODO: MAKE RAM STRUCTURE TO HOLD ALL DATA
    // TODO: WRITE DATA TO RAM STRUCTURE AS ITS RECEIVED/PARSED

    uint8_t flashEncryptionKey[FLASH_ENCRYPTION_KEY_LENGTH] = {0};

    // Check to make sure a key is not already written
    readEfuse(true, flashEncryptionKey, sizeof(flashEncryptionKey), EFUSE_FLASH_ENCRYPTION_KEY);
    for (int i = 0; i < FLASH_ENCRYPTION_KEY_LENGTH; i++) {
        if(flashEncryptionKey[i] != 0xFF) {
            SERIAL.printlnf("ERROR: Flash Encryption key already written! Index: %d, value 0x%02X", i, flashEncryptionKey[i]);
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
    }

    int result = validateCommandData(key, flashEncryptionKey, sizeof(flashEncryptionKey));
    if ( result != 0) {
        return result;
    }

    // Write flash encryption key to physical eFuse
    uint32_t eFuseWrite = this->writeEfuse(true, flashEncryptionKey, sizeof(flashEncryptionKey), EFUSE_FLASH_ENCRYPTION_KEY);
    if(eFuseWrite == 0)
    {
        // Write flash encryption key lock bits
        uint8_t flashEncryptionKeyReadWriteForbidden = 0xDB; // Bit 2,5 = Read, Write Forbidden
        eFuseWrite = this->writeEfuse(true, &flashEncryptionKeyReadWriteForbidden, sizeof(flashEncryptionKeyReadWriteForbidden), EFUSE_PROTECTION_CONFIGURATION_LOW);
    }

    return eFuseWrite;
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
    uint8_t secureBootKeyBytes[SECURE_BOOT_KEY_LENGTH];

    // Check to make sure a key is not already written
    readEfuse(true, secureBootKeyBytes, sizeof(secureBootKeyBytes), EFUSE_SECURE_BOOT_KEY);
    for (int i = 0; i < SECURE_BOOT_KEY_LENGTH; i++) {
        if(secureBootKeyBytes[i] != 0xFF) {
            SERIAL.printlnf("ERROR: Secure boot key already written! Index: %d, value 0x%02X", i, secureBootKeyBytes[i]);
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
    }

    int result = validateCommandData(key, secureBootKeyBytes, sizeof(secureBootKeyBytes));
    if ( result != 0) {
        return result;
    }
    
    // Write key data to physical eFuse
    uint32_t eFuseWrite = this->writeEfuse(true, secureBootKeyBytes, sizeof(secureBootKeyBytes), EFUSE_SECURE_BOOT_KEY);
    if(eFuseWrite == 0)
    {
        // Lock key data bits
        uint8_t secureBootWriteForbidden = 0xBF; // Bit 6 = Write Forbidden
        eFuseWrite = this->writeEfuse(true, &secureBootWriteForbidden, sizeof(secureBootWriteForbidden), EFUSE_PROTECTION_CONFIGURATION_LOW);
    }

    return eFuseWrite;
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
    int result = validateCommandString(serial_number, SERIAL_NUMBER_LENGTH + EFUSE_WIFI_MAC_LENGTH);

    if (result == 0) {
        result = bufferMTPData((uint8_t *)serial_number, SERIAL_NUMBER_LENGTH, TesterMTPTypes::SERIAL_NUMBER);
    }
    
    strcpy(this->command_response, "test serial number response");
    return result;
}

int FactoryTester::GET_MOBILE_SECRET(void) {
    char mobileSecret[MOBILE_SECRET_LENGTH+1] = {0};
    readEfuse(false, (uint8_t *)mobileSecret, MOBILE_SECRET_LENGTH, EFUSE_USER_MTP + EFUSE_MOBILE_SECRET_OFFSET);
    strcpy(this->command_response, mobileSecret);
    return 0;
}

int FactoryTester::SET_MOBILE_SECRET(const char * mobile_secret) {
    int result = validateCommandString(mobile_secret, MOBILE_SECRET_LENGTH);

    if (result == 0){
        result = bufferMTPData((uint8_t *)mobile_secret, MOBILE_SECRET_LENGTH, TesterMTPTypes::MOBILE_SECRET);    
    }
    
    return result;
}

int FactoryTester::GET_HW_VERSION(void) {
    uint8_t hardwareData[HARDWARE_DATA_LENGTH] = {0};
    readEfuse(false, hardwareData, HARDWARE_DATA_LENGTH, EFUSE_USER_MTP + EFUSE_HARDWARE_DATA_OFFSET);

    return bin_to_hex(this->command_response, this->cmd_length, hardwareData, sizeof(hardwareData));
}

int FactoryTester::SET_HW_VERSION(const char * hardware_version) {
    uint8_t hardwareData[HARDWARE_DATA_LENGTH] = {0};
    int result = validateCommandData(hardware_version, hardwareData, sizeof(hardwareData)); 

    if (result == 0) {
        // Write validated input to eFuse
        result = bufferMTPData(hardwareData, HARDWARE_DATA_LENGTH, TesterMTPTypes::HARDWARE_DATA);
    }

    return result;
}

int FactoryTester::GET_HW_MODEL(void) {
    uint8_t hardwareModel[HARDWARE_MODEL_LENGTH] = {0};
    readEfuse(false, hardwareModel, HARDWARE_MODEL_LENGTH, EFUSE_USER_MTP + EFUSE_HARDWARE_MODEL_OFFSET);

    return bin_to_hex(this->command_response, this->cmd_length, hardwareModel, sizeof(hardwareModel));
}

int FactoryTester::SET_HW_MODEL(const char * hardware_model) {
    uint8_t hardwareModelBytes[HARDWARE_MODEL_LENGTH] = {0};
    int result = validateCommandData(hardware_model, hardwareModelBytes, sizeof(hardwareModelBytes));

    if (result == 0) {
        result = bufferMTPData(hardwareModelBytes, HARDWARE_MODEL_LENGTH, TesterMTPTypes::HARDWARE_MODEL);
    }

    return result;
}

int FactoryTester::GET_WIFI_MAC(void) {
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH] = {0};
    readEfuse(false, wifiMAC, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);

    return bin_to_hex(this->command_response, this->cmd_length, wifiMAC, sizeof(wifiMAC));
}

int FactoryTester::SET_WIFI_MAC(const char * wifi_mac) {
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH] = {0};
    int result = validateCommandData( wifi_mac, wifiMAC, sizeof(wifiMAC));

    if (result == 0) {
        result = writeEfuse(false, wifiMAC, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);
    }

    strcpy(this->command_response, "test wifimac response");
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

int FactoryTester::IS_READY(void) {
    return 0;
}

void FactoryTester::print_mtp(void) {
    uint8_t user_mtp[32];
    readEfuse(false, user_mtp, sizeof(user_mtp), EFUSE_USER_MTP);
    
    for(int i = 0; i < (int)sizeof(user_mtp); i++) {
        SERIAL.printf("%02X", user_mtp[i]);
    }
    SERIAL.println("");
}

int FactoryTester::TEST_COMMAND(void) {
    SERIAL.println("Raw MTP data");
    print_mtp();

    SERIAL.printlnf("Buffered MTP data");
    for (unsigned i = 0; i < sizeof(userMTPBuffer); i++){
        SERIAL.printf("%02X", userMTPBuffer[i]);
    }
    SERIAL.println("");
    return 0;
}
char * FactoryTester::get_command_response(void) {
    return command_response;
}


#include "application.h"
#include "Particle.h"

#include "FactoryTester.h"
#include "TesterCommandTypes.h"

extern "C" {
#define BOOLEAN AMBD_SDK_BOOLEAN
#include "rtl8721d.h"
#undef BOOLEAN
}

#define SERIAL    Serial1

//////////////////////////// Tron specific stuff ////////////////////////////

// eFuse data lengths
#define SECURE_BOOT_KEY_LENGTH 32
#define FLASH_ENCRYPTION_KEY_LENGTH 16
#define USER_MTP_DATA_LENGTH 32

// MTP data lengths
#define MOBILE_SECRET_LENGTH        15
#define SERIAL_NUMBER_LENGTH        9  // Bottom 6 characters are WIFI MAC, but not stored in MTP to save space
#define HARDWARE_DATA_LENGTH        4
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

#define MOCK_LOGICAL_EFUSE 1 // For testing MTP data, like serial, hw data, 

#define LOGICAL_EFUSE_SIZE 1024
static uint8_t logicalEfuseBuffer[LOGICAL_EFUSE_SIZE];
static uint8_t userMTPBuffer[USER_MTP_DATA_LENGTH];

///////////////////////////////////////////////////////////////////////////

struct TesterCommand {
    const char *name;
    TesterCommandType type;
};

const TesterCommand commands[] = {
        {"SET_FLASH_ENCRYPTION_KEY",     TesterCommandType::SET_FLASH_ENCRYPTION_KEY},
        {"ENABLE_FLASH_ENCRYPTION",      TesterCommandType::ENABLE_FLASH_ENCRYPTION},
        {"SET_SECURE_BOOT_KEY",          TesterCommandType::SET_SECURE_BOOT_KEY},
        {"GET_SECURE_BOOT_KEY",          TesterCommandType::GET_SECURE_BOOT_KEY},
        {"ENABLE_SECURE_BOOT",           TesterCommandType::ENABLE_SECURE_BOOT},
        {"TEST_COMMAND",                 TesterCommandType::TEST_COMMAND},
        // {"INFO",                TesterCommandType::INFO},
        {"READ_SERIAL_NUMBER",  TesterCommandType::READ_SERIAL_NUMBER},
        {"SET_SERIAL_NUMBER",   TesterCommandType::SET_SERIAL_NUMBER},
        {"READ_MOBILE_SECRET",  TesterCommandType::READ_MOBILE_SECRET},
        {"SET_MOBILE_SECRET",   TesterCommandType::SET_MOBILE_SECRET},
        {"READ_HW_DATA",        TesterCommandType::READ_HW_DATA},
        {"SET_HW_DATA",         TesterCommandType::SET_HW_DATA},
        {"READ_HW_MODEL",       TesterCommandType::READ_HW_MODEL},
        {"SET_HW_MODEL",        TesterCommandType::SET_HW_MODEL},
        {"READ_WIFI_MAC",       TesterCommandType::READ_WIFI_MAC},
        {"SET_WIFI_MAC",        TesterCommandType::SET_WIFI_MAC},
        // FORCE_MTP_WRITE?
        // Add commands above this line
        {"",                    TesterCommandType::END}
};

void FactoryTester::setup() {   
    memset(logicalEfuseBuffer, 0x00, sizeof(logicalEfuseBuffer));
    memset(userMTPBuffer, 0x00, sizeof(userMTPBuffer));
    SERIAL.println("Tron Ready!");
}

void FactoryTester::loop() {
    int c = -1;
    if (this->serialAvailable()) {
        c = this->serialRead();
        char s[2];
        s[0] = c;
        s[1] = 0;
        SERIAL.print(s);
    }

    if (c != -1) {
        checkSerial((char) c);
    }
}

int FactoryTester::serialAvailable() {
    int result = SERIAL.available();//user serial1
    return result;
}

int FactoryTester::serialRead() {
    if (SERIAL.available()) {//user serial1
        return SERIAL.read();
    }
    return 0;
}


/***
 *  if the command buffer ends with a semicolon, then lets check for a command, shall we?
 *  check through all our defined commands and see if we're in there!
 */
TesterCommandType FactoryTester::getCommandType() {
    command[cmd_index++] = 0;
    cmd_index = 0;

    int i = 0;
    TesterCommand cmd = commands[i];
    while (cmd.type != TesterCommandType::END) {
        String nameStr = String(String(cmd.name) + ":");
        const char *name = nameStr.c_str();

        //SERIAL.println("comparing " + String(name) + " for " + String(command));

        char *start = strstr(command, name);
        if (start != NULL) {
            //SERIAL.println("found it!");
            return cmd.type;
        }

        i++;
        cmd = commands[i];
    }
    return TesterCommandType::NONE;
}

const char *FactoryTester::getCommandVerb(TesterCommandType someCmd) {

    int i = 0;
    TesterCommand cmd = commands[i];
    while (cmd.type != TesterCommandType::END) {
        if (cmd.type == someCmd) {
            return cmd.name;
        }
        i++;
        cmd = commands[i];
    }

    return NULL;
}

void FactoryTester::checkSerial(char c) {
    if (cmd_index == 0) {
        memset(command, 0, cmd_length);
    }

    if (cmd_index < cmd_length) {
        command[cmd_index++] = c;
    } else {
        cmd_index = 0;
    }

    if (c != ';') {
        return;
    }

    SERIAL.println("checking command: " + (String)command);

    TesterCommandType type = this->getCommandType();
    bool wasValidCommand = (type != TesterCommandType::NONE) && (type != TesterCommandType::END);

    const char *cmd_name;
    if (wasValidCommand) {
        cmd_name = this->getCommandVerb(type);
        SERIAL.println(String(cmd_name) + "_OK:");
    } else {
        SERIAL.println("CMD_INVALID:");
    }
    // set to true if your command is async, and needs to respond with it's own ":CMD_FOO_DONE" response
    bool commandSendsDone = false;


    SERIAL.println(":INFO_END");

    switch (type) {

        case TesterCommandType::SET_FLASH_ENCRYPTION_KEY:
            this->SET_FLASH_ENCRYPTION_KEY();
            break;
        case TesterCommandType::ENABLE_FLASH_ENCRYPTION:
            this->ENABLE_FLASH_ENCRYPTION();
            break;   
        case TesterCommandType::SET_SECURE_BOOT_KEY:
            this->SET_SECURE_BOOT_KEY();
            break;   
        case TesterCommandType::GET_SECURE_BOOT_KEY:
            this->GET_SECURE_BOOT_KEY();
            break;   
        case TesterCommandType::ENABLE_SECURE_BOOT:
            this->ENABLE_SECURE_BOOT();
            break;           
        case TesterCommandType::TEST_COMMAND:
            this->TEST_COMMAND();
            break;
        case TesterCommandType::READ_SERIAL_NUMBER:
            this->READ_SERIAL_NUMBER();
            break;
        case TesterCommandType::SET_SERIAL_NUMBER:
            this->SET_SERIAL_NUMBER();
            break;   
        case TesterCommandType::READ_MOBILE_SECRET:
            this->READ_MOBILE_SECRET();
            break;
        case TesterCommandType::SET_MOBILE_SECRET:
            this->SET_MOBILE_SECRET();
            break;  
        case TesterCommandType::READ_HW_DATA:
            this->READ_HW_DATA();
            break;
        case TesterCommandType::SET_HW_DATA:
            this->SET_HW_DATA();
            break;  
        case TesterCommandType::READ_HW_MODEL:
            this->READ_HW_MODEL();
            break;
        case TesterCommandType::SET_HW_MODEL:
            this->SET_HW_MODEL();
            break;
        case TesterCommandType::READ_WIFI_MAC:
            this->READ_WIFI_MAC();
            break;
        case TesterCommandType::SET_WIFI_MAC:
            this->SET_WIFI_MAC();
            break;
        default:
        case TesterCommandType::NONE:
        case TesterCommandType::END:
            break;
    }

    if (wasValidCommand && !commandSendsDone) {
        cmd_name = this->getCommandVerb(type);
        SERIAL.println(":" + String(cmd_name) + "_DONE");
    }
}

/**
    tokenize 'cmd' into a parts array delimited by ":" or ";"
**/
uint8_t FactoryTester::tokenizeCommand(char *cmd, char *parts[], unsigned max_parts) {
    char *pch;
    unsigned idx = 0;

    for (unsigned i = 0; i < max_parts; i++) {
        parts[i] = NULL;
    }

    pch = strtok(cmd, ":;");
    while (pch != NULL) {
        if (idx < max_parts) {
            parts[idx++] = pch;
        }
        pch = strtok(NULL, ":;");
    }

    return idx;
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

////////////////////////////////////////// Tron Changes Below /////////////////////////////////////////////////////

int FactoryTester::validateData(TesterCommandType commandType, int expected_token_length, uint8_t * output_bytes, int output_bytes_length, bool isHex) {
    int expectedTokenCount = 3; // Expect command verb, then data, then data repeated
    const char *myCommandVerb = this->getCommandVerb(commandType);
    char *parts[expectedTokenCount];
    char *start = strstr(command, myCommandVerb);

    // Verify correct number of tokens
    int count = tokenizeCommand(start, parts, expectedTokenCount);
    if (count != expectedTokenCount) {
        SERIAL.printlnf("Validate Data FAILED: only %d tokens given", count);
        return -1; 
    }

    // Verify length
    uint8_t len = strlen(parts[1]);
    uint8_t len1 = strlen(parts[2]);
    uint32_t token_length_hex = isHex ? expected_token_length * 2 : expected_token_length;
    if (len != token_length_hex || len1 != token_length_hex) {
        SERIAL.printlnf("Validate Data FAILED: token length not correct, got %u and %u, expected: %lu", len, len1, token_length_hex);
        return -1;
    }

    // Compare part[1] and part[2].
    // TODO: Remove duplicate tokens if sent via USB?
    if ( strcmp(parts[1], parts[2]) != 0 ) {
        SERIAL.println("Validate Data FAILED: parts do not match");
        return -1;
    }

    if (isHex) {
        // Validate characters in the serial number.
        for (uint8_t i = 0; i < len; i++) {
            if (!isxdigit(parts[1][i])) {
                SERIAL.printlnf("Validate Hex Data FAILED: invalid char value 0x%02X at index %d", parts[1][i], i);
                return -1;        
            }
        }
        
        // Convert Hex string to binary data.
        if (hex_to_bin(parts[1], (char *)output_bytes, output_bytes_length) == -1) {
            SERIAL.printlnf("Validate Hex Data FAILED: hex_to_bin failed");
            return -1;
        }    
    }
    else {
        // TODO: Validate case here? Make all toUpper()?
        memcpy(output_bytes, parts[1], output_bytes_length);
    }


    return 0;
}


int FactoryTester::writeEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address) {    
    uint32_t eFuseWrite = EFUSE_SUCCESS;
    uint32_t i = 0;

    if (physical) {
        for (i = 0; (i < length) && (eFuseWrite == EFUSE_SUCCESS); i++) {
            //eFuseWrite = EFUSE_PMAP_WRITE8(0, address + i, &data[i], L25EOUTVOLTAGE); // TODO: UNCOMMENT OUT FOR ACTUAL EFUSE WRITES
            // SERIAL.printlnf("eFuse write byte 0x%02X @ 0x%X", data[i], (unsigned int)(address + i));
        }
    }
    else {
        #ifdef MOCK_LOGICAL_EFUSE
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

    // 0 = success, 1 = failure
    return !eFuseWrite;
}

int FactoryTester::readEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address) {    
    uint32_t eFuseRead = EFUSE_SUCCESS;
    uint32_t i = 0;

    if (physical) {
        for (i = 0; (i < length) && (eFuseRead == EFUSE_SUCCESS); i++) {    
            eFuseRead = EFUSE_PMAP_READ8(0, address + i, &data[i], L25EOUTVOLTAGE);
            SERIAL.printlnf("readEfuse 0x%02X @ 0x%X", data[i], (unsigned int)(address + i));
        }
    }
    else {
        #ifdef MOCK_LOGICAL_EFUSE
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

bool FactoryTester::bufferMTPData(uint8_t * data, int data_length, TesterMTPTypes mtp_type) {
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
            return false;
    }
    
    memcpy(userMTPBuffer + offset, data, data_length);

    mtp_data_buffered |= (static_cast<unsigned>(mtp_type));
    SERIAL.printlnf("mtp buffered: %d", mtp_data_buffered); // DEBUG

    // TODO: check if all MTP data set before writing
    writeEfuse(false, userMTPBuffer, sizeof(userMTPBuffer), EFUSE_USER_MTP);
    return false;
}

int FactoryTester::SET_FLASH_ENCRYPTION_KEY(void) {
    uint8_t flashEncryptionKey[FLASH_ENCRYPTION_KEY_LENGTH] = {0};

    // Check to make sure a key is not already written
    readEfuse(true, flashEncryptionKey, sizeof(flashEncryptionKey), EFUSE_FLASH_ENCRYPTION_KEY);
    for (int i = 0; i < FLASH_ENCRYPTION_KEY_LENGTH; i++) {
        if(flashEncryptionKey[i] != 0xFF) {
            SERIAL.printlnf("ERROR: Flash Encryption key already written! Index: %d, value 0x%02X", i, flashEncryptionKey[i]);
            return -1;
        }
    }

    if (validateData(TesterCommandType::SET_FLASH_ENCRYPTION_KEY, FLASH_ENCRYPTION_KEY_LENGTH, flashEncryptionKey, sizeof(flashEncryptionKey), true) != 0) {
        return -1;
    }

    // Write flash encryption key to physical eFuse
    uint32_t eFuseWrite = this->writeEfuse(true, flashEncryptionKey, sizeof(flashEncryptionKey), EFUSE_FLASH_ENCRYPTION_KEY);
    if(eFuseWrite == 0)
    {
        // Write flash encryption key lock bits
        uint8_t flashEncryptionKeyReadWriteForbidden = 0xDB; // Bit 2,5 = Read, Write Forbidden
        eFuseWrite = this->writeEfuse(true, &flashEncryptionKeyReadWriteForbidden, sizeof(flashEncryptionKeyReadWriteForbidden), EFUSE_SEC_CONFIG_ADDR0);
    }

    return eFuseWrite;
}

int FactoryTester::ENABLE_FLASH_ENCRYPTION(void) {
    uint8_t systemConfigByte = 0;
    readEfuse(false, &systemConfigByte, 1, EFUSE_SYSTEM_CONFIG);
    SERIAL.printlnf("System Config: %02X", systemConfigByte);

    systemConfigByte |= (BIT(2));
    SERIAL.printlnf("System Config: %02X", systemConfigByte);

    return this->writeEfuse(false, &systemConfigByte, sizeof(systemConfigByte), EFUSE_SYSTEM_CONFIG);
}

int FactoryTester::SET_SECURE_BOOT_KEY(void) {    
    uint8_t secureBootKeyBytes[SECURE_BOOT_KEY_LENGTH];

    // Check to make sure a key is not already written
    readEfuse(true, secureBootKeyBytes, sizeof(secureBootKeyBytes), EFUSE_SECURE_BOOT_KEY);
    for (int i = 0; i < SECURE_BOOT_KEY_LENGTH; i++) {
        if(secureBootKeyBytes[i] != 0xFF) {
            SERIAL.printlnf("ERROR: Secure boot key already written! Index: %d, value 0x%02X", i, secureBootKeyBytes[i]);
            return -1;
        }
    }

    if (validateData(TesterCommandType::SET_SECURE_BOOT_KEY, SECURE_BOOT_KEY_LENGTH, secureBootKeyBytes, sizeof(secureBootKeyBytes), true) != 0) {
        return -1;
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
    for(int i = 0; i < SECURE_BOOT_KEY_LENGTH; i++) {
        // TODO: Use the boot key somewhere, if desired
        SERIAL.printf("%02X", boot_key[i]);
    }

    return 0;
}

int FactoryTester::ENABLE_SECURE_BOOT(void) {
    uint8_t protectionConfigurationByte = 0;
    readEfuse(true, &protectionConfigurationByte, 1, EFUSE_PROTECTION_CONFIGURATION_HIGH);
    SERIAL.printlnf("Protection Byte: %02X", protectionConfigurationByte);

    protectionConfigurationByte &= ~(BIT(5));
    SERIAL.printlnf("Protection Byte: %02X", protectionConfigurationByte);

    // Write enable bit to physical eFuse
    return this->writeEfuse(true, &protectionConfigurationByte, 1, EFUSE_PROTECTION_CONFIGURATION_HIGH);
}

int FactoryTester::READ_SERIAL_NUMBER(void) {
    // P046WN148000000
    char fullSerialNumber[SERIAL_NUMBER_LENGTH + (EFUSE_WIFI_MAC_LENGTH * 2) + 1] = {0};
    readEfuse(false, (uint8_t *)fullSerialNumber, SERIAL_NUMBER_LENGTH, EFUSE_USER_MTP + EFUSE_SERIAL_NUMBER_OFFSET);

    uint8_t mac[EFUSE_WIFI_MAC_LENGTH+1] = {0};
    readEfuse(false, mac, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);

    for(int i = 0, j = (EFUSE_WIFI_MAC_LENGTH / 2); j < EFUSE_WIFI_MAC_LENGTH; i += 2, j++){
        snprintf(&fullSerialNumber[SERIAL_NUMBER_LENGTH + i], 3, "%02X", mac[j]);
    }

    SERIAL.printlnf("SERIAL_NUMBER: %s", fullSerialNumber);
    return 0;
}

int FactoryTester::SET_SERIAL_NUMBER(void) {
    //SET_SERIAL_NUMBER:P046WN148ABCDEF:P046WN148ABCDEF;

    // TODO: Consider taking 15 chars as input and writing to MAC address as well
    // OR: Only requiring 9 chars as input, and wifi mac explicitly sent elsewhere
    static const int FULL_SERIAL_NUMBER_LENGTH = SERIAL_NUMBER_LENGTH + EFUSE_WIFI_MAC_LENGTH;
    uint8_t serialNumber[FULL_SERIAL_NUMBER_LENGTH] = {0};

    int result = validateData(TesterCommandType::SET_SERIAL_NUMBER, FULL_SERIAL_NUMBER_LENGTH, serialNumber, FULL_SERIAL_NUMBER_LENGTH, false);

    if (result == 0) {
        // Write validated input to eFuse
        bufferMTPData(serialNumber, SERIAL_NUMBER_LENGTH, TesterMTPTypes::SERIAL_NUMBER);
    }

    return result;
}

int FactoryTester::READ_MOBILE_SECRET(void) {
    char mobileSecret[MOBILE_SECRET_LENGTH+1] = {0};
    readEfuse(false, (uint8_t *)mobileSecret, MOBILE_SECRET_LENGTH, EFUSE_USER_MTP + EFUSE_MOBILE_SECRET_OFFSET);
    SERIAL.printlnf("MOBILE_SECRET: %s", mobileSecret);
    return 0;
}

int FactoryTester::SET_MOBILE_SECRET(void) {
    //SET_MOBILE_SECRET:ABC123JKL000FFF:ABC123JKL000FFF;
    char mobileSecret[MOBILE_SECRET_LENGTH+1] = {0};
    int result = validateData(TesterCommandType::SET_MOBILE_SECRET, MOBILE_SECRET_LENGTH, (uint8_t*)mobileSecret, MOBILE_SECRET_LENGTH, false);

    if (result == 0) {
        // Write validated input to eFuse
        bufferMTPData((uint8_t *)mobileSecret, MOBILE_SECRET_LENGTH, TesterMTPTypes::MOBILE_SECRET);
    }

    return result;
}

int FactoryTester::READ_HW_DATA(void) {
    uint8_t hardwareData[HARDWARE_DATA_LENGTH] = {0};
    readEfuse(false, hardwareData, HARDWARE_DATA_LENGTH, EFUSE_USER_MTP + EFUSE_HARDWARE_DATA_OFFSET);

    SERIAL.print("HW_DATA: ");
    for(int i = 0; i < HARDWARE_DATA_LENGTH; i++){
        SERIAL.printf("%02X", hardwareData[i]);
    }
    SERIAL.println("");
    return 0;
}

int FactoryTester::SET_HW_DATA(void) {
    // SET_HW_DATA:FF0ABC0D:FF0ABC0D;
    uint8_t hardwareData[HARDWARE_DATA_LENGTH] = {0};
    int result = validateData(TesterCommandType::SET_HW_DATA, HARDWARE_DATA_LENGTH, hardwareData, HARDWARE_DATA_LENGTH, true);

    if (result == 0) {
        // Write validated input to eFuse
        bufferMTPData(hardwareData, HARDWARE_DATA_LENGTH, TesterMTPTypes::HARDWARE_DATA);
    }

    return result;
}

int FactoryTester::READ_HW_MODEL(void) {
    uint8_t hardwareModel[HARDWARE_MODEL_LENGTH] = {0};
    readEfuse(false, hardwareModel, HARDWARE_MODEL_LENGTH, EFUSE_USER_MTP + EFUSE_HARDWARE_MODEL_OFFSET);

    SERIAL.print("HW_MODEL: ");
    for(int i = 0; i < HARDWARE_MODEL_LENGTH; i++){
        SERIAL.printf("%02X", hardwareModel[i]);
    }
    SERIAL.println("");
    return 0;
}

int FactoryTester::SET_HW_MODEL(void) {
    // SET_HW_MODEL:0DFF0ABC:0DFF0ABC;
    uint8_t hardwareModel[HARDWARE_MODEL_LENGTH] = {0};
    int result = validateData(TesterCommandType::SET_HW_MODEL, HARDWARE_MODEL_LENGTH, hardwareModel, HARDWARE_MODEL_LENGTH, true);

    if (result == 0) {
        // Write validated input to eFuse
        bufferMTPData(hardwareModel, HARDWARE_MODEL_LENGTH, TesterMTPTypes::HARDWARE_MODEL);
    }

    return result;
}

int FactoryTester::READ_WIFI_MAC(void) {
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH] = {0};
    readEfuse(false, wifiMAC, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);

    SERIAL.print("WIFI_MAC: ");
    for(int i = 0; i < EFUSE_WIFI_MAC_LENGTH; i++){
        SERIAL.printf("%02X", wifiMAC[i]);
    }
    SERIAL.println("");
    return 0;
}

int FactoryTester::SET_WIFI_MAC(void) {
    // SET_WIFI_MAC:94944A0DFF0A:94944A0DFF0A;
    uint8_t wifiMAC[EFUSE_WIFI_MAC_LENGTH] = {0};
    int result = validateData(TesterCommandType::SET_WIFI_MAC, EFUSE_WIFI_MAC_LENGTH, wifiMAC, EFUSE_WIFI_MAC_LENGTH, true);

    if (result == 0) {
        // Write validated input to eFuse
        writeEfuse(false, wifiMAC, EFUSE_WIFI_MAC_LENGTH, EFUSE_WIFI_MAC);
    }

    return result;
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
    uint8_t test_data[6];
    readEfuse(false, test_data, sizeof(test_data), EFUSE_WIFI_MAC);
    SERIAL.printlnf("WIFI MAC: %02X%02X%02X%02X%02X%02X", test_data[0], test_data[1], test_data[2], test_data[3], test_data[4], test_data[5]);
    
    readEfuse(true, test_data, 2, EFUSE_PROTECTION_CONFIGURATION_LOW);
    SERIAL.printlnf("Security Bytes: %02X %02X", test_data[0], test_data[1]);

    print_mtp();
    uint8_t wifi_byte = 0x0A;
    writeEfuse(false, &wifi_byte, 1, EFUSE_WIFI_MAC+5);

    readEfuse(false, test_data, sizeof(test_data), EFUSE_WIFI_MAC);
    SERIAL.printlnf("WIFI MAC: %02X%02X%02X%02X%02X%02X", test_data[0], test_data[1], test_data[2], test_data[3], test_data[4], test_data[5]);

    ////////////////////////////////////////////////////////////
    // LETS BLOW OUT USER MTP HELL YEA
    // uint8_t user_mtp[32];
    // memset(user_mtp, 0xFF, sizeof(user_mtp));

    // // //writeEfuse(false, user_mtp, sizeof(user_mtp), EFUSE_USER_MTP);
    // // //readEfuse(false, user_mtp, sizeof(user_mtp), EFUSE_USER_MTP);
    // print_mtp();

    // SERIAL.println("START MTP BURNOUT");

    // bool quit = false;
    // int full_iterations = 0;
    // uint8_t test_pattern_a[] = {0xAA, 0xAA};
    // uint8_t test_pattern_5[] = {0x55, 0x55};

    // while(!quit) {
    //     readEfuse(false, user_mtp, sizeof(user_mtp), EFUSE_USER_MTP);
    //     for(int i = 0; i < 32; i += 2) {
    //         uint8_t mtp_byte_current = user_mtp[i];
    //         uint8_t * mtp_byte_new = 0;

    //         if(mtp_byte_current == 0xFF) {
    //             mtp_byte_new = test_pattern_a;
    //         }
    //         else if(mtp_byte_current == test_pattern_a[0]) {
    //             mtp_byte_new = test_pattern_5;
    //         }
    //         else if(mtp_byte_current == test_pattern_5[0]) {
    //             mtp_byte_new = test_pattern_a;
    //         }

    //         int writeResult = writeEfuse(false, mtp_byte_new, 2, EFUSE_USER_MTP+i);
    //         readEfuse(false, user_mtp, sizeof(user_mtp), EFUSE_USER_MTP);
    //         print_mtp( (16*full_iterations) + (i/2) + 1);

    //         if((mtp_byte_new[0] != user_mtp[i]) || (full_iterations == 3) || (writeResult != 0) ) {
    //             quit = true;
    //             break;
    //         }
    //     }
    //     full_iterations++;
    // }
    ////////////////////////////////////////////////////////////


    return 0;
}

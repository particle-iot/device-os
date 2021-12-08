/**
 ******************************************************************************
 * @file    FactoryTester.h
 * @authors mat / David
 * @date    30 Nov 2015
 ******************************************************************************
 */

#ifndef FACTORYTESTER_H
#define FACTORYTESTER_H

#include "TesterCommandTypes.h"

class FactoryTester {
private:
    static const unsigned cmd_length = 256;
    char command_response[cmd_length];

    static const unsigned usb_buffer_length = 2048;
    char usb_buffer[usb_buffer_length];

    static const String IS_READY;
    static const String SET_PERMANENT_BURN;
    static const String SET_DATA;
    static const String BURN_DATA;
    static const String VALIDATE_BURNED_DATA;
    static const String GET_DEVICE_ID;

    static const String PASS;
    static const String ERRORS;

    static const String EFUSE_READ_FAILURE;
    static const String EFUSE_WRITE_FAILURE;
    static const String DATA_DOES_NOT_MATCH;
    static const String NO_VALID_DATA_BUFFERED;
    static const String RSIP_ENABLE_FAILURE;
    static const String ALREADY_PROVISIONED;
    
    static const String FIELD;
    static const String MESSAGE;
    static const String CODE;
    static const String VERSION;

    int setData(MfgTestKeyType command, const char * commandData);
    bool burnData(Vector<int> &resultCodes, Vector<String> &resultStrings);
    bool validateData(Vector<int> &resultCodes, Vector<String> &resultStrings);
    bool isProvisioned();

    int validateCommandData(const char * data, uint8_t * output_bytes, int output_bytes_length);
    int validateCommandString(const char * data, int expectedLength);

    int writeEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address);
    int readEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address);

public:
    FactoryTester() {
        memset(command_response, 0, sizeof(command_response));
        memset(usb_buffer, 0, sizeof(usb_buffer));
    };

    void setup();

    int processUSBRequest(ctrl_request* request);

    // Provisioning Commands
    int setFlashEncryptionKey(const char * key);
    int setFlashEncryption();
    int setSecureBootKey(const char * key);
    int setSerialNumber(const char * serial_number);
    int setMobileSecret(const char * mobile_secret);
    int setHardwareVersion(const char * hardware_version);
    int setHardwareModel(const char * hardware_model);
    int setWifiMac(const char * wifi_mac);

    int getDeviceId();

    int testCommand();

    char * get_command_response(void);
};

#endif	/* FACTORYTESTER_H */

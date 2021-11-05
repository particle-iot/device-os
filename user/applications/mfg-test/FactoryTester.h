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
#include "TesterMTPTypes.h"

class FactoryTester {
private:
    static const unsigned cmd_length = 256;
    char command_response[cmd_length];

    static const String PASS;
    static const String RESPONSE;
    static const String ERROR;
    static const String ERROR_CODE;

    int executeCommand(TesterCommandType command, const char * commandData);

    int validateCommandData(const char * data, uint8_t * output_bytes, int output_bytes_length);
    int validateCommandString(const char * data, int expectedLength);

    int bufferMTPData(uint8_t * data, int data_length, TesterMTPTypes mtp_type);
    void print_mtp(void);

    int writeEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address);
    int readEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address);

public:
    FactoryTester() {
        memset(command_response, 0, sizeof(command_response));
    };

    void setup();

    int processUSBRequest(ctrl_request* request);

    /* Tester Commands */
    int SET_FLASH_ENCRYPTION_KEY(const char * key);
    int ENABLE_FLASH_ENCRYPTION();
    int SET_SECURE_BOOT_KEY(const char * key);
    int GET_SECURE_BOOT_KEY();
    int ENABLE_SECURE_BOOT();
    int SET_SERIAL_NUMBER(const char * serial_number);
    int GET_SERIAL_NUMBER();
    int SET_MOBILE_SECRET(const char * mobile_secret);
    int GET_MOBILE_SECRET();
    int SET_HW_VERSION(const char * hardware_version);
    int GET_HW_VERSION();
    int SET_HW_MODEL(const char * hardware_model);
    int GET_HW_MODEL();
    int SET_WIFI_MAC(const char * wifi_mac);
    int GET_WIFI_MAC();
    int GET_DEVICE_ID();
    int IS_READY();
    int TEST_COMMAND();
};

#endif	/* FACTORYTESTER_H */

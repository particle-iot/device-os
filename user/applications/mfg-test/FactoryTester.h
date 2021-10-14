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

    unsigned cmd_index;
    static const unsigned cmd_length = 256;
    char command[cmd_length];

    unsigned mtp_data_buffered = 0;

    void checkSerial(char c);

    void printInfo();

    void printItem(const char *name, const char *value);    

    uint8_t tokenizeCommand(char *cmd, char *parts[], unsigned max_parts);

    int validateData(TesterCommandType command, int expected_token_length, uint8_t * output_bytes, int output_bytes_length, bool isHex);

    bool bufferMTPData(uint8_t * data, int data_length, TesterMTPTypes mtp_type);

    void print_mtp(void);

    TesterCommandType getCommandType();
    const char* getCommandVerb(TesterCommandType someCmd);

public:
    FactoryTester() {
        memset(this, 0, sizeof(*this));
    }

    void setup();

    void loop();

    int serialAvailable();

    int serialRead();

    int writeEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address);
    int readEfuse(bool physical, uint8_t * data, uint32_t length, uint32_t address);

    /* Tester Commands */
    int SET_FLASH_ENCRYPTION_KEY();
    int ENABLE_FLASH_ENCRYPTION();
    int SET_SECURE_BOOT_KEY();
    int GET_SECURE_BOOT_KEY();
    int ENABLE_SECURE_BOOT();
    int TEST_COMMAND();
    int READ_SERIAL_NUMBER();
    int SET_SERIAL_NUMBER();
    int READ_MOBILE_SECRET();
    int SET_MOBILE_SECRET();
    int READ_HW_DATA();
    int SET_HW_DATA();
    int READ_HW_MODEL();
    int SET_HW_MODEL();
    int READ_WIFI_MAC();
    int SET_WIFI_MAC();
};

#endif	/* FACTORYTESTER_H */

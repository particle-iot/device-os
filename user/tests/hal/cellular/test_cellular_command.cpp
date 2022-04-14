#include "application.h"
#include "unit-test/unit-test.h"

SYSTEM_MODE(MANUAL);
//STARTUP(cellular_credentials_set("", "", "", NULL));
//STARTUP(Cellular.setActiveSim(EXTERNAL_SIM));

#define assertBlock(arg1,op,op_name,arg2) if (!Test::assertion<typeof(arg2)>(F(__FILE__),__LINE__,F(#arg1),(arg1),F(op_name),op,F(#arg2),(arg2))) while(1);
#define assertEqualBlock(arg1,arg2)       assertBlock(arg1,isEqual,"==",arg2)
#define assertNotEqualBlock(arg1,arg2)    assertBlock(arg1,isNotEqual,"!=",arg2)

Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, { 
                                {"ncp",     LOG_LEVEL_ALL},
                                {"app",     LOG_LEVEL_ALL},
                                {"lwip",    LOG_LEVEL_NONE},
                                {"ot",      LOG_LEVEL_NONE},
                                {"sys",     LOG_LEVEL_NONE},
                                {"system",  LOG_LEVEL_NONE}
                            });

static int at_resp_info_print(int type, const char* buf, int len, int* lines) {
    Serial.printlnf("┌-----------------------------------------------------┐");
    switch (type) {
        case NOT_FOUND:         Serial.printlnf("Type: UNKNOWN");           break;
        case WAIT:              Serial.printlnf("Type: WAIT");              break;
        case RESP_OK:           Serial.printlnf("Type: RESP_OK");           break;
        case RESP_ERROR:        Serial.printlnf("Type: RESP_ERROR");        break;
        case RESP_PROMPT:       Serial.printlnf("Type: RESP_PROMPT");       break;
        case RESP_ABORTED:      Serial.printlnf("Type: RESP_ABORTED");      break;

        case TYPE_OK:           Serial.printlnf("Type: TYPE_OK");           break;
        case TYPE_ERROR:        Serial.printlnf("Type: TYPE_ERROR");        break;
        case TYPE_CONNECT:      Serial.printlnf("Type: TYPE_CONNECT");      break;
        case TYPE_NOCARRIER:    Serial.printlnf("Type: TYPE_NOCARRIER");    break;
        case TYPE_NODIALTONE:   Serial.printlnf("Type: TYPE_NODIALTONE");   break;
        case TYPE_BUSY:         Serial.printlnf("Type: TYPE_BUSY");         break;
        case TYPE_NOANSWER:     Serial.printlnf("Type: TYPE_NOANSWER");     break;
        case TYPE_PROMPT:       Serial.printlnf("Type: TYPE_PROMPT");       break;
        case TYPE_PLUS:         Serial.printlnf("Type: TYPE_PLUS");         break;
        case TYPE_TEXT:         Serial.printlnf("Type: TYPE_TEXT");         break;
        case TYPE_ABORTED:      Serial.printlnf("Type: TYPE_ABORTED");      break;
        case TYPE_DBLNEWLINE:   Serial.printlnf("Type: TYPE_DBLNEWLINE");   break;
        default:
            Serial.printlnf("Type: SHOULD ENTER THIS CASE!"); break;
            break;
    }
    Serial.printf("len: %d, line: %d, hex data: ", len, *lines);
    for (int i = 0; i < len; i++) {
        Serial.printf("%02X ", buf[i]);
    }
    Serial.printlnf("");
    Serial.printf("text: ");
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\r') {
            Serial.printf("\\r");
        } else if (buf[i] == '\n') {
            Serial.printf("\\n");

        } else {
            Serial.printf("%c", buf[i]);
        }
    }
    Serial.printlnf("");

    Serial.printlnf("└-----------------------------------------------------┘");

    return WAIT;
}

template<typename... Targs>
static int send_cellular_command(int (*cb)(int type, const char* buf, int len, int* lines),
                                 int* lines, system_tick_t timeout_ms, const char* format, Targs... Fargs) 
{
    return Cellular.command(cb, lines, timeout_ms, format, Fargs...);
}

test(AT_CMD_01_set_and_read_command) {
    int lines;

    Cellular.on();

    // CMEE=1: use error code mode
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        assertEqualBlock(type, (int)TYPE_OK);
        return WAIT;
    }, &lines, 5000, "AT+CMEE=2\r\n"), (int)RESP_OK);

    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* curr_line) -> int {
        at_resp_info_print(type, buf, len, curr_line);
        if (*curr_line == 0) {
            assertEqualBlock(type, (int)TYPE_PLUS);
            char * dst = strstr(buf, "+CMEE: ");
            assertEqualBlock(dst!=NULL, true);
            assertEqualBlock(dst[strlen("+CMEE: ")]=='2', true);
        } else {
            assertEqualBlock(type, (int)TYPE_OK);
        }
        (*curr_line)++;
        return WAIT;
    }, &lines, 5000, "AT+CMEE?\r\n"), (int)RESP_OK);
}

test(AT_CMD_02_plus) {
    int lines;

    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        if (*lines == 0) {
            assertEqualBlock(type, (int)TYPE_PLUS);
            assertEqualBlock(strstr(buf, "+CCID:")!=NULL, true);
        } else {
            assertEqualBlock(type, (int)TYPE_OK);
        }
        (*lines)++;
        return WAIT;
    }, &lines, 5000, "AT+CCID?\r\n"), (int)RESP_OK);

    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        if (*lines == 0) {
            assertEqualBlock(type, (int)TYPE_PLUS);
            assertEqualBlock(strstr(buf, "+CPIN:")!=NULL, true);
        } else {
            assertEqualBlock(type, (int)TYPE_OK);
        }
        (*lines)++;
        return WAIT;
    }, &lines, 5000, "AT+CPIN?\r\n"), (int)RESP_OK);
}

test(AT_CMD_03_unknown) {
    int lines;

    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        if (*lines == 0) {
            // Get SN number, e.g. 352753090314178
            assertEqualBlock(type, (int)TYPE_UNKNOWN);
        } else {
            assertEqualBlock(type, (int)TYPE_OK);
        }
        (*lines)++;
        return WAIT;
    }, &lines, 5000, "AT+GSN\r\n"), (int)RESP_OK);

    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        if (*lines == 0) {
            // Get ublox firmware version, e.g. 00.0.00.00.00 [Sep 07 2018 19:37:56]
            assertEqualBlock(type, (int)TYPE_UNKNOWN);
        } else {
            assertEqualBlock(type, (int)TYPE_OK);
        }
        (*lines)++;
        return WAIT;
    }, &lines, 5000, "AT+CGMR\r\n"), (int)RESP_OK);
}

test(AT_CMD_04_error_test) {
    int lines;

    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        if (*lines == 0) {
            // Get SN number, e.g. 352753090314178
            assertEqualBlock(type, (int)TYPE_ERROR);
        } else {
            assertEqualBlock(type, (int)TYPE_OK);
        }
        (*lines)++;
        return WAIT;
    }, &lines, 5000, "AT+USOCL=7\r\n"), (int)RESP_ERROR);

    // +CMS ERROR: invalid memory index
    lines = 0;
    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        assertEqualBlock(type, (int)TYPE_ERROR);
        return WAIT;
    }, &lines, 5000, "AT+CMSS=1000\r\n"), (int)RESP_ERROR);
}

test(AT_CMD_05_no_carrier) {
    int lines = 0;

    assertEqualBlock(send_cellular_command([](int type, const char* buf, int len, int* lines) -> int {
        at_resp_info_print(type, buf, len, lines);
        assertEqualBlock(type, (int)TYPE_NOCARRIER);
        return WAIT;
    }, &lines, 5000, "atd1234567890\r\n"), (int)RESP_ERROR);
}


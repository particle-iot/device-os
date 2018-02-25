// git clone the latest `develop` branch of the `firmware` repo
// COMPILE from firmware/modules $
// make clean all PLATFORM_ID=10 APPDIR=~/code/fw-apps/electron-troubleshooting COMPILE_LTO=n DEBUG_BUILD=y -s program-dfu
//
// NOTE: APPDIR=~/code/fw-apps/electron-troubleshooting can be APP=electron-troubleshooting if that's easier

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "cellular_hal.h"

USBSerial1LogHandler logHandler(LOG_LEVEL_ALL);

// +UUPING: <retry_num>,<p_size>,<remote_hostname>,<remote_ip>,<ttl>,<rtt>
// +UUPING: 1,32,"www.l-google.com","72.14.234.104",55,768
// +UUPINGER: <error_code> +UUPINGER: 12
struct MDM_PING {
    uint16_t size;
    int retry_num;
    int p_size;
    char remote_hostname[64];
    char remote_ip[16];
    int ttl;
    int rtt;
    int error_code;
    const char* error_msg;

    MDM_PING() {
        memset(this, 0, sizeof(*this));
        error_code = -1; // default no response
        error_msg = getPingError(error_code);
        size = sizeof(*this);
    }

    const char* getPingError(int err) {
        switch(err) {
        case 0: return "Success (no error)";
        case 1: // "Internal error (ping level)"
        case 2: //      |
        case 3: //      |
        case 4: //      |
        case 5: //      |
        case 6: return "Internal error (ping level)";
        case 7: return "Empty remote host";
        case 8: return "Cannot resolve host";
        case 9: return "Unsupported IP version (RFU)";
        case 10: return "Invalid IPv4 address";
        case 11: return "Invalid IPv6 address (RFU)";
        case 12: return "Remote host too long";
        case 13: return "Invalid payload size";
        case 14: return "Invalid TTL value";
        case 15: return "Invalid timeout value";
        case 16: return "Invalid retries number";
        case 17: return "PSD or CSD connection not established";
        case 100: // "Internal error (ICMP level)"
        case 101: //    |
        case 102: //    |
        case 103: //    |
        case 104: //    |
        case 105: return "Internal error (ICMP level)";
        case 106: return "Error creating socket for ICMP";
        case 107: return "Error settings socket options for ICMP";
        case 108: return "Cannot end ICMP packet";
        case 109: return "Read for ICMP packet failed";
        case 110: return "Received unexpected ICMP packet";
        case 111: // "Internal error (socket level)"
        case 112: //    |
        case 113: //    |
        case 114: //    |
        case 115: return "Internal error (socket level)";
        case -1: return "No response";
        default: return "Error unknown";
        }
    }
};

typedef struct {
    int cid = 0;
    int tx_sess_bytes = 0;
    int rx_sess_bytes = 0;
    int tx_total_bytes = 0;
    int rx_total_bytes = 0;
} MDM_DATA_USAGE;

MDM_DATA_USAGE _data_usage;

static inline int _cbUGCNTRD(int type, const char* buf, int len, MDM_DATA_USAGE* data)
{
    if ((type == TYPE_PLUS) && data) {
        int a,b,c,d,e;
        // +UGCNTRD: 31,2704,1819,2724,1839\r\n
        // +UGCNTRD: <cid>,<tx_sess_bytes>,<rx_sess_bytes>,<tx_total_bytes>,<rx_total_bytes>
        if (sscanf(buf, "\r\n+UGCNTRD: %d,%d,%d,%d,%d\r\n", &a,&b,&c,&d,&e) == 5) {
            data->cid = a;
            data->tx_sess_bytes = b;
            data->rx_sess_bytes = c;
            data->tx_total_bytes = d;
            data->rx_total_bytes = e;
        }
    }
    return WAIT;
}

// Prototypes
void checkPingResponse(int ret, MDM_PING& pingResults);
void checkForURCResponse(int ret, int& data, uint32_t timeout_ms);
void processATcommand(void);
void processSerial1(void);
void waitForEnter(void);
void showHelp(void);
void enterCredentials(void);
String readInputString(void);

Serial1LogHandler logHandler(LOG_LEVEL_ALL); // for full debugging

//SYSTEM_MODE(MANUAL);
SYSTEM_MODE(SEMI_AUTOMATIC);

static inline int _cbCCID(int type, const char* buf, int len, char* ccid)
{
    if ((type == TYPE_PLUS) && ccid) {
        if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", ccid) == 1)
            /*nothing*/;
    }
    return WAIT;
}

static inline int _cbPING(int type, const char* buf, int len, MDM_PING* ping)
{
    // +UUPING: <retry_num>,<p_size>,<remote_hostname>,<remote_ip>,<ttl>,<rtt>
    // +UUPING: 1,32,"www.l-google.com","72.14.234.104",55,768
    // +UUPINGER: <error_code>
    // +UUPINGER: 17
    if ((type == TYPE_PLUS) && ping) {
        // char line[256];
        // strncpy(line, buf, len);
        // line[len] = '\0';
        // Serial.printf("LINE: %s",line);
        int count = 0;
        if ( (count = sscanf(buf, "\r\n+UUPING: %d,%d,\"%[^\"]\",\"%[^\"]\",%d,%d\r\n",
            &ping->retry_num,
            &ping->p_size,
            ping->remote_hostname,
            ping->remote_ip,
            &ping->ttl,
            &ping->rtt) ) > 0) {
            ping->error_code = 0; // Success code on successful ping response
            ping->error_msg = "Success";
        }
        else if (sscanf(buf, "\r\n+UUPINGER: %d\r\n", &ping->error_code) == 1) {
            ping->error_msg = ping->getPingError(ping->error_code); // Lookup error message based on error_code
        }
    }
    return WAIT;
}

static inline int _cbSTRING(int type, const char* buf, int len, int* data)
{
    if (type == TYPE_UNKNOWN) {
        char line[1024+64];
        strncpy(line, buf, len);
        line[len] = '\0';
        String line2 = String(line);
        line2.replace("\r\n\r\n", "\r\n");
        Serial.printlnf("%s", line2.c_str());
    }
    else if (type == TYPE_PLUS) {
        char line[1024+64];
        if (sscanf(buf, "\r\n%[^\r]\r\n", line) == 1) {
            Serial.printlnf("%s", line);
        }
    }
    return WAIT;
}

// 3rd Party SIM credentials
//STARTUP(cellular_credentials_set("broadband", "", "", NULL)); // AT&T

bool debugging_loopback = false;

void process_debug_loopback()
{
    if (debugging_loopback) processSerial1();
    else Serial1.flush();
}

Timer timer(1, process_debug_loopback);

void setup()
{
    Serial.begin(9600);
    waitForEnter();

    Serial.println("Running in SEMI_AUTOMATIC mode, cellular modem is OFF at boot,"
                    "\r\nnot connected to the cellular network or cloud."
                    "\r\nSystem RGB LED will typically stay White, until connecting to cloud."
                    "\r\nConnect a serial to USB adapter on the TX pin to see debugging output."
                    "\r\nSuggested workflow: o, d, r, p, P, p, C, s, s, z");

    showHelp();
    timer.start();
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    static int count = 1;
    if (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == 'C') {
            Serial.print("Connecting to the cloud: ");
            Particle.connect();
            if (waitFor(Particle.connected, 30000)) {
                Serial.println("OK!");
            }
            else {
                Serial.println("ERROR!\r\nTimed out after 30 seconds!");
            }
        }
        else if (c == 'c') {
            Serial.print("Disconnecting from the cloud: ");
            Particle.disconnect();
            if (waitFor(Particle.disconnected, 30000)) {
                Serial.println("OK!");
            }
            else {
                Serial.println("ERROR!\r\nTimed out after 30 seconds!");
            }
        }
        else if (c == 'o') {
            Serial.print("Turning on the modem: ");
            cellular_result_t result = -1;
            result = cellular_on(NULL);
            if (result) Serial.println("ERROR!\r\nFailure powering on the modem!");
            else Serial.println("OK!");
        }
        /*
        else if (c == 'o') {
            Serial.print("Turning off the modem: ");
            cellular_result_t result = -1;
            result = cellular_off(NULL);
            if (result) Serial.println("ERROR!\r\nFailure powering off the modem!");
            else Serial.println("OK!");
        }
        */
        else if (c == 's') {
            Serial.printf("Publishing the \"b\" event name with count \"%d\" data. Look in your dashboard.particle.io logs: ", count);
            if (!Particle.publish("b",String(count++))) {
                Serial.println("ERROR!\r\nCould not publish. Is the Electron breathing CYAN? Type [C] to connect to the cloud.");
            }
            else {
                Serial.println("OK!");
            }
        }
        else if (c == 'i') {
            Serial.print("Get the SIM card ID (ICCID): ");
            char ccid[32] = "";
            if ((RESP_OK == Cellular.command(_cbCCID, ccid, 5000, "AT+CCID\r\n"))
                && (strcmp(ccid,"") != 0))
            {
                Serial.printlnf("%s\r\n", ccid);
            }
            else {
                Serial.println("ERROR!\r\nCould not retreive ICCID. Is your SIM card inserted?");
            }
        }
        else if (c == 'r') {
            //CellularSignal s = Cellular.RSSI();
            // Read it directly because we do not have network_ready() true when joining the data connection in this app
            Serial.print("Getting signal strength: ");
            CellularSignalHal s;
            cellular_result_t result = cellular_signal(s, NULL);
            int bars = 0;
            if (s.rssi < 0) {
                if (s.rssi >= -57) bars = 5;
                else if (s.rssi > -68) bars = 4;
                else if (s.rssi > -80) bars = 3;
                else if (s.rssi > -92) bars = 2;
                else if (s.rssi > -104) bars = 1;
            }
            if (result != 0) {
                Serial.println("ERROR!\r\nFailure reading RSSI! Did you turn the modem on? Press [o]");
            }
            else {
                Serial.printlnf("RSSI: %ddBm, QUAL: %ddB, BARS: %d", s.rssi, s.qual, bars);
            }
        }
        else if (c == 'd') {
            cellular_result_t result = -1;
            Serial.print("Connecting to the cellular network: ");
            result = cellular_init(NULL);
            if (result) {
                Serial.println("ERROR!\r\nFailed modem initialization! Did you turn the modem on? Particle SIM installed?");
            }
            else {
                result = cellular_register(NULL);
                if (result) {
                    Serial.println("ERROR!\r\nFailed to register to cellular network! Do you have the Particle SIM installed?"
                                         "\r\nTry removing power completely, and re-applying.");
                }
                else {
                    CellularCredentials* savedCreds;
                    savedCreds = cellular_credentials_get(NULL);
                    result = cellular_gprs_attach(savedCreds, NULL);
                    if (result) {
                        Serial.println("ERROR!\r\nFailed to GPRS attach! Did you activate your Particle SIM?");
                    }
                    else {
                        Serial.println("OK!");
                    }
                }
            }
        }
        else if (c == 'D') {
            cellular_result_t result = -1;
            enterCredentials(); // first set the credentials
            Serial.print("Connecting to the cellular network with 3rd-Party APN: ");
            result = cellular_init(NULL);
            if (result) {
                Serial.println("ERROR!\r\nFailed modem initialization! Did you turn the modem on? Particle SIM installed?");
            }
            else {
                result = cellular_register(NULL);
                if (result) {
                    Serial.println("ERROR!\r\nFailed to register to cellular network! Do you have the Particle SIM installed?"
                                         "\r\nTry removing power completely, and re-applying.");
                }
                else {
                    CellularCredentials* savedCreds;
                    //cellular_credentials_set("broadband", "", "", NULL);
                    savedCreds = cellular_credentials_get(NULL);
                    result = cellular_gprs_attach(savedCreds, NULL);
                    if (result) {
                        Serial.println("ERROR!\r\nFailed to GPRS attach! Did you activate your Particle SIM?");
                    }
                    else {
                        Serial.println("OK!");
                    }
                }
            }
        }
        /*
        else if (c == 'd') {
            Serial.print("Disconnecting from the cellular network: ");
            cellular_result_t result = -1;
            result = cellular_gprs_detach(NULL);
            if (result) {
                Serial.println("ERROR!\r\nFailed to GPRS detach! Was the modem on and connected to the cellular network?");
            }
            else {
                Serial.println("OK!");
            }
        }
        else if (c == 'x') {
            cellular_result_t result = -1;
            CellularCredentials* savedCreds;
            savedCreds = cellular_credentials_get(NULL);
            result = cellular_gprs_attach(savedCreds, NULL);
            if (result) {
                Serial.println("ERROR!\r\nFailed to GPRS attach! Did you activate your Particle SIM?");
            }
            else {
                Serial.println("OK!");
            }
        }
        */
        else if (c == 'a') {
            processATcommand();
        }
        else if (c == 'N') {
            int data;
            Serial.println("Scanning for available networks, could take up to 3 minutes...\r\n");
            int ret;
            int final_result = false;
            Serial.println("[ AT+COPS=? ]");
            ret = Cellular.command(_cbSTRING, &data, 3*60000, "AT+COPS=?\r\n");
            if (ret == RESP_OK) {
                Serial.println("[ AT+COPS=5 ]");
                ret = Cellular.command(_cbSTRING, &data, 3*60000, "AT+COPS=5\r\n");
                if (ret == RESP_OK) {
                    final_result = true; // AT+COPN doesn't return OK
                }
            }
            if (final_result) {
                Serial.println("\r\nScan complete!");
            }
            else {
                Serial.println("\r\nScan incomplete! Power cycle the modem and try again.");
            }
        }
        else if (c == 'n') {
            int data;
            Serial.println("Scanning for available networks, could take up to 3 minutes...\r\n");
            int ret;
            int final_result = false;
            Serial.println("\r\n[ AT+COPN ]");
            ret = Cellular.command(_cbSTRING, &data, 3*60000, "AT+COPN\r\n");
            if (final_result) {
                Serial.println("\r\nScan complete!");
            }
            else {
                Serial.println("\r\nScan incomplete! Power cycle the modem and try again.");
            }
        }
        else if (c == 'P') {
            MDM_PING pingResults;
            //pingResults.error_code = 1000;
            Serial.print("Pinging www.bing.com (Consumes 64 bytes per ping): ");
            int ret;
            ret = Cellular.command(_cbPING, &pingResults, 5000, "AT+UPING=\"www.bing.com\",1,4,5000,32\r\n");
            checkPingResponse(ret, pingResults);
        }
        else if (c == 'p') {
            MDM_PING pingResults;
            // pingResults.error_code = 1000;
            Serial.print("Pinging Google DNS 8.8.8.8 (Consumes 240 bytes per ping): ");
            int ret;
            ret = Cellular.command(_cbPING, &pingResults, 5000, "AT+UPING=\"8.8.8.8\",1,4,5000,32\r\n");
            checkPingResponse(ret, pingResults);
        }
        else if (c == 'z') {
            Serial.print("\r\nBattery will still charge in this mode. Press RESET to wake the system back up."
                         "\r\nTurning off the cellular modem, and putting processor in deep sleep: ");
            cellular_result_t result = -1;
            result = cellular_off(NULL);
            if (result) Serial.println("ERROR!\r\nFailure powering off the modem!");
            else Serial.println("OK!");
            delay(1000);
            System.sleep(SLEEP_MODE_DEEP);
        }
        else if (c == 'U') {
            Serial.println("Read counters of sent or received PSD data!");
            if (RESP_OK == Cellular.command(_cbUGCNTRD, &_data_usage, 10000, "AT+UGCNTRD\r\n")) {
                Serial.printlnf("CID: %d SESSION TX: %d RX: %d TOTAL TX: %d RX: %d\r\n",
                    _data_usage.cid,
                    _data_usage.tx_sess_bytes, _data_usage.rx_sess_bytes,
                    _data_usage.tx_total_bytes, _data_usage.rx_total_bytes);
            }
        }
        else if (c == 'u') {
            Serial.println("Set/reset counter of sent or received PSD data!");
            Cellular.command("AT+UGCNTSET=?\r\n"); // test available limits, see log output
            Cellular.command("AT+UGCNTSET=%d,0,0\r\n", _data_usage.cid);
        }
        else if (c == 'K') {
            Serial.println("Debugging loopback enabled!"
                "\r\nBe sure to jumper TX to RX to see Serial1"
                "\r\ndebugging output mixed into Serial USB output.");
            debugging_loopback = true;
        }
        else if (c == 'k') {
            Serial.println("Debugging loopback disabled!");
            debugging_loopback = false;
        }
        else if (c == 'h') {
            showHelp();
        }
        else {
            Serial.println("Bad command!");
        }
        while (Serial.available()) Serial.read(); // Flush the input buffer
    }

    //if (Particle.connected()) Particle.process(); // Required for MANUAL mode
}

void processSerial1() {
    if (Serial1.available() > 0) {
        while (Serial1.available()) {
            char c = Serial1.read();
            Serial.write(c);
        }
    }
}

void checkPingResponse(int ret, MDM_PING& pingResults) {
    if (WAIT == ret) {
        Serial.println("ERROR!\r\nNo response from modem, did you turn it on? Press [o]");
        return;
    }

    if ((RESP_OK == ret) && (pingResults.error_code == 0))
    {
        Serial.printlnf("OK!\r\n"/*RN:%d,*/"%d bytes from %s IP:%s TTL:%d RTT:%d ms",
            //pingResults.retry_num, // we're only using 1 ping in this app
            pingResults.p_size,
            pingResults.remote_hostname,
            pingResults.remote_ip,
            pingResults.ttl,
            pingResults.rtt);
    }
    else {
        if (RESP_OK == ret) {
            // Send AT to check for URCs that were slow
            int tries = 0;
            do {
                if (tries++ > 20) break;
                delay(500);
                ret = Cellular.command(_cbPING, &pingResults, 5000, "AT\r\n");
            }
            while ((RESP_OK != ret) || (pingResults.error_code != 0));
            if ((RESP_OK == ret) && (pingResults.error_code == 0))
            {
                Serial.printlnf("OK!\r\n"/*RN:%d,*/"%d bytes from %s IP:%s TTL:%d RTT:%d ms",
                    //pingResults.retry_num, // we're only using 1 ping in this app
                    pingResults.p_size,
                    pingResults.remote_hostname,
                    pingResults.remote_ip,
                    pingResults.ttl,
                    pingResults.rtt);
            }
            else {
                Serial.printlnf("ERROR!\r\nERROR %d : %s", pingResults.error_code, pingResults.error_msg);
            }
        }
        if (RESP_ERROR == ret) {
            Serial.printlnf("ERROR!\r\nSend ping requests slower, once per 2 seconds or more.");
            if (RESP_OK == Cellular.command("AT+CEER\r\n")) {
                // do nothing, check logs.
            }
        }
    }
}

/*
void checkForURCResponse(int ret, int& data, uint32_t timeout_ms) {
    if (WAIT == ret) {
        Serial.println("ERROR!\r\nNo response from modem, did you turn it on? Press [o]");
        return;
    }

    if (RESP_OK == ret) {
        // Send AT to check for URCs that were slow
        uint32_t tries = 0;
        do {
            if (tries++ > (timeout_ms/500)) break;
            delay(500);
            ret = Cellular.command(_cbSTRING, &data, 5000, "AT\r\n");
        }
        while (RESP_OK != ret);
    }

    if (RESP_ERROR == ret) {
        Serial.printlnf("ERROR!\r\nWhile checking for URC Response.");
        if (RESP_OK == Cellular.command("AT+CEER\r\n")) {
            // do nothing, check logs.
        }
    }
}
*/

void processATcommand() {
    static bool once = false;
    if (!once) {
        once = true;
        Serial.println("Please be careful with AT commands. BACKSPACE and"
                   "\r\nESCAPE keys can be used to cancel and modify commands.");
    }
    Serial.print("Enter an AT command: ");
    int done = false;
    String cmd = "";
    while (!done) {
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\r') {
                Serial.println();
                if(cmd != "") {
                    if (RESP_OK == Cellular.command("%s\r\n", cmd.c_str())) {
                        Serial.printlnf("%s :command request sent!", cmd.c_str());
                    }
                    else {
                        Serial.printlnf("%s :command request was not recognized by the modem!", cmd.c_str());
                    }
                }
                cmd = "";
                done = true;
            }
            else if (c == 27) { // ESC
                if (cmd.length() > 0) {
                    for (uint32_t x=0; x<cmd.length(); x++) {
                        Serial.print('\b');
                    }
                    for (uint32_t x=0; x<cmd.length(); x++) {
                        Serial.print(' ');
                    }
                    for (uint32_t x=0; x<cmd.length(); x++) {
                        Serial.print('\b');
                    }
                }
                Serial.println("command aborted!");
                cmd = "";
                done = true;
            }
            else if ((c == 8 || c == 127)) { // BACKSPACE
                if (cmd.length() > 0) {
                    cmd.remove(cmd.length()-1, 1);
                    Serial.print("\b \b");
                }
            }
            else {
                cmd += c;
                Serial.print(c);
            }
        }
        Particle.process();
    }
}

String readInputString() {
    bool done = false;
    String cmd = "";
    while (!done) {
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\r') {
                Serial.println();
                done = true;
            }
            else if (c == 27) { // ESC
                if (cmd.length() > 0) {
                    for (uint32_t x=0; x<cmd.length(); x++) {
                        Serial.print('\b');
                    }
                    for (uint32_t x=0; x<cmd.length(); x++) {
                        Serial.print(' ');
                    }
                    for (uint32_t x=0; x<cmd.length(); x++) {
                        Serial.print('\b');
                    }
                }
                cmd = "";
                done = true;
            }
            else if ((c == 8 || c == 127)) { // BACKSPACE
                if (cmd.length() > 0) {
                    cmd.remove(cmd.length()-1, 1);
                    Serial.print("\b \b");
                }
            }
            else {
                cmd += c;
                Serial.print(c);
            }
        }
        Particle.process();
    } // while (!done)

    return cmd;
}

void enterCredentials() {
    static bool once = false;
    if (!once) {
        once = true;
        Serial.println("You will be prompted to enter your APN, USERNAME and PASSWORD."
                   "\r\nMost 3rd-Party SIMs don't require USERNAME and PASSWORD so just"
                   "\r\npress ENTER on those to set them as empty values."
                   "\r\nPressing ENTER on all 3 will restore Particle's APN.\r\n");
    }

    String APN, USERNAME, PASSWORD;
    Serial.print("APN: ");
    APN = readInputString();
    Serial.print("USERNAME: ");
    USERNAME = readInputString();
    Serial.print("PASSWORD: ");
    PASSWORD = readInputString();

    cellular_result_t result = cellular_credentials_set(APN.c_str(), USERNAME.c_str(), PASSWORD.c_str(), NULL);
    if (result) {
        Serial.println("\r\nERROR! Credentials Not Set!");
    } else {
        Serial.println("\r\nCredentials Set!");
    }
}

void waitForEnter() {
    while (!Serial.available()) {
        Particle.process();
        Serial.print("Press ENTER\r"); // prints in place over and over
        delay(1000);
    }
    while (Serial.available()) Serial.read(); // Flush the input buffer
}

void showHelp() {
    Serial.println("\r\nPress a key to run a command:"
                   "\r\n[o] turn the cellular modem ON"
                   "\r\n[d|D] cellular data connection (Particle SIM|3rd Party SIM)"
                   "\r\n[n] scan the cellular network for all operators"
                   "\r\n[N] scan the cellular network for operators we can connect to"
                   "\r\n[r] get the RSSI and QUAL values"
                   "\r\n[P|p] send ping to www.bing.com|Google DNS 8.8.8.8"
                   "\r\n[C|c] to connect|disconnect the cloud"
                   "\r\n[s] publish a \"b\" event name and \"count++\" data"
                   "\r\n[i] read the SIM ICCID"
                   "\r\n[z] turn off the cellular modem, and go to deep sleep"
                   "\r\n[K|k] enable|disable Serial1 debugging loopback"
                   "\r\n[h] show this help menu\r\n");
}

/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef HAL_CELLULAR_EXCLUDE

/* Includes -----------------------------------------------------------------*/
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mdm_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "mdmapn_hal.h"
#include "stm32f2xx.h"
#include "dns_client.h"
#include "service_debug.h"
#include "bytes2hexbuf.h"
#include "hex_to_bytes.h"
#include "concurrent_hal.h"
#include <mutex>
#include "net_hal.h"
#include <limits>

std::recursive_mutex mdm_mutex;

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/

#define PROFILE         "0"   //!< this is the psd profile used
#define MAX_SIZE        1024  //!< max expected messages (used with RX)
#define USO_MAX_WRITE   1024  //!< maximum number of bytes to write to socket (used with TX)
#define MDM_SOCKET_SEND_RETRIES (1) //!< maximum number of times to retry a socket send in case of error

// ID of the PDP context used to configure the default EPS bearer when registering in an LTE network
// Note: There are no PDP contexts in LTE, SARA-R4 uses this naming for the sake of simplicity
#define PDP_CONTEXT 1

// Enable hex mode for socket commands. SARA-R410M-01B has a bug which causes truncation of
// data read from a socket if the data contains a null byte
// #define SOCKET_HEX_MODE

// Timeouts for various AT commands based on R4 & U2/G3 AT command manual as of Jan 2019
#define AT_TIMEOUT      (  1 * 1000)
#define USOCL_TIMEOUT   ( 10 * 1000) /* 120s for R4 (TCP only, optimizing for UDP with 10s), 1s for U2/G3 */
#define USOCO_TIMEOUT   ( 10 * 1000) /* 120s for R4 (TCP only, optimizing for UDP with 10s), 1s for U2/G3 */
#define USOCR_TIMEOUT   (  1 * 1000)
#define USOCTL_TIMEOUT  (  1 * 1000)
#define USOWR_TIMEOUT   ( 10 * 1000) /* 120s for R4 (optimizing for non-blocking behavior with 10s), 1s for U2/G3 */
#define USORD_TIMEOUT   ( 10 * 1000) /* FIXME: 1s for R4/U2/G3, but longer timeouts required in deployments */
#define USOST_TIMEOUT   ( 40 * 1000) /*  10s for R4, 1s for U2/G3, changed to 40s due to R410 firmware
                                         L0.0.00.00.05.08,A.02.04 background DNS lookup and other factors */
#define USORF_TIMEOUT   ( 10 * 1000) /* FIXME: 1s for R4/U2/G3, but longer timeouts required in deployments */
#define CEER_TIMEOUT    (  1 * 1000)
#define CEREG_TIMEOUT   (  1 * 1000)
#define CGDCONT_TIMEOUT (  1 * 1000)
#define CGREG_TIMEOUT   (  1 * 1000)
#define COPS_TIMEOUT    (180 * 1000)
#define CPWROFF_TIMEOUT ( 40 * 1000)
#define CREG_TIMEOUT    (  1 * 1000)
#define CSQ_TIMEOUT     (  1 * 1000)
#define URAT_TIMEOUT    (  1 * 1000)
#define UPSD_TIMEOUT    (  1 * 1000)
#define UPSDA_TIMEOUT   (180 * 1000)
#define UPSND_TIMEOUT   (  1 * 1000)
#define UMNOPROF_TIMEOUT ( 1 * 1000)
#define CEDRXS_TIMEOUT  (  1 * 1000)
#define CFUN_TIMEOUT    (180 * 1000)

// num sockets
#define NUMSOCKETS      ((int)(sizeof(_sockets)/sizeof(*_sockets)))
//! test if it is a socket is ok to use
#define ISSOCKET(s)     (((s) >= 0) && ((s) < NUMSOCKETS) && (_sockets[s].handle != MDM_SOCKET_ERROR))
//! check for timeout
#define TIMEOUT(t, ms)  ((ms != TIMEOUT_BLOCKING) && ((HAL_Timer_Get_Milli_Seconds() - t) > ms))
//! registration ok check helper
#define REG_OK(r)       ((r == REG_HOME) || (r == REG_ROAMING))
//! registration done check helper (no need to poll further)
#define REG_DONE(r)     ((r == REG_HOME) || (r == REG_ROAMING) || (r == REG_DENIED))
//! helper to make sure that lock unlock pair is always balanced
#define LOCK()      std::lock_guard<std::recursive_mutex> __mdm_guard(mdm_mutex);
//! helper to make sure that lock unlock pair is always balanced
#define UNLOCK()

static volatile uint32_t gprs_timeout_start;
static volatile uint32_t gprs_timeout_duration;

inline void ARM_GPRS_TIMEOUT(uint32_t dur) {
    gprs_timeout_start = HAL_Timer_Get_Milli_Seconds();
    gprs_timeout_duration = dur;
    DEBUG("GPRS WD Set %d",(dur));
}
inline bool IS_GPRS_TIMEOUT() {
    return gprs_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-gprs_timeout_start)>gprs_timeout_duration);
}

inline void CLR_GPRS_TIMEOUT() {
    gprs_timeout_duration = 0;
    DEBUG("GPRS WD Cleared, was %d", gprs_timeout_duration);
}

namespace {

AcT toCellularAccessTechnology(int rat) {
    switch (static_cast<UbloxSaraCellularAccessTechnology>(rat)) {
        case UBLOX_SARA_RAT_GSM:
        case UBLOX_SARA_RAT_GSM_COMPACT:
            return ACT_GSM;
        case UBLOX_SARA_RAT_UTRAN:
            return ACT_UTRAN;
        case UBLOX_SARA_RAT_GSM_EDGE:
            return ACT_EDGE;
        case UBLOX_SARA_RAT_UTRAN_HSDPA:
        case UBLOX_SARA_RAT_UTRAN_HSUPA:
        case UBLOX_SARA_RAT_UTRAN_HSDPA_HSUPA:
            return ACT_UTRAN;
        case UBLOX_SARA_RAT_LTE:
            return ACT_LTE;
        case UBLOX_SARA_RAT_EC_GSM_IOT:
            return ACT_LTE_CAT_M1;
        case UBLOX_SARA_RAT_E_UTRAN:
            return ACT_LTE_CAT_NB1;
        default:
            return ACT_UNKNOWN;
    }
}

} // anonymous

#ifdef MDM_DEBUG
 #if 0 // colored terminal output using ANSI escape sequences
  #define COL(c) "\033[" c
 #else
  #define COL(c) ""
 #endif
 #define DEF COL("39m")
 #define BLA COL("30m")
 #define RED COL("31m")
 #define GRE COL("32m")
 #define YEL COL("33m")
 #define BLU COL("34m")
 #define MAG COL("35m")
 #define CYA COL("36m")
 #define WHY COL("37m")

void dumpAtCmd(const char* buf, int len)
{
    DEBUG_D(" %3d \"", len);
    while (len --) {
        char ch = *buf++;
        if ((ch > 0x1F) && (ch < 0x7F)) { // is printable
            if      (ch == '%')  DEBUG_D("%%");
            else if (ch == '"')  DEBUG_D("\\\"");
            else if (ch == '\\') DEBUG_D("\\\\");
            else DEBUG_D("%c", ch);
        } else {
            if      (ch == '\a') DEBUG_D("\\a"); // BEL (0x07)
            else if (ch == '\b') DEBUG_D("\\b"); // Backspace (0x08)
            else if (ch == '\t') DEBUG_D("\\t"); // Horizontal Tab (0x09)
            else if (ch == '\n') DEBUG_D("\\n"); // Linefeed (0x0A)
            else if (ch == '\v') DEBUG_D("\\v"); // Vertical Tab (0x0B)
            else if (ch == '\f') DEBUG_D("\\f"); // Formfeed (0x0C)
            else if (ch == '\r') DEBUG_D("\\r"); // Carriage Return (0x0D)
            else                 DEBUG_D("\\x%02x", (unsigned char)ch);
        }
    }
    DEBUG_D("\"\r\n");
}

void MDMParser::_debugPrint(int level, const char* color, const char* format, ...)
{
    if (_debugLevel >= level)
    {
        if (color) DEBUG_D(color);
        va_list args;
        va_start(args, format);
        log_printf_v(LOG_LEVEL_TRACE, LOG_THIS_CATEGORY(), nullptr, format, args);
        va_end(args);
        if (color) DEBUG_D(DEF);
        DEBUG_D("\r\n");
    }
}

#define MDM_ERROR(_fmt, ...)  do {_debugPrint(0, RED, _fmt, ##__VA_ARGS__);}while(0)
#define MDM_INFO(_fmt, ...)   do {_debugPrint(1, GRE, _fmt, ##__VA_ARGS__);}while(0)
#define MDM_TRACE(_fmt, ...)  do {_debugPrint(2, DEF, _fmt, ##__VA_ARGS__);}while(0)
#define MDM_TEST(_fmt, ...)   do {_debugPrint(3, CYA, _fmt, ##__VA_ARGS__);}while(0)

#else

#define MDM_ERROR(...) // no tracing
#define MDM_TEST(...)  // no tracing
#define MDM_INFO(...)  // no tracing
#define MDM_TRACE(...) // no tracing

#endif

/* Private variables --------------------------------------------------------*/

MDMParser* MDMParser::inst;

/* Extern variables ---------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/

MDMParser::MDMParser(void)
{
    inst = this;
    memset(&_dev, 0, sizeof(_dev));
    memset(&_net, 0, sizeof(_net));
    _net.cgi.location_area_code = 0xFFFF;
    _net.cgi.cell_id = 0xFFFFFFFF;
    _ip        = NOIP;
    _init      = false;
    _pwr       = false;
    _activated = false;
    _attached  = false;
    _attached_urc = false; // updated by GPRS detached/attached URC,
                           // used to notify system of prolonged GPRS detach.
    _power_mode = 1; // default power mode is AT+UPSV=1
    _cancel_all_operations = false;
    sms_cb = NULL;
    memset(_sockets, 0, sizeof(_sockets));
    _timePowerOn = 0;
    _timeRegistered = 0;
    for (int socket = 0; socket < NUMSOCKETS; socket ++)
        _sockets[socket].handle = MDM_SOCKET_ERROR;
#ifdef MDM_DEBUG
    _debugLevel = 3;
    _debugTime = HAL_Timer_Get_Milli_Seconds();
#endif
}

void MDMParser::setPowerMode(int mode) {
    _power_mode = mode;
}

void MDMParser::cancel(void) {
    MDM_INFO("\r\n[ Modem::cancel ] = = = = = = = = = = = = = = =");
    _cancel_all_operations = true;
}

void MDMParser::resume(void) {
    MDM_INFO("\r\n[ Modem::resume ] = = = = = = = = = = = = = = =");
    _cancel_all_operations = false;
}

void MDMParser::setSMSreceivedHandler(_CELLULAR_SMS_CB cb, void* data) {
    sms_cb = cb;
    sms_data = data;
}

void MDMParser::SMSreceived(int index) {
    sms_cb(sms_data, index); // call the SMS callback with the index of the new SMS
}

int MDMParser::send(const char* buf, int len)
{
#ifdef MDM_DEBUG
    if (_debugLevel >= 3) {
        DEBUG_D("%10.3f AT send    ", (HAL_Timer_Get_Milli_Seconds()-_debugTime)*0.001);
        dumpAtCmd(buf,len);
#ifdef MDM_DEBUG_TX_PIPE
        // When using this, look for dangling stuff in the TX pipe just before a send.
        // This is evidence of something not being fully sent for the last write to the pipe.
        electronMDM.txDump();
#endif // MDM_DEBUG_TX_PIPE
    }
#endif // MDM_DEBUG

    return _send(buf, len);
}

int MDMParser::sendFormated(const char* format, ...) {
    va_list args;
    va_start(args, format);
    const int ret = sendFormattedWithArgs(format, args);
    va_end(args);
    return ret;
}

int MDMParser::sendFormattedWithArgs(const char* format, va_list args) {
    if (_cancel_all_operations) {
        return 0;
    }
    va_list argsCopy;
    va_copy(argsCopy, args);
    char buf[128];
    int n = vsnprintf(buf, sizeof(buf), format, args);
    if (n >= 0) {
        if ((size_t)n < sizeof(buf)) {
            n = send(buf, n);
        } else {
            char buf[n + 1]; // Use larger buffer
            n = vsnprintf(buf, sizeof(buf), format, argsCopy);
            if (n >= 0) {
                n = send(buf, n);
            }
        }
    }
    va_end(argsCopy);
    return n;
}

bool MDMParser::_atOk(void)
{
    sendFormated("AT\r\n");
    return (RESP_OK == waitFinalResp(nullptr, nullptr, AT_TIMEOUT));
}

int MDMParser::waitFinalResp(_CALLBACKPTR cb /* = NULL*/,
                             void* param /* = NULL*/,
                             system_tick_t timeout_ms /*= 5000*/)
{
    if (_cancel_all_operations) return WAIT;

    // If we went from a GPRS attached state to detached via URC,
    // a WDT was set and now expired. Notify system of disconnect.
    if (IS_GPRS_TIMEOUT()) {
        _ip = NOIP;
        _attached = false;
        CLR_GPRS_TIMEOUT();
        // HAL_NET_notify_dhcp(false);
        HAL_NET_notify_disconnected();
    }

    char buf[MAX_SIZE + 64 /* add some more space for framing */];
    system_tick_t start = HAL_Timer_Get_Milli_Seconds();
    do {
        int ret = getLine(buf, sizeof(buf));
#ifdef MDM_DEBUG
        if ((_debugLevel >= 3) && (ret != WAIT) && (ret != NOT_FOUND))
        {
            int len = LENGTH(ret);
            int type = TYPE(ret);
            const char* s = (type == TYPE_UNKNOWN)? YEL "UNK" DEF :
                            (type == TYPE_TEXT)   ? MAG "TXT" DEF :
                            (type == TYPE_OK   )  ? GRE "OK " DEF :
                            (type == TYPE_ERROR)  ? RED "ERR" DEF :
                            (type == TYPE_ABORTED) ? RED "ABT" DEF :
                            (type == TYPE_PLUS)   ? CYA " + " DEF :
                            (type == TYPE_PROMPT) ? BLU " > " DEF :
                                                        "..." ;
            DEBUG_D("%10.3f AT read %s", (HAL_Timer_Get_Milli_Seconds()-_debugTime)*0.001, s);
            dumpAtCmd(buf, len);
            (void)s;
#ifdef MDM_DEBUG_RX_PIPE
            // When using this, look for dangling stuff in the RX pipe after a read.
            // This is evidence of something not being fully parsed, which could be
            // a multi-line URC or an error in parsing.  Could also comment this out
            // and uncomment the ones used in _getLine() for even more real-time feedback
            // with the downside of MUCH more logs.
            electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
        }
#endif // MDM_DEBUG
        if ((ret != WAIT) && (ret != NOT_FOUND))
        {
            int type = TYPE(ret);
            // handle unsolicited commands here
            if (type == TYPE_PLUS) {
                const char* cmd = buf+3; // assume all TYPE_PLUS commands have \r\n+ and skip
                int a, b, c, d, r;
                char s[32];

                // SMS Command ---------------------------------
                // +CNMI: <mem>,<index>
                if (sscanf(cmd, "CMTI: \"%*[^\"]\",%d", &a) == 1) {
                    DEBUG_D("New SMS at index %d\r\n", a);
                    if (sms_cb) SMSreceived(a);
                }
                // TODO: This should include other LTE devices,
                // perhaps adding _net.act < 7
                else if ((_dev.dev != DEV_SARA_R410) && (sscanf(cmd, "CIEV: 9,%d", &a) == 1)) {
                    DEBUG_D("CIEV matched: 9,%d\r\n", a);
                    // Wait until the system is attached before attempting to act on GPRS detach
                    if (_attached) {
                        _attached_urc = (a==2)?1:0;
                        if (!_attached_urc) ARM_GPRS_TIMEOUT(15*1000); // If detached, set WDT
                        else CLR_GPRS_TIMEOUT(); // else if re-attached clear WDT.
                    }
                // Socket Specific Command ---------------------------------
                // +USORD: <socket>,<length>
                } else if ((sscanf(cmd, "USORD: %d,%d", &a, &b) == 2)) {
                    int socket = _findSocket(a);
                    DEBUG_D("Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
                    if (socket != MDM_SOCKET_ERROR)
                        _sockets[socket].pending = b;
                // +UUSORD: <socket>,<length>
                } else if ((sscanf(cmd, "UUSORD: %d,%d", &a, &b) == 2)) {
                    int socket = _findSocket(a);
                    DEBUG_D("Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
                    if (socket != MDM_SOCKET_ERROR)
                        _sockets[socket].pending = b;
                // +USORF: <socket>,<length>
                } else if ((sscanf(cmd, "USORF: %d,%d", &a, &b) == 2)) {
                    int socket = _findSocket(a);
                    DEBUG_D("Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
                    if (socket != MDM_SOCKET_ERROR)
                        _sockets[socket].pending = b;
                // +UUSORF: <socket>,<length>
                } else if ((sscanf(cmd, "UUSORF: %d,%d", &a, &b) == 2)) {
                    int socket = _findSocket(a);
                    DEBUG_D("Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
                    if (socket != MDM_SOCKET_ERROR)
                        _sockets[socket].pending = b;
                // +UUSOCL: <socket>
                } else if ((sscanf(cmd, "UUSOCL: %d", &a) == 1)) {
                    int socket = _findSocket(a);
                    DEBUG_D("Socket %d: handle %d closed by remote host\r\n", socket, a);
                    if (socket != MDM_SOCKET_ERROR) {
                        _socketFree(socket);
                    }
                }

                // GSM/UMTS Specific -------------------------------------------
                // +UUPSDD: <profile_id>
                if (sscanf(cmd, "UUPSDD: %s", s) == 1) {
                    DEBUG_D("UUPSDD: %s matched\r\n", PROFILE);
                    if ( !strcmp(s, PROFILE) ) {
                        _ip = NOIP;
                        _attached = false;
                        DEBUG("PDP context deactivated remotely!\r\n");
                        // PDP context was remotely deactivated via URC,
                        // Notify system of disconnect.
                        HAL_NET_notify_dhcp(false);
                    }
                } else {
                    // +CREG|CGREG: <n>,<stat>[,<lac>,<ci>[,AcT[,<rac>]]] // reply to AT+CREG|AT+CGREG
                    // +CREG|CGREG: <stat>[,<lac>,<ci>[,AcT[,<rac>]]]     // URC
                    b = (int)0xFFFF; c = (int)0xFFFFFFFF; d = -1;
                    r = sscanf(cmd, "%s %*d,%d,\"%x\",\"%x\",%d",s,&a,&b,&c,&d);
                    if (r <= 1)
                        r = sscanf(cmd, "%s %d,\"%x\",\"%x\",%d",s,&a,&b,&c,&d);
                    if (r >= 2) {
                        Reg *reg = !strcmp(s, "CREG:")  ? &_net.csd :
                                   !strcmp(s, "CGREG:") ? &_net.psd :
                                   !strcmp(s, "CEREG:") ? &_net.eps : NULL;
                        if (reg) {
                            // network status
                            if      (a == 0) *reg = REG_NONE;     // 0: not registered, home network
                            else if (a == 1) *reg = REG_HOME;     // 1: registered, home network
                            else if (a == 2) *reg = REG_NONE;     // 2: not registered, but MT is currently searching a new operator to register to
                            else if (a == 3) *reg = REG_DENIED;   // 3: registration denied
                            else if (a == 4) *reg = REG_UNKNOWN;  // 4: unknown
                            else if (a == 5) *reg = REG_ROAMING;  // 5: registered, roaming
                            if ((r >= 3) && (b != (int)0xFFFF))      _net.cgi.location_area_code = b;
                            if ((r >= 4) && (c != (int)0xFFFFFFFF))  _net.cgi.cell_id  = c;
                            // access technology
                            if (r >= 5) {
                                _net.act = toCellularAccessTechnology(d);
                            }
                        }
                    }
                }
            } // end ==TYPE_PLUS
            if (cb) {
                int len = LENGTH(ret);
                int ret = cb(type, buf, len, param);
                if (WAIT != ret)
                    return ret;
            }
            if (type == TYPE_OK)
                return RESP_OK;
            if (type == TYPE_ERROR)
                return RESP_ERROR;
            if (type == TYPE_PROMPT)
                return RESP_PROMPT;
            if (type == TYPE_ABORTED)
                return RESP_ABORTED; // This means the current command was ABORTED, so retry your command if critical.
        }
        // relax a bit
        HAL_Delay_Milliseconds(1);
    }
    while (!TIMEOUT(start, timeout_ms) && !_cancel_all_operations);

    return WAIT;
}

int MDMParser::sendCommandWithArgs(const char* fmt, va_list args, _CALLBACKPTR cb, void* param, system_tick_t timeout)
{
    LOCK();
    sendFormattedWithArgs(fmt, args);
    const int ret = waitFinalResp(cb, param, timeout);
    UNLOCK();
    return ret;
}

void MDMParser::lock()
{
    mdm_mutex.lock();
}

void MDMParser::unlock()
{
    mdm_mutex.unlock();
}

int MDMParser::_cbCEDRXS(int type, const char* buf, int len, EdrxActs* edrxActs)
{
    if (((type == TYPE_PLUS) || (type == TYPE_UNKNOWN)) && edrxActs) {
        // if response is "\r\n+CEDRXS:\r\n", all AcT's disabled, do nothing
        if (strncmp(buf, "\r\n+CEDRXS:\r\n", len) != 0) {
            int a;
            if (sscanf(buf, "\r\n+CEDRXS: %1d[2-5]", &a) == 1 ||
                sscanf(buf, "+CEDRXS: %1d[2-5]", &a) == 1) {
                if (edrxActs->count < MDM_R410_EDRX_ACTS_MAX) {
                    edrxActs->act[edrxActs->count++] = a;
                }
            }
        }
    }
    return WAIT;
}

int MDMParser::_cbString(int type, const char* buf, int len, char* str)
{
    if (str && (type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n%s\r\n", str) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int MDMParser::_cbInt(int type, const char* buf, int len, int* val)
{
    if (val && (type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n%d\r\n", val) == 1)
            /*nothing*/;
    }
    return WAIT;
}

// ----------------------------------------------------------------

bool MDMParser::connect(
            const char* apn, const char* username,
            const char* password, Auth auth)
{
    bool ok = registerNet(apn);
/*
#ifdef MDM_DEBUG
    if (_debugLevel >= 1) {
        dumpNetStatus(&_net);
    }
#endif
*/
    if (!ok) {
        return false;
    }
    ok = pdp(apn);
/*
#ifdef MDM_DEBUG
    if (_debugLevel >= 1) {
        dumpNetStatus(&_net);
    }
#endif
*/
    if (!ok) {
        return false;
    }
    const MDM_IP ip = join(apn, username, password, auth);
/*
#ifdef MDM_DEBUG
    if (_debugLevel >= 1) {
        dumpIp(ip);
    }
#endif
*/
    if (ip == NOIP) {
        return false;
    }
    HAL_NET_notify_connected();
    HAL_NET_notify_dhcp(true);
    return true;
}

bool MDMParser::disconnect() {
    if (!deactivate()) {
        return false;
    }
    if (!detach()) {
        return false;
    }
    HAL_NET_notify_disconnected();
    return true;
}

void MDMParser::reset(void)
{
    MDM_INFO("[ Modem reset ]");
    unsigned delay = 100;
    if (_dev.dev == DEV_UNKNOWN || _dev.dev == DEV_SARA_R410) {
        delay = 10000; // SARA-R4: 10s
    }
    HAL_GPIO_Write(RESET_UC, 0);
    HAL_Delay_Milliseconds(delay);
    HAL_GPIO_Write(RESET_UC, 1);
    // reset power on and registered timers for memory issue power off delays
    _timePowerOn = 0;
    _timeRegistered = 0;
}

bool MDMParser::_powerOn(void)
{
    LOCK();

    /* Initialize I/O */
    STM32_Pin_Info* PIN_MAP_PARSER = HAL_Pin_Map();
    // This pin tends to stay low when floating on the output of the buffer (PWR_UB)
    // It shouldn't hurt if it goes low temporarily on STM32 boot, but strange behavior
    // was noticed when it was left to do whatever it wanted. By adding a 100k pull up
    // resistor all flakey behavior has ceased (i.e., the modem had previously stopped
    // responding to AT commands).  This is how we set it HIGH before enabling the OUTPUT.
    PIN_MAP_PARSER[PWR_UC].gpio_peripheral->BSRRL = PIN_MAP_PARSER[PWR_UC].gpio_pin;
    HAL_Pin_Mode(PWR_UC, OUTPUT);
    // This pin tends to stay high when floating on the output of the buffer (RESET_UB),
    // but we need to ensure it gets set high before being set to an OUTPUT.
    // If this pin goes LOW, the modem will be reset and all configuration will be lost.
    PIN_MAP_PARSER[RESET_UC].gpio_peripheral->BSRRL = PIN_MAP_PARSER[RESET_UC].gpio_pin;
    HAL_Pin_Mode(RESET_UC, OUTPUT);

    _dev.dev = DEV_UNKNOWN;
    _dev.lpm = LPM_ENABLED;

    HAL_Pin_Mode(LVLOE_UC, OUTPUT);
    HAL_GPIO_Write(LVLOE_UC, 0);

    // This connects to V_INT on the cellular modem. Monitoring V_INT can provide more
    // fine-grained detail on when the modem is powered-on (HIGH) or has completed a
    // power-off sequence and currently powered-off (LOW).
    // NOTE: Modem isn't necessarily ready to communicate on its AT interface when V_INT
    // first goes HIGH.
    HAL_Pin_Mode(RI_UC, INPUT_PULLDOWN);

    if (!_init) {
        MDM_INFO("[ ElectronSerialPipe::begin ] pipeTx=%d pipeRx=%d", electronMDM.txSize(), electronMDM.rxSize());

        // Here we initialize the UART with hardware flow control enabled, even though some of
        // the modems don't support it (SARA-R4 at the time of writing). It is assumed that the
        // modem still keeps the CTS pin in a correct state even if doesn't support the CTS/RTS
        // flow control
        electronMDM.begin(115200, true /* hwFlowControl */);
        _init = true;
    }

    MDM_INFO("\r\n[ Modem::powerOn ] = = = = = = = = = = = = = =");
    bool continue_cancel = false;
    bool retried_after_reset = false;

    int i = 10;
    while (i--) {
        // SARA-U2/LISA-U2 50..80us
        HAL_GPIO_Write(PWR_UC, 0); HAL_Delay_Milliseconds(50);
        HAL_GPIO_Write(PWR_UC, 1); HAL_Delay_Milliseconds(10);

        // SARA-G35 >5ms, LISA-C2 > 150ms, LEON-G2 >5ms, SARA-R4 >= 150ms
        HAL_GPIO_Write(PWR_UC, 0); HAL_Delay_Milliseconds(150);
        HAL_GPIO_Write(PWR_UC, 1); HAL_Delay_Milliseconds(100);

        // purge any messages
        purge();

        // Save desire to cancel, but since we are already here
        // trying to power up the modem when we received a cancel
        // resume AT parser to ensure it's ready to receive
        // power down commands.
        if (_cancel_all_operations) {
            continue_cancel = true;
            resume(); // make sure we can talk to the modem
        }

        // check interface
        if(_atOk()) {
            _pwr = true;
            // start power on timer for memory issue power off delays, assume not registered.
            _timePowerOn = HAL_Timer_Get_Milli_Seconds();
            _timeRegistered = 0;
            break;
        }
        else if (i==0 && !retried_after_reset) {
            retried_after_reset = true; // only perform reset & retry sequence once
            i = 10;
            reset();
        }

    }
    if (i < 0) {
        MDM_ERROR("[ No Reply from Modem ]\r\n");
    } else {
        // Determine type of the modem
        sendFormated("AT+CGMM\r\n");
        waitFinalResp(_cbCGMM, &_dev);
        if (_dev.dev == DEV_SARA_R410) {
            // SARA-R410 doesn't support hardware flow control, reinitialize the UART
            electronMDM.begin(115200, false /* hwFlowControl */);
            // Power saving modes defined by the +UPSV command are not supported
            _dev.lpm = LPM_DISABLED;
        }
    }

    if (continue_cancel) {
        cancel();
        goto failure;
    }

    // Flush any on-boot URCs that can cause syncing issues later
    waitFinalResp(NULL,NULL,200);

    // echo off
    sendFormated("ATE0\r\n");
    if(RESP_OK != waitFinalResp())
        goto failure;
    // enable verbose error messages
    sendFormated("AT+CMEE=2\r\n");
    if(RESP_OK != waitFinalResp())
        goto failure;
    // Configures sending of URCs from MT to DTE for indications
    sendFormated("AT+CMER=1,0,0,2,1\r\n");
    if(RESP_OK != waitFinalResp())
        goto failure;
    // set baud rate
    sendFormated("AT+IPR=115200\r\n");
    if (RESP_OK != waitFinalResp())
        goto failure;
    // wait some time until baudrate is applied
    HAL_Delay_Milliseconds(100); // SARA-G > 40ms

    UNLOCK();
    return true;
failure:
    UNLOCK();
    return false;
}

bool MDMParser::powerOn(const char* simpin)
{
    LOCK();
    memset(&_dev, 0, sizeof(_dev));
    bool retried_after_reset = false;

    /* Power on the modem and perform basic initialization */
    if (!_powerOn())
        goto failure;

    /* The ATI command is undocumented, and in practice the response
     * time varies greatly. On inital power-on of the module, ATI
     * will respond with "OK" before a device type number, which
     * requires wasting time in a for() loop to solve.
     * Instead, use AT+CGMM and _dev.model for future use of module identification.
     *
     * identify the module
     * sendFormated("ATI\r\n");
     * if (RESP_OK != waitFinalResp(_cbATI, &_dev.dev))
     *     goto failure;
     * if (_dev.dev == DEV_UNKNOWN)
     *     goto failure;
     */

    // check the sim card
    for (int i = 0; (i < 5) && (_dev.sim != SIM_READY) && !_cancel_all_operations; i++) {
        sendFormated("AT+CPIN?\r\n");
        int ret = waitFinalResp(_cbCPIN, &_dev.sim);
        // having an error here is ok (sim may still be initializing)
        if ((RESP_OK != ret) && (RESP_ERROR != ret)) {
            goto failure;
        }
        else if (i==4 && (RESP_OK != ret) && !retried_after_reset) {
            retried_after_reset = true; // only perform reset & retry sequence once
            i = 0;
            if(!powerOff())
                reset();
            /* Power on the modem and perform basic initialization again */
            if (!_powerOn())
                goto failure;
        }
        // Enter PIN if needed
        if (_dev.sim == SIM_PIN) {
            if (!simpin) {
                MDM_ERROR("SIM PIN not available\r\n");
                goto failure;
            }
            sendFormated("AT+CPIN=%s\r\n", simpin);
            if (RESP_OK != waitFinalResp(_cbCPIN, &_dev.sim))
                goto failure;
        } else if (_dev.sim != SIM_READY) {
            // wait for up to one second while looking for slow "+CPIN: READY" URCs
            waitFinalResp(_cbCPIN, &_dev.sim, 1000);
        }
    }
    if (_dev.sim != SIM_READY) {
        if (_dev.sim == SIM_MISSING) {
            MDM_ERROR("SIM not inserted\r\n");
        }
        goto failure;
    }
    if (_dev.dev == DEV_UNKNOWN) {
        MDM_ERROR("Unknown modem type");
        goto failure;
    }

    UNLOCK();
    return true;
failure:
    if (_cancel_all_operations) {
        // fake out the has_credentials() function so we don't end up in listening mode
        _dev.sim = SIM_READY;
        // return true to prevent from entering Listening Mode
        // UNLOCK();
        // return true;
    }
    UNLOCK();
    return false;
}

bool MDMParser::init(DevStatus* status)
{
    LOCK();
    MDM_INFO("\r\n[ Modem::init ] = = = = = = = = = = = = = = =");
    if (_dev.dev == DEV_SARA_R410) {
        // TODO: Without this delay, some commands, such as +CIMI, may return a SIM failure error.
        // This probably has something to do with the SIM initialization. Should we check the SIM
        // status via +USIMSTAT in addition to +CPIN?
        HAL_Delay_Milliseconds(250);
    }
    // Returns the product serial number, IMEI (International Mobile Equipment Identity)
    sendFormated("AT+CGSN\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.imei))
        goto failure;

    if (_dev.sim != SIM_READY) {
        if (_dev.sim == SIM_MISSING)
            MDM_ERROR("SIM not inserted\r\n");
        goto failure;
    }
    // get the manufacturer
    sendFormated("AT+CGMI\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.manu))
        goto failure;
    // get the version
    sendFormated("AT+CGMR\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.ver))
        goto failure;
    // ATI9 (get version and app version)
    // example output
    // 16 "\r\n08.90,A01.13\r\n" G350 (newer)
    // 16 "\r\n08.70,A00.02\r\n" G350 (older)
    // 28 "\r\nL0.0.00.00.05.06,A.02.00\r\n" (memory issue)
    // 28 "\r\nL0.0.00.00.05.07,A.02.02\r\n" (demonstrator)
    // 28 "\r\nL0.0.00.00.05.08,A.02.04\r\n" (maintenance)
    memset(_verExtended, 0, sizeof(_verExtended));
    sendFormated("ATI9\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _verExtended))
        goto failure;
    // Test for Memory Issue version
    _memoryIssuePresent = false;
    if (!strcmp("L0.0.00.00.05.06,A.02.00", _verExtended)) {
        _memoryIssuePresent = true;
    }
    // DEBUG_D("MODEM AND APP VERSION: %s (%s)\r\n", _verExtended, (_memoryIssuePresent) ? "MEMISSUE" : "OTHER");
    // Returns the ICCID (Integrated Circuit Card ID) of the SIM-card.
    // ICCID is a serial number identifying the SIM.
    sendFormated("AT+CCID\r\n");
    if (RESP_OK != waitFinalResp(_cbCCID, _dev.ccid))
        goto failure;
    // Setup SMS in text mode
    sendFormated("AT+CMGF=1\r\n");
    if (RESP_OK != waitFinalResp())
        goto failure;
    // setup new message indication
    sendFormated("AT+CNMI=2,1\r\n");
    if (RESP_OK != waitFinalResp())
        goto failure;
    // Request IMSI (International Mobile Subscriber Identification)
    sendFormated("AT+CIMI\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.imsi))
        goto failure;
    // Reformat the operator string to be numeric
    // (allows the capture of `mcc` and `mnc`)
    sendFormated("AT+COPS=3,2\r\n");
    if (RESP_OK != waitFinalResp(nullptr, nullptr, COPS_TIMEOUT))
        goto failure;
#ifdef SOCKET_HEX_MODE
    // Enable hex mode for socket commands
    sendFormated("AT+UDCONF=1,1\r\n");
    if (waitFinalResp() != RESP_OK) {
        goto failure;
    }
#endif // SOCKET_HEX_MODE
    // enable power saving
    if (_dev.lpm != LPM_DISABLED) {
        // enable power saving (requires flow control, cts at least)
        sendFormated("AT+UPSV=%d\r\n", _power_mode);
        if (RESP_OK != waitFinalResp())
            goto failure;
        if (_power_mode != 0) {
            _dev.lpm = LPM_ACTIVE;
        }
    }
    // All _dev items collected without error, populate status.
    if (status) {
        memcpy(status, &_dev, sizeof(DevStatus));
    }
    if (_dev.dev == DEV_SARA_R410) {
        bool resetNeeded = false;
        int umnoprof = UBLOX_SARA_UMNOPROF_NONE;

        sendFormated("AT+UMNOPROF?\r\n");
        if (RESP_ERROR == waitFinalResp(_cbUMNOPROF, &umnoprof, UMNOPROF_TIMEOUT)) {
            goto reset_failure;
        }
        if (umnoprof == UBLOX_SARA_UMNOPROF_SW_DEFAULT) {
            sendFormated("AT+UMNOPROF=%d\r\n", UBLOX_SARA_UMNOPROF_SIM_SELECT);
            waitFinalResp(); // Not checking for error since we will reset either way
            goto reset_failure;
        }
        // Force Power Saving mode to be disabled for good measure
        sendFormated("AT+CPSMS=0\r\n");
        if (RESP_OK != waitFinalResp()) {
            goto failure;
        }
        sendFormated("AT+CPSMS?\r\n");
        if (RESP_OK != waitFinalResp()) {
            goto failure;
        }
        // Force eDRX mode to be disabled
        // 18/23 hardware doesn't seem to be disabled by default
        sendFormated("AT+CEDRXS?\r\n");
        // Reset the detected count each time we check for eDRX AcTs enabled
        EdrxActs _edrxActs;
        if (RESP_ERROR == waitFinalResp(_cbCEDRXS, &_edrxActs, CEDRXS_TIMEOUT)) {
            goto reset_failure;
        }
        for (int i = 0; i < _edrxActs.count; i++) {
            sendFormated("AT+CEDRXS=3,%d\r\n", _edrxActs.act[i]);
            waitFinalResp(); // Not checking for error since we will reset either way
            resetNeeded = true;
        }
        if (resetNeeded) {
            goto reset_failure;
        }
    } // if (_dev.dev == DEV_SARA_R410)
    UNLOCK();
    return true;

failure:
    UNLOCK();
    return false;

reset_failure:
    // Don't get stuck in a reset-retry loop
    static int resetFailureAttempts = 0;
    if (resetFailureAttempts++ < 5) {
        sendFormated("AT+CFUN=15\r\n");
        waitFinalResp(nullptr, nullptr, CFUN_TIMEOUT);
        _init = false; // invalidate init and prevent cellular registration
        _pwr = false;  //   |
        // When this exits false, ARM_WLAN_WD 1 will fire and timeout after 30s.
        // MDMParser::powerOn and MDMParser::init will then be retried by the system.
    } // else {
        // stop resetting, and try to register.
        // Preventing cellular registration gets us back to retrying init() faster,
        // and the timeout of 3 attempts is arbitrary but allows us to register and
        // connect even with an error, since that error is persisting and might just
        // be a fluke or bug in the modem. Allowing to continue on eventually helps
        // the device recover in odd situations we can't predict.
    // }
    UNLOCK();
    return false;
}

bool MDMParser::powerOff(void)
{
    LOCK();
    bool ok = false;
    bool continue_cancel = false;
    bool check_ri = false;
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::powerOff ] = = = = = = = = = = = = = =");
        if (_cancel_all_operations) {
            continue_cancel = true;
            resume(); // make sure we can use the AT parser
        }
        // Try to power off
        for (int i=0; i<3; i++) { // try 3 times
            if (!_atOk()) {
                if (_dev.dev == DEV_SARA_R410) {
                    check_ri = true;
                    // If memory issue is present, ensure we don't force a power off too soon
                    // to avoid hitting the 124 day memory housekeeping issue, AT+CPWROFF will
                    // handle this delay automatically, or timeout after 40s.
                    if (_memoryIssuePresent) {
                        MDM_INFO("\r\n[ Modem::powerOff ] AT Failure, waiting up to 30s to power off with PWR_UC...");
                        system_tick_t now = HAL_Timer_Get_Milli_Seconds();
                        if (_timePowerOn == 0) {
                            // fallback to max timeout of 30s to be safe
                            _timePowerOn = now;
                        }
                        // check for timeout (VINT == LOW, Powered on 30s ago, Registered 20s ago)
                        do {
                            now = HAL_Timer_Get_Milli_Seconds();
                            // prefer to timeout 20s after registration if we are registered
                            if (_timeRegistered) {
                                if (now - _timeRegistered >= 20000UL) {
                                    break;
                                }
                            } else if (_timePowerOn && now - _timePowerOn >= 30000UL) {
                                break;
                            }
                            HAL_Delay_Milliseconds(100); // just wait
                        } while ( HAL_GPIO_Read(RI_UC) );
                        // reset timers
                        _timeRegistered = 0;
                        _timePowerOn = 0;
                    }
                }
                MDM_INFO("\r\n[ Modem::powerOff ] AT Failure, trying PWR_UC...");
                // Skip power off sequence if power is already off
                if (_dev.dev == DEV_SARA_R410 && !HAL_GPIO_Read(RI_UC)) {
                    break;
                }
                HAL_GPIO_Write(PWR_UC, 0);
                // >1.5 seconds on SARA R410M
                // >1 second on SARA U2
                // plus a little extra for good luck
                HAL_Delay_Milliseconds(1600);
                HAL_GPIO_Write(PWR_UC, 1);
                break;
            } else {
                sendFormated("AT+CPWROFF\r\n");
                int ret = waitFinalResp(nullptr, nullptr, CPWROFF_TIMEOUT);
                if (RESP_OK == ret) {
                    if (_dev.dev == DEV_SARA_R410) {
                        check_ri = true;
                    }
                    break;
                } else if (RESP_ABORTED == ret) {
                    MDM_INFO("\r\n[ Modem::powerOff ] found ABORTED, retrying...");
                } else {
                    MDM_INFO("\r\n[ Modem::powerOff ] timeout, retrying...");
                }
            }
        }
        // Verify power off, or delay
        if (check_ri) {
            system_tick_t t0 = HAL_Timer_Get_Milli_Seconds();
            while (HAL_GPIO_Read(RI_UC) && !TIMEOUT(t0, 10000)) {
                HAL_Delay_Milliseconds(1); // just wait
            }
            // if V_INT is low, indicate power is off
            if (!HAL_GPIO_Read(RI_UC)) {
                _pwr = false;
            }
            MDM_INFO("\r\n[ Modem::powerOff ] took %lu ms", HAL_Timer_Get_Milli_Seconds() - t0);
        } else {
            _pwr = false;
            // todo - add if these are automatically done on power down
            //_activated = false;
            //_attached = false;
            HAL_Delay_Milliseconds(1000); // give peace a chance
        }
    }

    // Close serial connection
    electronMDM.end();
    _init = false;
    MDM_INFO("[ ElectronSerialPipe::end ] pipeTx=%d pipeRx=%d", electronMDM.txSize(), electronMDM.rxSize());

    HAL_Pin_Mode(PWR_UC, INPUT);
    HAL_Pin_Mode(RESET_UC, INPUT);
    HAL_Pin_Mode(LVLOE_UC, INPUT);

    ok = !_pwr;

    if (continue_cancel) cancel();
    UNLOCK();
    return ok;
}

int MDMParser::_cbUMNOPROF(int type, const char *buf, int len, int* i)
{
    if ((type == TYPE_PLUS) && i){
        int a;
        *i = -1;
        if (sscanf(buf, "\r\n+UMNOPROF: %d", &a) == 1) {
            *i = a;
        }
    }
    return WAIT;
}

int MDMParser::_cbCGMM(int type, const char* buf, int len, DevStatus* s)
{
    if (type == TYPE_UNKNOWN && s) {
        static_assert(sizeof(DevStatus::model) == 16, "The format string below needs to be updated accordingly");
        if (sscanf(buf, "\r\n%15s\r\n", s->model) == 1) {
            if (strstr(s->model, "SARA-G350")) {
                s->dev = DEV_SARA_G350;
            } else if (strstr(s->model, "SARA-U260")) {
                s->dev = DEV_SARA_U260;
            } else if (strstr(s->model, "SARA-U270")) {
                s->dev = DEV_SARA_U270;
            } else if (strstr(s->model, "SARA-U201")) {
                s->dev = DEV_SARA_U201;
            } else if (strstr(s->model, "SARA-R410")) {
                s->dev = DEV_SARA_R410;
            }
        }
    }
    return WAIT;
}

int MDMParser::_cbCPIN(int type, const char* buf, int len, Sim* sim)
{
    if (sim) {
        if (type == TYPE_PLUS){
            char s[16];
            if (sscanf(buf, "\r\n+CPIN: %[^\r]\r\n", s) >= 1)
                *sim = (0 == strcmp("READY", s)) ? SIM_READY : SIM_PIN;
        } else if (type == TYPE_ERROR) {
            if (strstr(buf, "+CME ERROR: SIM not inserted"))
                *sim = SIM_MISSING;
        }
    }
    return WAIT;
}

int MDMParser::_cbCCID(int type, const char* buf, int len, char* ccid)
{
    if ((type == TYPE_PLUS) && ccid) {
        if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", ccid) == 1) {
            //DEBUG_D("Got CCID: %s\r\n", ccid);
        }
    }
    return WAIT;
}

bool MDMParser::_checkEpsReg(void) {
    // On the SARA R410M check EPS registration
    if (_dev.dev == DEV_SARA_R410) {
        LOCK();
        int r;
        sendFormated("AT+CEREG?\r\n");
        r = waitFinalResp(nullptr, nullptr, CEREG_TIMEOUT);
        UNLOCK();
        if (r != RESP_OK || !REG_OK(_net.eps)) {
            return false;
        }
    }
    return true;
}

// only intended for use on SARA R410M to detect issue where CAT M1+NB1 is
// currently an invalid configuration but also the default configuration
#define CB_URAT_DEFAULT_CONFIG "\r\n+URAT: 7,8" // the invalid configuration string
int MDMParser::_cbURAT(int type, const char *buf, int len, bool *matched_default)
{
    if ((type == TYPE_PLUS) && matched_default) {
        *matched_default = false;
        if (!strncmp(CB_URAT_DEFAULT_CONFIG, buf, strlen(CB_URAT_DEFAULT_CONFIG))) {
            *matched_default = true;
        }
    }
    return WAIT;
}

bool MDMParser::registerNet(const char* apn, NetStatus* status /*= NULL*/, system_tick_t timeout_ms /*= 180000*/)
{
    LOCK();
    if (_init && _pwr && _dev.dev != DEV_UNKNOWN) {
        MDM_INFO("\r\n[ Modem::register ] = = = = = = = = = = = = = =");
        // Check to see if we are already connected. If so don't issue these
        // commands as they will knock us off the cellular network.
        bool ok = false;
        // We may already be connected quickly on boot, or connect within the first
        // call to checkNetStatus(), so these registration URCs need to be set up at
        // least once to ensure they continue to work later on when using _checkEpsReg()
        if (_dev.dev == DEV_SARA_R410) {
            // reset registered timer for memory issue power off delays
            _timeRegistered = 0;
            // Set up the EPS network registration URC
            sendFormated("AT+CEREG=2\r\n");
            if (RESP_OK != waitFinalResp(nullptr, nullptr, CEREG_TIMEOUT)) {
                goto failure;
            }
        } else {
            // Set up the GPRS network registration URC
            sendFormated("AT+CGREG=2\r\n");
            if (RESP_OK != waitFinalResp(nullptr, nullptr, CGREG_TIMEOUT)) {
                goto failure;
            }
            // Set up the GSM network registration URC
            sendFormated("AT+CREG=2\r\n");
            if (RESP_OK != waitFinalResp(nullptr, nullptr, CREG_TIMEOUT)) {
                goto failure;
            }
        }
        if (!(ok = checkNetStatus())) {
            if (_dev.dev == DEV_SARA_R410) {
                bool set_cgdcont = false;
                bool set_rat = false;
                // On SARA R410M [Cat-M1(7) & Cat-NB1(8)] is an invalid default configuration.
                // Detect and force Cat-MB1 only when detected.
                sendFormated("AT+URAT?\r\n");
                // This may fail in some cases due to UMNOPROF setting, so do not check for error.
                // In the case of error, set_rat will remain false and the URAT setting will not be altered.
                waitFinalResp(_cbURAT, &set_rat, URAT_TIMEOUT);
                // Get default context settings
                sendFormated("AT+CGDCONT?\r\n");
                CGDCONTparam ctx = {};
                if (waitFinalResp(_cbCGDCONT, &ctx, CGDCONT_TIMEOUT) != RESP_OK) {
                    goto failure;
                }
                // TODO: SARA-R410-01B modules come preconfigured with AT&T's APN ("broadband"), which may
                // cause network registration issues with MVNO providers and third party SIM cards. As a
                // workaround the code below sets a blank APN if it detects that the current context is
                // configured to use the dual stack IPv4/IPv6 capability ("IPV4V6"), which is the case for
                // the factory default settings. Ideally, setting of a default APN should be based on IMSI
                if (strcmp(ctx.type, "IP") != 0 || strcmp(ctx.apn, apn ? apn : "") != 0) {
                    set_cgdcont = true;
                }
                if (set_cgdcont || set_rat) {
                    // Stop the network registration and update the context settings
                    if (!_atOk()) {
                        goto failure;
                    }
                    sendFormated("AT+COPS=2,2\r\n");
                    if (waitFinalResp(nullptr, nullptr, COPS_TIMEOUT) != RESP_OK) {
                        goto failure;
                    }
                    // Force Cat-M1 mode
                    if (set_rat) {
                        MDM_INFO("[ Modem::register ] Invalid Cat-M1/NB1 mode on SARA R410M, forcing Cat-M1");
                        sendFormated("AT+URAT=7\r\n");
                        if (waitFinalResp(nullptr, nullptr, URAT_TIMEOUT) != RESP_OK) {
                            goto failure;
                        }
                    }
                    // Update the context settings
                    if (set_cgdcont) {
                        sendFormated("AT+CGDCONT=%d,\"IP\",\"%s\"\r\n", PDP_CONTEXT, apn ? apn : "");
                        if (waitFinalResp() != RESP_OK) {
                            goto failure;
                        }
                    }
                }
                // Make sure automatic network registration is enabled
                if (!_atOk()) {
                    goto failure;
                }
                sendFormated("AT+COPS=0,2\r\n");
                if (waitFinalResp(nullptr, nullptr, COPS_TIMEOUT) != RESP_OK) {
                    goto failure;
                }
            } else {
                // Show enabled RATs
                sendFormated("AT+URAT?\r\n");
                waitFinalResp(nullptr, nullptr, URAT_TIMEOUT);
            }
            // Now check every 15 seconds for 5 minutes to see if we're connected to the tower (GSM, GPRS and LTE)
            system_tick_t start = HAL_Timer_Get_Milli_Seconds();
            while (!(ok = checkNetStatus(status)) && !TIMEOUT(start, timeout_ms) && !_cancel_all_operations) {
                system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                while ((HAL_Timer_Get_Milli_Seconds() - start < 15000UL) && !_cancel_all_operations) {
                    // Pump the URCs looking for +CREG/+CGREG/+CEREG connection events.
                    waitFinalResp(nullptr, nullptr, 10);
                    if ((REG_OK(_net.csd) && REG_OK(_net.psd)) || REG_OK(_net.eps)) {
                        break; // force another checkNetStatus() call
                    }
                }
            }
            if (_net.csd == REG_DENIED) MDM_ERROR("CSD Registration Denied\r\n");
            if (_net.psd == REG_DENIED) MDM_ERROR("PSD Registration Denied\r\n");
            if (_net.eps == REG_DENIED) MDM_ERROR("EPS Registration Denied\r\n");
        }
        UNLOCK();
        return ok;
    }
failure:
    UNLOCK();
    return false;
}

bool MDMParser::checkNetStatus(NetStatus* status /*= NULL*/)
{
    bool ok = false;
    LOCK();
    // TODO: optimize, add a param/variable to prevent clearing _net if
    // arriving here after break() from inner while() in registerNet().
    memset(&_net, 0, sizeof(_net));
    _net.cgi.location_area_code = 0xFFFF;
    _net.cgi.cell_id = 0xFFFFFFFF;
    if (_dev.dev == DEV_SARA_R410) {
        // check EPS registration (LTE)
        sendFormated("AT+CEREG?\r\n");
        waitFinalResp(nullptr, nullptr, CEREG_TIMEOUT);
    } else {
        // check CSD registration (GSM)
        sendFormated("AT+CREG?\r\n");
        waitFinalResp(nullptr, nullptr, CREG_TIMEOUT); // don't fail as service could be not subscribed

        // check PSD registration (GPRS)
        sendFormated("AT+CGREG?\r\n");
        waitFinalResp(nullptr, nullptr, CGREG_TIMEOUT); // don't fail as service could be not subscribed
    }
    if (REG_OK(_net.csd) || REG_OK(_net.psd) || REG_OK(_net.eps)) {
        // get the current operator and radio access technology we are connected to
        if (!_atOk()) {
            goto failure;
        }
        // Reformat the operator string to be numeric
        // (allows the capture of `mcc` and `mnc`)
        sendFormated("AT+COPS=3,2\r\n");
        if (RESP_OK != waitFinalResp(nullptr, nullptr, COPS_TIMEOUT)) {
            goto failure;
        }
        sendFormated("AT+COPS?\r\n");
        if (RESP_OK != waitFinalResp(_cbCOPS, &_net, COPS_TIMEOUT)) {
            goto failure;
        }
        // get the signal strength indication
        sendFormated("AT+CSQ\r\n");
        if (RESP_OK != waitFinalResp(_cbCSQ, &_net, CSQ_TIMEOUT)) {
            goto failure;
        }
        // +CREG, +CGREG, +COPS do not contain <AcT> for G350 devices.
        // Force _net.act to ACT_GSM to ensure Device Diagnostics and
        // RSSI() API's have a RAT for conversion lookup purposes.
        if (_dev.dev == DEV_SARA_G350) {
            _net.act = ACT_GSM;
        }
    }
    if (status) {
        memcpy(status, &_net, sizeof(NetStatus));
    }
    // don't return true until fully registered
    if (_dev.dev == DEV_SARA_R410) {
        ok = REG_OK(_net.eps);
        if (_memoryIssuePresent && ok) {
            // start registered timer for memory issue power off delays.
            _timeRegistered = HAL_Timer_Get_Milli_Seconds();
        }
    } else {
        ok = REG_OK(_net.csd) && REG_OK(_net.psd);
    }
    UNLOCK();
    return ok;
failure:
    UNLOCK();
    return false;
}

bool MDMParser::getSignalStrength(NetStatus &status)
{
    bool ok = false;
    LOCK();
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::getSignalStrength ] = = = = = = = = = =");

        // Reformat the operator string to be numeric
        // (allows the capture of `mcc` and `mnc`)
        sendFormated("AT+COPS=3,2\r\n");
        if (RESP_OK != waitFinalResp(nullptr, nullptr, COPS_TIMEOUT))
            goto cleanup;

        // We do receive RAT changes asynchronously via CREG URCs, but
        // just in case we'll update it here explicitly.
        sendFormated("AT+COPS?\r\n");
        if (RESP_OK != waitFinalResp(_cbCOPS, &_net, COPS_TIMEOUT)) {
            goto cleanup;
        }

        // +CREG, +CGREG, +COPS do not contain <AcT> for G350 devices.
        // Force _net.act to ACT_GSM to ensure Device Diagnostics and
        // RSSI() API's have a RAT for conversion lookup purposes.
        if (_dev.dev == DEV_SARA_G350) {
            _net.act = ACT_GSM;
        }

        sendFormated("AT+CSQ\r\n");
        if (RESP_OK == waitFinalResp(_cbCSQ, &_net, CSQ_TIMEOUT)) {
            ok = true;
            status = _net;
        }
    }

cleanup:
    UNLOCK();
    return ok;
}

bool MDMParser::getDataUsage(MDM_DataUsage &data)
{
    bool ok = false;
    LOCK();
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::getDataUsage ] = = = = = = = = = =");
        sendFormated("AT+UGCNTRD\r\n");
        if (RESP_OK == waitFinalResp(_cbUGCNTRD, &_data_usage)) {
            ok = true;
            data.cid = _data_usage.cid;
            data.tx_session = _data_usage.tx_session;
            data.rx_session = _data_usage.rx_session;
            data.tx_total = _data_usage.tx_total;
            data.rx_total = _data_usage.rx_total;
        }
    }
    UNLOCK();
    return ok;
}

bool MDMParser::getCellularGlobalIdentity(CellularGlobalIdentity& cgi_) {
    LOCK();

    // Reformat the operator string to be numeric (allows the capture of `mcc` and `mnc`)
    sendFormated("AT+COPS=3,2\r\n");
    if (RESP_OK != waitFinalResp(nullptr, nullptr, COPS_TIMEOUT))
        goto failure;

    // We receive `lac` and `ci` changes asynchronously via CREG URCs, however we need to explicitly
    // update the `mcc` and `mnc` to confirm the operator has not changed.
    sendFormated("AT+COPS?\r\n");
    if (RESP_OK != waitFinalResp(_cbCOPS, &_net, COPS_TIMEOUT))
        goto failure;

    switch (cgi_.version)
    {
    case CGI_VERSION_1:
    default:
    {
        // Confirm user is expecting the correct amount of data
        if (cgi_.size < sizeof(_net.cgi))
            goto failure;

        cgi_ = _net.cgi;
        cgi_.size = sizeof(_net.cgi);
        cgi_.version = CGI_VERSION_1;
        break;
    }
    }

    // +CREG, +CGREG, +COPS do not contain <AcT> for G350 devices.
    // Force _net.act to ACT_GSM to ensure Device Diagnostics and
    // RSSI() API's have a RAT for conversion lookup purposes.
    if (_dev.dev == DEV_SARA_G350) {
        _net.act = ACT_GSM;
    }

    // CGI value has been updated successfully
    UNLOCK();
    return true;
failure:
    UNLOCK();
    return false;
}

void MDMParser::_setBandSelectString(MDM_BandSelect &data, char* bands, int index /*= 0*/) {
    char band[5];
    for (int x=index; x<data.count; x++) {
        sprintf(band, "%d", data.band[x]);
        strcat(bands, band);
        if ((x+1) < data.count) strcat(bands, ",");
    }
}

bool MDMParser::setBandSelect(MDM_BandSelect &data)
{
    bool ok = false;
    LOCK();
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::setBandSelect ] = = = = = = = = = =");

        char bands_to_set[22] = "";
        _setBandSelectString(data, bands_to_set, 0);
        if (strcmp(bands_to_set,"") == 0)
            goto failure;

        // create default bands string
        MDM_BandSelect band_avail;
        if (!getBandAvailable(band_avail))
            goto failure;

        char band_defaults[22] = "";
        if (band_avail.band[0] == BAND_DEFAULT)
            _setBandSelectString(band_avail, band_defaults, 1);

        // create selected bands string
        MDM_BandSelect band_sel;
        if (!getBandSelect(band_sel))
            goto failure;

        char bands_selected[22] = "";
        _setBandSelectString(band_sel, bands_selected, 0);

        if (strcmp(bands_to_set, "0") == 0) {
            if (strcmp(bands_selected, band_defaults) == 0) {
                ok = true;
                goto success;
            }
        }

        if (strcmp(bands_selected, bands_to_set) != 0) {
            sendFormated("AT+UBANDSEL=%s\r\n", bands_to_set);
            if (RESP_OK == waitFinalResp(NULL,NULL,40000)) {
                ok = true;
            }
        }
        else {
            ok = true;
        }
    }
success:
    UNLOCK();
    return ok;
failure:
    UNLOCK();
    return false;
}

bool MDMParser::getBandSelect(MDM_BandSelect &data)
{
    bool ok = false;
    LOCK();
    if (_init && _pwr) {
        MDM_BandSelect data_sel;
        MDM_INFO("\r\n[ Modem::getBandSelect ] = = = = = = = = = =");
        sendFormated("AT+UBANDSEL?\r\n");
        if (RESP_OK == waitFinalResp(_cbBANDSEL, &data_sel)) {
            ok = true;
            memcpy(&data, &data_sel, sizeof(MDM_BandSelect));
        }
    }
    UNLOCK();
    return ok;
}

bool MDMParser::getBandAvailable(MDM_BandSelect &data)
{
    bool ok = false;
    LOCK();
    if (_init && _pwr) {
        MDM_BandSelect data_avail;
        MDM_INFO("\r\n[ Modem::getBandAvailable ] = = = = = = = = = =");
        sendFormated("AT+UBANDSEL=?\r\n");
        if (RESP_OK == waitFinalResp(_cbBANDAVAIL, &data_avail)) {
            ok = true;
            memcpy(&data, &data_avail, sizeof(MDM_BandSelect));
        }
    }
    UNLOCK();
    return ok;
}

int MDMParser::_cbUGCNTRD(int type, const char* buf, int len, MDM_DataUsage* data)
{
    if ((type == TYPE_PLUS) && data) {
        int a,b,c,d,e;
        // +UGCNTRD: 31,2704,1819,2724,1839\r\n
        // +UGCNTRD: <cid>,<tx_sess_bytes>,<rx_sess_bytes>,<tx_total_bytes>,<rx_total_bytes>
        if (sscanf(buf, "\r\n+UGCNTRD: %d,%d,%d,%d,%d\r\n", &a,&b,&c,&d,&e) == 5) {
            data->cid = a;
            data->tx_session = b;
            data->rx_session = c;
            data->tx_total = d;
            data->rx_total = e;
        }
    }
    return WAIT;
}

int MDMParser::_cbBANDAVAIL(int type, const char* buf, int len, MDM_BandSelect* data)
{
    if ((type == TYPE_PLUS) && data) {
        int c;
        int b[5];
        // \r\n+UBANDSEL: (0,850,900,1800,1900)\r\n
        if ((c = sscanf(buf, "\r\n+UBANDSEL: (%d,%d,%d,%d,%d)\r\n", &b[0],&b[1],&b[2],&b[3],&b[4])) > 0) {
            for (int i=0; i<c; i++) {
                data->band[i] = (MDM_Band)b[i];
            }
            data->count = c;
        }
    }
    return WAIT;
}

int MDMParser::_cbBANDSEL(int type, const char* buf, int len, MDM_BandSelect* data)
{
    if ((type == TYPE_PLUS) && data) {
        int c;
        int b[4];
        // \r\n+UBANDSEL: 850\r\n
        // \r\n+UBANDSEL: 850,1900\r\n
        if ((c = sscanf(buf, "\r\n+UBANDSEL: %d,%d,%d,%d\r\n", &b[0],&b[1],&b[2],&b[3])) > 0) {
            for (int i=0; i<c; i++) {
                data->band[i] = (MDM_Band)b[i];
            }
            data->count = c;
        }
    }
    return WAIT;
}

int MDMParser::_cbCOPS(int type, const char* buf, int len, NetStatus* status)
{
    if ((type == TYPE_PLUS) && status)
    {
        int act = 99;
        char mobileCountryCode[4] = {0};
        char mobileNetworkCode[4] = {0};

        // +COPS: <mode>[,<format>,<oper>[,<AcT>]]
        if (::sscanf(buf, "\r\n+COPS: %*d,%*d,\"%3[0-9]%3[0-9]\",%d", mobileCountryCode,
                   mobileNetworkCode, &act) >= 1)
        {
            // Preserve digit format data
            const int mnc_digits = ::strnlen(mobileNetworkCode, sizeof(mobileNetworkCode));
            if (2 == mnc_digits)
            {
                status->cgi.cgi_flags |= CGI_FLAG_TWO_DIGIT_MNC;
            }
            else
            {
                status->cgi.cgi_flags &= ~CGI_FLAG_TWO_DIGIT_MNC;
            }

            // `atoi` returns zero on error, which is an invalid `mcc` and `mnc`
            status->cgi.mobile_country_code = static_cast<uint16_t>(::atoi(mobileCountryCode));
            status->cgi.mobile_network_code = static_cast<uint16_t>(::atoi(mobileNetworkCode));

            status->act = toCellularAccessTechnology(act);
        }
    }
    return WAIT;
}

int MDMParser::_cbCNUM(int type, const char* buf, int len, char* num)
{
    if ((type == TYPE_PLUS) && num){
        int a;
        if ((sscanf(buf, "\r\n+CNUM: \"My Number\",\"%31[^\"]\",%d", num, &a) == 2) &&
            ((a == 129) || (a == 145))) {
        }
    }
    return WAIT;
}

int MDMParser::_cbCSQ(int type, const char* buf, int len, NetStatus* status)
{
    if ((type == TYPE_PLUS) && status){
        int a,b;
        char _qual[] = { 49, 43, 37, 25, 19, 13, 7, 0 }; // see 3GPP TS 45.008 [20] subclause 8.2.4
        // +CSQ: <rssi>,<qual>
        if (sscanf(buf, "\r\n+CSQ: %d,%d",&a,&b) == 2) {
            if (a != 99) status->rssi = -113 + 2*a;  // 0: -113 1: -111 ... 30: -53 dBm with 2 dBm steps
            if ((b != 99) && (b < (int)sizeof(_qual))) status->qual = _qual[b];  //

            switch (status->act) {
            case ACT_GSM:
            case ACT_EDGE:
                status->rxlev = (a != 99) ? (2 * a) : a;
                status->rxqual = b;
                break;
            case ACT_UTRAN:
                status->rscp = (a != 99) ? (status->rssi + 116) : 255;
                status->ecno = (b != 99) ? std::min((7 + (7 - b) * 6), 44) : 255;
                break;
            case ACT_LTE:
            case ACT_LTE_CAT_M1:
            case ACT_LTE_CAT_NB1:
                status->rsrp = (a != 99) ? (a * 97)/31 : 255; // [0,31] -> [0,97]
                status->rsrq = (b != 99) ? (b * 34)/7 : 255;  // [0, 7] -> [0,34]
                break;
            default:
                // Unknown access tecnhology
                status->asu = std::numeric_limits<int32_t>::min();
                status->aqual = std::numeric_limits<int32_t>::min();
                break;
            }
        }
    }
    return WAIT;
}

int MDMParser::_cbUACTIND(int type, const char* buf, int len, int* i)
{
    if ((type == TYPE_PLUS) && i){
        int a;
        if (sscanf(buf, "\r\n+UACTIND: %d", &a) == 1) {
            *i = a;
        }
    }
    return WAIT;
}

// ----------------------------------------------------------------
// setup the PDP context

bool MDMParser::pdp(const char* apn)
{
    bool ok = true;
    // bool is3G = _dev.dev == DEV_SARA_U260 || _dev.dev == DEV_SARA_U270;
    LOCK();
    if (_init && _pwr) {

// todo - refactor
// This is setting up an external PDP context, join() creates an internal one
// which is ultimately the one that's used by the system. So no need for this.
#if 0
        MDM_INFO("Modem::pdp\r\n");

        DEBUG_D("Define the PDP context 1 with PDP type \"IP\" and APN \"%s\"\r\n", apn);
        sendFormated("AT+CGDCONT=1,\"IP\",\"%s\"\r\n", apn);
        if (RESP_OK != waitFinalResp(NULL, NULL, 2000))
            goto failure;

        if (is3G) {
            MDM_INFO("Define a QoS profile for PDP context 1");
            /* with Traffic Class 3 (background),
             * maximum bit rate 64 kb/s both for UL and for DL, no Delivery Order requirements,
             * a maximum SDU size of 320 octets, an SDU error ratio of 10-4, a residual bit error
             * ratio of 10-5, delivery of erroneous SDUs allowed and Traffic Handling Priority 3.
             */
            sendFormated("AT+CGEQREQ=1,3,64,64,,,0,320,\"1E4\",\"1E5\",1,,3\r\n");
            if (RESP_OK != waitFinalResp(NULL, NULL, 2000))
                goto failure;
        }

        MDM_INFO("Activate PDP context 1...");
        sendFormated("AT+CGACT=1,1\r\n");
        if (RESP_OK != waitFinalResp(NULL, NULL, 20000)) {
            sendFormated("AT+CEER\r\n");
            waitFinalResp();

            MDM_INFO("Test PDP context 1 for non-zero IP address...");
            sendFormated("AT+CGPADDR=1\r\n");
            if (RESP_OK != waitFinalResp(NULL, NULL, 2000))

            MDM_INFO("Read the PDP contexts’ parameters...");
            sendFormated("AT+CGDCONT?\r\n");
            // +CGPADDR: 1, "99.88.111.88"
            if (RESP_OK != waitFinalResp(NULL, NULL, 2000))

            if (is3G) {
                MDM_INFO("Read the negotiated QoS profile for PDP context 1...");
                sendFormated("AT+CGEQNEG=1\r\n");
                goto failure;
            }
        }

        MDM_INFO("Test PDP context 1 for non-zero IP address...");
        sendFormated("AT+CGPADDR=1\r\n");
        if (RESP_OK != waitFinalResp(NULL, NULL, 2000))
            goto failure;

        MDM_INFO("Read the PDP contexts’ parameters...");
        sendFormated("AT+CGDCONT?\r\n");
        // +CGPADDR: 1, "99.88.111.88"
        if (RESP_OK != waitFinalResp(NULL, NULL, 2000))
            goto failure;

        if (is3G) {
            MDM_INFO("Read the negotiated QoS profile for PDP context 1...");
            sendFormated("AT+CGEQNEG=1\r\n");
            if (RESP_OK != waitFinalResp(NULL, NULL, 2000))
                goto failure;
        }

        _activated = true; // PDP
#endif
        UNLOCK();
        return ok;
    }
// failure:
    UNLOCK();
    return false;
}

// ----------------------------------------------------------------
// internet connection

MDM_IP MDMParser::join(const char* apn /*= NULL*/, const char* username /*= NULL*/,
                              const char* password /*= NULL*/, Auth auth /*= AUTH_DETECT*/)
{
    LOCK();
    if (_init && _pwr && _dev.dev != DEV_UNKNOWN) {
        MDM_INFO("\r\n[ Modem::join ] = = = = = = = = = = = = = = = =");
        _ip = NOIP;
        if (_dev.dev == DEV_SARA_R410) {
            // Get local IP address associated with the default profile
            sendFormated("AT+CGPADDR=%d\r\n", PDP_CONTEXT);
            if (waitFinalResp(_cbCGPADDR, &_ip) != RESP_OK) {
                goto failure;
            }
            // FIXME: The existing code seems to use `_activated` and `_attached` flags kind of interchangeably
            _activated = true;
        } else {
            int a = 0;
            bool force = false; // If we are already connected, don't force a reconnect.

            // perform GPRS attach
            sendFormated("AT+CGATT=1\r\n");
            if (RESP_OK != waitFinalResp(NULL,NULL,3*60*1000))
                goto failure;

            // Check the if the PSD profile is activated (a=1)
            sendFormated("AT+UPSND=" PROFILE ",8\r\n");
            if (RESP_OK != waitFinalResp(_cbUPSND, &a, UPSND_TIMEOUT))
                goto failure;
            if (a == 1) {
                _activated = true; // PDP activated
                if (force) {
                    // deactivate the PSD profile if it is already activated
                    sendFormated("AT+UPSDA=" PROFILE ",4\r\n");
                    if (RESP_OK != waitFinalResp(nullptr, nullptr, UPSDA_TIMEOUT))
                        goto failure;
                    a = 0;
                }
            }
            if (a == 0) {
                bool ok = false;
                _activated = false; // PDP deactived
                // try to lookup the apn settings from our local database by mccmnc
                const char* config = NULL;
                if (!apn && !username && !password)
                    config = apnconfig(_dev.imsi);

                // Set up the dynamic IP address assignment.
                sendFormated("AT+UPSD=" PROFILE ",7,\"0.0.0.0\"\r\n");
                if (RESP_OK != waitFinalResp(nullptr, nullptr, UPSD_TIMEOUT))
                    goto failure;

                do {
                    if (config) {
                        apn      = _APN_GET(config);
                        username = _APN_GET(config);
                        password = _APN_GET(config);
                        DEBUG_D("Testing APN Settings(\"%s\",\"%s\",\"%s\")\r\n", apn, username, password);
                    }
                    // Set up the APN
                    if (apn && *apn) {
                        sendFormated("AT+UPSD=" PROFILE ",1,\"%s\"\r\n", apn);
                        if (RESP_OK != waitFinalResp(nullptr, nullptr, UPSD_TIMEOUT))
                            goto failure;
                    }
                    if (username && *username) {
                        sendFormated("AT+UPSD=" PROFILE ",2,\"%s\"\r\n", username);
                        if (RESP_OK != waitFinalResp(nullptr, nullptr, UPSD_TIMEOUT))
                            goto failure;
                    }
                    if (password && *password) {
                        sendFormated("AT+UPSD=" PROFILE ",3,\"%s\"\r\n", password);
                        if (RESP_OK != waitFinalResp(nullptr, nullptr, UPSD_TIMEOUT))
                            goto failure;
                    }
                    // try different Authentication Protocols
                    // 0 = none
                    // 1 = PAP (Password Authentication Protocol)
                    // 2 = CHAP (Challenge Handshake Authentication Protocol)
                    for (int i = AUTH_NONE; i <= AUTH_CHAP && !ok; i ++) {
                        if ((auth == AUTH_DETECT) || (auth == i)) {
                            // Set up the Authentication Protocol
                            sendFormated("AT+UPSD=" PROFILE ",6,%d\r\n", i);
                            if (RESP_OK != waitFinalResp(nullptr, nullptr, UPSD_TIMEOUT))
                                goto failure;
                            // Activate the PSD profile and make connection
                            sendFormated("AT+UPSDA=" PROFILE ",3\r\n");
                            if (RESP_OK == waitFinalResp(nullptr, nullptr, UPSDA_TIMEOUT)) {
                                _activated = true; // PDP activated
                                ok = true;
                            }
                        }
                    }
                } while (!ok && config && *config); // maybe use next setting ?
                if (!ok) {
                    MDM_ERROR("Your modem APN/password/username may be wrong\r\n");
                    goto failure;
                }
            }
            //Get local IP address
            sendFormated("AT+UPSND=" PROFILE ",0\r\n");
            if (RESP_OK != waitFinalResp(_cbUPSND, &_ip, UPSND_TIMEOUT))
                goto failure;
            // Get the primary DNS server (logs only), don't fail on error.
            sendFormated("AT+UPSND=" PROFILE ",1\r\n");
            waitFinalResp(nullptr, nullptr, UPSND_TIMEOUT);
            // Get the secondary DNS server (logs only), don't fail on error.
            sendFormated("AT+UPSND=" PROFILE ",2\r\n");
            waitFinalResp(nullptr, nullptr, UPSND_TIMEOUT);
        }
        UNLOCK();
        _attached = true;  // GPRS
        return _ip;
    }
failure:
    UNLOCK();
    return NOIP;
}

int MDMParser::_cbUDOPN(int type, const char* buf, int len, char* mccmnc)
{
    if ((type == TYPE_PLUS) && mccmnc) {
        if (sscanf(buf, "\r\n+UDOPN: 0,\"%[^\"]\"", mccmnc) == 1)
            ;
    }
    return WAIT;
}

int MDMParser::_cbCGPADDR(int type, const char* buf, int len, MDM_IP* ip) {
    if (type == TYPE_PLUS && ip) {
        int cid, a, b, c, d;
        // +CGPADDR: <cid>,<PDP_addr>
        // TODO: IPv6
        if (sscanf(buf, "\r\n+CGPADDR: %d,%d.%d.%d.%d", &cid, &a, &b, &c, &d) == 5) {
            *ip = IPADR(a, b, c, d);
        }
    }
    return WAIT;
}

int MDMParser::_cbCGDCONT(int type, const char* buf, int len, CGDCONTparam* param) {
    if (type == TYPE_PLUS || type == TYPE_UNKNOWN) {
        buf = (const char*)memchr(buf, '+', len); // Skip leading new line characters
        if (buf) {
            int id;
            CGDCONTparam p = {};
            static_assert(sizeof(p.type) == 8 && sizeof(p.apn) == 32, "The format string below needs to be updated accordingly");
            if (sscanf(buf, "+CGDCONT: %d,\"%7[^,],\"%31[^,],", &id, p.type, p.apn) == 3 && id == PDP_CONTEXT) {
                p.type[strlen(p.type) - 1] = '\0'; // Trim trailing quote character
                p.apn[strlen(p.apn) - 1] = '\0';
                *param = p;
            }
        }
    }
    return WAIT;
}

int MDMParser::_cbCMIP(int type, const char* buf, int len, MDM_IP* ip)
{
    if ((type == TYPE_UNKNOWN) && ip) {
        int a,b,c,d;
        if (sscanf(buf, "\r\n" IPSTR, &a,&b,&c,&d) == 4)
            *ip = IPADR(a,b,c,d);
    }
    return WAIT;
}

int MDMParser::_cbUPSND(int type, const char* buf, int len, int* act)
{
    if ((type == TYPE_PLUS) && act) {
        if (sscanf(buf, "\r\n+UPSND: %*d,%*d,%d", act) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int MDMParser::_cbUPSND(int type, const char* buf, int len, MDM_IP* ip)
{
    if ((type == TYPE_PLUS) && ip) {
        int a,b,c,d;
        // +UPSND=<profile_id>,<param_tag>[,<dynamic_param_val>]
        if (sscanf(buf, "\r\n+UPSND: " PROFILE ",0,\"" IPSTR "\"", &a,&b,&c,&d) == 4)
            *ip = IPADR(a,b,c,d);
    }
    return WAIT;
}

int MDMParser::_cbUDNSRN(int type, const char* buf, int len, MDM_IP* ip)
{
    if ((type == TYPE_PLUS) && ip) {
        int a,b,c,d;
        if (sscanf(buf, "\r\n+UDNSRN: \"" IPSTR "\"", &a,&b,&c,&d) == 4)
            *ip = IPADR(a,b,c,d);
    }
    return WAIT;
}

bool MDMParser::reconnect(void)
{
    bool ok = false;
    LOCK();
    if (_activated) {
        MDM_INFO("\r\n[ Modem::reconnect ] = = = = = = = = = = = = = =");
        if (!_attached) {
            /* Activates the PDP context assoc. with this profile */
            /* If GPRS is detached, this will force a re-attach */
            sendFormated("AT+UPSDA=" PROFILE ",3\r\n");
            if (RESP_OK == waitFinalResp(nullptr, nullptr, UPSDA_TIMEOUT)) {

                //Get local IP address
                sendFormated("AT+UPSND=" PROFILE ",0\r\n");
                if (RESP_OK == waitFinalResp(_cbUPSND, &_ip, UPSND_TIMEOUT)) {
                    ok = true;
                    _attached = true;
                }
            }
        }
    }
    UNLOCK();
    return ok;
}

// TODO - refactor deactivate() and detach()
// deactivate() can be called before detach() but not vice versa or
// deactivate() will ERROR because its PDP context will already be
// deactivated.
// _attached and _activated flags are currently associated inversely
// to what's happening.  When refactoring, consider combining...
bool MDMParser::deactivate(void)
{
    bool ok = false;
    bool continue_cancel = false;
    LOCK();
    if (_attached) {
        if (_cancel_all_operations) {
            continue_cancel = true;
            resume(); // make sure we can use the AT parser
        }
        MDM_INFO("\r\n[ Modem::deactivate ] = = = = = = = = = = = = =");
        if (_ip != NOIP) {
            if (_dev.dev == DEV_SARA_R410) {
                // The default context cannot be deactivated
                _ip = NOIP;
                _attached = false;
                ok = true;
            } else {
                /* Deactivates the PDP context assoc. with this profile
                 * ensuring that no additional data is sent or received
                 * by the device. */
                sendFormated("AT+UPSDA=" PROFILE ",4\r\n");
                if (RESP_OK == waitFinalResp(nullptr, nullptr, UPSDA_TIMEOUT)) {
                    _ip = NOIP;
                    ok = true;
                    _attached = false;
                }
            }
        }
    }
    if (continue_cancel) cancel();
    UNLOCK();
    return ok;
}

bool MDMParser::detach(void)
{
    bool ok = false;
    bool continue_cancel = false;
    LOCK();
    if (_activated) {
        if (_cancel_all_operations) {
            continue_cancel = true;
            resume(); // make sure we can use the AT parser
        }
        MDM_INFO("\r\n[ Modem::detach ] = = = = = = = = = = = = = = =");
        if (_dev.dev == DEV_SARA_R410) {
            // TODO: There's no GPRS service in LTE, although the GRPS detach command still disables
            // the PSD connection. For now let's unregister from the network entirely, since the
            // behavior of the detach command in relation to LTE is not documented
            if (_atOk()) {
                sendFormated("AT+COPS=2,2\r\n");
                if (waitFinalResp(nullptr, nullptr, COPS_TIMEOUT) == RESP_OK) {
                    _activated = false;
                    ok = true;
                }
            }
        } else {
            // if (_ip != NOIP) {  // if we deactivate() first we won't have an IP
                /* Detach from the GPRS network and conserve network resources. */
                /* Any active PDP context will also be deactivated. */
                sendFormated("AT+CGATT=0\r\n");
                if (RESP_OK != waitFinalResp(NULL,NULL,3*60*1000)) {
                    ok = true;
                    _activated = false;
                }
            // }
        }
    }
    if (continue_cancel) cancel();
    UNLOCK();
    return ok;
}

MDM_IP MDMParser::gethostbyname(const char* host)
{
    MDM_IP ip = NOIP;
    LOCK();
    if (_dev.dev == DEV_SARA_R410) {
        // Current uBlox firmware (L0.0.00.00.05.05) doesn't support +UDNSRN command, so we have to
        // use our own DNS client
        particle::getHostByName(host, &ip);
    } else {
        int a,b,c,d;
        if (sscanf(host, IPSTR, &a,&b,&c,&d) == 4) {
            ip = IPADR(a,b,c,d);
        } else {
            sendFormated("AT+UDNSRN=0,\"%s\"\r\n", host);
            if (RESP_OK != waitFinalResp(_cbUDNSRN, &ip, 30*1000)) {
                ip = NOIP;
            }
        }
    }
    UNLOCK();
    return ip;
}

// ----------------------------------------------------------------
// sockets

int MDMParser::_cbUSOCR(int type, const char* buf, int len, int* handle)
{
    if ((type == TYPE_PLUS) && handle) {
        // +USOCR: socket
        if (sscanf(buf, "\r\n+USOCR: %d", handle) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int MDMParser::_cbUSOCTL(int type, const char* buf, int len, int* handle)
{
    if ((type == TYPE_PLUS) && handle) {
        // +USOCTL: socket,param_id,param_val
        if (sscanf(buf, "\r\n+USOCTL: %d,%*d,%*d", handle) == 1)
            /*nothing*/;
    }
    return WAIT;
}

/* Tries to close any currently unused socket handles */
int MDMParser::_socketCloseUnusedHandles() {
    bool ok = false;
    LOCK();

    for (int s = 0; s < NUMSOCKETS; s++) {
        // If this HANDLE is not found to be in use, try to close it
        if (_findSocket(s) == MDM_SOCKET_ERROR) {
            if (_socketCloseHandleIfOpen(s)) {
                ok = true; // If any actually close, return true
            }
        }
    }

    UNLOCK();
    return ok;
}

/* Tries to close the specified socket handle */
int MDMParser::_socketCloseHandleIfOpen(int socket_handle) {
    bool ok = false;
    LOCK();

    // Check if socket_handle is open
    // AT+USOCTL=0,1
    // +USOCTL: 0,1,0
    int handle = MDM_SOCKET_ERROR;
    sendFormated("AT+USOCTL=%d,1\r\n", socket_handle);
    if ((RESP_OK == waitFinalResp(_cbUSOCTL, &handle, USOCTL_TIMEOUT)) &&
        (handle != MDM_SOCKET_ERROR)) {
        DEBUG_D("Socket handle %d was open, now closing...\r\n", handle);
        // Close it if it's open
        // AT+USOCL=0
        // OK
        sendFormated("AT+USOCL=%d\r\n", handle);
        if (RESP_OK == waitFinalResp(nullptr, nullptr, USOCL_TIMEOUT)) {
            DEBUG_D("Socket handle %d was closed.\r\n", handle);
            ok = true;
        }
    }

    UNLOCK();
    return ok;
}

int MDMParser::_socketSocket(int socket, IpProtocol ipproto, int port)
{
    int rv = socket;
    LOCK();

    if (ipproto == MDM_IPPROTO_UDP) {
        // sending port can only be set on 2G/3G modules
        if (port != -1) {
            sendFormated("AT+USOCR=17,%d\r\n", port);
        } else {
            sendFormated("AT+USOCR=17\r\n");
        }
    } else /*(ipproto == MDM_IPPROTO_TCP)*/ {
        sendFormated("AT+USOCR=6\r\n");
    }
    int handle = MDM_SOCKET_ERROR;
    if ((RESP_OK == waitFinalResp(_cbUSOCR, &handle, USOCR_TIMEOUT)) &&
        (handle != MDM_SOCKET_ERROR)) {
        DEBUG_D("Socket %d: handle %d was created\r\n", socket, handle);
        _sockets[socket].handle     = handle;
        _sockets[socket].timeout_ms = TIMEOUT_BLOCKING;
        _sockets[socket].connected  = (ipproto == MDM_IPPROTO_UDP);
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = true;
    }
    else {
        rv = MDM_SOCKET_ERROR;
    }

    UNLOCK();
    return rv;
}

int MDMParser::socketSocket(IpProtocol ipproto, int port)
{
    int socket;
    LOCK();

    if (!_attached) {
        if (!reconnect()) {
            socket = MDM_SOCKET_ERROR;
        }
    }

    if (_attached && _atOk()) {
        // Check for any stale handles that are open on the modem but not currently associated
        // with a socket. These may occur after power cycling the STM32 with modem connected
        // or if a previous socket was not closed cleanly. socketFree() will unconditionally
        // free the socket even if the handle doesn't close on the modem.
        if (_socketCloseUnusedHandles())
        {
            DEBUG_D("%s: closed stale socket handle(s)\r\n", __func__);
        }

        // find an free socket
        socket = _findSocket(MDM_SOCKET_ERROR);
        DEBUG_D("socketSocket(%s)\r\n", (ipproto?"UDP":"TCP"));
        if (socket != MDM_SOCKET_ERROR) {
            int _socket = _socketSocket(socket, ipproto, port);
            if (_socket != MDM_SOCKET_ERROR) {
                socket = _socket;
            }
        }
    }
    UNLOCK();
    return socket;
}

bool MDMParser::socketConnect(int socket, const char * host, int port)
{
    MDM_IP ip = gethostbyname(host);
    if (ip == NOIP)
        return false;
    DEBUG_D("socketConnect(host: %s)\r\n", host);
    // connect to socket
    return socketConnect(socket, ip, port);
}

bool MDMParser::socketConnect(int socket, const MDM_IP& ip, int port)
{
    bool ok = false;
    LOCK();
    if (ISSOCKET(socket) && (!_sockets[socket].connected)) {
        // On the SARA R410M check EPS registration prior due to an issue
        // where the USOCO can lockup the modem if connection drops
        if (!_checkEpsReg()) {
            UNLOCK();
            return false;
        }
        DEBUG_D("socketConnect(%d,port:%d)\r\n", socket,port);
        sendFormated("AT+USOCO=%d,\"" IPSTR "\",%d\r\n", _sockets[socket].handle, IPNUM(ip), port);
        if (RESP_OK == waitFinalResp(nullptr, nullptr, USOCO_TIMEOUT)) {
            ok = _sockets[socket].connected = true;
        }
    }
    UNLOCK();
    return ok;
}

bool MDMParser::socketIsConnected(int socket)
{
    bool ok = false;
    LOCK();
    ok = ISSOCKET(socket) && _sockets[socket].connected;
    //DEBUG_D("socketIsConnected(%d) %s\r\n", socket, ok?"yes":"no");
    UNLOCK();
    return ok;
}

bool MDMParser::socketSetBlocking(int socket, system_tick_t timeout_ms)
{
    bool ok = false;
    LOCK();
    // DEBUG_D("socketSetBlocking(%d,%d)\r\n", socket,timeout_ms);
    if (ISSOCKET(socket)) {
        _sockets[socket].timeout_ms = timeout_ms;
        ok = true;
    }
    UNLOCK();
    return ok;
}

bool MDMParser::socketClose(int socket)
{
    bool ok = false;
    LOCK();
    if (ISSOCKET(socket)
        && (_sockets[socket].connected || _sockets[socket].open))
    {
        // On the SARA R410M check EPS registration prior due to an issue
        // where the USOCL can lockup the modem if connection drops and we
        // are trying to close a TCP socket (without using the async option).
        DEBUG_D("socketClose(%d)\r\n", socket);
        if (_checkEpsReg()) {
            sendFormated("AT+USOCL=%d\r\n", _sockets[socket].handle);
            if (RESP_ERROR == waitFinalResp(nullptr, nullptr, USOCL_TIMEOUT)) {
                if (_dev.dev != DEV_SARA_R410) {
                    sendFormated("AT+CEER\r\n"); // For logging visibility
                    waitFinalResp(nullptr, nullptr, CEER_TIMEOUT);
                }
            }
        }
        // Assume RESP_OK in most situations, and assume closed
        // already if we couldn't close it, even though this can
        // be false. Recovery added to socketSocket();
        _sockets[socket].connected = false;
        _sockets[socket].open = false;
        ok = true;
    }
    UNLOCK();
    return ok;
}

bool MDMParser::_socketFree(int socket)
{
    bool ok = false;
    LOCK();
    if ((socket >= 0) && (socket < NUMSOCKETS)) {
        if (_sockets[socket].handle != MDM_SOCKET_ERROR) {
            DEBUG_D("socketFree(%d)\r\n",  socket);
            _sockets[socket].handle     = MDM_SOCKET_ERROR;
            _sockets[socket].timeout_ms = TIMEOUT_BLOCKING;
            _sockets[socket].connected  = false;
            _sockets[socket].pending    = 0;
            _sockets[socket].open       = false;
        }
        ok = true;
    }
    UNLOCK();
    return ok; // only false if invalid socket
}

bool MDMParser::socketFree(int socket)
{
    // make sure it is closed
    socketClose(socket);
    return _socketFree(socket);
}

int MDMParser::socketSend(int socket, const char * buf, int len)
{
    //DEBUG_D("socketSend(%d,,%d)\r\n", socket,len);
#ifndef SOCKET_HEX_MODE
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk) {
            blk = cnt;
        }
        bool ok = false;
        {
            LOCK();
            if (ISSOCKET(socket)) {
                uint8_t retry_num = 0;
                int response = 0;
                do {
                    // On the SARA R410M check EPS registration prior due to an issue
                    // where the USOWR can lockup the modem if connection drops
                    if (!_checkEpsReg()) {
                        UNLOCK();
                        return MDM_SOCKET_ERROR;
                    }
                    sendFormated("AT+USOWR=%d,%d\r\n",_sockets[socket].handle,blk);
                    response = waitFinalResp(nullptr, nullptr, USOWR_TIMEOUT);
                    if (response == RESP_PROMPT) {
                        // HAL_Delay_Milliseconds(50);
                        send(buf, blk);
                        response = waitFinalResp(nullptr, nullptr, USOWR_TIMEOUT);
                        if (response == RESP_OK) {
                            ok = true;
                        }
                    } else if (response == RESP_OK && retry_num == 1) {
                        // 1st try no signal, command will timeout, during 2nd try if signal comes back
                        // modem will send data from first try and respond OK instead of PROMPT
                        ok = true;
                    }
                } while(!ok && retry_num++ < MDM_SOCKET_SEND_RETRIES);
            }
            UNLOCK();
        }
        if (!ok) {
            return MDM_SOCKET_ERROR;
        }
        buf += blk;
        cnt -= blk;
    }
    LOCK();
    if (ISSOCKET(socket) && (_sockets[socket].pending == 0)) {
        sendFormated("AT+USORD=%d,0\r\n", _sockets[socket].handle); // TCP
        waitFinalResp(nullptr, nullptr, USORD_TIMEOUT);
    }
    UNLOCK();
    return (len - cnt);
#else
    int bytesLeft = len;
    while (bytesLeft > 0) {
        LOCK();
        if (!ISSOCKET(socket)) {
            goto error;
        }
        // Maximum number of bytes that can be written via single +USOWR command
        size_t chunkSize = bytesLeft;
        if (chunkSize > USO_MAX_WRITE / 2) { // Half the maximum size in binary mode
            chunkSize = USO_MAX_WRITE / 2;
        }
        // Write command prefix
        char data[256]; // Hex data is written in chunks
        const int prefixSize = snprintf(data, sizeof(data), "AT+USOWR=%d,%u,\"", _sockets[socket].handle,
                (unsigned)chunkSize);
        if (prefixSize < 0 || prefixSize > (int)sizeof(data)) {
            goto error;
        }
        if (send(data, prefixSize) != prefixSize) {
            goto error;
        }
        // Write hex data
        do {
            size_t n = chunkSize;
            if (n > sizeof(data) / 2) {
                n = sizeof(data) / 2;
            }
            bytes2hexbuf_lower_case((const uint8_t*)buf, n, data);
            const size_t hexSize = n * 2;
            if (send(data, hexSize) != (int)hexSize) {
                goto error;
            }
            buf += n;
            bytesLeft -= n;
            chunkSize -= n;
        } while (chunkSize > 0);
        // Write command suffix
        if (!_atOk()) {
            goto error;
        }
        if (send("\"\r\n", 3) != 3) {
            goto error;
        }
        if (waitFinalResp(nullptr, nullptr, USOWR_TIMEOUT) != RESP_OK) {
            goto error;
        }
        UNLOCK();
    }
    return (len - bytesLeft);
error:
    UNLOCK();
    return MDM_SOCKET_ERROR;
#endif // defined(SOCKET_HEX_MODE)
}

int MDMParser::socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len)
{
    DEBUG_D("socketSendTo(%d," IPSTR ",%d,,%d)\r\n", socket,IPNUM(ip),port,len);
#ifndef SOCKET_HEX_MODE
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk) {
            blk = cnt;
        }
        bool ok = false;
        {
            LOCK();
            if (ISSOCKET(socket)) {
                uint8_t retry_num = 0;
                int response = 0;
                // On the SARA R410M check EPS registration prior due to an issue
                // where the USOST can lockup the modem if connection drops
                if (!_checkEpsReg()) {
                    UNLOCK();
                    return MDM_SOCKET_ERROR;
                }
                do {
                    sendFormated("AT+USOST=%d,\"" IPSTR "\",%d,%d\r\n",_sockets[socket].handle,IPNUM(ip),port,blk);
                    response = waitFinalResp(nullptr, nullptr, USOST_TIMEOUT);
                    if (response == RESP_PROMPT) {
                        // HAL_Delay_Milliseconds(50);
                        send(buf, blk);
                        response = waitFinalResp(nullptr, nullptr, USOST_TIMEOUT);
                        if (response == RESP_OK) {
                            ok = true;
                        }
                    } else if (response == RESP_OK && retry_num == 1) {
                        // 1st try no signal, command will timeout, during 2nd try if signal comes back
                        // modem will send data from first try and respond OK instead of PROMPT
                        ok = true;
                    }
                } while(!ok && retry_num++ < MDM_SOCKET_SEND_RETRIES);
            }
            UNLOCK();
        }
        if (!ok) {
            return MDM_SOCKET_ERROR;
        }
        buf += blk;
        cnt -= blk;
    }
    LOCK();
    if (ISSOCKET(socket) && (_sockets[socket].pending == 0)) {
        sendFormated("AT+USORF=%d,0\r\n", _sockets[socket].handle); // UDP
        waitFinalResp(nullptr, nullptr, USORF_TIMEOUT);
    }
    UNLOCK();
    return (len - cnt);
#else
    int bytesLeft = len;
    if (bytesLeft >= 0) {
        LOCK();
        if (!ISSOCKET(socket)) {
            goto error;
        }
        // Maximum number of bytes that can be written via +USOST command
        if (bytesLeft > USO_MAX_WRITE / 2) { // Half the maximum size in binary mode
            MDM_ERROR("Maximum UDP packet size exceeded");
            goto error;
        }
        // Write command prefix
        char data[256]; // Hex data is written in chunks
        const int prefixSize = snprintf(data, sizeof(data), "AT+USOST=%d,\"" IPSTR "\",%d,%d,\"",
                _sockets[socket].handle, IPNUM(ip), port, bytesLeft);
        if (prefixSize < 0 || prefixSize > (int)sizeof(data)) {
            goto error;
        }
        if (send(data, prefixSize) != prefixSize) {
            goto error;
        }
        // Write hex data
        do {
            size_t n = bytesLeft;
            if (n > sizeof(data) / 2) {
                n = sizeof(data) / 2;
            }
            bytes2hexbuf_lower_case((const uint8_t*)buf, n, data);
            const size_t hexSize = n * 2;
            if (send(data, hexSize) != (int)hexSize) {
                goto error;
            }
            buf += n;
            bytesLeft -= n;
        } while (bytesLeft > 0);
        // Write command suffix
        if (send("\"\r\n", 3) != 3) {
            goto error;
        }
        if (waitFinalResp(nullptr, nullptr, USOST_TIMEOUT) != RESP_OK) {
            goto error;
        }
        UNLOCK();
    }
    return (len - bytesLeft);
error:
    UNLOCK();
    return MDM_SOCKET_ERROR;
#endif // defined(SOCKET_HEX_MODE)
}

int MDMParser::socketReadable(int socket)
{
    int pending = MDM_SOCKET_ERROR;
    if (_cancel_all_operations)
            return MDM_SOCKET_ERROR;
    LOCK();
    if (ISSOCKET(socket) && _sockets[socket].connected) {
        // DEBUG_D("socketReadable(%d)\r\n", socket);
        // Allow to receive unsolicited commands.
        // Set to 10ms timeout to mimic previous response when waitFinalResp()
        // contained 10ms busy wait and we used a 0ms timeout value below.
        waitFinalResp(nullptr, nullptr, 10);
        if (_sockets[socket].connected)
           pending = _sockets[socket].pending;
    }
    UNLOCK();
    return pending;
}

int MDMParser::_cbUSORD(int type, const char* buf, int len, USORDparam* param)
{
#ifndef SOCKET_HEX_MODE
    if ((type == TYPE_PLUS) && param) {
        int sz, sk;
        if ((sscanf(buf, "\r\n+USORD: %d,%d,", &sk, &sz) == 2) &&
            (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
            memcpy(param->buf, &buf[len-1-sz], sz);
            param->len = sz;
        } else {
            param->len = 0;
        }
    }
    return WAIT;
#else
    if ((type == TYPE_UNKNOWN || type == TYPE_PLUS) && param) {
        int socket, size;
        if (sscanf(buf, "+USORD: %d,%d,", &socket, &size) == 2 &&
                buf[len - size * 2 - 2] == '\"' && buf[len - 1] == '\"') {
            particle::hexToBytes(buf + len - size * 2 - 1, param->buf, size);
            param->len = size;
        } else {
            param->len = 0;
        }
    }
    return WAIT;
#endif // defined(SOCKET_HEX_MODE)
}

int MDMParser::socketRecv(int socket, char* buf, int len)
{
    int cnt = 0;
/*
    DEBUG_D("socketRecv(%d,%d)\r\n", socket, len);
#ifdef MDM_DEBUG
    memset(buf, '\0', len);
#endif
*/
    system_tick_t start = HAL_Timer_Get_Milli_Seconds();
    while (len) {
        // DEBUG_D("socketRecv: LEN: %d\r\n", len);
#ifndef SOCKET_HEX_MODE
        int blk = MAX_SIZE; // still need space for headers and unsolicited  commands
#else
        int blk = MAX_SIZE / 4;
#endif
        if (len < blk) blk = len;
        bool ok = false;
        {
            LOCK();
            if (ISSOCKET(socket)) {
                if (_sockets[socket].connected) {
                    int available = socketReadable(socket);
                    if (available<0)  {
                        // DEBUG_D("socketRecv: SOCKET CLOSED or NO AVAIL DATA\r\n");
                        // Socket may have been closed remotely during read, or no more data to read.
                        // Zero the `len` to break out of the while(len), and set `ok` to true so
                        // we return the `cnt` recv'd up until the socket was closed.
                        len = 0;
                        ok = true;
                    }
                    else
                    {
                        if (blk > available)    // only read up to the amount available. When 0,
                            blk = available;// skip reading and check timeout.
                        if (blk > 0) {
                            // DEBUG_D("socketRecv: _cbUSORD\r\n");
                            sendFormated("AT+USORD=%d,%d\r\n",_sockets[socket].handle, blk);
                            USORDparam param;
                            param.buf = buf;
                            if (RESP_OK == waitFinalResp(_cbUSORD, &param)) {
                                blk = param.len;
                                _sockets[socket].pending -= blk;
                                len -= blk;
                                cnt += blk;
                                buf += blk;
                                ok = true;
                            }
                        } else if (!TIMEOUT(start, _sockets[socket].timeout_ms)) {
                            // DEBUG_D("socketRecv: WAIT FOR URCs\r\n");
                            ok = (WAIT == waitFinalResp(NULL,NULL,0)); // wait for URCs
                        } else {
                            // DEBUG_D("socketRecv: TIMEOUT\r\n");
                            len = 0;
                            ok = true;
                        }
                    }
                } else {
                    // DEBUG_D("socketRecv: SOCKET NOT CONNECTED\r\n");
                    len = 0;
                    ok = true;
                }
            }
            UNLOCK();
        }
        if (!ok) {
            // DEBUG_D("socketRecv: ERROR\r\n");
            return MDM_SOCKET_ERROR;
        }
    }
    LOCK();
    if (ISSOCKET(socket) && (_sockets[socket].pending == 0)) {
        sendFormated("AT+USORD=%d,0\r\n", _sockets[socket].handle); // TCP
        waitFinalResp(NULL, NULL, 10*1000);
    }
    UNLOCK();
    // DEBUG_D("socketRecv: %d \"%*s\"\r\n", cnt, cnt, buf-cnt);
    return cnt;
}

int MDMParser::_cbUSORF(int type, const char* buf, int len, USORFparam* param)
{
#ifndef SOCKET_HEX_MODE
    if ((type == TYPE_PLUS) && param) {
        int sz, sk, p, a,b,c,d;
        int r;
        r = sscanf(buf, "\r\n\r\n+USORF: %d,\"" IPSTR "\",%d,%d,", &sk,&a,&b,&c,&d,&p,&sz);
        if ((r == 7) && (buf[len-sz-2-2] == '\"') && (buf[len-1-2] == '\"')) {
            memcpy(param->buf, &buf[len-1-sz-2], sz);
            param->ip = IPADR(a,b,c,d);
            param->port = p;
            param->len = sz;
        } else {
            r = sscanf(buf, "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,", &sk,&a,&b,&c,&d,&p,&sz);
            if ((r == 7) && (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
                memcpy(param->buf, &buf[len-1-sz], sz);
                param->ip = IPADR(a,b,c,d);
                param->port = p;
                param->len = sz;
            } else {
                param->len = 0;
            }
        }
    }
    return WAIT;
#else
    if ((type == TYPE_UNKNOWN || type == TYPE_PLUS) && param) {
        int socket, size, port, ip1, ip2, ip3, ip4;
        if (sscanf(buf, "+USORF: %d,\"" IPSTR "\",%d,%d,", &socket, &ip1, &ip2, &ip3, &ip4, &port, &size) == 7 &&
                buf[len - size * 2 - 2] == '\"' && buf[len - 1] == '\"') {
            particle::hexToBytes(buf + len - size * 2 - 1, param->buf, size);
            param->ip = IPADR(ip1, ip2, ip3, ip4);
            param->port = port;
            param->len = size;
        } else {
            param->len = 0;
        }
    }
    return WAIT;
#endif // defined(SOCKET_HEX_MODE)
}

int MDMParser::socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len)
{
    int cnt = 0;

    // DEBUG_D("socketRecvFrom(%d,,%d)\r\n", socket, len);
#ifdef MDM_DEBUG
    memset(buf, '\0', len);
#endif

    system_tick_t start = HAL_Timer_Get_Milli_Seconds();
    while (len) {
#ifndef SOCKET_HEX_MODE
        int blk = MAX_SIZE; // still need space for headers and unsolicited commands
#else
        int blk = MAX_SIZE / 4;
#endif
        if (len < blk) blk = len;
        bool ok = false;
        {
            LOCK();
            if (ISSOCKET(socket)) {
                if (blk > 0) {
                    sendFormated("AT+USORF=%d,%d\r\n",_sockets[socket].handle, blk);
                    USORFparam param;
                    param.buf = buf;
                    if (RESP_OK == waitFinalResp(_cbUSORF, &param)) {
                        *ip = param.ip;
                        *port = param.port;
                        blk = param.len;
                        _sockets[socket].pending -= blk;
                        len -= blk;
                        cnt += blk;
                        buf += blk;
                        len = 0; // done
                        ok = true;
                    }
                } else if (!TIMEOUT(start, _sockets[socket].timeout_ms)) {
                    ok = (WAIT == waitFinalResp(NULL,NULL,0)); // wait for URCs
                } else {
                    len = 0; // no more data and socket closed or timed-out
                    ok = true;
                }
            }
            UNLOCK();
        }
        if (!ok) {
            DEBUG_D("socketRecv: ERROR\r\n");
            return MDM_SOCKET_ERROR;
        }
    }
    LOCK();
    if (ISSOCKET(socket) && (_sockets[socket].pending == 0)) {
        sendFormated("AT+USORF=%d,0\r\n", _sockets[socket].handle); // UDP
        waitFinalResp(NULL, NULL, 10*1000);
    }
    UNLOCK();
    // DEBUG_D("socketRecv: %d \"%*s\"\r\n", cnt, cnt, buf-cnt);
    return cnt;
}

int MDMParser::_findSocket(int handle) {
    for (int socket = 0; socket < NUMSOCKETS; socket ++) {
        if (_sockets[socket].handle == handle)
            return socket;
    }
    return MDM_SOCKET_ERROR;
}

// ----------------------------------------------------------------

int MDMParser::_cbCMGL(int type, const char* buf, int len, CMGLparam* param)
{
    if ((type == TYPE_PLUS) && param && param->num) {
        // +CMGL: <ix>,...
        int ix;
        if (sscanf(buf, "\r\n+CMGL: %d,", &ix) == 1)
        {
            *param->ix++ = ix;
            param->num--;
        }
    }
    return WAIT;
}

int MDMParser::smsList(const char* stat /*= "ALL"*/, int* ix /*=NULL*/, int num /*= 0*/) {
    int ret = -1;
    LOCK();
    sendFormated("AT+CMGL=\"%s\"\r\n", stat);
    CMGLparam param;
    param.ix = ix;
    param.num = num;
    if (RESP_OK == waitFinalResp(_cbCMGL, &param))
        ret = num - param.num;
    UNLOCK();
    return ret;
}

bool MDMParser::smsSend(const char* num, const char* buf)
{
    bool ok = false;
    LOCK();
    sendFormated("AT+CMGS=\"%s\"\r\n",num);
    if (RESP_PROMPT == waitFinalResp(NULL,NULL,150*1000)) {
        send(buf, strlen(buf));
        const char ctrlZ = 0x1A;
        send(&ctrlZ, sizeof(ctrlZ));
        ok = (RESP_OK == waitFinalResp());
    }
    UNLOCK();
    return ok;
}

bool MDMParser::smsDelete(int ix)
{
    bool ok = false;
    LOCK();
    sendFormated("AT+CMGD=%d\r\n",ix);
    ok = (RESP_OK == waitFinalResp());
    UNLOCK();
    return ok;
}

int MDMParser::_cbCMGR(int type, const char* buf, int len, CMGRparam* param)
{
    if (param) {
        if (type == TYPE_PLUS) {
            if (sscanf(buf, "\r\n+CMGR: \"%*[^\"]\",\"%[^\"]", param->num) == 1) {
            }
        } else if ((type == TYPE_UNKNOWN) && (buf[len-2] == '\r') && (buf[len-1] == '\n')) {
            memcpy(param->buf, buf, len-2);
            param->buf[len-2] = '\0';
        }
    }
    return WAIT;
}

bool MDMParser::smsRead(int ix, char* num, char* buf, int len)
{
    bool ok = false;
    LOCK();
    CMGRparam param;
    param.num = num;
    param.buf = buf;
    sendFormated("AT+CMGR=%d\r\n",ix);
    ok = (RESP_OK == waitFinalResp(_cbCMGR, &param));
    UNLOCK();
    return ok;
}

// ----------------------------------------------------------------

int MDMParser::_cbCUSD(int type, const char* buf, int len, char* resp)
{
    if ((type == TYPE_PLUS) && resp) {
        // +USD: \"%*[^\"]\",\"%[^\"]\",,\"%*[^\"]\",%d,%d,%d,%d,\"*[^\"]\",%d,%d"..);
        if (sscanf(buf, "\r\n+CUSD: %*d,\"%[^\"]\",%*d", resp) == 1) {
            /*nothing*/
        }
    }
    return WAIT;
}

bool MDMParser::ussdCommand(const char* cmd, char* buf)
{
    bool ok = false;
    LOCK();
    *buf = '\0';
    // 2G/3G devices only
    sendFormated("AT+CUSD=1,\"%s\"\r\n",cmd);
    ok = (RESP_OK == waitFinalResp(_cbCUSD, buf));
    UNLOCK();
    return ok;
}

// ----------------------------------------------------------------

int MDMParser::_cbUDELFILE(int type, const char* buf, int len, void*)
{
    if ((type == TYPE_ERROR) && strstr(buf, "+CME ERROR: FILE NOT FOUND"))
        return RESP_OK; // file does not exist, so all ok...
    return WAIT;
}

bool MDMParser::delFile(const char* filename)
{
    bool ok = false;
    LOCK();
    sendFormated("AT+UDELFILE=\"%s\"\r\n", filename);
    ok = (RESP_OK == waitFinalResp(_cbUDELFILE));
    UNLOCK();
    return ok;
}

int MDMParser::writeFile(const char* filename, const char* buf, int len)
{
    bool ok = false;
    LOCK();
    sendFormated("AT+UDWNFILE=\"%s\",%d\r\n", filename, len);
    if (RESP_PROMPT == waitFinalResp()) {
        send(buf, len);
        ok = (RESP_OK == waitFinalResp());
    }
    UNLOCK();
    return ok ? len : -1;
}

int MDMParser::readFile(const char* filename, char* buf, int len)
{
    URDFILEparam param;
    param.filename = filename;
    param.buf = buf;
    param.sz = len;
    param.len = 0;
    LOCK();
    sendFormated("AT+URDFILE=\"%s\"\r\n", filename, len);
    if (RESP_OK != waitFinalResp(_cbURDFILE, &param))
        param.len = -1;
    UNLOCK();
    return param.len;
}

int MDMParser::_cbURDFILE(int type, const char* buf, int len, URDFILEparam* param)
{
    if ((type == TYPE_PLUS) && param && param->filename && param->buf) {
        char filename[48];
        int sz;
        if ((sscanf(buf, "\r\n+URDFILE: \"%[^\"]\",%d,", filename, &sz) == 2) &&
            (0 == strcmp(param->filename, filename)) &&
            (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
            param->len = (sz < param->sz) ? sz : param->sz;
            memcpy(param->buf, &buf[len-1-sz], param->len);
        }
    }
    return WAIT;
}

// ----------------------------------------------------------------
bool MDMParser::setDebug(int level)
{
#ifdef MDM_DEBUG
    if ((_debugLevel >= -1) && (level >= -1) &&
        (_debugLevel <=  3) && (level <=  3)) {
        _debugLevel = level;
        return true;
    }
#endif
    return false;
}

void MDMParser::dumpDevStatus(DevStatus* status)
{
    MDM_INFO("\r\n[ Modem::devStatus ] = = = = = = = = = = = = = =");
    const char* txtDev[] = { "Unknown", "SARA-G350", "LISA-U200", "LISA-C200", "SARA-U260",
                                "SARA-U270", "LEON-G200", "SARA-U201", "SARA-R410" };
    if (status->dev < sizeof(txtDev)/sizeof(*txtDev) && (status->dev != DEV_UNKNOWN))
        DEBUG_D("  Device:       %s\r\n", txtDev[status->dev]);
    const char* txtLpm[] = { "Disabled", "Enabled", "Active" };
    if (status->lpm < sizeof(txtLpm)/sizeof(*txtLpm))
        DEBUG_D("  Power Save:   %s\r\n", txtLpm[status->lpm]);
    const char* txtSim[] = { "Unknown", "Missing", "Pin", "Ready" };
    if (status->sim < sizeof(txtSim)/sizeof(*txtSim) && (status->sim != SIM_UNKNOWN))
        DEBUG_D("  SIM:          %s\r\n", txtSim[status->sim]);
    if (*status->ccid)
        DEBUG_D("  CCID:         %s\r\n", status->ccid);
    if (*status->imei)
        DEBUG_D("  IMEI:         %s\r\n", status->imei);
    if (*status->imsi)
        DEBUG_D("  IMSI:         %s\r\n", status->imsi);
    if (*status->meid)
        DEBUG_D("  MEID:         %s\r\n", status->meid); // LISA-C
    if (*status->manu)
        DEBUG_D("  Manufacturer: %s\r\n", status->manu);
    if (*status->model)
        DEBUG_D("  Model:        %s\r\n", status->model);
    if (*status->ver)
        DEBUG_D("  Version:      %s\r\n", status->ver);
}

void MDMParser::dumpNetStatus(NetStatus *status)
{
    MDM_INFO("\r\n[ Modem::netStatus ] = = = = = = = = = = = = = =");
    const char* txtReg[] = { "Unknown", "Denied", "None", "Home", "Roaming" };
    if (status->csd < sizeof(txtReg)/sizeof(*txtReg) && (status->csd != REG_UNKNOWN))
        DEBUG_D("  CSD Registration:    %s\r\n", txtReg[status->csd]);
    if (status->psd < sizeof(txtReg)/sizeof(*txtReg) && (status->psd != REG_UNKNOWN))
        DEBUG_D("  PSD Registration:    %s\r\n", txtReg[status->psd]);
    const char* txtAct[] = { "Unknown", "GSM", "Edge", "3G", "CDMA" };
    if (status->act < sizeof(txtAct)/sizeof(*txtAct) && (status->act != ACT_UNKNOWN))
        DEBUG_D("  Access Technology:   %s\r\n", txtAct[status->act]);
    if (status->rssi)
        DEBUG_D("  Signal Strength:     %d dBm\r\n", status->rssi);
    if (status->qual)
        DEBUG_D("  Signal Quality:      %d\r\n", status->qual);
    if (status->cgi.mobile_country_code != 0)
        DEBUG_D("  Mobile Country Code: %d\r\n", status->cgi.mobile_country_code);
    if (status->cgi.mobile_network_code != 0)
    {
        if (CGI_FLAG_TWO_DIGIT_MNC & status->cgi.cgi_flags)
            DEBUG_D("  Mobile Network Code: %02d\r\n", status->cgi.mobile_network_code);
        else
            DEBUG_D("  Mobile Network Code: %03d\r\n", status->cgi.mobile_network_code);
    }
    if (status->cgi.location_area_code != 0xFFFF)
        DEBUG_D("  Location Area Code:  %04X\r\n", status->cgi.location_area_code);
    if (status->cgi.cell_id != 0xFFFFFFFF)
        DEBUG_D("  Cell ID:             %08X\r\n", status->cgi.cell_id);
    if (*status->num)
        DEBUG_D("  Phone Number:        %s\r\n", status->num);
}

void MDMParser::dumpIp(MDM_IP ip)
{
    if (ip != NOIP) {
        DEBUG_D("\r\n[ Modem:IP " IPSTR " ] = = = = = = = = = = = = = =\r\n", IPNUM(ip));
    }
}

// ----------------------------------------------------------------
int MDMParser::_parseMatch(Pipe<char>* pipe, int len, const char* sta, const char* end)
{
    int o = 0;
    if (sta) {
        while (*sta) {
            if (++o > len)                  return WAIT;
            char ch = pipe->next();
            if (*sta++ != ch)               return NOT_FOUND;
        }
    }
    if (!end)                               return o; // no termination
    // at least any char
    if (++o > len)                      return WAIT;
    pipe->next();
    // check the end
    int x = 0;
    while (end[x]) {
        if (++o > len)                      return WAIT;
        char ch = pipe->next();
        x = (end[x] == ch) ? x + 1 :
            (end[0] == ch) ? 1 :
                            0;
    }
    return o;
}

int MDMParser::_parseFormated(Pipe<char>* pipe, int len, const char* fmt)
{
    int o = 0;
    int num = 0;
    if (fmt) {
        while (*fmt) {
            if (++o > len)                  return WAIT;
            char ch = pipe->next();
            if (*fmt == '%') {
                fmt++;
                if (*fmt == 'd') { // numeric
                    fmt ++;
                    num = 0;
                    while (ch >= '0' && ch <= '9') {
                        num = num * 10 + (ch - '0');
                        if (++o > len)      return WAIT;
                        ch = pipe->next();
                    }
                }
                else if (*fmt == 'c') { // char buffer (takes last numeric as length)
                    fmt ++;
                    while (num --) {
                        if (++o > len)      return WAIT;
                        ch = pipe->next();
                    }
                }
                else if (*fmt == 's') {
                    fmt ++;
                    if (ch != '\"')         return NOT_FOUND;
                    do {
                        if (++o > len)      return WAIT;
                        ch = pipe->next();
                    } while (ch != '\"');
                    if (++o > len)          return WAIT;
                    ch = pipe->next();
                }
            }
            if (*fmt++ != ch)               return NOT_FOUND;
        }
    }
    return o;
}

int MDMParser::_getLine(Pipe<char>* pipe, char* buf, int len)
{
    int unkn = 0;
    int sz = pipe->size();
    int fr = pipe->free();
    if (len > sz)
        len = sz;
    while (len > 0)
    {
        static struct {
              const char* fmt;                              int type;
        } lutF[] = {
            { "\r\n+USORD: %d,%d,\"%c\"",                   TYPE_PLUS       },
            { "\r\n\r\n+USORF: %d,\"" IPSTR "\",%d,%d,\"%c\"\r\n",  TYPE_PLUS }, // R410 firmware L0.0.00.00.05.08,A.02.04
            { "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,\"%c\"\r\n",  TYPE_PLUS     },
            { "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,\"%c\"",  TYPE_PLUS       },
            { "\r\n+URDFILE: %s,%d,\"%c\"",                 TYPE_PLUS       },
        };
        static struct {
              const char* sta;          const char* end;    int type;
        } lut[] = {
            { "\r\n\r\r\nOK\r\n",       NULL,               TYPE_OK         }, // R410 firmware L0.0.00.00.05.08,A.02.04
            { "\r\nOK\r\n",             NULL,               TYPE_OK         },
            { "OK\r\n",                 NULL,               TYPE_OK         }, // Necessary to clean up after TYPE_USORF_1 is parsed
            { "\r\nERROR\r\n",          NULL,               TYPE_ERROR      },
            { "\r\n+CME ERROR:",        "\r\n",             TYPE_ERROR      },
            { "\r\n+CMS ERROR:",        "\r\n",             TYPE_ERROR      },
            { "\r\nRING\r\n",           NULL,               TYPE_RING       },
            { "\r\nCONNECT\r\n",        NULL,               TYPE_CONNECT    },
            { "\r\nNO CARRIER\r\n",     NULL,               TYPE_NOCARRIER  },
            { "\r\nNO DIALTONE\r\n",    NULL,               TYPE_NODIALTONE },
            { "\r\nBUSY\r\n",           NULL,               TYPE_BUSY       },
            { "\r\nNO ANSWER\r\n",      NULL,               TYPE_NOANSWER   },
            { "\r\r\n\r\r\n+USORF:",    NULL,               TYPE_USORF_1    }, // R410 firmware L0.0.00.00.05.08,A.02.04
            { "\r\n+",                  "\r\n",             TYPE_PLUS       },
            { "\r\n@",                  NULL,               TYPE_PROMPT     }, // Sockets
            { "\r\n>",                  NULL,               TYPE_PROMPT     }, // SMS
            { "\n>",                    NULL,               TYPE_PROMPT     }, // File
            { "\r\nABORTED\r\n",        NULL,               TYPE_ABORTED    }, // Current command aborted
            { "\r\n\r\n",               NULL,               TYPE_DBLNEWLINE }, // Double CRLF detected, R410 firmware L0.0.00.00.05.07,A.02.02
            { "\r\n",                   "\r\n",             TYPE_UNKNOWN    }, // If all else fails, break up generic strings
        };
        for (int i = 0; i < (int)(sizeof(lutF)/sizeof(*lutF)); i ++) {
            pipe->set(unkn);
#ifdef MDM_DEBUG_RX_PIPE
            // electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
            // DEBUG_D("lutF [%d] len:%d sz:%d sz now:%d\r\n", i, len, sz, pipe->size());
            int ln = _parseFormated(pipe, len, lutF[i].fmt);
            if (ln == WAIT && fr) {
                // DEBUG_D("lutF ln == WAIT len:%d\r\n", len);
#ifdef MDM_DEBUG_RX_PIPE
                // electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
                return WAIT;
            }
            if ((ln != NOT_FOUND) && (unkn > 0)) {
                // DEBUG_D("lutF ln != NOT_FOUND (%d) len:%d\r\n", i, len);
#ifdef MDM_DEBUG_RX_PIPE
                // electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
                return TYPE_UNKNOWN | pipe->get(buf, unkn);
            }
            if (ln > 0) {
                // DEBUG_D("lutF matched (%d)\r\n", i);
                return lutF[i].type  | pipe->get(buf, ln);
            }
        }
        for (int i = 0; i < (int)(sizeof(lut)/sizeof(*lut)); i ++) {
            pipe->set(unkn);
#ifdef MDM_DEBUG_RX_PIPE
            // electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
            // DEBUG_D("lut [%d] len:%d sz:%d sz now:%d\r\n", i, len, sz, pipe->size());
            int ln = _parseMatch(pipe, len, lut[i].sta, lut[i].end);
            if (ln == WAIT && fr) {
                // DEBUG_D("lut ln == WAIT len:%d\r\n", len);
#ifdef MDM_DEBUG_RX_PIPE
                // electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
                return WAIT;
            }

            // Double CRLF detected, discard these two bytes
            // This resolves a case on R410 where double "\r\n" is generated after +CME ERROR response specifically
            // following a USOST command, but missing on U260/U270, which would otherwise generate "\r\n\r\n@" prompt
            // which is not parseable.  We do it this way because it's safer to detect and discard these instead
            // of parsing CME ERROR with double CRLF endings because that can strip CRLF from the beginning of
            // some URCs which make them unparseable.
            //
            // This also fixes the previous TYPE_DBLNEWLINE usage:
            // Double CRLF detected, discard these two bytes
            // This resolves a case on G350 where "\r\n" is generated after +USORF response, but missing
            // on U260/U270, which would otherwise generate "\r\n\r\nOK\r\n" which is not parseable.
            if ((ln > 0) && (lut[i].type == TYPE_DBLNEWLINE) && (unkn == 0)) {
                // DEBUG_D("lut TYPE_DBLNEWLINE\r\n");
                return TYPE_UNKNOWN | pipe->get(buf, 2);
            }

            // Double CRLF and CR detected, discard these 4 bytes
            // This resolves a case on R410 firmware L0.0.00.00.05.08,A.02.04 where "\r\r\n\r" is generated
            // before a "\r\n+USORF:" response, which would otherwise generate "\r\r\n\r\r\n+USORF:" which is not parseable.
            if ((ln > 0) && (lut[i].type == TYPE_USORF_1) && (unkn == 0)) {
                // DEBUG_D("lut TYPE_USORF_1\r\n");
                return TYPE_UNKNOWN | pipe->get(buf, 4);
            }

            if ((ln != NOT_FOUND) && (unkn > 0)) {
                // DEBUG_D("lut ln != NOT_FOUND (%d) len:%d\r\n", i, len);
#ifdef MDM_DEBUG_RX_PIPE
                // electronMDM.rxDump();
#endif // MDM_DEBUG_RX_PIPE
                return TYPE_UNKNOWN | pipe->get(buf, unkn);
            }

            if (ln > 0) {
                // DEBUG_D("lut matched (%d)\r\n", i);
                return lut[i].type | pipe->get(buf, ln);
            }
        }
        // UNKNOWN
        unkn ++;
        len--;
    }
    return WAIT;
}

// ----------------------------------------------------------------
// Electron Serial Implementation
// ----------------------------------------------------------------

MDMElectronSerial::MDMElectronSerial(int rxSize /*= 256*/, int txSize /*= 256*/) :
    ElectronSerialPipe(rxSize, txSize)
{
#ifdef MDM_DEBUG
        //_debugLevel = -1;
#endif

// Important to set _dev.lpm = LPM_ENABLED; when HW FLOW CONTROL enabled.
}

MDMElectronSerial::~MDMElectronSerial(void)
{
    powerOff();
}

int MDMElectronSerial::_send(const void* buf, int len)
{
    return put((const char*)buf, len, true/*=blocking*/);
}

int MDMElectronSerial::getLine(char* buffer, int length)
{
    int ret = _getLine(&_pipeRx, buffer, length);
    rxResume();
    return ret;
}

void MDMElectronSerial::pause()
{
    LOCK();
    rxPause();
}

void MDMElectronSerial::resumeRecv()
{
    LOCK();
    rxResume();
}

#endif // !defined(HAL_CELLULAR_EXCLUDE)


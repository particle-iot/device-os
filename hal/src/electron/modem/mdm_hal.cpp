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

/* Includes -----------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "mdm_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "mdmapn_hal.h"
#include "stm32f2xx.h"
#include "service_debug.h"
#include "concurrent_hal.h"
#include <mutex>
#include "net_hal.h"

std::recursive_mutex mdm_mutex;

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/

#define PROFILE         "0"   //!< this is the psd profile used
#define MAX_SIZE        1024  //!< max expected messages (used with RX)
#define USO_MAX_WRITE   1024  //!< maximum number of bytes to write to socket (used with TX)
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
        va_list args;
        va_start (args, format);
        if (color) DEBUG_D(color);
        DEBUG_D(format, args);
        if (color) DEBUG_D(DEF);
        va_end (args);
        DEBUG_D("\r\n");
    }
}
// Warning: Do not use these for anything other than constant char messages,
// they will yield incorrect values for integers.  Use DEBUG_D() instead.
#define MDM_ERROR(...)  do {_debugPrint(0, RED, __VA_ARGS__);}while(0)
#define MDM_INFO(...)   do {_debugPrint(1, GRE, __VA_ARGS__);}while(0)
#define MDM_TRACE(...)  do {_debugPrint(2, DEF, __VA_ARGS__);}while(0)
#define MDM_TEST(...)   do {_debugPrint(3, CYA, __VA_ARGS__);}while(0)

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
    _net.lac = 0xFFFF;
    _net.ci = 0xFFFFFFFF;
    _ip        = NOIP;
    _init      = false;
    _pwr       = false;
    _activated = false;
    _attached  = false;
    _attached_urc = false; // updated by GPRS detached/attached URC,
                           // used to notify system of prolonged GPRS detach.
    _cancel_all_operations = false;
    memset(_sockets, 0, sizeof(_sockets));
    for (int socket = 0; socket < NUMSOCKETS; socket ++)
        _sockets[socket].handle = MDM_SOCKET_ERROR;
#ifdef MDM_DEBUG
    _debugLevel = 3;
    _debugTime = HAL_Timer_Get_Milli_Seconds();
#endif
}

void MDMParser::cancel(void) {
    MDM_INFO("\r\n[ Modem::cancel ] = = = = = = = = = = = = = = =");
    _cancel_all_operations = true;
}

void MDMParser::resume(void) {
    MDM_INFO("\r\n[ Modem::resume ] = = = = = = = = = = = = = = =");
    _cancel_all_operations = false;
}

int MDMParser::send(const char* buf, int len)
{
#ifdef MDM_DEBUG
    if (_debugLevel >= 3) {
        DEBUG_D("%10.3f AT send    ", (HAL_Timer_Get_Milli_Seconds()-_debugTime)*0.001);
        dumpAtCmd(buf,len);
    }
#endif
    return _send(buf, len);
}

int MDMParser::sendFormated(const char* format, ...) {
    if (_cancel_all_operations) return 0;

    char buf[MAX_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf,sizeof(buf), format, args);
    va_end(args);
    return send(buf, len);
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
        }
#endif
        if ((ret != WAIT) && (ret != NOT_FOUND))
        {
            int type = TYPE(ret);
            // handle unsolicited commands here
            if (type == TYPE_PLUS) {
                const char* cmd = buf+3;
                int a, b, c, d, r;
                char s[32];

                // SMS Command ---------------------------------
                // +CNMI: <mem>,<index>
                if (sscanf(cmd, "CMTI: \"%*[^\"]\",%d", &a) == 1) {
                    DEBUG_D("New SMS at index %d\r\n", a);
                }
                else if ((sscanf(cmd, "CIEV: 9,%d", &a) == 1)) {
                    DEBUG_D("CIEV matched: 9,%d\r\n", a);
                    // Wait until the system is attached before attempting to act on GPRS detach
                    if (_attached) {
                        _attached_urc = (a==2)?1:0;
                        if (!_attached_urc) ARM_GPRS_TIMEOUT(15*1000); // If detached, set WDT
                        else CLR_GPRS_TIMEOUT(); // else if re-attached clear WDT.
                    }
                // Socket Specific Command ---------------------------------
                // +UUSORD: <socket>,<length>
                } else if ((sscanf(cmd, "UUSORD: %d,%d", &a, &b) == 2)) {
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
                                   !strcmp(s, "CGREG:") ? &_net.psd : NULL;
                        if (reg) {
                            // network status
                            if      (a == 0) *reg = REG_NONE;     // 0: not registered, home network
                            else if (a == 1) *reg = REG_HOME;     // 1: registered, home network
                            else if (a == 2) *reg = REG_NONE;     // 2: not registered, but MT is currently searching a new operator to register to
                            else if (a == 3) *reg = REG_DENIED;   // 3: registration denied
                            else if (a == 4) *reg = REG_UNKNOWN;  // 4: unknown
                            else if (a == 5) *reg = REG_ROAMING;  // 5: registered, roaming
                            if ((r >= 3) && (b != (int)0xFFFF))      _net.lac = b; // location area code
                            if ((r >= 4) && (c != (int)0xFFFFFFFF))  _net.ci  = c; // cell ID
                            // access technology
                            if (r >= 5) {
                                if      (d == 0) _net.act = ACT_GSM;      // 0: GSM
                                else if (d == 1) _net.act = ACT_GSM;      // 1: GSM COMPACT
                                else if (d == 2) _net.act = ACT_UTRAN;    // 2: UTRAN
                                else if (d == 3) _net.act = ACT_EDGE;     // 3: GSM with EDGE availability
                                else if (d == 4) _net.act = ACT_UTRAN;    // 4: UTRAN with HSDPA availability
                                else if (d == 5) _net.act = ACT_UTRAN;    // 5: UTRAN with HSUPA availability
                                else if (d == 6) _net.act = ACT_UTRAN;    // 6: UTRAN with HSDPA and HSUPA availability
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
        HAL_Delay_Milliseconds(10);
    }
    while (!TIMEOUT(start, timeout_ms) && !_cancel_all_operations);

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
            const char* simpin,
            const char* apn, const char* username,
            const char* password, Auth auth)
{
    bool ok = powerOn(simpin);
    if (!ok)
        return false;
    ok = init();
#ifdef MDM_DEBUG
    if (_debugLevel >= 1) dumpDevStatus(&_dev);
#endif
    if (!ok)
        return false;
    ok = registerNet();
#ifdef MDM_DEBUG
    if (_debugLevel >= 1) dumpNetStatus(&_net);
#endif
    if (!ok)
        return false;
    MDM_IP ip = join(apn,username,password,auth);
#ifdef MDM_DEBUG
    if (_debugLevel >= 1) dumpIp(ip);
#endif
    if (ip == NOIP)
        return false;
    return true;
}

void MDMParser::reset(void)
{
    MDM_INFO("[ Modem reset ]");
    HAL_GPIO_Write(RESET_UC, 0);
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(RESET_UC, 1);
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

#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS
    _dev.lpm = LPM_ENABLED;
#else
    HAL_Pin_Mode(RTS_UC, OUTPUT);
    HAL_GPIO_Write(RTS_UC, 0); // VERY IMPORTANT FOR CORRECT OPERATION W/O HW FLOW CONTROL!!
#endif

    HAL_Pin_Mode(LVLOE_UC, OUTPUT);
    HAL_GPIO_Write(LVLOE_UC, 0);

    if (!_init) {
        MDM_INFO("[ ElectronSerialPipe::begin ] = = = = = = = =");

        /* Instantiate the USART3 hardware */
        electronMDM.begin(115200);

        /* Initialize only once */
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

        // SARA-G35 >5ms, LISA-C2 > 150ms, LEON-G2 >5ms
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
        sendFormated("AT\r\n");
        int r = waitFinalResp(NULL,NULL,1000);
        if(RESP_OK == r) {
            _pwr = true;
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
    }

    if (continue_cancel) {
        cancel();
        goto failure;
    }

    // echo off
    sendFormated("AT E0\r\n");
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
        static bool retried_after_reset = false;
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
            system_tick_t start = HAL_Timer_Get_Milli_Seconds();
            while ((HAL_Timer_Get_Milli_Seconds() - start < 1000UL) && !_cancel_all_operations); // just wait
        }
    }
    if (_dev.sim != SIM_READY) {
        if (_dev.sim == SIM_MISSING) {
            MDM_ERROR("SIM not inserted\r\n");
        }
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
    // get the model identification
    sendFormated("AT+CGMM\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.model))
        goto failure;
    // get the version
    sendFormated("AT+CGMR\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.ver))
        goto failure;
    // Returns the ICCID (Integrated Circuit Card ID) of the SIM-card.
    // ICCID is a serial number identifying the SIM.
    sendFormated("AT+CCID\r\n");
    if (RESP_OK != waitFinalResp(_cbCCID, _dev.ccid))
        goto failure;
    // enable power saving
    if (_dev.lpm != LPM_DISABLED) {
         // enable power saving (requires flow control, cts at least)
        sendFormated("AT+UPSV=1\r\n");
        if (RESP_OK != waitFinalResp())
            goto failure;
        _dev.lpm = LPM_ACTIVE;
    }
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
    if (status)
        memcpy(status, &_dev, sizeof(DevStatus));
    UNLOCK();
    return true;
failure:
    UNLOCK();

    return false;
}

bool MDMParser::powerOff(void)
{
    LOCK();
    bool ok = false;
    bool continue_cancel = false;
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::powerOff ] = = = = = = = = = = = = = =");
        if (_cancel_all_operations) {
            continue_cancel = true;
            resume(); // make sure we can use the AT parser
        }
        for (int i=0; i<3; i++) { // try 3 times
            sendFormated("AT+CPWROFF\r\n");
            int ret = waitFinalResp(NULL,NULL,40*1000);
            if (RESP_OK == ret) {
                _pwr = false;
                // todo - add if these are automatically done on power down
                //_activated = false;
                //_attached = false;
                ok = true;
                break;
            }
            else if (RESP_ABORTED == ret) {
                MDM_INFO("\r\n[ Modem::powerOff ] found ABORTED, retrying...");
            }
            else {
                MDM_INFO("\r\n[ Modem::powerOff ] timeout, retrying...");
            }
        }
    }
    HAL_Pin_Mode(PWR_UC, INPUT);
    HAL_Pin_Mode(RESET_UC, INPUT);
#if USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS
#else
    HAL_Pin_Mode(RTS_UC, INPUT);
#endif
    HAL_Pin_Mode(LVLOE_UC, INPUT);

    if (continue_cancel) cancel();
    UNLOCK();
    return ok;
}

int MDMParser::_cbATI(int type, const char* buf, int len, Dev* dev)
{
    if ((type == TYPE_UNKNOWN) && dev) {
        if      (strstr(buf, "SARA-G350")) *dev = DEV_SARA_G350;
        else if (strstr(buf, "LISA-U200")) *dev = DEV_LISA_U200;
        else if (strstr(buf, "LISA-C200")) *dev = DEV_LISA_C200;
        else if (strstr(buf, "SARA-U260")) *dev = DEV_SARA_U260;
        else if (strstr(buf, "SARA-U270")) *dev = DEV_SARA_U270;
        else if (strstr(buf, "LEON-G200")) *dev = DEV_LEON_G200;
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

bool MDMParser::registerNet(NetStatus* status /*= NULL*/, system_tick_t timeout_ms /*= 180000*/)
{
    LOCK();
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::register ] = = = = = = = = = = = = = =");
        // Check to see if we are already connected. If so don't issue these
        // commands as they will knock us off the cellular network.
        if (checkNetStatus() == false) {
            // setup the GPRS network registration URC (Unsolicited Response Code)
            // 0: (default value and factory-programmed value): network registration URC disabled
            // 1: network registration URC enabled
            // 2: network registration and location information URC enabled
            sendFormated("AT+CGREG=2\r\n");
            if (RESP_OK != waitFinalResp())
                goto failure;
            // setup the network registration URC (Unsolicited Response Code)
            // 0: (default value and factory-programmed value): network registration URC disabled
            // 1: network registration URC enabled
            // 2: network registration and location information URC enabled
            sendFormated("AT+CREG=2\r\n");
            if (RESP_OK != waitFinalResp())
                goto failure;
            // Now check every 15 seconds for 5 minutes to see if we're connected to the tower (GSM and GPRS)
            system_tick_t start = HAL_Timer_Get_Milli_Seconds();
            while (!checkNetStatus(status) && !TIMEOUT(start, timeout_ms) && !_cancel_all_operations) {
                system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                while ((HAL_Timer_Get_Milli_Seconds() - start < 15000UL) && !_cancel_all_operations); // just wait
                //HAL_Delay_Milliseconds(15000);
            }
            if (_net.csd == REG_DENIED) MDM_ERROR("CSD Registration Denied\r\n");
            if (_net.psd == REG_DENIED) MDM_ERROR("PSD Registration Denied\r\n");
            // if (_net.csd == REG_DENIED || _net.psd == REG_DENIED) {
            //     sendFormated("AT+CEER\r\n");
            //     waitFinalResp();
            // }
        }
        UNLOCK();
        return REG_OK(_net.csd) && REG_OK(_net.psd);
    }
failure:
    UNLOCK();
    return false;
}

bool MDMParser::checkNetStatus(NetStatus* status /*= NULL*/)
{
    bool ok = false;
    LOCK();
    memset(&_net, 0, sizeof(_net));
    _net.lac = 0xFFFF;
    _net.ci = 0xFFFFFFFF;

    // check registration
    sendFormated("AT+CREG?\r\n");
    waitFinalResp();     // don't fail as service could be not subscribed

    // check PSD registration
    sendFormated("AT+CGREG?\r\n");
    waitFinalResp(); // don't fail as service could be not subscribed

    if (REG_OK(_net.csd) || REG_OK(_net.psd))
    {
        sendFormated("AT+COPS?\r\n");
        if (RESP_OK != waitFinalResp(_cbCOPS, &_net))
            goto failure;
        // get the MSISDNs related to this subscriber
        sendFormated("AT+CNUM\r\n");
        if (RESP_OK != waitFinalResp(_cbCNUM, _net.num))
            goto failure;

        // get the signal strength indication
        sendFormated("AT+CSQ\r\n");
        if (RESP_OK != waitFinalResp(_cbCSQ, &_net))
            goto failure;
    }
    if (status) {
        memcpy(status, &_net, sizeof(NetStatus));
    }
    // don't return true until fully registered
    ok = REG_OK(_net.csd) && REG_OK(_net.psd);
    UNLOCK();
    return ok;
failure:
    //unlock();
    return false;
}

bool MDMParser::getSignalStrength(NetStatus &status)
{
    bool ok = false;
    LOCK();
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::getSignalStrength ] = = = = = = = = = =");
        sendFormated("AT+CSQ\r\n");
        if (RESP_OK == waitFinalResp(_cbCSQ, &_net)) {
            ok = true;
            status.rssi = _net.rssi;
            status.qual = _net.qual;
        }
    }
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
            if (RESP_OK == waitFinalResp(NULL,NULL,10000)) {
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
    if ((type == TYPE_PLUS) && status){
        int act = 99;
        // +COPS: <mode>[,<format>,<oper>[,<AcT>]]
        if (sscanf(buf, "\r\n+COPS: %*d,%*d,\"%[^\"]\",%d",status->opr,&act) >= 1) {
            if      (act == 0) status->act = ACT_GSM;      // 0: GSM,
            else if (act == 2) status->act = ACT_UTRAN;    // 2: UTRAN
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
    if (_init && _pwr) {
        MDM_INFO("\r\n[ Modem::join ] = = = = = = = = = = = = = = = =");
        _ip = NOIP;
        int a = 0;
        bool force = false; // If we are already connected, don't force a reconnect.

        // perform GPRS attach
        sendFormated("AT+CGATT=1\r\n");
        if (RESP_OK != waitFinalResp(NULL,NULL,3*60*1000))
            goto failure;

        // Check the if the PSD profile is activated (a=1)
        sendFormated("AT+UPSND=" PROFILE ",8\r\n");
        if (RESP_OK != waitFinalResp(_cbUPSND, &a))
            goto failure;
        if (a == 1) {
            _activated = true; // PDP activated
            if (force) {
                // deactivate the PSD profile if it is already activated
                sendFormated("AT+UPSDA=" PROFILE ",4\r\n");
                if (RESP_OK != waitFinalResp(NULL,NULL,40*1000))
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
            if (RESP_OK != waitFinalResp())
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
                    if (RESP_OK != waitFinalResp())
                        goto failure;
                }
                if (username && *username) {
                    sendFormated("AT+UPSD=" PROFILE ",2,\"%s\"\r\n", username);
                    if (RESP_OK != waitFinalResp())
                        goto failure;
                }
                if (password && *password) {
                    sendFormated("AT+UPSD=" PROFILE ",3,\"%s\"\r\n", password);
                    if (RESP_OK != waitFinalResp())
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
                        if (RESP_OK != waitFinalResp())
                            goto failure;
                        // Activate the PSD profile and make connection
                        sendFormated("AT+UPSDA=" PROFILE ",3\r\n");
                        if (RESP_OK == waitFinalResp(NULL,NULL,150*1000)) {
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
        if (RESP_OK != waitFinalResp(_cbUPSND, &_ip))
            goto failure;

        UNLOCK();
        _attached = true;  // GPRS
        return _ip;
    }
failure:
    unlock();
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
            if (RESP_OK == waitFinalResp(NULL, NULL, 150*1000)) {

                //Get local IP address
                sendFormated("AT+UPSND=" PROFILE ",0\r\n");
                if (RESP_OK == waitFinalResp(_cbUPSND, &_ip)) {
                    ok = true;
                    _attached = true;
                }
            }
        }
    }
    UNLOCK();
    return ok;
}

// TODO - refactor disconnect() and detach()
// disconnect() can be called before detach() but not vice versa or
// disconnect() will ERROR because its PDP context will already be
// deactivated.
// _attached and _activated flags are currently associated inversely
// to what's happening.  When refactoring, consider combining...
bool MDMParser::disconnect(void)
{
    bool ok = false;
    bool continue_cancel = false;
    LOCK();
    if (_attached) {
        if (_cancel_all_operations) {
            continue_cancel = true;
            resume(); // make sure we can use the AT parser
        }
        MDM_INFO("\r\n[ Modem::disconnect ] = = = = = = = = = = = = =");
        if (_ip != NOIP) {
            /* Deactivates the PDP context assoc. with this profile
             * ensuring that no additional data is sent or received
             * by the device. */
            sendFormated("AT+UPSDA=" PROFILE ",4\r\n");
            if (RESP_OK == waitFinalResp()) {
                _ip = NOIP;
                ok = true;
                _attached = false;
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
        // if (_ip != NOIP) {  // if we disconnect() first we won't have an IP
            /* Detach from the GPRS network and conserve network resources. */
            /* Any active PDP context will also be deactivated. */
            sendFormated("AT+CGATT=0\r\n");
            if (RESP_OK != waitFinalResp(NULL,NULL,3*60*1000)) {
                ok = true;
                _activated = false;
            }
        // }
    }
    if (continue_cancel) cancel();
    UNLOCK();
    return ok;
}

MDM_IP MDMParser::gethostbyname(const char* host)
{
    MDM_IP ip = NOIP;
    int a,b,c,d;
    if (sscanf(host, IPSTR, &a,&b,&c,&d) == 4)
        ip = IPADR(a,b,c,d);
    else {
        LOCK();
        sendFormated("AT+UDNSRN=0,\"%s\"\r\n", host);
        if (RESP_OK != waitFinalResp(_cbUDNSRN, &ip, 30*1000))
            ip = NOIP;
        UNLOCK();
    }
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
    if ((RESP_OK == waitFinalResp(_cbUSOCTL, &handle)) &&
        (handle != MDM_SOCKET_ERROR)) {
        DEBUG_D("Socket handle %d was open, now closing...\r\n", handle);
        // Close it if it's open
        // AT+USOCL=0
        // OK
        sendFormated("AT+USOCL=%d\r\n", handle);
        if (RESP_OK == waitFinalResp()) {
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
        }
    } else /*(ipproto == MDM_IPPROTO_TCP)*/ {
        sendFormated("AT+USOCR=6\r\n");
    }
    int handle = MDM_SOCKET_ERROR;
    if ((RESP_OK == waitFinalResp(_cbUSOCR, &handle)) &&
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
    static bool checkedOnce = false;
    LOCK();

    if (!_attached) {
        if (!reconnect()) {
            socket = MDM_SOCKET_ERROR;
        }
    }

    if (_attached) {
        if (!checkedOnce) {
            checkedOnce = true; // prevent re-entry
            DEBUG_D("On first socketSocket use, free all open sockets\r\n");
            // Clean up any open sockets, we may have power cycled the STM32
            // while the modem remained connected.
            for (int s = 0; s < NUMSOCKETS; s++) {
                _socketCloseHandleIfOpen(s);
                // re-initialize the socket element
                _socketFree(s);
            }
        }

        // find an free socket
        socket = _findSocket(MDM_SOCKET_ERROR);
        DEBUG_D("socketSocket(%s)\r\n", (ipproto?"UDP":"TCP"));
        if (socket != MDM_SOCKET_ERROR) {
            int _socket = _socketSocket(socket, ipproto, port);
            if (_socket != MDM_SOCKET_ERROR) {
                socket = _socket;
            }
            else {
                // A socket should be available, but errored on trying to create one
                if (_socketCloseUnusedHandles()) {
                    // find a new free socket and try again
                    _socket = _findSocket(MDM_SOCKET_ERROR);
                    socket = _socketSocket(_socket, ipproto, port);
                }
                else {
                    // We tried to close unused handles, but also failed.
                    socket = MDM_SOCKET_ERROR;
                }
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
        DEBUG_D("socketConnect(%d,port:%d)\r\n", socket,port);
        sendFormated("AT+USOCO=%d,\"" IPSTR "\",%d\r\n", _sockets[socket].handle, IPNUM(ip), port);
        if (RESP_OK == waitFinalResp())
            ok = _sockets[socket].connected = true;
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
        DEBUG_D("socketClose(%d)\r\n", socket);
        sendFormated("AT+USOCL=%d\r\n", _sockets[socket].handle);
        if (RESP_ERROR == waitFinalResp()) {
            sendFormated("AT+CEER\r\n"); // For logging visibility
            waitFinalResp();
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
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            LOCK();
            if (ISSOCKET(socket)) {
                sendFormated("AT+USOWR=%d,%d\r\n",_sockets[socket].handle,blk);
                if (RESP_PROMPT == waitFinalResp()) {
                    HAL_Delay_Milliseconds(50);
                    send(buf, blk);
                    if (RESP_OK == waitFinalResp())
                        ok = true;
                }
            }
            UNLOCK();
        }
        if (!ok)
            return MDM_SOCKET_ERROR;
        buf += blk;
        cnt -= blk;
    }
    return (len - cnt);
}

int MDMParser::socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len)
{
    DEBUG_D("socketSendTo(%d," IPSTR ",%d,,%d)\r\n", socket,IPNUM(ip),port,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            LOCK();
            if (ISSOCKET(socket)) {
                sendFormated("AT+USOST=%d,\"" IPSTR "\",%d,%d\r\n",_sockets[socket].handle,IPNUM(ip),port,blk);
                if (RESP_PROMPT == waitFinalResp()) {
                    HAL_Delay_Milliseconds(50);
                    send(buf, blk);
                    if (RESP_OK == waitFinalResp())
                        ok = true;
                }
            }
            UNLOCK();
        }
        if (!ok)
            return MDM_SOCKET_ERROR;
        buf += blk;
        cnt -= blk;
    }
    return (len - cnt);
}

int MDMParser::socketReadable(int socket)
{
    int pending = MDM_SOCKET_ERROR;
    if (_cancel_all_operations)
            return MDM_SOCKET_ERROR;
    LOCK();
    if (ISSOCKET(socket) && _sockets[socket].connected) {
            //DEBUG_D("socketReadable(%d)\r\n", socket);
        // allow to receive unsolicited commands
        waitFinalResp(NULL, NULL, 0);
        if (_sockets[socket].connected)
           pending = _sockets[socket].pending;
    }
    UNLOCK();
    return pending;
}

int MDMParser::_cbUSORD(int type, const char* buf, int len, USORDparam* param)
{
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
        int blk = MAX_SIZE; // still need space for headers and unsolicited  commands
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
                            DEBUG_D("socketRecv: _cbUSORD\r\n");
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
    // DEBUG_D("socketRecv: %d \"%*s\"\r\n", cnt, cnt, buf-cnt);
    return cnt;
}

int MDMParser::_cbUSORF(int type, const char* buf, int len, USORFparam* param)
{
    if ((type == TYPE_PLUS) && param) {
        int sz, sk, p, a,b,c,d;
        int r = sscanf(buf, "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,",
            &sk,&a,&b,&c,&d,&p,&sz);
        if ((r == 7) && (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
            memcpy(param->buf, &buf[len-1-sz], sz);
            param->ip = IPADR(a,b,c,d);
            param->port = p;
            param->len = sz;
        } else {
            param->len = 0;
        }
    }
    return WAIT;
}

int MDMParser::socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len)
{
    int cnt = 0;
    //DEBUG_D("socketRecvFrom(%d,,%d)\r\n", socket, len);
#ifdef MDM_DEBUG
    memset(buf, '\0', len);
#endif
    system_tick_t start = HAL_Timer_Get_Milli_Seconds();
    while (len) {
        int blk = MAX_SIZE; // still need space for headers and unsolicited commands
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
    //DEBUG_D("socketRecv: %d \"%*s\"\r\n", cnt, cnt, buf-cnt);
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
    const char* txtDev[] = { "Unknown", "SARA-G350", "LISA-U200", "LISA-C200", "SARA-U260", "SARA-U270", "LEON-G200" };
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
        DEBUG_D("  CSD Registration:   %s\r\n", txtReg[status->csd]);
    if (status->psd < sizeof(txtReg)/sizeof(*txtReg) && (status->psd != REG_UNKNOWN))
        DEBUG_D("  PSD Registration:   %s\r\n", txtReg[status->psd]);
    const char* txtAct[] = { "Unknown", "GSM", "Edge", "3G", "CDMA" };
    if (status->act < sizeof(txtAct)/sizeof(*txtAct) && (status->act != ACT_UNKNOWN))
        DEBUG_D("  Access Technology:  %s\r\n", txtAct[status->act]);
    if (status->rssi)
        DEBUG_D("  Signal Strength:    %d dBm\r\n", status->rssi);
    if (status->qual)
        DEBUG_D("  Signal Quality:     %d\r\n", status->qual);
    if (*status->opr)
        DEBUG_D("  Operator:           %s\r\n", status->opr);
    if (status->lac != 0xFFFF)
        DEBUG_D("  Location Area Code: %04X\r\n", status->lac);
    if (status->ci != 0xFFFFFFFF)
        DEBUG_D("  Cell ID:            %08X\r\n", status->ci);
    if (*status->num)
        DEBUG_D("  Phone Number:       %s\r\n", status->num);
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
            { "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,\"%c\"",  TYPE_PLUS       },
            { "\r\n+URDFILE: %s,%d,\"%c\"",                 TYPE_PLUS       },
        };
        static struct {
              const char* sta;          const char* end;    int type;
        } lut[] = {
            { "\r\nOK\r\n",             NULL,               TYPE_OK         },
            { "\r\nERROR\r\n",          NULL,               TYPE_ERROR      },
            { "\r\n+CME ERROR:",        "\r\n",             TYPE_ERROR      },
            { "\r\n+CMS ERROR:",        "\r\n",             TYPE_ERROR      },
            { "\r\nRING\r\n",           NULL,               TYPE_RING       },
            { "\r\nCONNECT\r\n",        NULL,               TYPE_CONNECT    },
            { "\r\nNO CARRIER\r\n",     NULL,               TYPE_NOCARRIER  },
            { "\r\nNO DIALTONE\r\n",    NULL,               TYPE_NODIALTONE },
            { "\r\nBUSY\r\n",           NULL,               TYPE_BUSY       },
            { "\r\nNO ANSWER\r\n",      NULL,               TYPE_NOANSWER   },
            { "\r\n+",                  "\r\n",             TYPE_PLUS       },
            { "\r\n@",                  NULL,               TYPE_PROMPT     }, // Sockets
            { "\r\n>",                  NULL,               TYPE_PROMPT     }, // SMS
            { "\n>",                    NULL,               TYPE_PROMPT     }, // File
            { "\r\nABORTED\r\n",        NULL,               TYPE_ABORTED    }, // Current command aborted
        };
        for (int i = 0; i < (int)(sizeof(lutF)/sizeof(*lutF)); i ++) {
            pipe->set(unkn);
            int ln = _parseFormated(pipe, len, lutF[i].fmt);
            if (ln == WAIT && fr)
                return WAIT;
            if ((ln != NOT_FOUND) && (unkn > 0))
                return TYPE_UNKNOWN | pipe->get(buf, unkn);
            if (ln > 0)
                return lutF[i].type  | pipe->get(buf, ln);
        }
        for (int i = 0; i < (int)(sizeof(lut)/sizeof(*lut)); i ++) {
            pipe->set(unkn);
            int ln = _parseMatch(pipe, len, lut[i].sta, lut[i].end);
            if (ln == WAIT && fr)
                return WAIT;
            if ((ln != NOT_FOUND) && (unkn > 0))
                return TYPE_UNKNOWN | pipe->get(buf, unkn);
            if (ln > 0)
                return lut[i].type | pipe->get(buf, ln);
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
    return _getLine(&_pipeRx, buffer, length);
}

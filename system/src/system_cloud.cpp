/**
 ******************************************************************************
 * @file    system_cloud.cpp
 * @author  Satish Nair, Zachary Crockett, Mohit Bhoite, Matthew McGowan
 * @version V1.0.0
 * @date    13-March-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * @brief   
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "spark_wiring.h"
#include "spark_wiring_network.h"
#include "spark_wiring_cloud.h"
#include "system_network.h"
#include "system_task.h"
#include "system_update.h"
#include "string_convert.h"
#include "spark_protocol_functions.h"
#include "spark_protocol.h"
#include "socket_hal.h"
#include "core_hal.h"
#include "core_subsys_hal.h"
#include "deviceid_hal.h"
#include "inet_hal.h"
#include "rtc_hal.h"
#include "ota_flash_hal.h"
#include "product_store_hal.h"
#include "rgbled.h"
#include "spark_macros.h"
#include "string.h"
#include <stdarg.h>
#include "append_list.h"

#ifndef SPARK_NO_CLOUD

int userVarType(const char *varKey);
const void *getUserVar(const char *varKey);
int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved);

SparkProtocol* sp;

// initialize the sp value so we have a local copy of it in this module
struct SystemCloudStartup {
    SystemCloudStartup() {
        sp = spark_protocol_instance();
    }
};
SystemCloudStartup system_cloud_startup;

static sock_handle_t sparkSocket = socket_handle_invalid();

extern uint8_t LED_RGB_BRIGHTNESS;

// LED_Signaling_Override
volatile uint8_t LED_Spark_Signal;
const uint32_t VIBGYOR_Colors[] = {
    0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000
};
const int VIBGYOR_Size = sizeof (VIBGYOR_Colors) / sizeof (uint32_t);
int VIBGYOR_Index;

/**
 * This is necessary since spark_protocol_instance() was defined in both system_cloud
 * and communication dynalibs. (Not sure why - just an oversight.)
 * Renaming this method, but keeping in the dynalib for backwards compatibility with wiring code
 * version 1. Wiring code compiled against version 2 will not use this function, since the
 * code will be linked to spark_protocol_instance() in comms lib.
 */
SparkProtocol* system_cloud_protocol_instance(void)
{
    return sp;
}

struct User_Var_Lookup_Table_t
{
    const void *userVar;
    Spark_Data_TypeDef userVarType;
    char userVarKey[USER_VAR_KEY_LENGTH];
};

static append_list<User_Var_Lookup_Table_t> vars(5); 

struct User_Func_Lookup_Table_t
{    
    void* pUserFuncData;
    cloud_function_t pUserFunc;    
    char userFuncKey[USER_FUNC_KEY_LENGTH];
};

static append_list<User_Func_Lookup_Table_t> funcs(5);


SubscriptionScope::Enum convert(Spark_Subscription_Scope_TypeDef subscription_type)
{
    return(subscription_type==MY_DEVICES) ? SubscriptionScope::MY_DEVICES : SubscriptionScope::FIREHOSE;
}

bool spark_subscribe(const char *eventName, EventHandler handler, void* data, 
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved)
{        
    auto event_scope = convert(scope);
    bool success = spark_protocol_add_event_handler(sp, eventName, handler, event_scope, deviceID, NULL);
    if (success && spark_connected())
    {
        if (deviceID)
            success = spark_protocol_send_subscription_device(sp, eventName, deviceID);
        else
            success = spark_protocol_send_subscription_scope(sp, eventName, event_scope);
    }
    return success;
}


inline EventType::Enum convert(Spark_Event_TypeDef eventType) {
    return eventType==PUBLIC ? EventType::PUBLIC : EventType::PRIVATE;
}

bool spark_send_event(const char* name, const char* data, int ttl, Spark_Event_TypeDef eventType, void* reserved)
{
    return spark_protocol_send_event(sp, name, data, ttl, convert(eventType), NULL);
}

User_Var_Lookup_Table_t* find_var_by_key(const char* varKey)
{    
    for (int i = vars.size(); i-->0; )
    {            
        if (0 == strncmp(vars[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
        {
            return &vars[i];
        }
    }
    return NULL;
}

bool spark_variable(const char *varKey, const void *userVar, Spark_Data_TypeDef userVarType, void* reserved)
{
    User_Var_Lookup_Table_t* item = NULL;
    if (NULL != userVar && NULL != varKey && strlen(varKey)<=USER_VAR_KEY_LENGTH)
    {
        if ((item=find_var_by_key(varKey)) || (item=vars.add()))
        {
            item->userVar = userVar;
            item->userVarType = userVarType;
            memset(item->userVarKey, 0, USER_VAR_KEY_LENGTH);
            memcpy(item->userVarKey, varKey, USER_VAR_KEY_LENGTH);            
        }
    }
    return item!=NULL;
}

User_Func_Lookup_Table_t* find_func_by_key(const char* funcKey)
{    
    for (int i = funcs.size(); i-->0; )
    {            
        if (0 == strncmp(funcs[i].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH))
        {
            return &funcs[i];
        }
    }
    return NULL;
}

int call_raw_user_function(void* data, const char* param, void* reserved)
{
    user_function_int_str_t* fn = (user_function_int_str_t*)(data);
    String p(param);
    return (*fn)(p);
}

bool spark_function2(const cloud_function_descriptor* desc, void* reserved)
{    
    User_Func_Lookup_Table_t* item = NULL;    
    if (NULL != desc->fn && NULL != desc->funcKey && strlen(desc->funcKey)<=USER_FUNC_KEY_LENGTH)
    {                                
        if ((item=find_func_by_key(desc->funcKey)) || (item = funcs.add()))
        {
            item->pUserFunc = desc->fn;
            item->pUserFuncData = desc->data;
            memset(item->userFuncKey, 0, USER_FUNC_KEY_LENGTH);
            memcpy(item->userFuncKey, desc->funcKey, USER_FUNC_KEY_LENGTH);            
        }
    }    
    return item!=NULL;
}

/**
 * This is the original released signature for firmware version 0 and needs to remain like this.
 * (The original returned void - we can safely change to 
 */
bool spark_function(const char *funcKey, p_user_function_int_str_t pFunc, void* reserved)
{
    bool result;
    if (funcKey) {
        cloud_function_descriptor desc;
        desc.funcKey = funcKey;
        desc.fn = call_raw_user_function;
        desc.data = (void*)pFunc;
        result = spark_function2(&desc, NULL);
    }
    else {
        result = spark_function2((cloud_function_descriptor*)pFunc, reserved);
    }        
    return result;
}

inline uint8_t isSocketClosed()
{
    uint8_t closed = socket_active_status(sparkSocket) == SOCKET_STATUS_INACTIVE;

    if (closed)
    {
        DEBUG("get_socket_active_status(sparkSocket=%d)==SOCKET_STATUS_INACTIVE", sparkSocket);
    }
    if (closed && sparkSocket != socket_handle_invalid())
    {
        DEBUG("!!!!!!closed && sparkSocket(%d) != SOCKET_INVALID", sparkSocket);
    }
    if (!socket_handle_valid(sparkSocket))
    {
        DEBUG("sparkSocket is not valid");
        closed = true;
    }
    return closed;
}

bool spark_connected(void)
{
    if (SPARK_CLOUD_SOCKETED && SPARK_CLOUD_CONNECTED)
        return true;
    else
        return false;
}

void spark_connect(void)
{
    //Schedule Spark's cloud connection and handshake
    SPARK_WLAN_SLEEP = 0;
    SPARK_CLOUD_CONNECT = 1;
}

void spark_disconnect(void)
{
    //Schedule Spark's cloud disconnection
    SPARK_CLOUD_CONNECT = 0;
}

void spark_process(void)
{
    // run the background processing loop, and specifically also pump cloud events
    Spark_Idle_Events(true);
}

/**
 * This is the internal function called by the background loop to pump cloud events.
 */
void Spark_Process_Events()
{
    if (SPARK_CLOUD_SOCKETED && !Spark_Communication_Loop())
    {
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;
    }
}

// Returns number of bytes sent or -1 if an error occurred

int Spark_Send(const unsigned char *buf, uint32_t buflen)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed())
    {
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
        //break from any blocking loop
        return -1;
    }

    // send returns negative numbers on error
    int bytes_sent = socket_send(sparkSocket, buf, buflen);
    return bytes_sent;
}

// Returns number of bytes received or -1 if an error occurred

int Spark_Receive(unsigned char *buf, uint32_t buflen)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed())
    {
        //break from any blocking loop
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
        return -1;
    }

    static int spark_receive_last_bytes_received = 0;
    static volatile system_tick_t spark_receive_last_request_millis = 0;
    //no delay between successive socket_receive() calls for cloud
    //not connected or ota flash in process or on last data receipt
    if ((SPARK_CLOUD_CONNECTED != 1) || (SPARK_FLASH_UPDATE == 1)
        || (&spark_receive_last_bytes_received > 0)
        || ((millis() - spark_receive_last_request_millis) > SPARK_RECEIVE_DELAY_MILLIS))
    {
        spark_receive_last_bytes_received = socket_receive(sparkSocket, buf, buflen, 0);
        spark_receive_last_request_millis = millis();
    }

    return spark_receive_last_bytes_received;
}

int numUserFunctions(void)
{
    return funcs.size();
}

const char* getUserFunctionKey(int function_index)
{    
    return funcs[function_index].userFuncKey;    
}

int numUserVariables(void)
{
    return vars.size();
}

const char* getUserVariableKey(int variable_index)
{
    return vars[variable_index].userVarKey;
}

SparkReturnType::Enum wrapVarTypeInEnum(const char *varKey)
{
    switch (userVarType(varKey))
    {
        case 1:
            return SparkReturnType::BOOLEAN;
        case 4:
            return SparkReturnType::STRING;
        case 9:
            return SparkReturnType::DOUBLE;
        case 2:
        default:
            return SparkReturnType::INT;
    }
}

const char* CLAIM_EVENTS = "spark/device/claim/";

void SystemEvents(const char* name, const char* data)
{
    if (!strncmp(name, CLAIM_EVENTS, strlen(CLAIM_EVENTS))) {
        HAL_Set_Claim_Code(NULL);
    }
}    

void Spark_Protocol_Init(void)
{    
    if (!spark_protocol_is_initialized(sp))
    {
        product_details_t info;
        info.size = sizeof(info);
        spark_protocol_get_product_details(sp, &info);
        
        // User code was run, so persist the current values stored in the comms lib.
        // These will either have been left as default or overridden via PRODUCT_ID/PRODUCT_VERSION macros
        if (system_mode()!=SAFE_MODE) {
            HAL_SetProductStore(PRODUCT_STORE_ID, info.product_id);
            HAL_SetProductStore(PRODUCT_STORE_VERSION, info.product_version);        
        }
        else {      // user code was not executed, use previously persisted values
            info.product_id = HAL_GetProductStore(PRODUCT_STORE_ID);
            info.product_version = HAL_GetProductStore(PRODUCT_STORE_VERSION);
            if (info.product_id!=0xFFFF)
                spark_protocol_set_product_id(sp, info.product_id);
            if (info.product_version!=0xFFFF)
                spark_protocol_set_product_firmware_version(sp, info.product_version);
        }
        
        SparkCallbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.size = sizeof(callbacks);
        callbacks.send = Spark_Send;
        callbacks.receive = Spark_Receive;
        callbacks.prepare_for_firmware_update = Spark_Prepare_For_Firmware_Update;
        callbacks.finish_firmware_update = Spark_Finish_Firmware_Update;
        callbacks.calculate_crc = HAL_Core_Compute_CRC32;
        callbacks.save_firmware_chunk = Spark_Save_Firmware_Chunk;
        callbacks.signal = Spark_Signal;
        callbacks.millis = HAL_Timer_Get_Milli_Seconds;
        callbacks.set_time = system_set_time;

        SparkDescriptor descriptor;
        memset(&descriptor, 0, sizeof(descriptor));
        descriptor.size = sizeof(descriptor);
        descriptor.num_functions = numUserFunctions;
        descriptor.get_function_key = getUserFunctionKey;
        descriptor.call_function = userFuncSchedule;
        descriptor.num_variables = numUserVariables;
        descriptor.get_variable_key = getUserVariableKey;
        descriptor.variable_type = wrapVarTypeInEnum;
        descriptor.get_variable = getUserVar;
        descriptor.was_ota_upgrade_successful = HAL_OTA_Flashed_GetStatus;
        descriptor.ota_upgrade_status_sent = HAL_OTA_Flashed_ResetStatus;
        descriptor.append_system_info = system_module_info;

        unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
        unsigned char private_key[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];

        SparkKeys keys;
        keys.size = sizeof(keys);
        keys.server_public = pubkey;
        keys.core_private = private_key;

        // ensure the private key is read first since the public key may be derived from it.
        private_key_generation_t genspec;
        genspec.size = sizeof(genspec);
        genspec.gen = PRIVATE_KEY_GENERATE_MISSING;
        HAL_FLASH_Read_CorePrivateKey(private_key, &genspec);
        HAL_FLASH_Read_ServerPublicKey(pubkey);

        uint8_t id_length = HAL_device_ID(NULL, 0);
        uint8_t id[id_length];
        HAL_device_ID(id, id_length);
        spark_protocol_init(sp, (const char*) id, keys, callbacks, descriptor);
        
        Spark.subscribe("spark", SystemEvents);
    }
}

void system_set_time(time_t time, unsigned, void*)
{
    HAL_RTC_Set_UnixTime(time);
}

const int CLAIM_CODE_SIZE = 63;

int Spark_Handshake(void)
{
    int err = spark_protocol_handshake(sp);
    if (!err)
    {
        char buf[CLAIM_CODE_SIZE + 1];
        if (!HAL_Get_Claim_Code(buf, sizeof (buf)) && *buf)
        {
            Spark.publish("spark/device/claim/code", buf, 60, PRIVATE);
        }
        
        ultoa(HAL_OTA_FlashLength(), buf, 10);
        Spark.publish("spark/hardware/max_binary", buf, 60, PRIVATE);

        ultoa(HAL_OTA_ChunkSize(), buf, 10);
        Spark.publish("spark/hardware/ota_chunk_size", buf, 60, PRIVATE);

        if (!HAL_core_subsystem_version(buf, sizeof (buf)))
        {
            Spark.publish("spark/" SPARK_SUBSYSTEM_EVENT_NAME, buf, 60, PRIVATE);
        }

        Multicast_Presence_Announcement();
        spark_protocol_send_subscriptions(sp);
        // important this comes at the end since it requires a response from the cloud.
        spark_protocol_send_time_request(sp);
        Spark_Process_Events();
    }
    return err;
}

// Returns true if all's well or
//         false on error, meaning we're probably disconnected

inline bool Spark_Communication_Loop(void)
{
    return spark_protocol_event_loop(sp);
}

/* This function MUST NOT BlOCK!
 * It will be executed every 1ms if LED_Signaling_Start() is called
 * and stopped as soon as LED_Signaling_Stop() is called */
void LED_Signaling_Override(void)
{
    static uint8_t LED_Signaling_Timing = 0;
    if (0 < LED_Signaling_Timing)
    {
        --LED_Signaling_Timing;
    }
    else
    {
        LED_SetSignalingColor(VIBGYOR_Colors[VIBGYOR_Index]);
        LED_On(LED_RGB);

        LED_Signaling_Timing = 100; // 100 ms

        ++VIBGYOR_Index;
        if (VIBGYOR_Index >= VIBGYOR_Size)
        {
            VIBGYOR_Index = 0;
        }
    }
}

void Spark_Signal(bool on, unsigned, void*)
{
    if (on)
    {
        LED_Signaling_Start();
        LED_Spark_Signal = 1;
    }
    else
    {
        LED_Signaling_Stop();
        LED_Spark_Signal = 0;
    }
}

int Internet_Test(void)
{
    long testSocket;
    sockaddr_t testSocketAddr;
    int testResult = 0;
    DEBUG("socket");
    testSocket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, 53, NIF_DEFAULT);
    DEBUG("socketed testSocket=%d", testSocket);


    if (testSocket < 0)
    {
        return -1;
    }

    // the family is always AF_INET
    testSocketAddr.sa_family = AF_INET;

    // the destination port: 53
    testSocketAddr.sa_data[0] = 0;
    testSocketAddr.sa_data[1] = 53;

    // the destination IP address: 8.8.8.8
    testSocketAddr.sa_data[2] = 8;
    testSocketAddr.sa_data[3] = 8;
    testSocketAddr.sa_data[4] = 8;
    testSocketAddr.sa_data[5] = 8;

    uint32_t ot = HAL_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    DEBUG("connect");
    testResult = socket_connect(testSocket, &testSocketAddr, sizeof (testSocketAddr));
    DEBUG("connected testResult=%d", testResult);
    HAL_WLAN_SetNetWatchDog(ot);

#if defined(SEND_ON_CLOSE)
    DEBUG("send");
    char c = 0;
    int rc = send(testSocket, &c, 1, 0);
    DEBUG("send %d", rc);
#endif
    DEBUG("Close");
    socket_close(testSocket);

    //if connection fails, testResult returns -1
    return testResult;
}

// Same return value as connect(), -1 on error

int Spark_Connect(void)
{
    DEBUG("sparkSocket Now =%d", sparkSocket);

    // Close Original

    Spark_Disconnect();

    sparkSocket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, SPARK_SERVER_PORT, NIF_DEFAULT);
    DEBUG("socketed sparkSocket=%d", sparkSocket);

    if (!socket_handle_valid(sparkSocket))
    {
        return -1;
    }
    sockaddr_t tSocketAddr;
    // the family is always AF_INET
    tSocketAddr.sa_family = AF_INET;

    // the destination port
    tSocketAddr.sa_data[0] = (SPARK_SERVER_PORT & 0xFF00) >> 8;
    tSocketAddr.sa_data[1] = (SPARK_SERVER_PORT & 0x00FF);

    ServerAddress server_addr;
    HAL_FLASH_Read_ServerAddress(&server_addr);

    bool ip_resolve_failed = false;
    IPAddress ip_addr;

    switch (server_addr.addr_type)
    {
        case IP_ADDRESS:
            ip_addr = server_addr.ip;
            break;

        default:
        case INVALID_INTERNET_ADDRESS:
        {
            const char default_domain[] = "device.spark.io";
            // Make sure we copy the NULL terminator, so subsequent strlen() calls on server_addr.domain return the correct length
            memcpy(server_addr.domain, default_domain, strlen(default_domain) + 1);
            // and fall through to domain name case
        }

        case DOMAIN_NAME:
            int attempts = 3;
            while (!ip_addr && 0 < --attempts)
            {
                inet_gethostbyname(server_addr.domain, strnlen(server_addr.domain, 126), &ip_addr.raw(), NIF_DEFAULT, NULL);
                HAL_Delay_Milliseconds(1);                
            }
            ip_resolve_failed = !ip_addr;
    }

    int rv = -1;
    if (!ip_resolve_failed) 
    {
        if (!ip_addr)
        {
            // final fallback in case where flash invalid
            ip_addr = (54 << 24) | (208 << 16) | (229 << 8) | 4;
            //ip_addr = (52<<24) | (0<<16) | (3<<8) | 40;
        }
    
        tSocketAddr.sa_data[2] = ip_addr[0];
        tSocketAddr.sa_data[3] = ip_addr[1];
        tSocketAddr.sa_data[4] = ip_addr[2];
        tSocketAddr.sa_data[5] = ip_addr[3];

        uint32_t ot = HAL_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
        DEBUG("connect");
        rv = socket_connect(sparkSocket, &tSocketAddr, sizeof (tSocketAddr));
        DEBUG("connected connect=%d", rv);
        HAL_WLAN_SetNetWatchDog(ot);
    }
    if (rv)     // error - prevent socket leaks
        Spark_Disconnect();
    return rv;
}

int Spark_Disconnect(void)
{
    int retVal = 0;
    DEBUG("");
    if (socket_handle_valid(sparkSocket))
    {
#if defined(SEND_ON_CLOSE)
        DEBUG("send");
        char c = 0;
        int rc = send(sparkSocket, &c, 1, 0);
        DEBUG("send %d", rc);
#endif
        DEBUG("Close");
        retVal = socket_close(sparkSocket);
        DEBUG("Closed retVal=%d", retVal);
        sparkSocket = socket_handle_invalid();
    }
    return retVal;
}

int userVarType(const char *varKey)
{
    User_Var_Lookup_Table_t* item = find_var_by_key(varKey);
    return item ? item->userVarType : -1;
}

const void *getUserVar(const char *varKey)
{
    User_Var_Lookup_Table_t* item = find_var_by_key(varKey);
    return item ? item->userVar : NULL;
}

int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved)
{
    // for now, we invoke the function directly and return the result via the callback    
    User_Func_Lookup_Table_t* item = find_func_by_key(funcKey);
    int result = item ? item->pUserFunc(item->pUserFuncData, paramString, NULL) : -1;
    callback((const void*)result, SparkReturnType::INT);
    return 0;
}

void HAL_WLAN_notify_socket_closed(sock_handle_t socket)
{
    if (sparkSocket==socket)
    {
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;
    }
}

#else

void HAL_WLAN_notify_socket_closed(sock_handle_t socket)
{    
}

#endif


inline void concat_nibble(String& result, uint8_t nibble)
{
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 39;
    result.concat(hex_digit);
}

String bytes2hex(const uint8_t* buf, unsigned len)
{
    String result;
    for (unsigned i = 0; i < len; ++i)
    {
        concat_nibble(result, (buf[i] >> 4));
        concat_nibble(result, (buf[i] & 0xF));
    }
    return result;
}

String spark_deviceID(void)
{
    unsigned len = HAL_device_ID(NULL, 0);
    uint8_t id[len];
    HAL_device_ID(id, len);
    return bytes2hex(id, len);
}


static inline char ascii_nibble(uint8_t nibble) {
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 7;
    return hex_digit;    
}

static inline char* concat_nibble(char* p, uint8_t nibble)
{    
    *p++ = ascii_nibble(nibble);
    return p;
}

char* bytes2hexbuf(const uint8_t* buf, unsigned len, char* out)
{
    unsigned i;
    char* result = out;
    for (i = 0; i < len; ++i)
    {
        concat_nibble(out, (buf[i] >> 4));
        out++;
        concat_nibble(out, (buf[i] & 0xF));
        out++;
    }
    return result;
}

void Multicast_Presence_Announcement(void)
{
#ifndef SPARK_NO_CLOUD    
    long multicast_socket = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, NIF_DEFAULT);
    if (!socket_handle_valid(multicast_socket))
        return;

    unsigned char announcement[19];
    uint8_t id_length = HAL_device_ID(NULL, 0);
    uint8_t id[id_length];
    HAL_device_ID(id, id_length);
    spark_protocol_presence_announcement(sp, announcement, (const char *) id);

    // create multicast address 224.0.1.187 port 5683
    sockaddr_t addr;
    addr.sa_family = AF_INET;
    addr.sa_data[0] = 0x16; // port MSB
    addr.sa_data[1] = 0x33; // port LSB
    addr.sa_data[2] = 0xe0; // IP MSB
    addr.sa_data[3] = 0x00;
    addr.sa_data[4] = 0x01;
    addr.sa_data[5] = 0xbb; // IP LSB

    //why loop here? Uncommenting this leads to SOS(HardFault Exception) on local cloud
    //for (int i = 3; i > 0; i--)
    {
        socket_sendto(multicast_socket, announcement, 19, 0, &addr, sizeof (sockaddr_t));
    }

    socket_close(multicast_socket);
#endif
}

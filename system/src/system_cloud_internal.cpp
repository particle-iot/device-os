/**
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

#include "spark_wiring_string.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_ticks.h"
#include "spark_wiring_ipaddress.h"
#include "system_cloud_internal.h"
#include "system_mode.h"
#include "system_network.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_user.h"
#include "spark_wiring_string.h"
#include "spark_protocol_functions.h"
#include "append_list.h"
#include "core_hal.h"
#include "deviceid_hal.h"
#include "ota_flash_hal.h"
#include "product_store_hal.h"
#include "rtc_hal.h"
#include "socket_hal.h"
#include "rgbled.h"
#include "spark_macros.h"   // for S2M
#include "string_convert.h"
#include <stdint.h>
#include "core_hal.h"
#include "hal_platform.h"
#include "system_string_interpolate.h"
#include "dtls_session_persist.h"

#define IPNUM(ip)       ((ip)>>24)&0xff,((ip)>>16)&0xff,((ip)>> 8)&0xff,((ip)>> 0)&0xff

#ifndef SPARK_NO_CLOUD

int userVarType(const char *varKey);
const void *getUserVar(const char *varKey);
int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved);


static sock_handle_t sparkSocket = socket_handle_invalid();

extern uint8_t LED_RGB_BRIGHTNESS;

// LED_Signaling_Override
volatile uint8_t LED_Spark_Signal;
const uint32_t VIBGYOR_Colors[] = {
    0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000
};
const int VIBGYOR_Size = sizeof (VIBGYOR_Colors) / sizeof (uint32_t);
int VIBGYOR_Index;

ProtocolFacade* sp;

/**
 * This is necessary since spark_protocol_instance() was defined in both system_cloud
 * and communication dynalibs. (Not sure why - just an oversight.)
 * Renaming this method, but keeping in the dynalib for backwards compatibility with wiring code
 * version 1. Wiring code compiled against version 2 will not use this function, since the
 * code will be linked to spark_protocol_instance() in comms lib.
 */
ProtocolFacade* system_cloud_protocol_instance(void)
{
	if (!sp)
		sp = spark_protocol_instance();
    return sp;
}

static append_list<User_Var_Lookup_Table_t> vars(5);
static append_list<User_Func_Lookup_Table_t> funcs(5);

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


User_Var_Lookup_Table_t* find_var_by_key_or_add(const char* varKey)
{
    User_Var_Lookup_Table_t* result = find_var_by_key(varKey);
    return result ? result : vars.add();
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

User_Func_Lookup_Table_t* find_func_by_key_or_add(const char* funcKey)
{
    User_Func_Lookup_Table_t* result = find_func_by_key(funcKey);
    return result ? result : funcs.add();
}

int call_raw_user_function(void* data, const char* param, void* reserved)
{
    user_function_int_str_t* fn = (user_function_int_str_t*)(data);
    String p(param);
    return (*fn)(p);
}

/**
 * Register a function.
 * @param desc
 * @param reserved
 * @return
 */
bool spark_function_internal(const cloud_function_descriptor* desc, void* reserved)
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


void invokeEventHandlerInternal(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const char* event_name, const char* data, void* reserved)
{
    if(handlerInfo->handler_data)
    {
        EventHandlerWithData handler = (EventHandlerWithData) handlerInfo->handler;
        handler(handlerInfo->handler_data, event_name, data);
    }
    else
    {
        handlerInfo->handler(event_name, data);
    }
}

void invokeEventHandlerString(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const String& name, const String& data, void* reserved)
{
    invokeEventHandlerInternal(handlerInfoSize, handlerInfo, name.c_str(), data.c_str(), reserved);
}


void invokeEventHandler(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const char* event_name, const char* event_data, void* reserved)
{
    if (system_thread_get_state(NULL)==spark::feature::DISABLED)
    {
        invokeEventHandlerInternal(handlerInfoSize, handlerInfo, event_name, event_data, reserved);
    }
    else
    {
        // copy the buffers to dynamically allocated storage.
        String name(event_name);
        String data(event_data);
        APPLICATION_THREAD_CONTEXT_ASYNC(invokeEventHandlerString(handlerInfoSize, handlerInfo, name, event_data, reserved));
    }
}

volatile uint32_t lastCloudEvent = 0;

/**
 * This is the internal function called by the background loop to pump cloud events.
 */
void Spark_Process_Events()
{
    if (SPARK_CLOUD_SOCKETED && !Spark_Communication_Loop())
    {
        WARN("Communication loop error, closing cloud socket");
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;
    }
    else
    {
        lastCloudEvent = millis();
    }
}

void decode_endpoint(const sockaddr_t& socket_addr, IPAddress& ip, uint16_t& port)
{
	// assume IPv4 for now.
	ip.set_ipv4(
			socket_addr.sa_data[2],
			socket_addr.sa_data[3],
			socket_addr.sa_data[4],
			socket_addr.sa_data[5]
	);
	port = socket_addr.sa_data[0] << 8 | socket_addr.sa_data[1];
}

void encode_endpoint(sockaddr_t& tSocketAddr, const IPAddress& ip_addr, const uint16_t port)
{
    // the destination port
    tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
    tSocketAddr.sa_data[1] = (port & 0x00FF);

    tSocketAddr.sa_data[2] = ip_addr[0];
    tSocketAddr.sa_data[3] = ip_addr[1];
    tSocketAddr.sa_data[4] = ip_addr[2];
    tSocketAddr.sa_data[5] = ip_addr[3];
}


#if HAL_PLATFORM_CLOUD_UDP
struct Endpoint
{
	IPAddress address;
	uint16_t port;
};

struct SessionConnection
{
	/**
	 * The previously used address.
	 */
	sockaddr_t address;

	/**
	 * The checksum of the server address data that was used
	 * to derive the connection address.
	 */
	uint32_t server_address_checksum;
};


SessionConnection cloud_endpoint;

int Spark_Send_UDP(const unsigned char* buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed())
    {
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
        //break from any blocking loop
        return -1;
    }
    DEBUG("send %d", buflen);
	return socket_sendto(sparkSocket, buf, buflen, 0, &cloud_endpoint.address, sizeof(cloud_endpoint.address));
}

int Spark_Receive_UDP(unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed())
    {
        //break from any blocking loop
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
        return -1;
    }

    sockaddr_t addr;
    socklen_t size = sizeof(addr);
	int received = socket_receivefrom(sparkSocket, buf, buflen, 0, &addr, &size);

	if (received!=0) {
		DEBUG("received %d", received);
	}
	if (received>0)
    {
#if PLATFORM_ID!=3
    		// filter out by destination IP and port
    		// todo - IPv6 will need to change this
    		if (memcmp(&addr.sa_data, &cloud_endpoint.address.sa_data, 6)) {
    			// ignore the packet if from a different source
    			received = 0;
    		    DEBUG("received from a different address %d.%d.%d.%d:%d",
    		    		cloud_endpoint.address.sa_data[2],
				cloud_endpoint.address.sa_data[3],
				cloud_endpoint.address.sa_data[4],
				cloud_endpoint.address.sa_data[5],
				((cloud_endpoint.address.sa_data[0]<<8)+cloud_endpoint.address.sa_data[1])
    		    );

    		}
#endif
    }
    return received;
}


#endif



// Returns number of bytes sent or -1 if an error occurred
int Spark_Send(const unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed())
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
int Spark_Receive(unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed())
    {
        //break from any blocking loop
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
        return -1;
    }

    static int spark_receive_last_bytes_received = 0;
//    static volatile system_tick_t spark_receive_last_request_millis = 0;
    //no delay between successive socket_receive() calls for cloud
    //not connected or ota flash in process or on last data receipt
    {
        spark_receive_last_bytes_received = socket_receive(sparkSocket, buf, buflen, 0);
        //spark_receive_last_request_millis = millis();
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
const char* RESET_EVENT = "spark/device/reset";

void SystemEvents(const char* name, const char* data)
{
    if (!strncmp(name, CLAIM_EVENTS, strlen(CLAIM_EVENTS))) {
        HAL_Set_Claim_Code(NULL);
    }
    if (!strcmp(name, RESET_EVENT)) {
        if (data && *data) {
            if (!strcmp("safe mode", data))
                System.enterSafeMode();
            else if (!strcmp("dfu", data))
                System.dfu(false);
            else if (!strcmp("reboot", data))
                System.reset();
        }
    }
}

using particle::protocol::SessionPersistOpaque;

#if HAL_PLATFORM_CLOUD_UDP
int Spark_Save(const void* buffer, size_t length, uint8_t type, void* reserved)
{
	if (type==SparkCallbacks::PERSIST_SESSION)
	{
		static_assert(sizeof(SessionPersistOpaque::connection)>=sizeof(cloud_endpoint),"connection space in session is not large enough");

		// save the current connection to the persisted session
		SessionPersistOpaque* persist = (SessionPersistOpaque*)buffer;
		if (persist->is_valid())
		{
			memcpy(persist->connection_data(), &cloud_endpoint, sizeof(cloud_endpoint));
		}
		return HAL_System_Backup_Save(0, buffer, length, nullptr);
	}
	return -1;	// eek. define a constant for this error - Unknown Type.
}

int Spark_Restore(void* buffer, size_t max_length, uint8_t type, void* reserved)
{
	size_t length = 0;
	int error = HAL_System_Backup_Restore(0, buffer, max_length, &length, nullptr);
	if (error)
		length = 0;
	return length;
}
#endif

void Spark_Protocol_Init(void)
{
	system_cloud_protocol_instance();

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

        const bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);

        SparkCallbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.size = sizeof(callbacks);
        callbacks.protocolFactory = udp ? PROTOCOL_DTLS : PROTOCOL_LIGHTSSL;
#if HAL_PLATFORM_CLOUD_UDP
        if (udp)
        {
            callbacks.send = Spark_Send_UDP;
            callbacks.receive = Spark_Receive_UDP;
            callbacks.transport_context = &cloud_endpoint;
            callbacks.save = Spark_Save;
            callbacks.restore = Spark_Restore;
        }
        else
#endif
        {
        		callbacks.send = Spark_Send;
        		callbacks.receive = Spark_Receive;
        		callbacks.transport_context = nullptr;
        }
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
        descriptor.call_event_handler = invokeEventHandler;

        // todo - this pushes a lot of data on the stack! refactor to remove heavy stack usage
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

        Particle.subscribe("spark", SystemEvents);

        CommunicationsHandlers handlers;
        handlers.size = sizeof(handlers);
        handlers.random_seed_from_cloud = &random_seed_from_cloud;
        spark_protocol_communications_handlers(sp, &handlers);
    }
}

void system_set_time(time_t time, unsigned, void*)
{
    HAL_RTC_Set_UnixTime(time);
}

const int CLAIM_CODE_SIZE = 63;

int Spark_Handshake(bool presence_announce)
{
	DEBUG("starting handshake announce=%d", presence_announce);
    int err = spark_protocol_handshake(sp);
    if (!err)
    {
        char buf[CLAIM_CODE_SIZE + 1];
        if (!HAL_Get_Claim_Code(buf, sizeof (buf)) && *buf)
        {
            Particle.publish("spark/device/claim/code", buf, 60, PRIVATE);
        }

        // open up for possibility of retrieving multiple ID datums
        if (!HAL_Get_Device_Identifier(NULL, buf, sizeof(buf), 0, NULL) && *buf) {
            Particle.publish("spark/device/ident/0", buf, 60, PRIVATE);
        }

        bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#if PLATFORM_ID!=PLATFORM_ELECTRON || !defined(MODULAR_FIRMWARE)
        ultoa(HAL_OTA_FlashLength(), buf, 10);
        Particle.publish("spark/hardware/max_binary", buf, 60, PRIVATE);
#endif

        uint32_t chunkSize = HAL_OTA_ChunkSize();
        if (chunkSize!=512 || !udp) {
        		ultoa(chunkSize, buf, 10);
        		Particle.publish("spark/hardware/ota_chunk_size", buf, 60, PRIVATE);
        }
        if (system_mode()==SAFE_MODE)
            Particle.publish("spark/device/safemode","", 60, PRIVATE);
#if defined(SPARK_SUBSYSTEM_EVENT_NAME)
        if (!HAL_core_subsystem_version(buf, sizeof (buf)) && *buf)
        {
            Particle.publish("spark/" SPARK_SUBSYSTEM_EVENT_NAME, buf, 60, PRIVATE);
        }
#endif

        if (presence_announce)
        		Multicast_Presence_Announcement();
        spark_protocol_send_subscriptions(sp);
        // important this comes at the end since it requires a response from the cloud.
        spark_protocol_send_time_request(sp);
        Spark_Process_Events();
    }
    if (err==particle::protocol::SESSION_RESUMED)
    {
    		DEBUG("cloud connected from existing session.");
    		err = 0;
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

size_t system_interpolate(const char* var, size_t var_len, char* buf, size_t buf_len)
{
	if (var_len==2 && memcmp("id", var, 2)==0)
	{
		String deviceID = spark_deviceID();
		if (buf_len>deviceID.length()) {
			memcpy(buf, deviceID.c_str(), deviceID.length());
			return deviceID.length();
		}
	}
	return 0;
}

int Internet_Test(void)
{
    long testSocket;
    sockaddr_t testSocketAddr;
    int testResult = 0;
    DEBUG("Internet test socket");
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

    uint32_t ot = HAL_NET_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    DEBUG("Connect Attempt");
    testResult = socket_connect(testSocket, &testSocketAddr, sizeof (testSocketAddr));
    DEBUG("socket_connect()=%s", (testResult ? "fail":"success"));
    HAL_NET_SetNetWatchDog(ot);

#if defined(SEND_ON_CLOSE)
    DEBUG("Send Attempt");
    char c = 0;
    int rc = send(testSocket, &c, 1, 0);
    DEBUG("send()=%d", rc);
#endif
    DEBUG("Close");
    socket_close(testSocket);

    //if connection fails, testResult returns -1
    return testResult;
}

#if HAL_PLATFORM_CLOUD_UDP
uint32_t compute_session_checksum(ServerAddress& addr)
{
	uint32_t checksum = HAL_Core_Compute_CRC32((uint8_t*)&addr, sizeof(addr));
	return checksum;
}


/**
 * Determines if the existing session is valid and contains a valid ip_address and port
 */
int determine_session_connection_address(IPAddress& ip_addr, uint16_t& port, ServerAddress& server_addr)
{
	SessionPersistOpaque persist;
	if (Spark_Restore(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr)==sizeof(persist) && persist.is_valid())
	{
		SessionConnection* connection = (SessionConnection*)persist.connection_data();
		if (connection->server_address_checksum==compute_session_checksum(server_addr))
		{
			IPAddress addr; uint16_t p;
			decode_endpoint(connection->address, addr, p);
			if (addr && p)
			{
				ip_addr = addr;
				port = p;
				DEBUG("using IP/port from session");
				return 0;
			}
		}
		else
		{
			// discard the session
			persist.invalidate();
			Spark_Save(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);
			INFO("connection checksum mismatch - discarded session");
		}
	}
	return -1;
}
#endif

/**
 */
int determine_connection_address(IPAddress& ip_addr, uint16_t& port, ServerAddress& server_addr, bool udp)
{
#if HAL_PLATFORM_CLOUD_UDP
	// todo - how to determine if the underlying connection has changed so that we invalidate the existing session?
	// for now, the user will have to manually reset the connection (e.g. by powering off the device.)
	if (udp && !determine_session_connection_address(ip_addr, port, server_addr)) {
		return 0;
	}
#endif

	bool ip_address_error = false;
    switch (server_addr.addr_type)
    {
        case IP_ADDRESS:
            // DEBUG("IP_ADDRESS");
            ip_addr = server_addr.ip;
            if (server_addr.port!=0 && server_addr.port!=65535)
            		port = server_addr.port;
            break;

        default:
        case INVALID_INTERNET_ADDRESS:
        {
        		if (!udp)
        		{
				// DEBUG("INVALID_INTERNET_ADDRESS");
				const char default_domain[] = "device.spark.io";
				// Make sure we copy the NULL terminator, so subsequent strlen() calls on server_addr.domain return the correct length
				memcpy(server_addr.domain, default_domain, strlen(default_domain) + 1);
				// and fall through to domain name case
        		}
        		else
        		{
        			ip_address_error = true;
        			break;
        		}
        }

        case DOMAIN_NAME:
            // DEBUG("DOMAIN_NAME");
            if (server_addr.port!=0 && server_addr.port!=65535)
            		port = server_addr.port;

            char buf[96];
            system_string_interpolate(server_addr.domain, buf, sizeof(buf), system_interpolate);
            int attempts = 3;
            int rv = 0;
            while (!ip_addr && attempts-->0)
            {
                rv = inet_gethostbyname(buf, strnlen(buf, 96), &ip_addr.raw(), NIF_DEFAULT, NULL);
                HAL_Delay_Milliseconds(1);
            }
            ip_address_error = rv;
            if (ip_address_error) {
                ERROR("Cloud: unable to resolve IP for %s", buf);
            }
            else {
                INFO("Resolved host %s to %s", buf, String(ip_addr).c_str());
            }
    }

#if PLATFORM_ID<3
    // workaround for CC3000
    if (ip_address_error)
    {
        const WLanConfig* config = (WLanConfig*)network_config(0, 0, NULL);
        if (config && (config->nw.aucDNSServer.ipv4==((76<<24) | (83<<16) | (0<<8) | 0 ))) {
            // fallback to the default when the CC3000 DNS goes awol. see issue #139
            ip_addr.clear();
            ip_address_error = false;
        }
    }
#endif

	if (ip_address_error && (!HAL_PLATFORM_CLOUD_UDP || !udp))
	{
		// TCP - final fallback in case where flash invalid
		ip_addr = (54 << 24) | (208 << 16) | (229 << 8) | 4;
		//ip_addr = (52<<24) | (0<<16) | (3<<8) | 40;
		ip_address_error = false;
	}

	return ip_address_error;
}

// Same return value as connect(), -1 on error
int spark_cloud_socket_connect()
{
    DEBUG("sparkSocket Now =%d", sparkSocket);

    // Close Original
    spark_cloud_socket_disconnect();

    const bool udp =
#if HAL_PLATFORM_CLOUD_UDP
    (HAL_Feature_Get(FEATURE_CLOUD_UDP));
#else
    false;
#endif

    uint16_t port = SPARK_SERVER_PORT;
    if (udp)
    		port = PORT_COAPS;

    ServerAddress server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    HAL_FLASH_Read_ServerAddress(&server_addr);
    DEBUG("HAL_FLASH_Read_ServerAddress() = type:%d,domain:%s,ip: %d.%d.%d.%d, port: %d", server_addr.addr_type, server_addr.domain, IPNUM(server_addr.ip), server_addr.port);

    bool ip_address_error = false;
    IPAddress ip_addr;
    int rv = -1;

    ip_address_error = determine_connection_address(ip_addr, port, server_addr, udp);
    if (!ip_address_error)
    {
    		uint8_t local_port_offset = (PLATFORM_ID==3) ? 100 : 0;
        sparkSocket = socket_create(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, udp ? IPPROTO_UDP : IPPROTO_TCP, port+local_port_offset, NIF_DEFAULT);
        DEBUG("socketed udp=%d, sparkSocket=%d, %d", udp, sparkSocket, socket_handle_valid(sparkSocket));
    }

	if (socket_handle_valid(sparkSocket))
	{
        sockaddr_t tSocketAddr;
        // the family is always AF_INET
        tSocketAddr.sa_family = AF_INET;

        encode_endpoint(tSocketAddr, ip_addr, port);
		DEBUG("connection attempt to %d.%d.%d.%d:%d", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], port);

#if HAL_PLATFORM_CLOUD_UDP
        if (udp)
        {
        		memcpy(&cloud_endpoint.address, &tSocketAddr, sizeof(cloud_endpoint));
        		cloud_endpoint.server_address_checksum = compute_session_checksum(server_addr);
        		rv = 0;
        }
        else
#endif
        {
			uint32_t ot = HAL_NET_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
			rv = socket_connect(sparkSocket, &tSocketAddr, sizeof (tSocketAddr));
			if (rv)
				ERROR("connection failed to %d.%d.%d.%d:%d, code=%d", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], port, rv);
			else
				INFO("connected to cloud %d.%d.%d.%d:%d", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], port);

			HAL_NET_SetNetWatchDog(ot);
        }
    }
    if (rv)     // error - prevent socket leaks
        spark_cloud_socket_disconnect();
    return rv;
}

int spark_cloud_socket_disconnect(void)
{
    int retVal = 0;
    if (socket_handle_valid(sparkSocket))
    {
#if defined(SEND_ON_CLOSE)
        DEBUG("Send Attempt");
        char c = 0;
        int rc = send(sparkSocket, &c, 1, 0);
        DEBUG("send()=%d", rc);
#endif
        DEBUG("Close Attempt");
        retVal = socket_close(sparkSocket);
        DEBUG("socket_close()=%s", (retVal ? "fail":"success"));
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
    const void* result = nullptr;
    if (item) {
    	if (item->update)
            result = item->update(item->userVarKey, item->userVarType, item->userVar, nullptr);
    	else
            result = item->userVar;
    }
    return result;
}

void userFuncScheduleImpl(User_Func_Lookup_Table_t* item, const char* paramString, bool freeParamString, SparkDescriptor::FunctionResultCallback callback)
{
    int result = item->pUserFunc(item->pUserFuncData, paramString, NULL);
    if (freeParamString)
        delete paramString;
    // run the cloud return on the system thread again
    SYSTEM_THREAD_CONTEXT_ASYNC(callback((const void*)result, SparkReturnType::INT));
    callback((const void*)long(result), SparkReturnType::INT);
}

int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved)
{
    // for now, we invoke the function directly and return the result via the callback
    User_Func_Lookup_Table_t* item = find_func_by_key(funcKey);
    if (!item)
        return -1;

#if PLATFORM_THREADING
    paramString = strdup(paramString);      // ensure we have a copy since the oriignal isn't guaranteed to be available once this function returns.
    APPLICATION_THREAD_CONTEXT_ASYNC_RESULT(userFuncScheduleImpl(item, paramString, true, callback), 0);
    userFuncScheduleImpl(item, paramString, true, callback);
#else
    userFuncScheduleImpl(item, paramString, false, callback);
#endif
    return 0;
}

void HAL_NET_notify_socket_closed(sock_handle_t socket)
{
    if (sparkSocket==socket)
    {
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;
    }
}

inline uint8_t spark_cloud_socket_closed()
{
    uint8_t closed = socket_active_status(sparkSocket) == SOCKET_STATUS_INACTIVE;

    if (closed)
    {
        DEBUG("socket_active_status(sparkSocket=%d)==SOCKET_STATUS_INACTIVE", sparkSocket);
    }
    if (closed && sparkSocket != socket_handle_invalid())
    {
        DEBUG("!!!!!!closed && sparkSocket(%d) != SOCKET_INVALID", sparkSocket);
    }
    if (!socket_handle_valid(sparkSocket))
    {
        DEBUG("Not valid: socket_handle_valid(sparkSocket) = %d", socket_handle_valid(sparkSocket));
        closed = true;
    }
    return closed;
}



#else

void HAL_NET_notify_socket_closed(sock_handle_t socket)
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
    if (!socket_handle_valid(multicast_socket)) {
        DEBUG("socket_handle_valid() = %d", socket_handle_valid(multicast_socket));
        return;
    }

    unsigned char announcement[19];
    uint8_t id_length = HAL_device_ID(NULL, 0);
    uint8_t id[id_length];
    HAL_device_ID(id, id_length);
    spark_protocol_presence_announcement(sp, announcement, id);

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
        DEBUG("socket_sendto()");
        socket_sendto(multicast_socket, announcement, 19, 0, &addr, sizeof (sockaddr_t));
    }
    DEBUG("socket_close(multicast_socket)");
    socket_close(multicast_socket);
#endif
}

const int SYSTEM_CLOUD_TIMEOUT = 15*1000;

bool system_cloud_active()
{
#ifndef SPARK_NO_CLOUD
    if (!SPARK_CLOUD_SOCKETED)
        return false;

#if HAL_PLATFORM_CLOUD_UDP
    if (!HAL_Feature_Get(FEATURE_CLOUD_UDP))
#endif
    {
        system_tick_t now = millis();
        if (SPARK_CLOUD_CONNECTED && ((now-lastCloudEvent))>SYSTEM_CLOUD_TIMEOUT)
        {
        	WARN("Disconnecting cloud due to inactivity! %d, %d", now, lastCloudEvent);
        	cloud_disconnect(false);
            return false;
        }
    }
#endif
    return true;
}

void Spark_Sleep(void)
{
	spark_protocol_command(sp, ProtocolCommands::SLEEP);
}

void Spark_Wake(void)
{
	spark_protocol_command(sp, ProtocolCommands::WAKE);
}

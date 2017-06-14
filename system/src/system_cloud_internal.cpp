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

#include "logging.h"

#include "spark_wiring_string.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_ticks.h"
#include "spark_wiring_ipaddress.h"
#include "spark_wiring_led.h"
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
#include "core_hal.h"
#include "hal_platform.h"
#include "system_string_interpolate.h"
#include "dtls_session_persist.h"
#include "bytes2hexbuf.h"
#include "system_event.h"

#include <stdio.h>
#include <stdint.h>

#define IPNUM(ip)       ((ip)>>24)&0xff,((ip)>>16)&0xff,((ip)>> 8)&0xff,((ip)>> 0)&0xff

#ifndef SPARK_NO_CLOUD

using particle::LEDStatus;

int userVarType(const char *varKey);
const void *getUserVar(const char *varKey);
int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved);

static int finish_ota_firmware_update(FileTransfer::Descriptor& file, uint32_t flags, void* module);
static void formatResetReasonEventData(int reason, uint32_t data, char *buf, size_t size);

static sock_handle_t sparkSocket = socket_handle_invalid();

extern uint8_t feature_cloud_udp;

ProtocolFacade* sp;

static uint32_t particle_key_errors = NO_ERROR;

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

inline uint32_t crc(const void* data, size_t len)
{
	return HAL_Core_Compute_CRC32((const uint8_t*)data, len);
}

template <typename T>
uint32_t crc(const T& t)
{
	return crc(&t, sizeof(t));
}

uint32_t string_crc(const char* s)
{
	return crc(s, strlen(s));
}

/**
 * Computes the checksum of the registered functions.
 * The function name is used to compute the checksum.
 */
uint32_t compute_functions_checksum()
{
	uint32_t checksum = 0;
	for (int i = funcs.size(); i-->0; )
    {
		checksum += string_crc(funcs[i].userFuncKey);
    }
	return checksum;
}

/**
 * Computes the checksum of the registered variables.
 * The checksum is derived from the variable name and type.
 */
uint32_t compute_variables_checksum()
{
	uint32_t checksum = 0;
	for (int i = vars.size(); i-->0; )
	{
		checksum += string_crc(vars[i].userVarKey);
		checksum += crc(vars[i].userVarType);
	}
	return checksum;
}

/**
 * Computes the checksum of all functions and variables.
 */
uint32_t compute_describe_app_checksum()
{
	uint32_t chk[2];
	chk[0] = compute_variables_checksum();
	chk[1] = compute_functions_checksum();
	return crc(chk, sizeof(chk));
}

uint32_t compute_describe_system_checksum()
{
    hal_system_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    HAL_System_Info(&info, true, NULL);
	uint32_t checksum = info.platform_id;
	for (int i=0; i<info.module_count; i++)
	{
		checksum += crc(info.modules[i].suffix->sha);
	}
	HAL_System_Info(&info, false, NULL);
    return checksum;
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
        APPLICATION_THREAD_CONTEXT_ASYNC(invokeEventHandlerString(handlerInfoSize, handlerInfo, name, data, reserved));
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

volatile bool cloud_socket_aborted = false;

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
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
        //break from any blocking loop
        return -1;
    }
    DEBUG("send %d", buflen);
	return socket_sendto(sparkSocket, buf, buflen, 0, &cloud_endpoint.address, sizeof(cloud_endpoint.address));
}

int Spark_Receive_UDP(unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        //break from any blocking loop
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
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
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
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
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        //break from any blocking loop
        DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
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
const char* KEY_RESTORE_EVENT = "spark/device/key/restore";

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
    if (!strncmp(name, KEY_RESTORE_EVENT, strlen(KEY_RESTORE_EVENT))) {
        // Restore PSK to DCT/DCD/FLASH
        LOG(INFO,"Restoring Public Server Key and Server Address to flash");
        bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
        unsigned char psk_buf[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];   // 320 (udp) vs 294 (tcp), allocate 320.
        unsigned char server_addr_buf[EXTERNAL_FLASH_SERVER_ADDRESS_LENGTH];
        memset(&psk_buf, 0xff, sizeof(psk_buf));
        memset(&server_addr_buf, 0xff, sizeof(server_addr_buf));
        if (udp) {
            memcpy(&psk_buf, backup_udp_public_server_key, sizeof(backup_udp_public_server_key));
            memcpy(&server_addr_buf, backup_udp_public_server_address, sizeof(backup_udp_public_server_address));
        } else {
            memcpy(&psk_buf, backup_tcp_public_server_key, sizeof(backup_tcp_public_server_key));
            memcpy(&server_addr_buf, backup_tcp_public_server_address, sizeof(backup_tcp_public_server_address));
        }
        HAL_FLASH_Write_ServerPublicKey(psk_buf, udp);
        HAL_FLASH_Write_ServerAddress(server_addr_buf, udp);
    }
}

#if HAL_PLATFORM_CLOUD_UDP
using particle::protocol::SessionPersistOpaque;
using particle::protocol::SessionPersistData;

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

void update_persisted_state(std::function<void(SessionPersistOpaque&)> fn)
{
	SessionPersistOpaque persist;
	if (Spark_Restore(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr)==sizeof(persist) && persist.is_valid())
	{
		fn(persist);
		Spark_Save(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);
	}
}

uint32_t compute_cloud_state_checksum(SparkAppStateSelector::Enum stateSelector, SparkAppStateUpdate::Enum operation, uint32_t value, void* reserved)
{
	if (operation==SparkAppStateUpdate::COMPUTE_AND_PERSIST ) {
		switch (stateSelector)
		{
		case SparkAppStateSelector::DESCRIBE_APP:
			update_persisted_state([](SessionPersistData& data){
				data.describe_app_crc = compute_describe_app_checksum();
			});
		case SparkAppStateSelector::DESCRIBE_SYSTEM:
			update_persisted_state([](SessionPersistData& data){
				data.describe_system_crc = compute_describe_system_checksum();
			});
		}
	}
	else if (operation==SparkAppStateUpdate::PERSIST && stateSelector==SparkAppStateSelector::SUBSCRIPTIONS)
	{
		update_persisted_state([value](SessionPersistData& data){
			data.subscriptions_crc = value;
		});
	}
	else if (operation==SparkAppStateUpdate::COMPUTE)
	{
		switch (stateSelector)
		{
		case SparkAppStateSelector::DESCRIBE_APP:
			return compute_describe_app_checksum();

		case SparkAppStateSelector::DESCRIBE_SYSTEM:
			return compute_describe_system_checksum();
		}
	}
	return 0;
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

        particle_key_errors = NO_ERROR;

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
        //callbacks.finish_firmware_update = Spark_Finish_Firmware_Update;
        callbacks.finish_firmware_update = finish_ota_firmware_update;
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
#if HAL_PLATFORM_CLOUD_UDP
        descriptor.app_state_selector_info = compute_cloud_state_checksum;
#endif
        // todo - this pushes a lot of data on the stack! refactor to remove heavy stack usage
        unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
        unsigned char private_key[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];
        memset(&pubkey, 0xff, sizeof(pubkey));
        memset(&private_key, 0xff, sizeof(private_key));

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

        // if public server key is erased, restore with a backup from system firmware
        if (pubkey[0] == 0xff) {
            LOG(WARN, "Public Server Key was blank, restoring.");
            if (udp) {
                memcpy(&pubkey, backup_udp_public_server_key, sizeof(backup_udp_public_server_key));
            }
            else {
                memcpy(&pubkey, backup_tcp_public_server_key, sizeof(backup_tcp_public_server_key));
            }
            particle_key_errors |= PUBLIC_SERVER_KEY_BLANK;
        }

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

void system_set_time(time_t time, unsigned param, void*)
{
    HAL_RTC_Set_UnixTime(time);
    system_notify_event(time_changed, time_changed_sync);
}

const int CLAIM_CODE_SIZE = 63;

int Spark_Handshake(bool presence_announce)
{
    cloud_socket_aborted = false; // Clear cancellation flag for socket operations
	LOG(INFO,"Starting handshake: presense_announce=%d", presence_announce);
    int err = spark_protocol_handshake(sp);
    if (!err)
    {
        char buf[CLAIM_CODE_SIZE + 1];
        if (!HAL_Get_Claim_Code(buf, sizeof (buf)) && *buf)
        {
            LOG(INFO,"Send spark/device/claim/code event");
            Particle.publish("spark/device/claim/code", buf, 60, PRIVATE);
        }

        // open up for possibility of retrieving multiple ID datums
        if (!HAL_Get_Device_Identifier(NULL, buf, sizeof(buf), 0, NULL) && *buf) {
            LOG(INFO,"Send spark/device/ident/0 event");
            Particle.publish("spark/device/ident/0", buf, 60, PRIVATE);
        }

        bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#if PLATFORM_ID!=PLATFORM_ELECTRON_PRODUCTION || !defined(MODULAR_FIRMWARE)
        ultoa(HAL_OTA_FlashLength(), buf, 10);
        LOG(INFO,"Send spark/hardware/max_binary event");
        Particle.publish("spark/hardware/max_binary", buf, 60, PRIVATE);
#endif

        uint32_t chunkSize = HAL_OTA_ChunkSize();
        if (chunkSize!=512 || !udp) {
            ultoa(chunkSize, buf, 10);
            LOG(INFO,"spark/hardware/ota_chunk_size event");
            Particle.publish("spark/hardware/ota_chunk_size", buf, 60, PRIVATE);
        }
        if (system_mode()==SAFE_MODE) {
            LOG(INFO,"Send spark/device/safemode event");
            Particle.publish("spark/device/safemode","", 60, PRIVATE);
        }
#if defined(SPARK_SUBSYSTEM_EVENT_NAME)
        if (!HAL_core_subsystem_version(buf, sizeof (buf)) && *buf)
        {
            LOG(INFO,"Send spark/" SPARK_SUBSYSTEM_EVENT_NAME " event");
            Particle.publish("spark/" SPARK_SUBSYSTEM_EVENT_NAME, buf, 60, PRIVATE);
        }
#endif
        uint8_t flag = 0;
        if (system_get_flag(SYSTEM_FLAG_PUBLISH_RESET_INFO, &flag, nullptr) == 0 && flag)
        {
            system_set_flag(SYSTEM_FLAG_PUBLISH_RESET_INFO, 0, nullptr); // Publish the reset info only once
            int reason = RESET_REASON_NONE;
            uint32_t data = 0;
            if (HAL_Core_Get_Last_Reset_Info(&reason, &data, nullptr) == 0 && reason != RESET_REASON_NONE)
            {
                char buf[64];
                formatResetReasonEventData(reason, data, buf, sizeof(buf));
                LOG(INFO,"Send spark/device/last_reset event");
                Particle.publish("spark/device/last_reset", buf, 60, PRIVATE);
            }
        }

        if (presence_announce) {
            Multicast_Presence_Announcement();
        }
        LOG(INFO,"Send subscriptions");
        spark_protocol_send_subscriptions(sp);
        // important this comes at the end since it requires a response from the cloud.
        spark_protocol_send_time_request(sp);
        Spark_Process_Events();
    }
    if (err==particle::protocol::SESSION_RESUMED)
    {
        LOG(INFO,"cloud connected from existing session.");
        err = 0;
        if (!HAL_RTC_Time_Is_Valid(nullptr) && spark_sync_time_last(nullptr, nullptr) == 0) {
            spark_protocol_send_time_request(sp);
            Spark_Process_Events();
        }
    }
    if (particle_key_errors != NO_ERROR) {
        char buf[sizeof(unsigned long)*8+1];
        ultoa((unsigned long)particle_key_errors, buf, 10);
        LOG(INFO,"Send event spark/device/key/error=%s", buf);
        Particle.publish("spark/device/key/error", buf, 60, PRIVATE);
    }
    return err;
}

// Returns true if all's well or
//         false on error, meaning we're probably disconnected

inline bool Spark_Communication_Loop(void)
{
    return spark_protocol_event_loop(sp);
}

namespace {

// LED status for the test signal that can be triggered from the cloud
class LEDCloudSignalStatus: public LEDStatus {
public:
    explicit LEDCloudSignalStatus(LEDPriority priority) :
            LEDStatus(LED_PATTERN_CUSTOM, priority),
            ticks_(0),
            index_(0) {
        updateColor();
    }

protected:
    virtual void update(system_tick_t t) override {
        if (t >= ticks_) {
            // Change LED color
            if (++index_ == COLOR_COUNT) {
                index_ = 0;
            }
            updateColor();
        } else {
            ticks_ -= t; // Update timing
        }
    }

private:
    uint16_t ticks_;
    uint8_t index_;

    void updateColor() {
        setColor(COLORS[index_]);
        ticks_ = 100;
    }

    static const uint32_t COLORS[];
    static const size_t COLOR_COUNT;
};

const uint32_t LEDCloudSignalStatus::COLORS[] = { 0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000 }; // VIBGYOR
const size_t LEDCloudSignalStatus::COLOR_COUNT = sizeof(LEDCloudSignalStatus::COLORS) / sizeof(LEDCloudSignalStatus::COLORS[0]);

} // namespace

void Spark_Signal(bool on, unsigned, void*)
{
    static LEDCloudSignalStatus ledCloudSignal(LED_PRIORITY_IMPORTANT);
    ledCloudSignal.setActive(on);
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
                // FIXME: the current session could be moved instead of discarded if the ports differ.
                if (port == p) {
                    DEBUG("using IP/port from session");
                    return 0;
                }
                else {
                    // discard the session
                    persist.invalidate();
                    Spark_Save(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);
                    INFO("connection port mismatch - discarded session");
                    return -1;
                }
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
            if (server_addr.port!=0 && server_addr.port!=65535) {
                port = server_addr.port;
                // DEBUG("PORT READ AS:%d", port);
            }

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

	return ip_address_error;
}

uint16_t cloud_udp_port = PORT_COAPS; // default Particle Cloud UDP port

void spark_cloud_udp_port_set(uint16_t port)
{
    cloud_udp_port = port;
}

// Same return value as connect(), -1 on error
int spark_cloud_socket_connect()
{
    DEBUG("sparkSocket Now =%d", sparkSocket);

    // Close Original
    spark_cloud_socket_disconnect(false);

    const bool udp =
#if HAL_PLATFORM_CLOUD_UDP
    (HAL_Feature_Get(FEATURE_CLOUD_UDP));
#else
    false;
#endif

    uint16_t port = SPARK_SERVER_PORT;
    if (udp) {
        port = cloud_udp_port;
    }

    ServerAddress server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    HAL_FLASH_Read_ServerAddress(&server_addr);
    // if server address is erased, restore with a backup from system firmware
    if (server_addr.addr_type == 0xff) {
        LOG(WARN, "Public Server Address was blank, restoring.");
        if (udp) {
            memcpy(&server_addr, backup_udp_public_server_address, sizeof(backup_udp_public_server_address));
        }
        else {
            memcpy(&server_addr, backup_tcp_public_server_address, sizeof(backup_tcp_public_server_address));
        }
        particle_key_errors |= SERVER_ADDRESS_BLANK;
    }
	switch (server_addr.addr_type)
    {
        case IP_ADDRESS:
            LOG(INFO,"Read Server Address = type:%d,domain:%s,ip: %d.%d.%d.%d, port: %d", server_addr.addr_type, server_addr.domain, IPNUM(server_addr.ip), server_addr.port);
            break;

        case DOMAIN_NAME:
            LOG(INFO,"Read Server Address = type:%d,domain:%s", server_addr.addr_type, server_addr.domain);
            break;

        default:
            LOG(WARN,"Read Server Address = type:%d,defaulting to device.spark.io", server_addr.addr_type);
    }

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
        spark_cloud_socket_disconnect(false);
    return rv;
}

int spark_cloud_socket_disconnect(bool graceful)
{
    int retVal = 0;
    if (socket_handle_valid(sparkSocket))
    {
#if defined(SEND_ON_CLOSE)
        LOG_DEBUG(TRACE, "Send Attempt");
        char c = 0;
        int rc = send(sparkSocket, &c, 1, 0);
        LOG_DEBUG(TRACE, "send()=%d", rc);
#endif
        if (graceful) {
            // Only TCP sockets can be half-closed
            retVal = socket_shutdown(sparkSocket, SHUT_WR);
            if (!retVal) {
                LOG_DEBUG(TRACE, "Half-closed cloud socket");
                if (!spark_protocol_command(sp, ProtocolCommands::DISCONNECT, 0, nullptr)) {
                    // Wait for an error (which means that the server closed our connection).
                    system_tick_t start = millis();
                    while (millis() - start < 5000) {
                        if (!Spark_Communication_Loop())
                            break;
                    }
                }
            } else {
                spark_protocol_command(sp, ProtocolCommands::DISCONNECT, 0, nullptr);
            }
        }
        LOG_DEBUG(TRACE, "Close Attempt");
        retVal = socket_close(sparkSocket);
        LOG_DEBUG(TRACE, "socket_close()=%s", (retVal ? "fail":"success"));
        if (!graceful) {
            spark_protocol_command(sp, ProtocolCommands::TERMINATE, 0, nullptr);
        }
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

int formatOtaUpdateStatusEventData(uint32_t flags, int result, hal_module_t* module, uint8_t *buf, size_t size)
{
    int res = 1;
    memset(buf, 0, size);

    BufferAppender appender(buf, size);
    appender.append("{");
    appender.append("\"r\":");
    appender.append(result ? "\"error\"" : "\"ok\"");

    if (flags & 1) {
        appender.append(",");
        res = ota_update_info(append_instance, &appender, module, false, NULL);
    }

    appender.append("}");

    return res;
}

int finish_ota_firmware_update(FileTransfer::Descriptor& file, uint32_t flags, void* buf)
{
    using namespace particle::protocol;
    hal_module_t module;

    int result = Spark_Finish_Firmware_Update(file, flags, &module);

    if (buf && (flags & (UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY)) == (UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY)) {
        formatOtaUpdateStatusEventData(flags, result, &module, (uint8_t*)buf, 255);
    }

    return result;
}

static const char* resetReasonString(System_Reset_Reason reason)
{
    switch (reason) {
    case RESET_REASON_UNKNOWN:
        return "unknown";
    case RESET_REASON_PIN_RESET:
        return "pin_reset";
    case RESET_REASON_POWER_MANAGEMENT:
        return "power_management";
    case RESET_REASON_POWER_DOWN:
        return "power_down";
    case RESET_REASON_POWER_BROWNOUT:
        return "power_brownout";
    case RESET_REASON_WATCHDOG:
        return "watchdog";
    case RESET_REASON_UPDATE:
        return "update";
    case RESET_REASON_UPDATE_ERROR:
        return "update_error";
    case RESET_REASON_UPDATE_TIMEOUT:
        return "update_timeout";
    case RESET_REASON_FACTORY_RESET:
        return "factory_reset";
    case RESET_REASON_SAFE_MODE:
        return "safe_mode";
    case RESET_REASON_DFU_MODE:
        return "dfu_mode";
    case RESET_REASON_PANIC:
        return "panic";
    case RESET_REASON_USER:
        return "user";
    default:
        return nullptr;
    }
}

static const char* panicCodeString(ePanicCode code)
{
    switch (code) {
    case HardFault:
        return "hard_fault";
    case MemManage:
        return "memory_fault";
    case BusFault:
        return "bus_fault";
    case UsageFault:
        return "usage_fault";
    case OutOfHeap:
        return "out_of_heap";
    case AssertionFailure:
        return "assert_failed";
    case StackOverflow:
        return "stack_overflow";
    default:
        return nullptr;
    }
}

static void formatResetReasonEventData(int reason, uint32_t data, char *buf, size_t size)
{
    // Reset reason
    int n = 0;
    const char* s = resetReasonString((System_Reset_Reason)reason);
    if (s) {
        n = snprintf(buf, size, "%s", s);
    } else {
        n = snprintf(buf, size, "%d", (int)reason); // Print as numeric code
    }
    if (n < 0 || n >= (int)size) {
        return;
    }
    buf += n;
    size -= n;
    // Additional data for selected reason codes
    if (reason == RESET_REASON_PANIC) {
        s = panicCodeString((ePanicCode)data);
        if (s) {
            n = snprintf(buf, size, ", %s", s);
        } else {
            n = snprintf(buf, size, ", %d", (int)data);
        }
    }
}

#else // SPARK_NO_CLOUD

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


bool system_cloud_active()
{
#ifndef SPARK_NO_CLOUD
	const int SYSTEM_CLOUD_TIMEOUT = 15*1000;
    if (!SPARK_CLOUD_SOCKETED)
        return false;

#if HAL_PLATFORM_CLOUD_UDP
    if (!feature_cloud_udp)
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
#ifndef SPARK_NO_CLOUD
	spark_protocol_command(sp, ProtocolCommands::SLEEP);
#endif
}

void Spark_Wake(void)
{
#ifndef SPARK_NO_CLOUD
	spark_protocol_command(sp, ProtocolCommands::WAKE);
#endif
}

void Spark_Abort() {
#ifndef SPARK_NO_CLOUD
    cloud_socket_aborted = true;
#endif
}

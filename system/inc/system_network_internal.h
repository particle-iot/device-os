#ifndef SYSTEM_NETWORK_INTERNAL_H
#define	SYSTEM_NETWORK_INTERNAL_H


enum eWanTimings
{
    CONNECT_TO_ADDRESS_MAX = S2M(10),
    DISCONNECT_TO_RECONNECT = S2M(2),
};
    
    
extern volatile uint8_t WLAN_DISCONNECT;
extern volatile uint8_t WLAN_DHCP;
extern volatile uint8_t WLAN_MANUAL_CONNECT;
extern volatile uint8_t WLAN_DELETE_PROFILES;
extern volatile uint8_t WLAN_SMART_CONFIG_START;
extern volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
extern volatile uint8_t WLAN_SERIAL_CONFIG_DONE;
extern volatile uint8_t WLAN_SMART_CONFIG_STOP;
extern volatile uint8_t WLAN_CAN_SHUTDOWN;

extern volatile uint8_t SPARK_WLAN_RESET;
extern volatile uint8_t SPARK_WLAN_SLEEP;
extern volatile uint8_t SPARK_WLAN_STARTED;

extern volatile uint8_t SPARK_LED_FADE;
void manage_smart_config();
void manage_ip_config();



#endif	/* SYSTEM_NETWORK_INTERNAL_H */


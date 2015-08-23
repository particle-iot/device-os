#ifndef SYSTEM_NETWORK_INTERNAL_H
#define	SYSTEM_NETWORK_INTERNAL_H


enum eWanTimings
{
    CONNECT_TO_ADDRESS_MAX = S2M(30),
    DISCONNECT_TO_RECONNECT = S2M(2),
};

extern volatile uint8_t WLAN_CONNECTED;
extern volatile uint8_t WLAN_DISCONNECT;
extern volatile uint8_t WLAN_DHCP;
extern volatile uint8_t WLAN_MANUAL_CONNECT;
extern volatile uint8_t WLAN_DELETE_PROFILES;
extern volatile uint8_t WLAN_SMART_CONFIG_START;
extern volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
extern volatile uint8_t WLAN_SERIAL_CONFIG_DONE;
extern volatile uint8_t WLAN_SMART_CONFIG_STOP;
extern volatile uint8_t WLAN_CAN_SHUTDOWN;

extern volatile uint8_t PARTICLE_WLAN_RESET;
extern volatile uint8_t PARTICLE_WLAN_SLEEP;
extern volatile uint8_t PARTICLE_WLAN_STARTED;

extern volatile uint8_t PARTICLE_LED_FADE;
void manage_smart_config();
void manage_ip_config();

extern uint32_t wlan_watchdog_duration;
extern uint32_t wlan_watchdog_base;

#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif


inline void ARM_WLAN_WD(uint32_t x) {
    wlan_watchdog_base = HAL_Timer_Get_Milli_Seconds();
    wlan_watchdog_duration = x;
    WAN_WD_DEBUG("WD Set %d",(x));
}
inline bool WLAN_WD_TO() {
    return wlan_watchdog_duration && ((HAL_Timer_Get_Milli_Seconds()-wlan_watchdog_base)>wlan_watchdog_duration);
}

inline void CLR_WLAN_WD() {
    wlan_watchdog_duration = 0;
    WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog_duration);
}



#endif	/* SYSTEM_NETWORK_INTERNAL_H */


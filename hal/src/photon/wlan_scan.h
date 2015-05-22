/* 
 * File:   wlan_scan.h
 * Author: mat1
 *
 * Created on March 23, 2015, 1:41 PM
 */

#ifndef WLAN_SCAN_H
#define	WLAN_SCAN_H

#include <stdint.h>

typedef void (*scan_ap_callback)(void* callback_data, const uint8_t* ssid, unsigned ssid_len, int rssi);
void wlan_scan_aps(scan_ap_callback callback, void* callback_data);


#endif	/* WLAN_SCAN_H */


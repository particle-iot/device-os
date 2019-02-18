/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
 */

#ifndef BLE_HAL_DEFINES_H
#define BLE_HAL_DEFINES_H


/**
 * @addtogroup ble_defines
 *
 * @brief
 *   This module provides BLE HAL layer macro definitions.
 * @{
 */

/** @defgroup BLE device address
 * @{ */
#define BLE_SIG_ADDR_LEN                                        (6)

// BLE device address type
#define BLE_SIG_ADDR_TYPE_PUBLIC                                0x00 /**< Public (identity) address.*/
#define BLE_SIG_ADDR_TYPE_RANDOM_STATIC                         0x01 /**< Random static (identity) address. */
#define BLE_SIG_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE             0x02 /**< Random private resolvable address. */
#define BLE_SIG_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE         0x03 /**< Random private non-resolvable address. */
#define BLE_SIG_ADDR_TYPE_ANONYMOUS                             0x7F /**< An advertiser may advertise without its address. This type of advertising is called anonymous. */
/** @} */


/** @defgroup BLE_UUID_VALUES Assigned Values for BLE UUIDs
 * @{ */
#define BLE_SIG_UUID_16BIT_LEN                                  (2)
#define BLE_SIG_UUID_128BIT_LEN                                 (16)

#define BLE_SIG_UUID_UNKOWN                                     0x0000 /**< Reserved UUID. */

// Found at https://www.bluetooth.com/specifications/gatt/declarations
#define BLE_SIG_UUID_PRIMARY_SVC_DECL                           0x2800 /**< BLE primary service declaration attribute UUID. */
#define BLE_SIG_UUID_SECONDARY_SVC_DECL                         0x2801 /**< BLE secondary service declaration attribute UUID. */
#define BLE_SIG_UUID_INCLUDE_SVC_DECL                           0x2802 /**< BLE include service declaration attribute UUID. */
#define BLE_SIG_UUID_CHAR_DECL                                  0x2803 /**< BLE characteristic declaration attribute UUID. */

// Found at https://www.bluetooth.com/specifications/gatt/services
#define BLE_SIG_UUID_GENERIC_ACCESS_SVC                         0x1800 /**< Generic Access Service UUID. */
#define BLE_SIG_UUID_GENERIC_ATTRIBUTE_SVC                      0x1801 /**< Generic Attribute Service UUID */
#define BLE_SIG_UUID_IMMEDIATE_ALERT_SVC                        0x1802 /**< Immediate Alert Service UUID */
#define BLE_SIG_UUID_LINK_LOSS_SVC                              0x1803 /**< Link Loss Service UUID */
#define BLE_SIG_UUID_TX_POWER_SVC                               0x1804 /**< Tx Power Service UUID */
#define BLE_SIG_UUID_CURRENT_TIME_SVC                           0x1805 /**< Current Time Service UUID */
#define BLE_SIG_UUID_REFERENCE_TIME_UPDATE_SVC                  0x1806 /**< Reference Time Update Service UUID */
#define BLE_SIG_UUID_NEXT_DST_CHANGE_SVC                        0x1807 /**< Next DST Change Service */
#define BLE_SIG_UUID_GLUCOSE_SVC                                0x1808 /**< Glucose Service UUID */
#define BLE_SIG_UUID_HEALTH_THERMONETER_SVC                     0x1809 /**< Health Thermometer Service UUID */
#define BLE_SIG_UUID_DEVICE_INFORMATION_SVC                     0x180A /**< Device Information Service UUID */
#define BLE_SIG_UUID_HEART_RATE_SVC                             0x180D /**< Heart Rate Service UUID */
#define BLE_SIG_UUID_PHONE_ALERT_STATUS_SVC                     0x180E /**< Phone Alert Status Service UUID */
#define BLE_SIG_UUID_BATTERY_SVC                                0x180F /**< Battery Service UUID */
#define BLE_SIG_UUID_BLOOD_PRESSURE_SVC                         0x1810 /**< Blood Pressure Service UUID */
#define BLE_SIG_UUID_ALERT_NOTIFICATION_SVC                     0x1811 /**< Alert Notification Service UUID */
#define BLE_SIG_UUID_HUMAN_INTERFACE_DEVICE_SVC                 0x1812 /**< Human Interface Device Service UUID */
#define BLE_SIG_UUID_SCAN_PARAMETERS_SVC                        0x1813 /**< Scan Parameters Service UUID */
#define BLE_SIG_UUID_RUNNING_SPEED_CADENCE_SVC                  0x1814 /**< Running Speed and Cadence Service UUID */
#define BLE_SIG_UUID_AUTOMATION_IO_SVC                          0x1815 /**< Automation IO Service UUID */
#define BLE_SIG_UUID_CYCLING_SPEED_CADENCE_SVC                  0x1816 /**< Cycling Speed and Cadence Service UUID */
#define BLE_SIG_UUID_CYCLING_POWER_SVC                          0x1818 /**< Cycling Power Service UUID */
#define BLE_SIG_UUID_LOCATION_NAVIGATION_SVC                    0x1819 /**< Location and Navigation Service UUID */
#define BLE_SIG_UUID_ENVIRONMENT_SENSING_SVC                    0x181A /**< Environment Sensing Service UUID */
#define BLE_SIG_UUID_BODY_COMPOSITION_SVC                       0x181B /**< Body Composition Service UUID */
#define BLE_SIG_UUID_USER_DATA_SVC                              0x181C /**< User Data Service UUID */
#define BLE_SIG_UUID_WEIGHT_SCALE_SVC                           0x181D /**< Weight Scale Service UUID */
#define BLE_SIG_UUID_BOND_MANAGEMENT_SVC                        0x181E /**< Bond Management Service UUID */
#define BLE_SIG_UUID_CONTINUOUS_GLUCOSE_MONITORING_SVC          0x181F /**< Continuous Glucose Monitoring Service UUID */
#define BLE_SIG_UUID_INTERNET_PROTOCOL_SUPPORT_SVC              0x1820 /**< Internet Protocol Support Service UUID */
#define BLE_SIG_UUID_INDOOR_POSITIONING_SVC                     0x1821 /**< Indoor Positioning Service UUID */
#define BLE_SIG_UUID_PULSE_OXIMETER_SVC                         0x1822 /**< Pulse Oximeter Service UUID */
#define BLE_SIG_UUID_HTTP_PROXY_SVC                             0x1823 /**< HTTP Proxy Service UUID */
#define BLE_SIG_UUID_TRANSPORT_DISCOVERY_SVC                    0x1824 /**< Transport Discovery Service UUID */
#define BLE_SIG_UUID_OBJECT_TRANSFER_SVC                        0x1825 /**< Object Transfer Service UUID */
#define BLE_SIG_UUID_FITNESS_MACHINE_SVC                        0x1826 /**< Fitness Machine Service UUID */
#define BLE_SIG_UUID_MESH_PROVISIONING_SVC                      0x1827 /**< Mesh Provisioning Service UUID */
#define BLE_SIG_UUID_MESH_PROXY_SVC                             0x1828 /**< Mesh Proxy Service UUID */
#define BLE_SIG_UUID_RECONNECTION_CONFIGURATION_SVC             0x1829 /**< Reconnection Configuration Service UUID */
#define BLE_SIG_UUID_INSULIN_DELIVERY_SVC                       0x183A /**< Insulin Delivery Service UUID */

// Found at https://www.bluetooth.com/specifications/gatt/characteristics
#define BLE_SIG_UUID_DEVICE_NAME_CHAR                           0x2A00 /**< Device Name Characteristic UUID. It shall be included in the Generic Access Service. */
#define BLE_SIG_UUID_APPEARANCE_CHAR                            0x2A01 /**< Appearance Characteristic UUID. It shall be included in the Generic Access Service. */
#define BLE_SIG_UUID_PERIPHERAL_PRIVACY_FLAG_CHAR               0x2A02 /**< Peripheral Privacy Flag Characteristic UUID. It shall be included in the Generic Access Service. */
#define BLE_SIG_UUID_RECONNECTION_ADDRESS_CHAR                  0x2A03 /**< Reconnection Address Characteristic UUID. It shall be included in the Generic Access Service. */
#define BLE_SIG_UUID_PPCP_CHAR                                  0x2A04 /**< Peripheral Preferred Connection Parameters Characteristic UUID. It shall be included in the Generic Access Service. */
#define BLE_SIG_UUID_SERVICE_CHANGED_CHAR                       0x2A05 /**< Service Changed Characteristic UUID. It shall be included in the Generic Attribute Service. */

// Found at https://www.bluetooth.com/specifications/gatt/descriptors
#define BLE_SIG_UUID_CHAR_EXTENDED_PROPERTIES_DESC              0x2900 /**< Characteristic Extended Properties Descriptor UUID. */
#define BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC                 0x2901 /**< Characteristic User Description Descriptor UUID. */
#define BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC                    0x2902 /**< Client Characteristic Configuration Descriptor UUID. */
#define BLE_SIG_UUID_SERVER_CHAR_CONFIG_DESC                    0x2903 /**< Server Characteristic Configuration Descriptor UUID. */
#define BLE_SIG_UUID_CHAR_PRESENT_FORMAT_DESC                   0x2904 /**< Characteristic Presentation Format Descriptor UUID. */
#define BLE_SIG_UUID_CHAR_AGGREGATE_FORMAT                      0x2905 /**< Characteristic Aggregate Format Descriptor UUID. */
/** @} */


/** @defgroup BLE_APPEARANCES Bluetooth Appearance values
 *  @note Retrieved from http://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
 * @{ */
#define BLE_SIG_APPEARANCE_UNKNOWN                              0    /**< Unknown. */
#define BLE_SIG_APPEARANCE_GENERIC_PHONE                        64   /**< Generic Phone. */
#define BLE_SIG_APPEARANCE_GENERIC_COMPUTER                     128  /**< Generic Computer. */
#define BLE_SIG_APPEARANCE_GENERIC_WATCH                        192  /**< Generic Watch. */
#define BLE_SIG_APPEARANCE_WATCH_SPORTS_WATCH                   193  /**< Watch: Sports Watch. */
#define BLE_SIG_APPEARANCE_GENERIC_CLOCK                        256  /**< Generic Clock. */
#define BLE_SIG_APPEARANCE_GENERIC_DISPLAY                      320  /**< Generic Display. */
#define BLE_SIG_APPEARANCE_GENERIC_REMOTE_CONTROL               384  /**< Generic Remote Control. */
#define BLE_SIG_APPEARANCE_GENERIC_EYE_GLASSES                  448  /**< Generic Eye-glasses. */
#define BLE_SIG_APPEARANCE_GENERIC_TAG                          512  /**< Generic Tag. */
#define BLE_SIG_APPEARANCE_GENERIC_KEYRING                      576  /**< Generic Keyring. */
#define BLE_SIG_APPEARANCE_GENERIC_MEDIA_PLAYER                 640  /**< Generic Media Player. */
#define BLE_SIG_APPEARANCE_GENERIC_BARCODE_SCANNER              704  /**< Generic Barcode Scanner. */
#define BLE_SIG_APPEARANCE_GENERIC_THERMOMETER                  768  /**< Generic Thermometer. */
#define BLE_SIG_APPEARANCE_THERMOMETER_EAR                      769  /**< Thermometer: Ear. */
#define BLE_SIG_APPEARANCE_GENERIC_HEART_RATE_SENSOR            832  /**< Generic Heart rate Sensor. */
#define BLE_SIG_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT    833  /**< Heart Rate Sensor: Heart Rate Belt. */
#define BLE_SIG_APPEARANCE_GENERIC_BLOOD_PRESSURE               896  /**< Generic Blood Pressure. */
#define BLE_SIG_APPEARANCE_BLOOD_PRESSURE_ARM                   897  /**< Blood Pressure: Arm. */
#define BLE_SIG_APPEARANCE_BLOOD_PRESSURE_WRIST                 898  /**< Blood Pressure: Wrist. */
#define BLE_SIG_APPEARANCE_GENERIC_HID                          960  /**< Human Interface Device (HID). */
#define BLE_SIG_APPEARANCE_HID_KEYBOARD                         961  /**< Keyboard (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_MOUSE                            962  /**< Mouse (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_JOYSTICK                         963  /**< Joystick (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_GAMEPAD                          964  /**< Gamepad (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_DIGITIZERSUBTYPE                 965  /**< Digitizer Tablet (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_CARD_READER                      966  /**< Card Reader (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_DIGITAL_PEN                      967  /**< Digital Pen (HID Subtype). */
#define BLE_SIG_APPEARANCE_HID_BARCODE                          968  /**< Barcode Scanner (HID Subtype). */
#define BLE_SIG_APPEARANCE_GENERIC_GLUCOSE_METER                1024 /**< Generic Glucose Meter. */
#define BLE_SIG_APPEARANCE_GENERIC_RUNNING_WALKING_SENSOR       1088 /**< Generic Running Walking Sensor. */
#define BLE_SIG_APPEARANCE_RUNNING_WALKING_SENSOR_IN_SHOE       1089 /**< Running Walking Sensor: In-Shoe. */
#define BLE_SIG_APPEARANCE_RUNNING_WALKING_SENSOR_ON_SHOE       1090 /**< Running Walking Sensor: On-Shoe. */
#define BLE_SIG_APPEARANCE_RUNNING_WALKING_SENSOR_ON_HIP        1091 /**< Running Walking Sensor: On-Hip. */
#define BLE_SIG_APPEARANCE_GENERIC_CYCLING                      1152 /**< Generic Cycling. */
#define BLE_SIG_APPEARANCE_CYCLING_CYCLING_COMPUTER             1153 /**< Cycling: Cycling Computer. */
#define BLE_SIG_APPEARANCE_CYCLING_SPEED_SENSOR                 1154 /**< Cycling: Speed Sensor. */
#define BLE_SIG_APPEARANCE_CYCLING_CADENCE_SENSOR               1155 /**< Cycling: Cadence Sensor. */
#define BLE_SIG_APPEARANCE_CYCLING_POWER_SENSOR                 1156 /**< Cycling: Power Sensor. */
#define BLE_SIG_APPEARANCE_CYCLING_SPEED_CADENCE_SENSOR         1157 /**< Cycling: Speed and Cadence Sensor. */
#define BLE_SIG_APPEARANCE_GENERIC_PULSE_OXIMETER               3136 /**< Generic Pulse Oximeter. */
#define BLE_SIG_APPEARANCE_PULSE_OXIMETER_FINGERTIP             3137 /**< Fingertip (Pulse Oximeter subtype). */
#define BLE_SIG_APPEARANCE_PULSE_OXIMETER_WRIST_WORN            3138 /**< Wrist Worn(Pulse Oximeter subtype). */
#define BLE_SIG_APPEARANCE_GENERIC_WEIGHT_SCALE                 3200 /**< Generic Weight Scale. */
#define BLE_SIG_APPEARANCE_GENERIC_OUTDOOR_SPORTS_ACT           5184 /**< Generic Outdoor Sports Activity. */
#define BLE_SIG_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_DISP          5185 /**< Location Display Device (Outdoor Sports Activity subtype). */
#define BLE_SIG_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_AND_NAV_DISP  5186 /**< Location and Navigation Display Device (Outdoor Sports Activity subtype). */
#define BLE_SIG_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_POD           5187 /**< Location Pod (Outdoor Sports Activity subtype). */
#define BLE_SIG_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_AND_NAV_POD   5188 /**< Location and Navigation Pod (Outdoor Sports Activity subtype). */
/** @} */


/**@defgroup BLE_GAP_AD_TYPE_DEFINITIONS GAP Advertising and Scan Response Data format
 * @note Found at https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 * @{ */
#define BLE_SIG_AD_TYPE_FLAGS                                   0x01 /**< Flags for discoverability. */
#define BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE       0x02 /**< Partial list of 16 bit service UUIDs. */
#define BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE             0x03 /**< Complete list of 16 bit service UUIDs. */
#define BLE_SIG_AD_TYPE_32BIT_SERVICE_UUID_MORE_AVAILABLE       0x04 /**< Partial list of 32 bit service UUIDs. */
#define BLE_SIG_AD_TYPE_32BIT_SERVICE_UUID_COMPLETE             0x05 /**< Complete list of 32 bit service UUIDs. */
#define BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE      0x06 /**< Partial list of 128 bit service UUIDs. */
#define BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE            0x07 /**< Complete list of 128 bit service UUIDs. */
#define BLE_SIG_AD_TYPE_SHORT_LOCAL_NAME                        0x08 /**< Short local device name. */
#define BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME                     0x09 /**< Complete local device name. */
#define BLE_SIG_AD_TYPE_TX_POWER_LEVEL                          0x0A /**< Transmit power level. */
#define BLE_SIG_AD_TYPE_CLASS_OF_DEVICE                         0x0D /**< Class of device. */
#define BLE_SIG_AD_TYPE_SIMPLE_PAIRING_HASH_C                   0x0E /**< Simple Pairing Hash C. */
#define BLE_SIG_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R             0x0F /**< Simple Pairing Randomizer R. */
#define BLE_SIG_AD_TYPE_SECURITY_MANAGER_TK_VALUE               0x10 /**< Security Manager TK Value. */
#define BLE_SIG_AD_TYPE_SECURITY_MANAGER_OOB_FLAGS              0x11 /**< Security Manager Out Of Band Flags. */
#define BLE_SIG_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE         0x12 /**< Slave Connection Interval Range. */
#define BLE_SIG_AD_TYPE_SOLICITED_SERVICE_UUIDS_16BIT           0x14 /**< List of 16-bit Service Solicitation UUIDs. */
#define BLE_SIG_AD_TYPE_SOLICITED_SERVICE_UUIDS_128BIT          0x15 /**< List of 128-bit Service Solicitation UUIDs. */
#define BLE_SIG_AD_TYPE_SERVICE_DATA                            0x16 /**< Service Data - 16-bit UUID. */
#define BLE_SIG_AD_TYPE_PUBLIC_TARGET_ADDRESS                   0x17 /**< Public Target Address. */
#define BLE_SIG_AD_TYPE_RANDOM_TARGET_ADDRESS                   0x18 /**< Random Target Address. */
#define BLE_SIG_AD_TYPE_APPEARANCE                              0x19 /**< Appearance. */
#define BLE_SIG_AD_TYPE_ADVERTISING_INTERVAL                    0x1A /**< Advertising Interval. */
#define BLE_SIG_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS             0x1B /**< LE Bluetooth Device Address. */
#define BLE_SIG_AD_TYPE_LE_ROLE                                 0x1C /**< LE Role. */
#define BLE_SIG_AD_TYPE_SIMPLE_PAIRING_HASH_C256                0x1D /**< Simple Pairing Hash C-256. */
#define BLE_SIG_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R256          0x1E /**< Simple Pairing Randomizer R-256. */
#define BLE_SIG_AD_TYPE_32BIT_SERVICE_SOLICITATION_UUID         0x1F /**< List of 32-bit Service Solicitation UUIDs. */
#define BLE_SIG_AD_TYPE_SERVICE_DATA_32BIT_UUID                 0x20 /**< Service Data - 32-bit UUID. */
#define BLE_SIG_AD_TYPE_SERVICE_DATA_128BIT_UUID                0x21 /**< Service Data - 128-bit UUID. */
#define BLE_SIG_AD_TYPE_LESC_CONFIRMATION_VALUE                 0x22 /**< LE Secure Connections Confirmation Value */
#define BLE_SIG_AD_TYPE_LESC_RANDOM_VALUE                       0x23 /**< LE Secure Connections Random Value */
#define BLE_SIG_AD_TYPE_URI                                     0x24 /**< URI */
#define BLE_SIG_AD_TYPE_INDOOR_POSITIONING                      0x25 /**< Indoor Positioning */
#define BLE_SIG_AD_TYPE_TRANSPORT_DISCOVERY_DATA                0x26 /**< Transport Discovery Data */
#define BLE_SIG_AD_TYPE_LE_SUPPORTED_FEATURES                   0x27 /**< LE Supported Features */
#define BLE_SIG_AD_TYPE_CHANNEL_MAP_UPDATE_INDICATION           0x28 /**< Channel Map Update Indication */
#define BLE_SIG_AD_TYPE_PB_ADV                                  0x29 /**< PB-ADV */
#define BLE_SIG_AD_TYPE_MESH_MESSAGE                            0x2A /**< Mesh Message */
#define BLE_SIG_AD_TYPE_MESH_BEACON                             0x2B /**< Mesh Beacon */
#define BLE_SIG_AD_TYPE_3D_INFORMATION_DATA                     0x3D /**< 3D Information Data. */
#define BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA              0xFF /**< Manufacturer Specific Data. */
/**@} */


/**@defgroup BLE_GAP_ADV_FLAGS GAP Advertisement Flags
 * @{ */
#define BLE_SIG_ADV_FLAG_LE_LIMITED_DISC_MODE                   (0x01) /**< LE Limited Discoverable Mode. */
#define BLE_SIG_ADV_FLAG_LE_GENERAL_DISC_MODE                   (0x02) /**< LE General Discoverable Mode. */
#define BLE_SIG_ADV_FLAG_BR_EDR_NOT_SUPPORTED                   (0x04) /**< BR/EDR not supported. */
#define BLE_SIG_ADV_FLAG_LE_BR_EDR_CONTROLLER                   (0x08) /**< Simultaneous LE and BR/EDR, Controller. */
#define BLE_SIG_ADV_FLAG_LE_BR_EDR_HOST                         (0x10) /**< Simultaneous LE and BR/EDR, Host. */
#define BLE_SIG_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE             (BLE_SIG_ADV_FLAG_LE_LIMITED_DISC_MODE | BLE_SIG_ADV_FLAG_BR_EDR_NOT_SUPPORTED) /**< LE Limited Discoverable Mode, BR/EDR not supported. */
#define BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE             (BLE_SIG_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_SIG_ADV_FLAG_BR_EDR_NOT_SUPPORTED) /**< LE General Discoverable Mode, BR/EDR not supported. */
/**@} */


/**@defgroup BLE connection parameters limits
 * @{ */
#define BLE_SIG_CP_MIN_CONN_INTERVAL_NONE                       0xFFFF /**< No limit of the Minimal Connection Interval. */
#define BLE_SIG_CP_MIN_CONN_INTERVAL_MIN                        0x0006 /**< Minimal value of the Minimal Connection Interval, in units of 1.25 ms. */
#define BLE_SIG_CP_MIN_CONN_INTERVAL_MAX                        0x0C80 /**< Maximum value of the Minimal Connection Interval, in units of 1.25 ms. */
#define BLE_SIG_CP_MAX_CONN_INTERVAL_NONE                       0xFFFF /**< No limit of the Maximum Connection Interval. */
#define BLE_SIG_CP_MAX_CONN_INTERVAL_MIN                        0x0006 /**< Minimal value of the Maximum Connection Interval, in units of 1.25 ms. */
#define BLE_SIG_CP_MAX_CONN_INTERVAL_MAX                        0x0C80 /**< Maximal value of the Maximum Connection Interval, in units of 1.25 ms. */
#define BLE_SIG_CP_SLAVE_LATENCY_MAX                            0x01F3 /**< Maximal value of the Slave Latency. */
#define BLE_SIG_CP_CONN_SUP_TIMEOUT_NONE                        0xFFFF /**< No limit of the Connection Supervision Timeout. */
#define BLE_SIG_CP_CONN_SUP_TIMEOUT_MIN                         0x000A /**< Minimal value of the Connection Supervision Timeout, in units of 10 ms. */
#define BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX                         0x0C80 /**< Maximal value of the Connection Supervision Timeout, in units of 10 ms. */
/**@} */


/**@defgroup BLE Characteristic properties
 * @{ */
#define BLE_SIG_CHAR_PROP_BROADCAST                             0x01 /**< Broadcaster property bit mask. */
#define BLE_SIG_CHAR_PROP_READ                                  0x02 /**< Read property bit mask. */
#define BLE_SIG_CHAR_PROP_WRITE_WO_RESP                         0x04 /**< Write Without Response property bit mask. */
#define BLE_SIG_CHAR_PROP_WRITE                                 0x08 /**< Write With Response property bit mask. */
#define BLE_SIG_CHAR_PROP_NOTIFY                                0x10 /**< Notify property bit mask. */
#define BLE_SIG_CHAR_PROP_INDICATE                              0x20 /**< Indication property bit mask. */
#define BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES                      0x40 /**< Authenticated Signed Writes property bit mask. */
#define BLE_SIG_CHAR_PROP_EXTENDED_PROP                         0x80 /**< Extended Properties bit mask. */
/**@} */


/**@defgroup BLE Characteristic properties
 * @{ */
#define BLE_SIG_CCCD_VAL_DISABLED                               0x00 /**< Neither notification nor indication is enabled. */
#define BLE_SIG_CCCD_VAL_NOTIFICATION                           0x01 /**< Notification is enabled. */
#define BLE_SIG_CCCD_VAL_INDICATION                             0x02 /**< Indication is enabled. */
/**@} */


/**
 * @}
 */

#endif /* BLE_HAL_DEFINES_H */

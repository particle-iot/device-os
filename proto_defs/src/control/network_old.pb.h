/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.5 */

#ifndef PB_PARTICLE_CTRL_CONTROL_NETWORK_OLD_PB_H_INCLUDED
#define PB_PARTICLE_CTRL_CONTROL_NETWORK_OLD_PB_H_INCLUDED
#include <pb.h>
#include "control/extensions.pb.h"
#include "control/common.pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _particle_ctrl_NetworkState { 
    particle_ctrl_NetworkState_NETWORK_STATUS_NONE = 0, 
    particle_ctrl_NetworkState_DOWN = 1, 
    particle_ctrl_NetworkState_UP = 2 
} particle_ctrl_NetworkState;

typedef enum _particle_ctrl_IPConfiguration_Type { 
    particle_ctrl_IPConfiguration_Type_NONE = 0, 
    particle_ctrl_IPConfiguration_Type_DHCP = 1, 
    particle_ctrl_IPConfiguration_Type_STATIC = 2 
} particle_ctrl_IPConfiguration_Type;

/* Struct definitions */
typedef struct _particle_ctrl_DNSConfiguration { 
    pb_callback_t servers; 
} particle_ctrl_DNSConfiguration;

typedef struct _particle_ctrl_NetworkSetConfigurationReply { 
    char dummy_field;
} particle_ctrl_NetworkSetConfigurationReply;

typedef struct _particle_ctrl_IPConfiguration { 
    particle_ctrl_IPConfiguration_Type type; 
    particle_ctrl_IPAddress address; 
    particle_ctrl_IPAddress netmask; 
    particle_ctrl_IPAddress gateway; 
    particle_ctrl_IPAddress dhcp_server; 
    pb_callback_t hostname; 
} particle_ctrl_IPConfiguration;

typedef struct _particle_ctrl_NetworkGetConfigurationRequest { 
    int32_t interface; 
} particle_ctrl_NetworkGetConfigurationRequest;

typedef struct _particle_ctrl_NetworkGetStatusRequest { 
    int32_t interface; 
} particle_ctrl_NetworkGetStatusRequest;

typedef PB_BYTES_ARRAY_T(6) particle_ctrl_NetworkConfiguration_mac_t;
typedef struct _particle_ctrl_NetworkConfiguration { 
    int32_t interface; 
    particle_ctrl_NetworkState state; 
    pb_callback_t name; 
    particle_ctrl_NetworkConfiguration_mac_t mac; 
    particle_ctrl_IPConfiguration ipconfig; 
    particle_ctrl_DNSConfiguration dnsconfig; 
} particle_ctrl_NetworkConfiguration;

typedef struct _particle_ctrl_NetworkGetConfigurationReply { 
    particle_ctrl_NetworkConfiguration config; 
} particle_ctrl_NetworkGetConfigurationReply;

typedef struct _particle_ctrl_NetworkGetStatusReply { 
    particle_ctrl_NetworkConfiguration config; 
} particle_ctrl_NetworkGetStatusReply;

typedef struct _particle_ctrl_NetworkSetConfigurationRequest { 
    particle_ctrl_NetworkConfiguration config; 
} particle_ctrl_NetworkSetConfigurationRequest;


/* Helper constants for enums */
#define _particle_ctrl_NetworkState_MIN particle_ctrl_NetworkState_NETWORK_STATUS_NONE
#define _particle_ctrl_NetworkState_MAX particle_ctrl_NetworkState_UP
#define _particle_ctrl_NetworkState_ARRAYSIZE ((particle_ctrl_NetworkState)(particle_ctrl_NetworkState_UP+1))

#define _particle_ctrl_IPConfiguration_Type_MIN particle_ctrl_IPConfiguration_Type_NONE
#define _particle_ctrl_IPConfiguration_Type_MAX particle_ctrl_IPConfiguration_Type_STATIC
#define _particle_ctrl_IPConfiguration_Type_ARRAYSIZE ((particle_ctrl_IPConfiguration_Type)(particle_ctrl_IPConfiguration_Type_STATIC+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define particle_ctrl_NetworkGetStatusRequest_init_default {0}
#define particle_ctrl_NetworkGetStatusReply_init_default {particle_ctrl_NetworkConfiguration_init_default}
#define particle_ctrl_NetworkGetConfigurationRequest_init_default {0}
#define particle_ctrl_NetworkGetConfigurationReply_init_default {particle_ctrl_NetworkConfiguration_init_default}
#define particle_ctrl_NetworkSetConfigurationRequest_init_default {particle_ctrl_NetworkConfiguration_init_default}
#define particle_ctrl_NetworkSetConfigurationReply_init_default {0}
#define particle_ctrl_IPConfiguration_init_default {_particle_ctrl_IPConfiguration_Type_MIN, particle_ctrl_IPAddress_init_default, particle_ctrl_IPAddress_init_default, particle_ctrl_IPAddress_init_default, particle_ctrl_IPAddress_init_default, {{NULL}, NULL}}
#define particle_ctrl_DNSConfiguration_init_default {{{NULL}, NULL}}
#define particle_ctrl_NetworkConfiguration_init_default {0, _particle_ctrl_NetworkState_MIN, {{NULL}, NULL}, {0, {0}}, particle_ctrl_IPConfiguration_init_default, particle_ctrl_DNSConfiguration_init_default}
#define particle_ctrl_NetworkGetStatusRequest_init_zero {0}
#define particle_ctrl_NetworkGetStatusReply_init_zero {particle_ctrl_NetworkConfiguration_init_zero}
#define particle_ctrl_NetworkGetConfigurationRequest_init_zero {0}
#define particle_ctrl_NetworkGetConfigurationReply_init_zero {particle_ctrl_NetworkConfiguration_init_zero}
#define particle_ctrl_NetworkSetConfigurationRequest_init_zero {particle_ctrl_NetworkConfiguration_init_zero}
#define particle_ctrl_NetworkSetConfigurationReply_init_zero {0}
#define particle_ctrl_IPConfiguration_init_zero  {_particle_ctrl_IPConfiguration_Type_MIN, particle_ctrl_IPAddress_init_zero, particle_ctrl_IPAddress_init_zero, particle_ctrl_IPAddress_init_zero, particle_ctrl_IPAddress_init_zero, {{NULL}, NULL}}
#define particle_ctrl_DNSConfiguration_init_zero {{{NULL}, NULL}}
#define particle_ctrl_NetworkConfiguration_init_zero {0, _particle_ctrl_NetworkState_MIN, {{NULL}, NULL}, {0, {0}}, particle_ctrl_IPConfiguration_init_zero, particle_ctrl_DNSConfiguration_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define particle_ctrl_DNSConfiguration_servers_tag 1
#define particle_ctrl_IPConfiguration_type_tag   1
#define particle_ctrl_IPConfiguration_address_tag 2
#define particle_ctrl_IPConfiguration_netmask_tag 3
#define particle_ctrl_IPConfiguration_gateway_tag 4
#define particle_ctrl_IPConfiguration_dhcp_server_tag 5
#define particle_ctrl_IPConfiguration_hostname_tag 6
#define particle_ctrl_NetworkGetConfigurationRequest_interface_tag 1
#define particle_ctrl_NetworkGetStatusRequest_interface_tag 1
#define particle_ctrl_NetworkConfiguration_interface_tag 1
#define particle_ctrl_NetworkConfiguration_state_tag 2
#define particle_ctrl_NetworkConfiguration_name_tag 3
#define particle_ctrl_NetworkConfiguration_mac_tag 4
#define particle_ctrl_NetworkConfiguration_ipconfig_tag 5
#define particle_ctrl_NetworkConfiguration_dnsconfig_tag 6
#define particle_ctrl_NetworkGetConfigurationReply_config_tag 1
#define particle_ctrl_NetworkGetStatusReply_config_tag 1
#define particle_ctrl_NetworkSetConfigurationRequest_config_tag 1

/* Struct field encoding specification for nanopb */
#define particle_ctrl_NetworkGetStatusRequest_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    interface,         1)
#define particle_ctrl_NetworkGetStatusRequest_CALLBACK NULL
#define particle_ctrl_NetworkGetStatusRequest_DEFAULT NULL

#define particle_ctrl_NetworkGetStatusReply_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, MESSAGE,  config,            1)
#define particle_ctrl_NetworkGetStatusReply_CALLBACK NULL
#define particle_ctrl_NetworkGetStatusReply_DEFAULT NULL
#define particle_ctrl_NetworkGetStatusReply_config_MSGTYPE particle_ctrl_NetworkConfiguration

#define particle_ctrl_NetworkGetConfigurationRequest_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    interface,         1)
#define particle_ctrl_NetworkGetConfigurationRequest_CALLBACK NULL
#define particle_ctrl_NetworkGetConfigurationRequest_DEFAULT NULL

#define particle_ctrl_NetworkGetConfigurationReply_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, MESSAGE,  config,            1)
#define particle_ctrl_NetworkGetConfigurationReply_CALLBACK NULL
#define particle_ctrl_NetworkGetConfigurationReply_DEFAULT NULL
#define particle_ctrl_NetworkGetConfigurationReply_config_MSGTYPE particle_ctrl_NetworkConfiguration

#define particle_ctrl_NetworkSetConfigurationRequest_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, MESSAGE,  config,            1)
#define particle_ctrl_NetworkSetConfigurationRequest_CALLBACK NULL
#define particle_ctrl_NetworkSetConfigurationRequest_DEFAULT NULL
#define particle_ctrl_NetworkSetConfigurationRequest_config_MSGTYPE particle_ctrl_NetworkConfiguration

#define particle_ctrl_NetworkSetConfigurationReply_FIELDLIST(X, a) \

#define particle_ctrl_NetworkSetConfigurationReply_CALLBACK NULL
#define particle_ctrl_NetworkSetConfigurationReply_DEFAULT NULL

#define particle_ctrl_IPConfiguration_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    type,              1) \
X(a, STATIC,   SINGULAR, MESSAGE,  address,           2) \
X(a, STATIC,   SINGULAR, MESSAGE,  netmask,           3) \
X(a, STATIC,   SINGULAR, MESSAGE,  gateway,           4) \
X(a, STATIC,   SINGULAR, MESSAGE,  dhcp_server,       5) \
X(a, CALLBACK, SINGULAR, STRING,   hostname,          6)
#define particle_ctrl_IPConfiguration_CALLBACK pb_default_field_callback
#define particle_ctrl_IPConfiguration_DEFAULT NULL
#define particle_ctrl_IPConfiguration_address_MSGTYPE particle_ctrl_IPAddress
#define particle_ctrl_IPConfiguration_netmask_MSGTYPE particle_ctrl_IPAddress
#define particle_ctrl_IPConfiguration_gateway_MSGTYPE particle_ctrl_IPAddress
#define particle_ctrl_IPConfiguration_dhcp_server_MSGTYPE particle_ctrl_IPAddress

#define particle_ctrl_DNSConfiguration_FIELDLIST(X, a) \
X(a, CALLBACK, REPEATED, MESSAGE,  servers,           1)
#define particle_ctrl_DNSConfiguration_CALLBACK pb_default_field_callback
#define particle_ctrl_DNSConfiguration_DEFAULT NULL
#define particle_ctrl_DNSConfiguration_servers_MSGTYPE particle_ctrl_IPAddress

#define particle_ctrl_NetworkConfiguration_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    interface,         1) \
X(a, STATIC,   SINGULAR, UENUM,    state,             2) \
X(a, CALLBACK, SINGULAR, STRING,   name,              3) \
X(a, STATIC,   SINGULAR, BYTES,    mac,               4) \
X(a, STATIC,   SINGULAR, MESSAGE,  ipconfig,          5) \
X(a, STATIC,   SINGULAR, MESSAGE,  dnsconfig,         6)
#define particle_ctrl_NetworkConfiguration_CALLBACK pb_default_field_callback
#define particle_ctrl_NetworkConfiguration_DEFAULT NULL
#define particle_ctrl_NetworkConfiguration_ipconfig_MSGTYPE particle_ctrl_IPConfiguration
#define particle_ctrl_NetworkConfiguration_dnsconfig_MSGTYPE particle_ctrl_DNSConfiguration

extern const pb_msgdesc_t particle_ctrl_NetworkGetStatusRequest_msg;
extern const pb_msgdesc_t particle_ctrl_NetworkGetStatusReply_msg;
extern const pb_msgdesc_t particle_ctrl_NetworkGetConfigurationRequest_msg;
extern const pb_msgdesc_t particle_ctrl_NetworkGetConfigurationReply_msg;
extern const pb_msgdesc_t particle_ctrl_NetworkSetConfigurationRequest_msg;
extern const pb_msgdesc_t particle_ctrl_NetworkSetConfigurationReply_msg;
extern const pb_msgdesc_t particle_ctrl_IPConfiguration_msg;
extern const pb_msgdesc_t particle_ctrl_DNSConfiguration_msg;
extern const pb_msgdesc_t particle_ctrl_NetworkConfiguration_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define particle_ctrl_NetworkGetStatusRequest_fields &particle_ctrl_NetworkGetStatusRequest_msg
#define particle_ctrl_NetworkGetStatusReply_fields &particle_ctrl_NetworkGetStatusReply_msg
#define particle_ctrl_NetworkGetConfigurationRequest_fields &particle_ctrl_NetworkGetConfigurationRequest_msg
#define particle_ctrl_NetworkGetConfigurationReply_fields &particle_ctrl_NetworkGetConfigurationReply_msg
#define particle_ctrl_NetworkSetConfigurationRequest_fields &particle_ctrl_NetworkSetConfigurationRequest_msg
#define particle_ctrl_NetworkSetConfigurationReply_fields &particle_ctrl_NetworkSetConfigurationReply_msg
#define particle_ctrl_IPConfiguration_fields &particle_ctrl_IPConfiguration_msg
#define particle_ctrl_DNSConfiguration_fields &particle_ctrl_DNSConfiguration_msg
#define particle_ctrl_NetworkConfiguration_fields &particle_ctrl_NetworkConfiguration_msg

/* Maximum encoded size of messages (where known) */
/* particle_ctrl_NetworkGetStatusReply_size depends on runtime parameters */
/* particle_ctrl_NetworkGetConfigurationReply_size depends on runtime parameters */
/* particle_ctrl_NetworkSetConfigurationRequest_size depends on runtime parameters */
/* particle_ctrl_IPConfiguration_size depends on runtime parameters */
/* particle_ctrl_DNSConfiguration_size depends on runtime parameters */
/* particle_ctrl_NetworkConfiguration_size depends on runtime parameters */
#define particle_ctrl_NetworkGetConfigurationRequest_size 11
#define particle_ctrl_NetworkGetStatusRequest_size 11
#define particle_ctrl_NetworkSetConfigurationReply_size 0

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

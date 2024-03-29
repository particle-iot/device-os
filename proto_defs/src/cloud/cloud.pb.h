/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.5 */

#ifndef PB_PARTICLE_CLOUD_CLOUD_CLOUD_PB_H_INCLUDED
#define PB_PARTICLE_CLOUD_CLOUD_CLOUD_PB_H_INCLUDED
#include <pb.h>
#include "ledger.pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _particle_cloud_Request_Type { 
    particle_cloud_Request_Type_INVALID = 0, 
    particle_cloud_Request_Type_LEDGER_GET_INFO = 1, 
    particle_cloud_Request_Type_LEDGER_SET_DATA = 2, 
    particle_cloud_Request_Type_LEDGER_GET_DATA = 3, 
    particle_cloud_Request_Type_LEDGER_SUBSCRIBE = 4, 
    particle_cloud_Request_Type_LEDGER_NOTIFY_UPDATE = 5, 
    particle_cloud_Request_Type_LEDGER_RESET_INFO = 6 
} particle_cloud_Request_Type;

typedef enum _particle_cloud_Response_Result { 
    particle_cloud_Response_Result_OK = 0, 
    particle_cloud_Response_Result_ERROR = 1, 
    particle_cloud_Response_Result_LEDGER_NOT_FOUND = 2, 
    particle_cloud_Response_Result_LEDGER_INVALID_SYNC_DIRECTION = 3, 
    particle_cloud_Response_Result_LEDGER_SCOPE_CHANGED = 4, 
    particle_cloud_Response_Result_LEDGER_INVALID_DATA = 5, 
    particle_cloud_Response_Result_LEDGER_TOO_LARGE_DATA = 6 
} particle_cloud_Response_Result;

/* Struct definitions */
/* *
 A response for a ServerMovedPermanentlyRequest. */
typedef struct _particle_cloud_ServerMovedPermanentlyResponse { 
    char dummy_field;
} particle_cloud_ServerMovedPermanentlyResponse;

/* *
 Request message. */
typedef struct _particle_cloud_Request { 
    particle_cloud_Request_Type type; /* /< Request type. */
    pb_size_t which_data;
    union {
        particle_cloud_ledger_GetInfoRequest ledger_get_info;
        particle_cloud_ledger_SetDataRequest ledger_set_data;
        particle_cloud_ledger_GetDataRequest ledger_get_data;
        particle_cloud_ledger_SubscribeRequest ledger_subscribe;
        particle_cloud_ledger_NotifyUpdateRequest ledger_notify_update;
        particle_cloud_ledger_ResetInfoRequest ledger_reset_info;
    } data; 
} particle_cloud_Request;

/* *
 Response message. */
typedef struct _particle_cloud_Response { 
    /* *
 Result code.

 Possible result codes are defined by the `Result` enum. If the response is sent by the device,
 the result code may be negative in which case it indicates a Device OS system error:

 https://github.com/particle-iot/device-os/blob/develop/services/inc/system_error.h */
    int32_t result; 
    pb_callback_t message; /* /< Diagnostic message. */
    pb_size_t which_data;
    union {
        particle_cloud_ledger_GetInfoResponse ledger_get_info;
        particle_cloud_ledger_SetDataResponse ledger_set_data;
        particle_cloud_ledger_GetDataResponse ledger_get_data;
        particle_cloud_ledger_SubscribeResponse ledger_subscribe;
        particle_cloud_ledger_NotifyUpdateResponse ledger_notify_update;
        particle_cloud_ledger_ResetInfoResponse ledger_reset_info;
    } data; 
} particle_cloud_Response;

/* *
 A request sent to the device to notify it that it must disconnect from the current server and
 use another server for further connections to the Cloud. */
typedef struct _particle_cloud_ServerMovedPermanentlyRequest { 
    /* *
 The address of the new server.

 The address can be a domain name or IP address. A domain name may contain placeholder arguments
 such as `$id`. */
    pb_callback_t server_addr; 
    /* *
 The port number of the new server. The default value is 5684. */
    uint32_t server_port; 
    /* *
 The public key of the new server in DER format. */
    pb_callback_t server_pub_key; 
    /* *
 The signature of the server details. */
    pb_callback_t sign; 
} particle_cloud_ServerMovedPermanentlyRequest;


/* Helper constants for enums */
#define _particle_cloud_Request_Type_MIN particle_cloud_Request_Type_INVALID
#define _particle_cloud_Request_Type_MAX particle_cloud_Request_Type_LEDGER_RESET_INFO
#define _particle_cloud_Request_Type_ARRAYSIZE ((particle_cloud_Request_Type)(particle_cloud_Request_Type_LEDGER_RESET_INFO+1))

#define _particle_cloud_Response_Result_MIN particle_cloud_Response_Result_OK
#define _particle_cloud_Response_Result_MAX particle_cloud_Response_Result_LEDGER_TOO_LARGE_DATA
#define _particle_cloud_Response_Result_ARRAYSIZE ((particle_cloud_Response_Result)(particle_cloud_Response_Result_LEDGER_TOO_LARGE_DATA+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define particle_cloud_Request_init_default      {_particle_cloud_Request_Type_MIN, 0, {particle_cloud_ledger_GetInfoRequest_init_default}}
#define particle_cloud_Response_init_default     {0, {{NULL}, NULL}, 0, {particle_cloud_ledger_GetInfoResponse_init_default}}
#define particle_cloud_ServerMovedPermanentlyRequest_init_default {{{NULL}, NULL}, 0, {{NULL}, NULL}, {{NULL}, NULL}}
#define particle_cloud_ServerMovedPermanentlyResponse_init_default {0}
#define particle_cloud_Request_init_zero         {_particle_cloud_Request_Type_MIN, 0, {particle_cloud_ledger_GetInfoRequest_init_zero}}
#define particle_cloud_Response_init_zero        {0, {{NULL}, NULL}, 0, {particle_cloud_ledger_GetInfoResponse_init_zero}}
#define particle_cloud_ServerMovedPermanentlyRequest_init_zero {{{NULL}, NULL}, 0, {{NULL}, NULL}, {{NULL}, NULL}}
#define particle_cloud_ServerMovedPermanentlyResponse_init_zero {0}

/* Field tags (for use in manual encoding/decoding) */
#define particle_cloud_Request_type_tag          1
#define particle_cloud_Request_ledger_get_info_tag 2
#define particle_cloud_Request_ledger_set_data_tag 3
#define particle_cloud_Request_ledger_get_data_tag 4
#define particle_cloud_Request_ledger_subscribe_tag 5
#define particle_cloud_Request_ledger_notify_update_tag 6
#define particle_cloud_Request_ledger_reset_info_tag 7
#define particle_cloud_Response_result_tag       1
#define particle_cloud_Response_message_tag      2
#define particle_cloud_Response_ledger_get_info_tag 3
#define particle_cloud_Response_ledger_set_data_tag 4
#define particle_cloud_Response_ledger_get_data_tag 5
#define particle_cloud_Response_ledger_subscribe_tag 6
#define particle_cloud_Response_ledger_notify_update_tag 7
#define particle_cloud_Response_ledger_reset_info_tag 8
#define particle_cloud_ServerMovedPermanentlyRequest_server_addr_tag 1
#define particle_cloud_ServerMovedPermanentlyRequest_server_port_tag 2
#define particle_cloud_ServerMovedPermanentlyRequest_server_pub_key_tag 3
#define particle_cloud_ServerMovedPermanentlyRequest_sign_tag 4

/* Struct field encoding specification for nanopb */
#define particle_cloud_Request_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    type,              1) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_get_info,data.ledger_get_info),   2) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_set_data,data.ledger_set_data),   3) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_get_data,data.ledger_get_data),   4) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_subscribe,data.ledger_subscribe),   5) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_notify_update,data.ledger_notify_update),   6) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_reset_info,data.ledger_reset_info),   7)
#define particle_cloud_Request_CALLBACK NULL
#define particle_cloud_Request_DEFAULT NULL
#define particle_cloud_Request_data_ledger_get_info_MSGTYPE particle_cloud_ledger_GetInfoRequest
#define particle_cloud_Request_data_ledger_set_data_MSGTYPE particle_cloud_ledger_SetDataRequest
#define particle_cloud_Request_data_ledger_get_data_MSGTYPE particle_cloud_ledger_GetDataRequest
#define particle_cloud_Request_data_ledger_subscribe_MSGTYPE particle_cloud_ledger_SubscribeRequest
#define particle_cloud_Request_data_ledger_notify_update_MSGTYPE particle_cloud_ledger_NotifyUpdateRequest
#define particle_cloud_Request_data_ledger_reset_info_MSGTYPE particle_cloud_ledger_ResetInfoRequest

#define particle_cloud_Response_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, SINT32,   result,            1) \
X(a, CALLBACK, OPTIONAL, STRING,   message,           2) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_get_info,data.ledger_get_info),   3) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_set_data,data.ledger_set_data),   4) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_get_data,data.ledger_get_data),   5) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_subscribe,data.ledger_subscribe),   6) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_notify_update,data.ledger_notify_update),   7) \
X(a, STATIC,   ONEOF,    MESSAGE,  (data,ledger_reset_info,data.ledger_reset_info),   8)
#define particle_cloud_Response_CALLBACK pb_default_field_callback
#define particle_cloud_Response_DEFAULT NULL
#define particle_cloud_Response_data_ledger_get_info_MSGTYPE particle_cloud_ledger_GetInfoResponse
#define particle_cloud_Response_data_ledger_set_data_MSGTYPE particle_cloud_ledger_SetDataResponse
#define particle_cloud_Response_data_ledger_get_data_MSGTYPE particle_cloud_ledger_GetDataResponse
#define particle_cloud_Response_data_ledger_subscribe_MSGTYPE particle_cloud_ledger_SubscribeResponse
#define particle_cloud_Response_data_ledger_notify_update_MSGTYPE particle_cloud_ledger_NotifyUpdateResponse
#define particle_cloud_Response_data_ledger_reset_info_MSGTYPE particle_cloud_ledger_ResetInfoResponse

#define particle_cloud_ServerMovedPermanentlyRequest_FIELDLIST(X, a) \
X(a, CALLBACK, SINGULAR, STRING,   server_addr,       1) \
X(a, STATIC,   SINGULAR, UINT32,   server_port,       2) \
X(a, CALLBACK, SINGULAR, BYTES,    server_pub_key,    3) \
X(a, CALLBACK, SINGULAR, BYTES,    sign,              4)
#define particle_cloud_ServerMovedPermanentlyRequest_CALLBACK pb_default_field_callback
#define particle_cloud_ServerMovedPermanentlyRequest_DEFAULT NULL

#define particle_cloud_ServerMovedPermanentlyResponse_FIELDLIST(X, a) \

#define particle_cloud_ServerMovedPermanentlyResponse_CALLBACK NULL
#define particle_cloud_ServerMovedPermanentlyResponse_DEFAULT NULL

extern const pb_msgdesc_t particle_cloud_Request_msg;
extern const pb_msgdesc_t particle_cloud_Response_msg;
extern const pb_msgdesc_t particle_cloud_ServerMovedPermanentlyRequest_msg;
extern const pb_msgdesc_t particle_cloud_ServerMovedPermanentlyResponse_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define particle_cloud_Request_fields &particle_cloud_Request_msg
#define particle_cloud_Response_fields &particle_cloud_Response_msg
#define particle_cloud_ServerMovedPermanentlyRequest_fields &particle_cloud_ServerMovedPermanentlyRequest_msg
#define particle_cloud_ServerMovedPermanentlyResponse_fields &particle_cloud_ServerMovedPermanentlyResponse_msg

/* Maximum encoded size of messages (where known) */
/* particle_cloud_Response_size depends on runtime parameters */
/* particle_cloud_ServerMovedPermanentlyRequest_size depends on runtime parameters */
#if defined(particle_cloud_ledger_GetInfoRequest_size) && defined(particle_cloud_ledger_SetDataRequest_size) && defined(particle_cloud_ledger_SubscribeRequest_size) && defined(particle_cloud_ledger_NotifyUpdateRequest_size)
#define particle_cloud_Request_size              (2 + sizeof(union particle_cloud_Request_data_size_union))
union particle_cloud_Request_data_size_union {char f2[(6 + particle_cloud_ledger_GetInfoRequest_size)]; char f3[(6 + particle_cloud_ledger_SetDataRequest_size)]; char f5[(6 + particle_cloud_ledger_SubscribeRequest_size)]; char f6[(6 + particle_cloud_ledger_NotifyUpdateRequest_size)]; char f0[79];};
#endif
#define particle_cloud_ServerMovedPermanentlyResponse_size 0

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

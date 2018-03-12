/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2011 by Express Logic Inc.               */ 
/*                                                                        */ 
/*  This software is copyrighted by and is the sole property of Express   */ 
/*  Logic, Inc.  All rights, title, ownership, or other interests         */ 
/*  in the software remain the property of Express Logic, Inc.  This      */ 
/*  software may only be used in accordance with the corresponding        */ 
/*  license agreement.  Any unauthorized use, duplication, transmission,  */ 
/*  distribution, or disclosure of this software is expressly forbidden.  */ 
/*                                                                        */
/*  This Copyright notice may not be removed or modified without prior    */ 
/*  written consent of Express Logic, Inc.                                */ 
/*                                                                        */ 
/*  Express Logic, Inc. reserves the right to modify this software        */ 
/*  without notice.                                                       */ 
/*                                                                        */ 
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** NetX Component                                                        */
/**                                                                       */
/**   Simple Network Management Protocol (SNMP)                           */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_snmp.h                                           PORTABLE C      */ 
/*                                                           5.4          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Simple Network Management Protocol (SNMP)*/ 
/*    component, including all data types and external references.        */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included.                                                           */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */ 
/*  06-01-2009     William E. Lamie         Modified comment(s), added    */ 
/*                                            community parameter to      */ 
/*                                            SNMP v1 traps, changed      */ 
/*                                            NX_SNMP_TRAP_OBJECT to use  */ 
/*                                            a pointer for the data,     */ 
/*                                            added conditional for       */ 
/*                                            SNMP v3 info in             */ 
/*                                            NX_SNMP_AGENT type, added   */ 
/*                                            GET responses sent counter, */ 
/*                                            and added enterprise        */ 
/*                                            specific trap-type,         */ 
/*                                            resulting in version 5.1    */ 
/*  01-28-2010     William E. Lamie         Modified comment(s), changed  */ 
/*                                            default support for SNMP V2 */ 
/*                                            to only V2C, resulting in   */ 
/*                                            version 5.2                 */ 
/*  04-01-2010     Janet Christiansen       Modified comment(s),          */
/*                                            resulting in version 5.3    */
/*  07-15-2011     Janet Christiansen       Modified comment(s),          */
/*                                            add new trap send API for   */
/*                                            V2C and V3, resulting       */
/*                                            in version 5.4              */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_SNMP_H
#define NX_SNMP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

/* Include necessary digest and encryption files.  */

#include "nx_md5.h"
#include "nx_sha1.h"
#include "nx_des.h"


/* Default support of SNMP V2 to only V2C.  */

#define NX_SNMP_V2C_ONLY


/* Define the SNMP ID.  */

#define NX_SNMP_ID                          0x534E4D50UL


/* Define SNMP UDP socket create options.  */

#ifndef NX_SNMP_TYPE_OF_SERVICE
#define NX_SNMP_TYPE_OF_SERVICE             NX_IP_NORMAL
#endif

#ifndef NX_SNMP_FRAGMENT_OPTION
#define NX_SNMP_FRAGMENT_OPTION             NX_DONT_FRAGMENT
#endif  

#ifndef NX_SNMP_TIME_TO_LIVE
#define NX_SNMP_TIME_TO_LIVE                0x80
#endif

#ifndef NX_SNMP_AGENT_PRIORITY
#define NX_SNMP_AGENT_PRIORITY              16
#endif

#ifndef NX_SNMP_AGENT_TIMEOUT
#define NX_SNMP_AGENT_TIMEOUT               100
#endif

#ifndef NX_SNMP_MAX_OCTET_STRING
#define NX_SNMP_MAX_OCTET_STRING            255
#endif

#ifndef NX_SNMP_MAX_CONTEXT_STRING
#define NX_SNMP_MAX_CONTEXT_STRING          32
#endif

#ifndef NX_SNMP_MAX_USER_NAME
#define NX_SNMP_MAX_USER_NAME               64
#endif

#ifndef NX_SNMP_MAX_SECURITY_KEY
#define NX_SNMP_MAX_SECURITY_KEY            64
#endif

#ifndef NX_SNMP_PACKET_SIZE
#define NX_SNMP_PACKET_SIZE                 560         /* Minimum SNMP packet size                             */ 
#endif


/* Define the maximum number of packets that can be queued on the SNMP Agent's UDP Port.  */

#define NX_SNMP_QUEUE_DEPTH                 5
  

/* Define the SNMP versions supported.  */

#define NX_SNMP_VERSION_1                   0
#define NX_SNMP_VERSION_2C                  1
#define NX_SNMP_VERSION_2                   2                       /* SNMPv2u and SNMPv2 */
#define NX_SNMP_VERSION_3                   3


/* Define the SNMP/ANS1 control characters.  */

#define NX_SNMP_ANS1_SEQUENCE               0x30                    /* ANS1 Code for SEQUENCE               */
#define NX_SNMP_ANS1_INTEGER                0x02                    /* ANS1 Code for INTEGER                */ 
#define NX_SNMP_ANS1_BIT_STRING             0x03                    /* ANS1 Code for BIT STRING             */ 
#define NX_SNMP_ANS1_OCTET_STRING           0x04                    /* ANS1 Code for OCTET STRING           */ 
#define NX_SNMP_ANS1_NULL                   0x05                    /* ANS1 Code for NULL                   */ 
#define NX_SNMP_ANS1_OBJECT_ID              0x06                    /* ANS1 Code for OBJECT ID              */ 
#define NX_SNMP_ANS1_IP_ADDRESS             0x40                    /* ANS1 Code for IP ADDRESS             */ 
#define NX_SNMP_ANS1_COUNTER                0x41                    /* ANS1 Code for COUNTER                */ 
#define NX_SNMP_ANS1_GAUGE                  0x42                    /* ANS1 Code for GAUGE                  */ 
#define NX_SNMP_ANS1_TIME_TICS              0x43                    /* ANS1 Code for TIME TICS              */ 
#define NX_SNMP_ANS1_OPAQUE                 0x44                    /* ANS1 Code for OPAQUE                 */ 
#define NX_SNMP_ANS1_NSAP_ADDRESS           0x45                    /* ANS1 Code for NSAP ADDRESS           */ 
#define NX_SNMP_ANS1_COUNTER64              0x46                    /* ANS1 Code for COUNTER64              */ 
#define NX_SNMP_ANS1_UINTEGER32             0x47                    /* ANS1 Code for UINTEGER32             */ 
#define NX_SNMP_ANS1_NO_SUCH_OBJECT         0x80                    /* ANS1 Code for No Such Object         */ 
#define NX_SNMP_ANS1_NO_SUCH_INSTANCE       0x81                    /* ANS1 Code for No Such Instance       */
#define NX_SNMP_ANS1_END_OF_MIB_VIEW        0x82                    /* ANS1 Code for End of MIB view        */ 
#define NX_SNMP_ANS1_GET_REQUEST            0xA0                    /* ANS1 Code for SNMP GET               */
#define NX_SNMP_ANS1_GET_NEXT_REQUEST       0xA1                    /* ANS1 Code for SNMP GET NEXT          */
#define NX_SNMP_ANS1_GET_RESPONSE           0xA2                    /* ANS1 Code for SNMP GET RESPONSE      */
#define NX_SNMP_ANS1_SET_REQUEST            0xA3                    /* ANS1 Code for SNMP SET               */
#define NX_SNMP_ANS1_TRAP_REQUEST           0xA4                    /* ANS1 Code for SNMP TRAP              */
#define NX_SNMP_ANS1_GET_BULK_REQUEST       0xA5                    /* ANS1 Code for SNMP GET BULK (v2)     */
#define NX_SNMP_ANS1_INFORM_REQUEST         0xA6                    /* ANS1 Code for SNMP INFORM (v2)       */
#define NX_SNMP_ANS1_TRAP2_REQUEST          0xA7                    /* ANS1 Code for SNMP TRAP (v2)         */
#define NX_SNMP_ANS1_REPORT_REQUEST         0xA8                    /* ANS1 Code for SNMP REPORT (v3)       */ 
#define NX_SNMP_ANS1_MULTI_BYTES            0x80                    /* ANS1 Bit for Multiple Bytes          */ 


/* Define the SNMP application GET/GETNEXT/SET callback return error codes.  */

#define NX_SNMP_SUCCESS                     0                       /* Everything is okay                   */ 
#define NX_SNMP_ERROR_TOOBIG                1                       /* Can't fit reply in outgoing message  */ 
#define NX_SNMP_ERROR_NOSUCHNAME            2                       /* Object could not be found            */ 
#define NX_SNMP_ERROR_BADVALUE              3                       /* Invalid value of set operation       */
#define NX_SNMP_ERROR_READONLY              4                       /* Object is read only and cannot be set*/ 
#define NX_SNMP_ERROR_GENERAL               5                       /* General error                        */ 
#define NX_SNMP_ERROR_NOACCESS              6
#define NX_SNMP_ERROR_WRONGTYPE             7
#define NX_SNMP_ERROR_WRONGLENGTH           8
#define NX_SNMP_ERROR_WRONGENCODING         9
#define NX_SNMP_ERROR_WRONGVALUE            10
#define NX_SNMP_ERROR_NOCREATION            11
#define NX_SNMP_ERROR_INCONSISTENTVALUE     12
#define NX_SNMP_ERROR_RESOURCEUNAVAILABLE   13
#define NX_SNMP_ERROR_COMMITFAILED          14
#define NX_SNMP_ERROR_UNDOFAILED            15
#define NX_SNMP_ERROR_AUTHORIZATION         16
#define NX_SNMP_ERROR_NOTWRITABLE           17
#define NX_SNMP_ERROR_INCONSISTENTNAME      18


/* Define SNMP Traps.  */

#define NX_SNMP_TRAP_COLDSTART              0
#define NX_SNMP_TRAP_WARMSTART              1
#define NX_SNMP_TRAP_LINKDOWN               2
#define NX_SNMP_TRAP_LINKUP                 3
#define NX_SNMP_TRAP_AUTHENTICATE_FAILURE   4
#define NX_SNMP_TRAP_EGPNEIGHBORLOSS        5
#define NX_SNMP_TRAP_ENTERPRISESPECIFIC     6


/* Define SNMP requests for use by the agent request processing callback routine.  */

#define NX_SNMP_GET_REQUEST                 NX_SNMP_ANS1_GET_REQUEST        
#define NX_SNMP_GET_NEXT_REQUEST            NX_SNMP_ANS1_GET_NEXT_REQUEST   
#define NX_SNMP_SET_REQUEST                 NX_SNMP_ANS1_SET_REQUEST        
#define NX_SNMP_GET_BULK_REQUEST            NX_SNMP_ANS1_GET_BULK_REQUEST   


/* Define SNMP object type definitions.  These will be mapped to the internal ANS1 equivalents.  */

#define NX_SNMP_INTEGER                     NX_SNMP_ANS1_INTEGER             
#define NX_SNMP_BIT_STRING                  NX_SNMP_ANS1_BIT_STRING          
#define NX_SNMP_OCTET_STRING                NX_SNMP_ANS1_OCTET_STRING  
#define NX_SNMP_NULL                        NX_SNMP_ANS1_NULL          
#define NX_SNMP_OBJECT_ID                   NX_SNMP_ANS1_OBJECT_ID
#define NX_SNMP_IP_ADDRESS                  NX_SNMP_ANS1_IP_ADDRESS         
#define NX_SNMP_COUNTER                     NX_SNMP_ANS1_COUNTER            
#define NX_SNMP_GAUGE                       NX_SNMP_ANS1_GAUGE              
#define NX_SNMP_TIME_TICS                   NX_SNMP_ANS1_TIME_TICS          
#define NX_SNMP_OPAQUE                      NX_SNMP_ANS1_OPAQUE             
#define NX_SNMP_NSAP_ADDRESS                NX_SNMP_ANS1_NSAP_ADDRESS      
#define NX_SNMP_COUNTER64                   NX_SNMP_ANS1_COUNTER64          
#define NX_SNMP_UINTEGER32                  NX_SNMP_ANS1_UINTEGER32         
#define NX_SNMP_NO_SUCH_OBJECT              NX_SNMP_ANS1_NO_SUCH_OBJECT
#define NX_SNMP_NO_SUCH_INSTANCE            NX_SNMP_ANS1_NO_SUCH_INSTANCE
#define NX_SNMP_END_OF_MIB_VIEW             NX_SNMP_ANS1_END_OF_MIB_VIEW 


/* Define return code constants.  */

#define NX_SNMP_ERROR                       0x100                   /* SNMP internal error                  */ 
#define NX_SNMP_NEXT_ENTRY                  0x101                   /* SNMP found next entry                */ 
#define NX_SNMP_ENTRY_END                   0x102                   /* SNMP end of entry                    */ 
#define NX_SNMP_TIMEOUT                     0x101                   /* SNMP timeout occurred                */ 
#define NX_SNMP_FAILED                      0x102                   /* SNMP error                           */ 
#define NX_SNMP_POOL_ERROR                  0x103                   /* SNMP packet pool size error          */ 


/* Define the SNMP Agent UDP port number */

#ifndef NX_SNMP_AGENT_PORT
#define NX_SNMP_AGENT_PORT                  161                     /* Port for SNMP Agent                  */
#endif

#ifndef NX_SNMP_MANAGER_TRAP_PORT
#define NX_SNMP_MANAGER_TRAP_PORT           162                     /* Trap Port for SNMP Manager           */ 
#endif


/* Define SNMP v3 constants.  */

#define NX_SNMP_USM_SECURITY_MODEL          3                       /* Value for USM Security model         */
#define NX_SNMP_SECURITY_AUTHORIZE          0x1                     /* Authorization required bit           */ 
#define NX_SNMP_SECURITY_PRIVACY            0x2                     /* Privacy (encryption) required bit    */ 
#define NX_SNMP_SECURITY_REPORTABLE         0x4                     /* Reportable bit                       */ 


/* Define SNMP authentication/encryption key types.  */

#define NX_SNMP_MD5_KEY                     1
#define NX_SNMP_SHA_KEY                     2
#define NX_SNMP_DES_DEY                     3
#define NX_SNMP_AES_KEY                     4                       /* Future expansion                     */ 


/* Define SNMP USM digest size.  */

#define NX_SNMP_DIGEST_SIZE                 12                      /* Size of SNMP authentication digest   */ 
#define NX_SNMP_MD5_DIGEST_SIZE             16                      /* Size of pure MD5 digest              */ 
#define NX_SNMP_SHA_DIGEST_SIZE             20                      /* Size of pure SHA digest              */ 
#define NX_SNMP_DIGEST_WORKING_SIZE         64                      /* Size of keys for the digest routines */ 


#ifndef NX_SNMP_NO_SECURITY

/* Define the key data types that will be used by the digest and/or encryption routines.  */

typedef struct NX_SNMP_SECURITY_KEY_STRUCT
{

    UINT    nx_snmp_security_key_size;
    UINT    nx_snmp_security_key_type;
    UCHAR   nx_snmp_security_key[NX_SNMP_MAX_SECURITY_KEY];

} NX_SNMP_SECURITY_KEY;

#endif


/* Define the SNMP Agent object data structure.  This structure is used for holding a single
   variable's value.  It is passed between the SNMP Agent and the application callback functions
   in order to get/set variables.  */

typedef struct NX_SNMP_OBJECT_DATA_STRUCT
{

    UINT            nx_snmp_object_data_type;                       /* Type of SNMP data contained         */
    ULONG           nx_snmp_object_data_msw;                        /* Most significant 32 bits            */ 
    ULONG           nx_snmp_object_data_lsw;                        /* Least significant 32 bits           */ 
    UINT            nx_snmp_object_octet_string_size;               /* Size of OCTET string                */ 
    UCHAR           nx_snmp_object_octet_string[NX_SNMP_MAX_OCTET_STRING];

} NX_SNMP_OBJECT_DATA;


/* Define the SNMP Agent object list type that defines application objects to be included in the 
   trap calls.  */

typedef struct NX_SNMP_TRAP_OBJECT_STRUCT
{

    UCHAR               *nx_snmp_object_string_ptr;
    NX_SNMP_OBJECT_DATA *nx_snmp_object_data;
} NX_SNMP_TRAP_OBJECT;


/* Define the SNMP Agent data structure.  */

typedef struct NX_SNMP_AGENT_STRUCT 
{
    ULONG           nx_snmp_agent_id;                               /* SNMP Agent ID                        */
    CHAR           *nx_snmp_agent_name;                             /* Name of this SNMP Agent              */
    NX_IP          *nx_snmp_agent_ip_ptr;                           /* Pointer to associated IP structure   */ 
    NX_PACKET_POOL *nx_snmp_agent_packet_pool_ptr;                  /* Pointer to SNMP agent packet pool    */ 
    UINT            nx_snmp_agent_current_version;                  /* Current SNMP Version                 */ 
#ifndef NX_SNMP_DISABLE_V3
    ULONG           nx_snmp_agent_v3_message_id;                    /* SNMP v3 message id                   */ 
    ULONG           nx_snmp_agent_v3_message_size;                  /* SNMP v3 message size                 */ 
    ULONG           nx_snmp_agent_v3_message_security_type;         /* SNMP v3 message security type        */ 
    UCHAR           nx_snmp_agent_v3_message_security_options;      /* SNMP v3 message security flags       */ 
                                                                    /* SNMP v3 security context engine      */ 
    UCHAR           nx_snmp_agent_v3_security_context_engine[NX_SNMP_MAX_CONTEXT_STRING];
    UINT            nx_snmp_agent_v3_security_context_engine_size;  /* SNMP v3 security context engine size */
    ULONG           nx_snmp_agent_v3_security_engine_boots;         /* SNMP v3 security engine boot count   */ 
    ULONG           nx_snmp_agent_v3_security_engine_boot_time;     /* SNMP v3 security engine boot time    */ 
                                                                    /* SNMP v3 security user name           */ 
    UCHAR           nx_snmp_agent_v3_security_user_name[NX_SNMP_MAX_USER_NAME];
    UINT            nx_snmp_agent_v3_security_user_name_size;       /* SNMP v3 security user name size      */
                                                                    /* SNMP v3 security authentication      */ 
    UCHAR           nx_snmp_agent_v3_security_authentication[NX_SNMP_MAX_SECURITY_KEY];
    UINT            nx_snmp_agent_v3_security_authentication_size;  /* SNMP v3 security authentication size */
                                                                    /* SNMP v3 security privacy             */ 
    UCHAR           nx_snmp_agent_v3_security_privacy[NX_SNMP_MAX_SECURITY_KEY];
    UINT            nx_snmp_agent_v3_security_privacy_size;         /* SNMP v3 security privacy size        */
                                                                    /* SNMP v3 context engine               */ 
    UCHAR           nx_snmp_agent_v3_context_engine[NX_SNMP_MAX_CONTEXT_STRING];
    UINT            nx_snmp_agent_v3_context_engine_size;           /* SNMP v3 context engine size          */ 
                                                                    /* SNMP v3 context name                 */ 
    UCHAR           nx_snmp_agent_v3_context_name[NX_SNMP_MAX_CONTEXT_STRING];
    UINT            nx_snmp_agent_v3_context_name_size;             /* SNMP v3 context engine size          */ 
    UINT            nx_snmp_agent_v3_context_engine_boots;          /* SNMP v3 context engine boots         */ 
    UINT            nx_snmp_agent_v3_context_engine_boot_time;      /* SNMP v3 context engine boot time     */ 
    UINT            nx_snmp_agent_v3_context_salt_counter;          /* SNMP v3 context salt counter         */ 
#ifndef NX_SNMP_NO_SECURITY
    NX_SNMP_SECURITY_KEY
                   *nx_snmp_agent_v3_authentication_key;            /* SNMP v3 authentication key           */ 
    NX_SNMP_SECURITY_KEY
                   *nx_snmp_agent_v3_privacy_key;                   /* SNMP v3 privacy key                  */ 
    NX_MD5          nx_snmp_agent_v3_md5_data;                      /* SNMP v3 MD5 authentication data      */ 
    NX_SHA1         nx_snmp_agent_v3_sha_data;                      /* SNMP v3 SHA authentication data      */ 
    NX_DES          nx_snmp_agent_v3_des_data;                      /* SNMP v3 DES encryption data          */ 
#endif
#endif
    UCHAR                                                           /* Current SNMP Community               */ 
                    nx_snmp_agent_current_community_string[NX_SNMP_MAX_USER_NAME];
    UCHAR                                                           /* Current SNMP Object ID               */ 
                    nx_snmp_agent_current_octet_string[NX_SNMP_MAX_OCTET_STRING];
    NX_SNMP_OBJECT_DATA                                             /* Current SNMP Object information      */ 
                    nx_snmp_agent_current_object_data; 
    ULONG           nx_snmp_agent_current_manager_ip;               /* Current SNMP Manager IP address      */ 
    UINT            nx_snmp_agent_current_manager_port;             /* Current SNMP Manager UDP port        */ 

    ULONG           nx_snmp_agent_packets_received;                 /* Number of SNMP packets received      */ 
    ULONG           nx_snmp_agent_packets_sent;                     /* Number of SNMP packets sent          */ 
    ULONG           nx_snmp_agent_invalid_version;                  /* Number of invalid SNMP versions      */ 
    ULONG           nx_snmp_agent_total_get_variables;              /* Number of variables to get           */ 
    ULONG           nx_snmp_agent_total_set_variables;              /* Number of variables to set           */ 
    ULONG           nx_snmp_agent_too_big_errors;                   /* Number of too big errors sent        */ 
    ULONG           nx_snmp_agent_no_such_name_errors;              /* Number of no such name errors sent   */ 
    ULONG           nx_snmp_agent_bad_value_errors;                 /* Number of bad value errors sent      */ 
    ULONG           nx_snmp_agent_general_errors;                   /* Number of general errors sent        */ 

    ULONG           nx_snmp_agent_request_errors;                   /* Number of errors on SNMP requests    */ 
    ULONG           nx_snmp_agent_get_requests;                     /* Number of GET requests               */ 
    ULONG           nx_snmp_agent_getnext_requests;                 /* Number of GETNEXT requests           */ 
    ULONG           nx_snmp_agent_getbulk_requests;                 /* Number of GETBULK requests           */ 
    ULONG           nx_snmp_agent_set_requests;                     /* Number of SET requests               */ 
    ULONG           nx_snmp_agent_discovery_requests;               /* Number of DISCOVERY requests         */ 
    ULONG           nx_snmp_agent_internal_errors;                  /* Number of internal errors detected   */  
    ULONG           nx_snmp_agent_traps_sent;                       /* Number of traps sent                 */ 
    ULONG           nx_snmp_agent_reports_sent;                     /* Number of traps sent                 */ 
    ULONG           nx_snmp_agent_total_bytes_sent;                 /* Number of total bytes sent           */ 
    ULONG           nx_snmp_agent_total_bytes_received;             /* Number of total bytes received       */ 
    ULONG           nx_snmp_agent_unknown_requests;                 /* Number of unknown commands received  */ 
    ULONG           nx_snmp_agent_invalid_packets;                  /* Number of invalid SNMP packets       */ 
    ULONG           nx_snmp_agent_allocation_errors;                /* Number of allocation errors          */ 
    ULONG           nx_snmp_agent_username_errors;                  /* Number of community/username errors  */ 
    ULONG           nx_snmp_agent_authentication_errors;            /* Number of authentication errors      */ 
    ULONG           nx_snmp_agent_privacy_errors;                   /* Number of privacy errors             */ 
    ULONG           nx_snmp_agent_getresponse_sent;                 /* Number of getresponse sent           */ 
    NX_UDP_SOCKET   nx_snmp_agent_socket;                           /* SNMP Server UDP socket               */ 
    TX_THREAD       nx_snmp_agent_thread;                           /* SNMP agent thread                    */ 
    UINT (*nx_snmp_agent_get_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data);
    UINT (*nx_snmp_agent_getnext_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data);
    UINT (*nx_snmp_agent_set_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data);
    UINT (*nx_snmp_agent_username_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *username);

} NX_SNMP_AGENT;



#ifndef NX_SNMP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_snmp_agent_authenticate_key_use  _nx_snmp_agent_authenticate_key_use
#define nx_snmp_agent_community_get         _nx_snmp_agent_community_get
#define nx_snmp_agent_context_engine_set    _nx_snmp_agent_context_engine_set
#define nx_snmp_agent_context_name_set      _nx_snmp_agent_context_name_set
#define nx_snmp_agent_create                _nx_snmp_agent_create
#define nx_snmp_agent_delete                _nx_snmp_agent_delete
#define nx_snmp_agent_md5_key_create        _nx_snmp_agent_md5_key_create
#define nx_snmp_agent_privacy_key_use       _nx_snmp_agent_privacy_key_use
#define nx_snmp_agent_sha_key_create        _nx_snmp_agent_sha_key_create
#define nx_snmp_agent_start                 _nx_snmp_agent_start
#define nx_snmp_agent_stop                  _nx_snmp_agent_stop
#define nx_snmp_agent_trap_send             _nx_snmp_agent_trap_send
#define nx_snmp_agent_trapv2_send           _nx_snmp_agent_trapv2_send
#define nx_snmp_agent_trapv2_oid_send       _nx_snmp_agent_trapv2_oid_send
#define nx_snmp_agent_trapv3_send           _nx_snmp_agent_trapv3_send
#define nx_snmp_agent_trapv3_oid_send       _nx_snmp_agent_trapv3_oid_send
#define nx_snmp_object_compare              _nx_snmp_object_compare
#define nx_snmp_object_copy                 _nx_snmp_object_copy
#define nx_snmp_object_counter_get          _nx_snmp_object_counter_get
#define nx_snmp_object_counter_set          _nx_snmp_object_counter_set
#define nx_snmp_object_counter64_get        _nx_snmp_object_counter64_get
#define nx_snmp_object_counter64_set        _nx_snmp_object_counter64_set
#define nx_snmp_object_end_of_mib           _nx_snmp_object_end_of_mib
#define nx_snmp_object_gauge_get            _nx_snmp_object_gauge_get
#define nx_snmp_object_gauge_set            _nx_snmp_object_gauge_set
#define nx_snmp_object_id_get               _nx_snmp_object_id_get
#define nx_snmp_object_id_set               _nx_snmp_object_id_set
#define nx_snmp_object_integer_get          _nx_snmp_object_integer_get
#define nx_snmp_object_integer_set          _nx_snmp_object_integer_set
#define nx_snmp_object_ip_address_get       _nx_snmp_object_ip_address_get
#define nx_snmp_object_ip_address_set       _nx_snmp_object_ip_address_set
#define nx_snmp_object_no_instance          _nx_snmp_object_no_instance
#define nx_snmp_object_not_found            _nx_snmp_object_not_found
#define nx_snmp_object_octet_string_get     _nx_snmp_object_octet_string_get
#define nx_snmp_object_octet_string_set     _nx_snmp_object_octet_string_set
#define nx_snmp_object_string_get           _nx_snmp_object_string_get
#define nx_snmp_object_string_set           _nx_snmp_object_string_set
#define nx_snmp_object_timetics_get         _nx_snmp_object_timetics_get
#define nx_snmp_object_timetics_set         _nx_snmp_object_timetics_set

#else

/* Services with error checking.  */

#define nx_snmp_agent_authenticate_key_use  _nxe_snmp_agent_authenticate_key_use
#define nx_snmp_agent_community_get         _nxe_snmp_agent_community_get
#define nx_snmp_agent_context_engine_set    _nxe_snmp_agent_context_engine_set
#define nx_snmp_agent_context_name_set      _nxe_snmp_agent_context_name_set
#define nx_snmp_agent_create                _nxe_snmp_agent_create
#define nx_snmp_agent_delete                _nxe_snmp_agent_delete
#define nx_snmp_agent_md5_key_create        _nxe_snmp_agent_md5_key_create
#define nx_snmp_agent_privacy_key_use       _nxe_snmp_agent_privacy_key_use
#define nx_snmp_agent_sha_key_create        _nxe_snmp_agent_sha_key_create
#define nx_snmp_agent_start                 _nxe_snmp_agent_start
#define nx_snmp_agent_stop                  _nxe_snmp_agent_stop
#define nx_snmp_agent_trap_send             _nxe_snmp_agent_trap_send
#define nx_snmp_agent_trapv2_send           _nxe_snmp_agent_trapv2_send
#define nx_snmp_agent_trapv2_oid_send       _nxe_snmp_agent_trapv2_oid_send
#define nx_snmp_agent_trapv3_send           _nxe_snmp_agent_trapv3_send
#define nx_snmp_agent_trapv3_oid_send       _nxe_snmp_agent_trapv3_oid_send
#define nx_snmp_object_compare              _nxe_snmp_object_compare
#define nx_snmp_object_copy                 _nxe_snmp_object_copy
#define nx_snmp_object_counter_get          _nxe_snmp_object_counter_get
#define nx_snmp_object_counter_set          _nxe_snmp_object_counter_set
#define nx_snmp_object_counter64_get        _nxe_snmp_object_counter64_get
#define nx_snmp_object_counter64_set        _nxe_snmp_object_counter64_set
#define nx_snmp_object_end_of_mib           _nxe_snmp_object_end_of_mib
#define nx_snmp_object_gauge_get            _nxe_snmp_object_gauge_get
#define nx_snmp_object_gauge_set            _nxe_snmp_object_gauge_set
#define nx_snmp_object_id_get               _nxe_snmp_object_id_get
#define nx_snmp_object_id_set               _nxe_snmp_object_id_set
#define nx_snmp_object_integer_get          _nxe_snmp_object_integer_get
#define nx_snmp_object_integer_set          _nxe_snmp_object_integer_set
#define nx_snmp_object_ip_address_get       _nxe_snmp_object_ip_address_get
#define nx_snmp_object_ip_address_set       _nxe_snmp_object_ip_address_set
#define nx_snmp_object_no_instance          _nxe_snmp_object_no_instance
#define nx_snmp_object_not_found            _nxe_snmp_object_not_found
#define nx_snmp_object_octet_string_get     _nxe_snmp_object_octet_string_get
#define nx_snmp_object_octet_string_set     _nxe_snmp_object_octet_string_set
#define nx_snmp_object_string_get           _nxe_snmp_object_string_get
#define nx_snmp_object_string_set           _nxe_snmp_object_string_set
#define nx_snmp_object_timetics_get         _nxe_snmp_object_timetics_get
#define nx_snmp_object_timetics_set         _nxe_snmp_object_timetics_set

#endif

/* Define the prototypes accessible to the application software.  */

UINT    nx_snmp_agent_authenticate_key_use(NX_SNMP_AGENT *agent_ptr, NX_SNMP_SECURITY_KEY *key);
UINT    nx_snmp_agent_community_get(NX_SNMP_AGENT *agent_ptr, UCHAR **community_string_ptr);
UINT    nx_snmp_agent_context_engine_set(NX_SNMP_AGENT *agent_ptr, UCHAR *context_engine, UINT context_engine_size);
UINT    nx_snmp_agent_context_name_set(NX_SNMP_AGENT *agent_ptr, UCHAR *context_name, UINT context_name_size);
UINT    nx_snmp_agent_create(NX_SNMP_AGENT *agent_ptr, CHAR *snmp_agent_name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                UINT (*snmp_agent_username_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *username),
                UINT (*snmp_agent_get_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data),
                UINT (*snmp_agent_getnext_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data),
                UINT (*snmp_agent_set_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data));
UINT    nx_snmp_agent_delete(NX_SNMP_AGENT *agent_ptr);
UINT    nx_snmp_agent_md5_key_create(NX_SNMP_AGENT *agent_ptr, UCHAR *password, NX_SNMP_SECURITY_KEY *destination_key);
UINT    nx_snmp_agent_privacy_key_use(NX_SNMP_AGENT *agent_ptr, NX_SNMP_SECURITY_KEY *key);
UINT    nx_snmp_agent_sha_key_create(NX_SNMP_AGENT *agent_ptr, UCHAR *password, NX_SNMP_SECURITY_KEY *destination_key);
UINT    nx_snmp_agent_start(NX_SNMP_AGENT *agent_ptr);
UINT    nx_snmp_agent_stop(NX_SNMP_AGENT *agent_ptr);
UINT    nx_snmp_agent_trap_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UCHAR *enterprise, UINT trap_type, UINT trap_code, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    nx_snmp_agent_trapv2_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UINT trap_type, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    nx_snmp_agent_trapv2_oid_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UCHAR *oid, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    nx_snmp_agent_trapv3_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *username, UINT trap_type, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    nx_snmp_agent_trapv3_oid_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *username, UCHAR *oid, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    nx_snmp_object_compare(UCHAR *requested_object, UCHAR *reference_object);
UINT    nx_snmp_object_copy(UCHAR *source_object_name, UCHAR *destination_object_name);
UINT    nx_snmp_object_counter_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_counter_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_counter64_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_counter64_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_end_of_mib(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_gauge_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_gauge_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_id_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_id_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_integer_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_integer_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_ip_address_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_ip_address_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_no_instance(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_not_found(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_octet_string_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data, UINT length);
UINT    nx_snmp_object_octet_string_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_string_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_string_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_timetics_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    nx_snmp_object_timetics_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);

#else

/* SNMP source code is being compiled, do not perform any API mapping.  */

UINT    _nx_snmp_agent_authenticate_key_use(NX_SNMP_AGENT *agent_ptr, NX_SNMP_SECURITY_KEY *key);
UINT    _nxe_snmp_agent_authenticate_key_use(NX_SNMP_AGENT *agent_ptr, NX_SNMP_SECURITY_KEY *key);
UINT    _nx_snmp_agent_community_get(NX_SNMP_AGENT *agent_ptr, UCHAR **community_string_ptr);
UINT    _nxe_snmp_agent_community_get(NX_SNMP_AGENT *agent_ptr, UCHAR **community_string_ptr);
UINT    _nx_snmp_agent_context_engine_set(NX_SNMP_AGENT *agent_ptr, UCHAR *context_engine, UINT context_engine_size);
UINT    _nxe_snmp_agent_context_engine_set(NX_SNMP_AGENT *agent_ptr, UCHAR *context_engine, UINT context_engine_size);
UINT    _nx_snmp_agent_context_name_set(NX_SNMP_AGENT *agent_ptr, UCHAR *context_name, UINT context_name_size);
UINT    _nxe_snmp_agent_context_name_set(NX_SNMP_AGENT *agent_ptr, UCHAR *context_name, UINT context_name_size);
UINT    _nx_snmp_agent_create(NX_SNMP_AGENT *agent_ptr, CHAR *snmp_agent_name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                UINT (*snmp_agent_username_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *username),
                UINT (*snmp_agent_get_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data),
                UINT (*snmp_agent_getnext_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data),
                UINT (*snmp_agent_set_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data));
UINT    _nxe_snmp_agent_create(NX_SNMP_AGENT *agent_ptr, CHAR *snmp_agent_name, NX_IP *ip_ptr, VOID *stack_ptr, ULONG stack_size, NX_PACKET_POOL *pool_ptr,
                UINT (*snmp_agent_username_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *username),
                UINT (*snmp_agent_get_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data),
                UINT (*snmp_agent_getnext_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data),
                UINT (*snmp_agent_set_process)(struct NX_SNMP_AGENT_STRUCT *agent_ptr, UCHAR *object_requested, NX_SNMP_OBJECT_DATA *object_data));
UINT    _nx_snmp_agent_delete(NX_SNMP_AGENT *agent_ptr);
UINT    _nxe_snmp_agent_delete(NX_SNMP_AGENT *agent_ptr);
UINT    _nx_snmp_agent_md5_key_create(NX_SNMP_AGENT *agent_ptr, UCHAR *password, NX_SNMP_SECURITY_KEY *destination_key);
UINT    _nxe_snmp_agent_md5_key_create(NX_SNMP_AGENT *agent_ptr, UCHAR *password, NX_SNMP_SECURITY_KEY *destination_key);
UINT    _nx_snmp_agent_privacy_key_use(NX_SNMP_AGENT *agent_ptr, NX_SNMP_SECURITY_KEY *key);
UINT    _nxe_snmp_agent_privacy_key_use(NX_SNMP_AGENT *agent_ptr, NX_SNMP_SECURITY_KEY *key);
UINT    _nx_snmp_agent_sha_key_create(NX_SNMP_AGENT *agent_ptr, UCHAR *password, NX_SNMP_SECURITY_KEY *destination_key);
UINT    _nxe_snmp_agent_sha_key_create(NX_SNMP_AGENT *agent_ptr, UCHAR *password, NX_SNMP_SECURITY_KEY *destination_key);
UINT    _nx_snmp_agent_start(NX_SNMP_AGENT *agent_ptr);
UINT    _nxe_snmp_agent_start(NX_SNMP_AGENT *agent_ptr);
UINT    _nx_snmp_agent_stop(NX_SNMP_AGENT *agent_ptr);
UINT    _nxe_snmp_agent_stop(NX_SNMP_AGENT *agent_ptr);
UINT    _nx_snmp_agent_trap_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UCHAR *enterprise, UINT trap_type, UINT trap_code, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nxe_snmp_agent_trap_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UCHAR *enterprise, UINT trap_type, UINT trap_code, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nx_snmp_agent_trapv2_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UINT trap_type, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nxe_snmp_agent_trapv2_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UINT trap_type, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nx_snmp_agent_trapv3_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *username, UINT trap_type, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nxe_snmp_agent_trapv3_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *username, UINT trap_type, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nxe_snmp_agent_trapv2_oid_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UCHAR *oid, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nx_snmp_agent_trapv2_oid_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *community, UCHAR *oid, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nxe_snmp_agent_trapv3_oid_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *username, UCHAR *oid, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nx_snmp_agent_trapv3_oid_send(NX_SNMP_AGENT *agent_ptr, ULONG ip_address, UCHAR *username, UCHAR *oid, ULONG elapsed_time, NX_SNMP_TRAP_OBJECT *object_list_ptr);
UINT    _nx_snmp_object_compare(UCHAR *requested_object, UCHAR *reference_object);
UINT    _nxe_snmp_object_compare(UCHAR *requested_object, UCHAR *reference_object);
UINT    _nx_snmp_object_copy(UCHAR *source_object_name, UCHAR *destination_object_name);
UINT    _nxe_snmp_object_copy(UCHAR *source_object_name, UCHAR *destination_object_name);
UINT    _nx_snmp_object_counter_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_counter_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_counter_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_counter_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_counter64_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_counter64_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_counter64_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_counter64_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_end_of_mib(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_end_of_mib(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_gauge_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_gauge_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_gauge_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_gauge_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_id_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_id_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_id_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_id_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_integer_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_integer_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_integer_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_integer_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_ip_address_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_ip_address_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_ip_address_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_ip_address_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_no_instance(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_no_instance(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_not_found(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_not_found(VOID *not_used_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_octet_string_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data, UINT length);
UINT    _nxe_snmp_object_octet_string_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data, UINT length);
UINT    _nx_snmp_object_octet_string_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_octet_string_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_string_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_string_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_string_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_string_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_timetics_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_timetics_get(VOID *source_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_object_timetics_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nxe_snmp_object_timetics_set(VOID *destination_ptr, NX_SNMP_OBJECT_DATA *object_data);


/* Define internal SNMP routines.  */

VOID    _nx_snmp_agent_thread_entry(ULONG snmp_agent_address);
UINT    _nx_snmp_utility_community_get(UCHAR *buffer_ptr, UCHAR *community_string);
UINT    _nx_snmp_utility_community_set(UCHAR *buffer_ptr, UCHAR *community_string);
UINT    _nx_snmp_utility_error_info_get(UCHAR *buffer_ptr, UINT *error_code, UINT *error_index);
UINT    _nx_snmp_utility_error_info_set(UCHAR *buffer_ptr, UINT error_code, UINT error_index);
UINT    _nx_snmp_utility_object_id_get(UCHAR *buffer_ptr, UCHAR *object_string);
UINT    _nx_snmp_utility_object_id_set(UCHAR *buffer_ptr, UCHAR *object_string, UCHAR *buffer_end);
UINT    _nx_snmp_utility_object_data_get(UCHAR *buffer_ptr, NX_SNMP_OBJECT_DATA *object_data);
UINT    _nx_snmp_utility_object_data_set(UCHAR *buffer_ptr, NX_SNMP_OBJECT_DATA *object_data, UCHAR *buffer_end);
UINT    _nx_snmp_utility_octet_get(UCHAR *buffer_ptr, UCHAR *octet_string, UINT *octet_length);
UINT    _nx_snmp_utility_octet_set(UCHAR *buffer_ptr, UCHAR *octet_string, UINT octet_length);
UINT    _nx_snmp_utility_sequence_get(UCHAR *buffer_ptr, UINT *sequence_value);
UINT    _nx_snmp_utility_sequence_set(UCHAR *buffer_ptr, UINT sequence_value);
UINT    _nx_snmp_utility_request_id_get(UCHAR *buffer_ptr, ULONG *request_id);
UINT    _nx_snmp_utility_request_id_set(UCHAR *buffer_ptr, ULONG request_id);
UINT    _nx_snmp_utility_request_type_get(UCHAR *buffer_ptr, UINT *request_type, UINT *request_length);
UINT    _nx_snmp_utility_request_type_set(UCHAR *buffer_ptr, UINT request_type, UINT request_length);
UINT    _nx_snmp_utility_version_get(UCHAR *buffer_ptr, UINT *snmp_version);
UINT    _nx_snmp_utility_version_set(UCHAR *buffer_ptr, UINT snmp_version);
VOID    _nx_snmp_version_error_response(NX_SNMP_AGENT *agent_ptr, NX_PACKET *packet_ptr, UCHAR *request_type_ptr, UCHAR *error_string_ptr, UINT status, UINT objects);
VOID    _nx_snmp_version_1_process(NX_SNMP_AGENT *agent_ptr, NX_PACKET *packet_ptr);
VOID    _nx_snmp_version_2_process(NX_SNMP_AGENT *agent_ptr, NX_PACKET *packet_ptr);
VOID    _nx_snmp_version_3_discovery_respond(NX_SNMP_AGENT *agent_ptr);
VOID    _nx_snmp_version_3_process(NX_SNMP_AGENT *agent_ptr, NX_PACKET *packet_ptr);

#endif

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  


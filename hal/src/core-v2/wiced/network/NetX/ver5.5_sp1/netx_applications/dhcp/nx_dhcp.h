/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2013 by Express Logic Inc.               */ 
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
/**   Dynamic Host Configuration Protocol (DHCP)                          */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_dhcp.h                                           PORTABLE C      */ 
/*                                                           5.5 (sp1)    */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Dynamic Host Configuration Protocol      */ 
/*    (DHCP) component, including all data types and external references. */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included.                                                           */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */ 
/*  04-01-2010     Janet Christiansen       Modified comment(s), changed  */ 
/*                                            NX_BOOTP_OFFSET_END to      */ 
/*                                            conform to RFC, changed     */ 
/*                                            calculations of ticks per   */ 
/*                                            second, added multiple      */ 
/*                                            constants, and added        */ 
/*                                            support for multihomed IP   */ 
/*                                            instances, resulting in     */ 
/*                                            version 5.1                 */
/*  07-15-2011     Janet Christiansen       Modified comment(s), added    */ 
/*                                            new internal services, added*/ 
/*                                            new APIs, added new DHCP    */ 
/*                                            configuration options,      */
/*                                            added multihome support,    */ 
/*                                            corrected the minimum       */
/*                                            NX_DHCP_PACKET_PAYLOAD,     */
/*                                            added support for BOOTP     */
/*                                            protocol, and added option  */ 
/*                                            to test DHCP address with   */ 
/*                                            ARP probe, resulting in     */ 
/*                                            version 5.2                 */
/*  02-13-2012     Janet Christiansen       Modified comment(s), and      */
/*                                            corrected the definition of */
/*                                            NX_PACKET_ALLOCATE_TIMEOUT, */
/*                                            resulting in version 5.3    */
/*  05-17-2012     Janet Christiansen       Modified comment(s), and      */
/*                                            created nx_dhcp_decline and */
/*                                            nx_dhcp_add_randomize,      */
/*                                            increased queue depth to 4, */
/*                                            added more fields to NX_DHCP*/
/*                                            data type to track time     */
/*                                            remaining in renew and      */
/*                                            rebind states,              */
/*                                            resulting in version 5.4    */
/*  04-30-2013     Janet Christiansen       Modified comment(s), and      */
/*                                            added new fields to the DHCP*/
/*                                            Client to streamline Server */
/*                                            network data processing,    */
/*                                            added an internal flag for  */
/*                                            the Skip Discovery option,  */
/*                                            added support for clear     */
/*                                            broadcast flag service,     */
/*                                            resulting in version 5.5    */
/*  07-12-2013     Janet Christiansen       Modified comment(s), and      */
/*                                            added support for           */
/*                                            NX_DHCP_CLIENT_RESTORE_STATE*/
/*                                            and added a seconds elapsed */
/*                                            counter field, and corrected*/
/*                                            the DHCP message size buffer*/
/*                                            NX_DHCP_BUFFER_SIZE to 548  */
/*                                            to comply with RFC 2131,    */
/*                                            resulting in version 5.5-sp1*/
/**************************************************************************/ 

#ifndef  NX_DHCP_H
#define  NX_DHCP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

#include "nx_api.h"
#include "nx_udp.h"
#include "nx_ip.h"


/* Determine if the DHCP client needs to support multi homing. First check if
   this is a NetX v5.3 or later. */

#if ((__NETX_MAJOR_VERSION__  == 5) && (__NETX_MINOR_VERSION__  >= 3))
#define  MULTI_HOME_NETX
#endif  /* NETX VERSION check */


/* Set NX_MAX_PHYSICAL_INTERFACES to 1 in nx_api.h for NetX environments that
   do not support multihomed devices.  */

/* Define the DHCP ID that is used to mark the DHCP structure as created.  */

#define NX_DHCP_ID                      0x44484350UL

/* Define the size of the BOOT buffer. This should be large enough for all the
   required DHCP header fields plus the minimum requirement of 312 bytes of option data
   (total: 548 bytes) as per RFC 2131; Section 2. Protocol Summary. */
    
#ifndef NX_BOOT_BUFFER_SIZE
#define NX_BOOT_BUFFER_SIZE             548
#endif


/* Define the DHCP stack size.  */

#ifndef NX_DHCP_THREAD_STACK_SIZE
#define NX_DHCP_THREAD_STACK_SIZE       1024
#endif

/* Define the DHCP stack priority. This priority must be high enough to insure the 
   DHCP client gets scheduled promptly, and thus assigned an IP address.  Assigning
   it a higher priority increases the risk of 'starving' out the IP thread task which
   needs to initialize the network driver (which is required to be able to transmit packets). */

#ifndef NX_DHCP_THREAD_PRIORITY
#define NX_DHCP_THREAD_PRIORITY         3
#endif

/* Define DHCP timeout between DHCP messages processed. */

#ifndef NX_DHCP_TIME_INTERVAL
#define NX_DHCP_TIME_INTERVAL           (1 * _nx_system_ticks_per_second)   
#endif

/* 
   Define the packet payload size, keeping in mind the DHCP Client must support
   at least a 548 byte DHCP message as per RFC 2131 and allow room for Ethernet, UDP and 
   IP headers.  

   Note: If the host network is not Ethernet, e.g. IEEE 802.3, you may need to 
   redefine NX_PHYSICAL_HEADER for a different size header accordingly in nx_api.h.
*/
#ifndef NX_DHCP_PACKET_PAYLOAD
#define NX_DHCP_PACKET_PAYLOAD          (NX_BOOT_BUFFER_SIZE + NX_PHYSICAL_HEADER +  sizeof(NX_IP_HEADER) + sizeof(NX_UDP_HEADER))
#endif /* NX_DHCP_PACKET_PAYLOAD */

/* 
   Define the packet pool size. 
*/

#ifndef NX_DHCP_PACKET_POOL_SIZE        
#define NX_DHCP_PACKET_POOL_SIZE        (5 * NX_DHCP_PACKET_PAYLOAD)
#endif

/* 
    Define the wait option for packet allocation.
*/

#ifndef NX_PACKET_ALLOCATE_TIMEOUT
#define NX_PACKET_ALLOCATE_TIMEOUT      (NX_DHCP_TIME_INTERVAL)
#endif


/* Define time out options for retransmission in seconds.  */

/* Define the minimum amount of time to retransmit a DHCP IP address request. The 
   recommended wait time is 4 seconds in RFC 2131. */
   
#ifndef NX_DHCP_MIN_RETRANS_TIMEOUT 
#define NX_DHCP_MIN_RETRANS_TIMEOUT     (4 * _nx_system_ticks_per_second)
#endif

/* Define the maximum amount of time to retransmit a DHCP IP address request. The 
   recommended wait time is 64 seconds in RFC 2131. */

#ifndef NX_DHCP_MAX_RETRANS_TIMEOUT 
#define NX_DHCP_MAX_RETRANS_TIMEOUT     (64 * _nx_system_ticks_per_second)
#endif

/* Define the minimum amount of time to retransmit a DHCP renew/rebind request. The 
   recommended wait time is 60 seconds in RFC 2131. */

#ifndef NX_DHCP_MIN_RENEW_TIMEOUT 
#define NX_DHCP_MIN_RENEW_TIMEOUT       (60 * _nx_system_ticks_per_second) 
#endif


/* Define the number of times we decrement the lease timeout in the BOUND state
   before checking for a server response. The default value of 0xFFFFFFFF indicates
   not to check. A reasonable value may be 60 * NX_DHCP_TIME_INTERVAL for roughly
   once per hour. */

#ifndef NX_DHCP_TIMEOUT_DECREMENTS
#define NX_DHCP_TIMEOUT_DECREMENTS      (0xFFFFFFFF)
#endif

/* Define UDP socket create options.  */

#ifndef NX_DHCP_TYPE_OF_SERVICE
#define NX_DHCP_TYPE_OF_SERVICE         NX_IP_NORMAL
#endif

#ifndef NX_DHCP_FRAGMENT_OPTION
#define NX_DHCP_FRAGMENT_OPTION         NX_DONT_FRAGMENT
#endif  

#ifndef NX_DHCP_TIME_TO_LIVE
#define NX_DHCP_TIME_TO_LIVE            0x80
#endif

#ifndef NX_DHCP_QUEUE_DEPTH
#define NX_DHCP_QUEUE_DEPTH             4
#endif

/*  Enable support for client state preserved between reboots   
#define NX_DHCP_CLIENT_RESTORE_STATE  
*/

/* This enables an ARP probe for verifying the assigned DHCP address is
   not owned by another host.  This is recommended, but not required
   by RFC 2131 (4.4.1).      
*/
#define NX_DHCP_CLIENT_SEND_ARP_PROBE



#ifdef NX_DHCP_CLIENT_SEND_ARP_PROBE

/*  Define the wait option to receive a response to the ARP probe in timer ticks. */
#ifndef NX_DHCP_ARP_PROBE_TIMEOUT
#define NX_DHCP_ARP_PROBE_TIMEOUT       1000
#endif

#endif

/* To enable BOOTP protocol instead of DHCP, define this option.  
#define NX_DHCP_ENABLE_BOOTP       
*/ 



/* Define the BootP Message Area Offsets.  The DHCP message format is identical to that of BootP, except
   for the Vendor options that start at the offset specified by NX_BOOTP_OFFSET_OPTIONS.  */

#define NX_BOOTP_OFFSET_OP              0       /* 1 BootP Operation 1=req, 2=reply                         */
#define NX_BOOTP_OFFSET_HTYPE           1       /* 1 Hardware type 1 = Ethernet                             */
#define NX_BOOTP_OFFSET_HLEN            2       /* 1 Hardware address length, 6 for Ethernet                */
#define NX_BOOTP_OFFSET_HOPS            3       /* 1 Number of hops, usually 0                              */
#define NX_BOOTP_OFFSET_XID             4       /* 4 Transaction ID, pseudo random number                   */
#define NX_BOOTP_OFFSET_SECS            8       /* 2 Seconds since boot                                     */
#define NX_BOOTP_OFFSET_FLAGS           10      /* 2 Flags, 0x80 = Broadcast response, 0 = unicast response */
#define NX_BOOTP_OFFSET_CLIENT_IP       12      /* 4 Initial client IP, used as dest for unicast response   */
#define NX_BOOTP_OFFSET_YOUR_IP         16      /* 4 Assigned IP, initialized to 0.0.0.0                    */
#define NX_BOOTP_OFFSET_SERVER_IP       20      /* 4 Server IP, usually initialized to 0.0.0.0              */
#define NX_BOOTP_OFFSET_GATEWAY_IP      24      /* 4 gateway IP, usually 0.0.0.0, only for BootP and TFTP   */
#define NX_BOOTP_OFFSET_CLIENT_HW       28      /* 16 Client hardware address                               */
#define NX_BOOTP_OFFSET_SERVER_NM       44      /* 64 Server name, nulls if unused                          */
#define NX_BOOTP_OFFSET_BOOT_FILE       108     /* 128 Boot file name, null if unused                       */
#define NX_BOOTP_OFFSET_VENDOR          236     /* 64 Vendor options, set first 4 bytes to a magic number   */
#define NX_BOOTP_OFFSET_OPTIONS         240     /* First variable vendor option                             */
#define NX_BOOTP_OFFSET_END             NX_BOOT_BUFFER_SIZE     /* End of BOOTP buffer (NX_DHCP_PACKET_PAYLOAD-(16+20+8)    */

#define NX_DHCP_OPTION_ADDRESS_SIZE    4


/* Define the DHCP Specific Vendor Extensions. */

#define NX_DHCP_OPTION_PAD              0
#define NX_DHCP_OPTION_PAD_SIZE         0
#define NX_DHCP_OPTION_SUBNET_MASK      1
#define NX_DHCP_OPTION_SUBNET_MASK_SIZE NX_DHCP_OPTION_ADDRESS_SIZE
#define NX_DHCP_OPTION_TIME_OFFSET      2
#define NX_DHCP_OPTION_TIME_OFFSET_SIZE 4
#define NX_DHCP_OPTION_GATEWAYS         3
#define NX_DHCP_OPTION_TIMESVR          4
#define NX_DHCP_OPTION_DNS_SVR          6
#define NX_DHCP_OPTION_HOST_NAME        12
#define NX_DHCP_OPTION_DNS_NAME         15
#define NX_DHCP_OPTION_NTP_SVR          42
#define NX_DHCP_OPTION_VENDOR_OPTIONS   43
#define NX_DHCP_OPTION_DHCP_IP_REQ      50
#define NX_DHCP_OPTION_DHCP_IP_REQ_SIZE NX_DHCP_OPTION_ADDRESS_SIZE
#define NX_DHCP_OPTION_DHCP_LEASE       51
#define NX_DHCP_OPTION_DHCP_LEASE_SIZE  4
#define NX_DHCP_OPTION_DHCP_TYPE        53
#define NX_DHCP_OPTION_DHCP_TYPE_SIZE   1
#define NX_DHCP_OPTION_DHCP_SERVER      54
#define NX_DHCP_OPTION_DHCP_SERVER_SIZE NX_DHCP_OPTION_ADDRESS_SIZE
#define NX_DHCP_OPTION_DHCP_PARAMETERS  55
#define NX_DHCP_OPTION_DHCP_MESSAGE     56
#define NX_DHCP_OPTION_RENEWAL          58
#define NX_DHCP_OPTION_RENEWAL_SIZE     4
#define NX_DHCP_OPTION_REBIND           59
#define NX_DHCP_OPTION_REBIND_SIZE      4
#define NX_DHCP_OPTION_CLIENT_ID        61
#define NX_DHCP_OPTION_CLIENT_ID_SIZE   7 /* 1 byte for address type (01 = Ethernet), 6 bytes for address */ 
#define NX_DHCP_OPTION_FDQN             81
#define NX_DHCP_OPTION_FDQN_FLAG_N      8
#define NX_DHCP_OPTION_FDQN_FLAG_E      4
#define NX_DHCP_OPTION_FDQN_FLAG_O      2
#define NX_DHCP_OPTION_FDQN_FLAG_S      1
#define NX_DHCP_OPTION_END              255
#define NX_DHCP_OPTION_END_SIZE         0



/* Define various BootP/DHCP constants.  */

#define NX_DHCP_SERVER_UDP_PORT         67
#define NX_DHCP_SERVER_TCP_PORT         67
#define NX_DHCP_CLIENT_UDP_PORT         68
#define NX_DHCP_CLIENT_TCP_PORT         68

#define NX_BOOTP_OP_REQUEST             1
#define NX_BOOTP_OP_REPLY               2
#define NX_BOOTP_TYPE_ETHERNET          1
#define NX_BOOTP_HLEN_ETHERNET          6
#define NX_BOOTP_FLAGS_BROADCAST        0x80 
#define NX_BOOTP_FLAGS_UNICAST          0x00
#define NX_BOOTP_MAGIC_COOKIE           IP_ADDRESS(99, 130, 83, 99)
#define NX_BOOTP_NO_ADDRESS             IP_ADDRESS(0, 0, 0, 0)
#define NX_BOOTP_BC_ADDRESS             IP_ADDRESS(255, 255, 255, 255)
#define NX_AUTO_IP_ADDRESS              IP_ADDRESS(169, 254, 0, 0)
#define NX_AUTO_IP_ADDRESS_MASK         0xFFFF0000UL

#define NX_DHCP_INFINITE_LEASE          0xffffffffUL


/* Define the DHCP Message Types.  */

#define NX_DHCP_TYPE_DHCPDISCOVER       1
#define NX_DHCP_TYPE_DHCPOFFER          2
#define NX_DHCP_TYPE_DHCPREQUEST        3
#define NX_DHCP_TYPE_DHCPDECLINE        4
#define NX_DHCP_TYPE_DHCPACK            5
#define NX_DHCP_TYPE_DHCPNACK           6
#define NX_DHCP_TYPE_DHCPRELEASE        7
#define NX_DHCP_TYPE_DHCPINFORM         8
#define NX_DHCP_TYPE_DHCPFORCERENEW     9

#ifdef NX_DHCP_ENABLE_BOOTP
#define NX_DHCP_TYPE_BOOT_REQUEST       10
#endif

/* Define the states of the DHCP state machine.  */

#define NX_DHCP_STATE_BOOT              1       /* Started with a previous address                          */
#define NX_DHCP_STATE_INIT              2       /* Started with no previous address                         */
#define NX_DHCP_STATE_SELECTING         3       /* Waiting to identify a DHCP server                        */
#define NX_DHCP_STATE_REQUESTING        4       /* Address requested, waiting for the Ack                   */
#define NX_DHCP_STATE_BOUND             5       /* Address established, no time outs                        */
#define NX_DHCP_STATE_RENEWING          6       /* Address established, renewal time out                    */
#define NX_DHCP_STATE_REBINDING         7       /* Address established, renewal and rebind time out         */
#define NX_DHCP_STATE_FORCERENEW        8       /* Address established, force renewal                       */


/* Define error codes from DHCP API.  */

#define NX_DHCP_ERROR                   0x90    /* General DHCP error code                                  */ 
#define NX_DHCP_NO_RESPONSE             0x91    /* No response from server for option request               */ 
#define NX_DHCP_BAD_IP_ADDRESS          0x92    /* Bad IP address or invalid interface input                */ 
#define NX_DHCP_ALREADY_STARTED         0x93    /* DHCP was already started                                 */
#define NX_DHCP_NOT_BOUND               0x94    /* DHCP is not in a bound state                             */ 
#define NX_DHCP_DEST_TO_SMALL           0x95    /* DHCP response is too big for destination                 */ 
#define NX_DHCP_NOT_STARTED             0x96    /* DHCP was not started when stop was issued                */ 
#define NX_DHCP_PARSE_ERROR             0x97    /* Error extracting DHCP option data                        */
#define NX_DHCP_BAD_XID                 0x98    /* DHCP packet received with mismatched XID                 */
#define NX_DHCP_BAD_MAC_ADDRESS         0x99    /* DHCP packet received with mismatched MAC address         */
#define NX_DHCP_BAD_INTERFACE_INDEX     0x9A    /* Invalid network interface specified                      */
#define NX_DHCP_INVALID_MESSAGE         0x9B    /* Invalid message received or requested to send            */
#define NX_DHCP_INVALID_PAYLOAD         0x9C    /* Client receives DHCP message exceeding packet payload    */
#define NX_DHCP_INVALID_IP_REQUEST      0x9D    /* Null IP address input for requesting IP address          */

#ifdef NX_DHCP_CLIENT_RESTORE_STATE
/* Define a Client record for restore DHCP Client state from non volatile memory/across reboots. */

typedef struct NX_DHCP_CLIENT_RECORD_STRUCT 
{
    UCHAR           nx_dhcp_state;              /* The current state of the DHCP Client                     */
    ULONG           nx_dhcp_ip_address;         /* Server assigned IP Address                               */ 
    ULONG           nx_dhcp_network_mask;       /* Server assigned network mask                             */  
    ULONG           nx_dhcp_gateway_address;    /* Server assigned gateway address                          */  
#ifdef MULTI_HOME_NETX
    UINT            nx_dhcp_interface_index;    /* Index of DHCP Client network interface                   */  
#endif
    ULONG           nx_dhcp_timeout;            /* The current value of any timeout, in seconds             */
    ULONG           nx_dhcp_server_ip;          /* The server IP Address                                    */
    ULONG           nx_dhcp_lease_remain_time;  /* Time remaining before lease expires                      */
    ULONG           nx_dhcp_lease_time;         /* The current Lease Time in seconds                        */
    ULONG           nx_dhcp_renewal_time;       /* Renewal Time in seconds                                  */
    ULONG           nx_dhcp_rebind_time;        /* Rebind Time in seconds                                   */
    ULONG           nx_dhcp_renewal_remain_time;/* Time remaining to renew (before rebinding necessary)     */
    ULONG           nx_dhcp_rebind_remain_time; /* Time remaining to rebind (before lease expires)          */

} NX_DHCP_CLIENT_RECORD;
#endif /* NX_DHCP_CLIENT_RESTORE_STATE */

/* Define the DHCP structure that contains all the information necessary for this DHCP 
   instance.  */

typedef struct NX_DHCP_STRUCT 
{
    ULONG           nx_dhcp_id;                 /* DHCP Structure ID                                        */
    CHAR           *nx_dhcp_name;               /* DHCP name supplied at create                             */ 
    UCHAR           nx_dhcp_started;            /* DHCP started flag                                        */ 
    UCHAR           nx_dhcp_state;              /* The current state of the DHCP Client                     */
    UCHAR           nx_dhcp_sleep_flag;         /* The flag indicating the DHCP Client is sleeping          */ 
    ULONG           nx_dhcp_xid;                /* Transaction ID for all transmissions                     */
    ULONG           nx_dhcp_seconds;            /* Number of seconds elapsed waiting for server OFFER       */
    ULONG           nx_dhcp_ip_address;         /* Server assigned IP Address                               */ 
    UINT            nx_dhcp_skip_discovery;     /* Indicate if host should skip the discovery message       */
    ULONG           nx_dhcp_network_mask;       /* Server assigned network mask                             */  
    ULONG           nx_dhcp_gateway_address;    /* Server assigned gateway address                          */  
    UINT            nx_dhcp_clear_broadcast;    /* Client sends messages with unicast reply requested       */
#ifdef MULTI_HOME_NETX
    UINT            nx_dhcp_interface_index;    /* Index of DHCP Client network interface                   */  
#endif
    NX_IP           *nx_dhcp_ip_ptr;            /* The associated IP pointer for this DHCP instance         */ 
    ULONG           nx_dhcp_server_ip;          /* The server IP Address                                    */
    ULONG           nx_dhcp_internal_errors;    /* The number of internal DHCP errors encountered           */ 
    ULONG           nx_dhcp_discoveries_sent;   /* The number of Discovery attempts made                    */ 
    ULONG           nx_dhcp_offers_received;    /* The number of Offers received                            */ 
    ULONG           nx_dhcp_requests_sent;      /* The number of Request attempts made                      */ 
    ULONG           nx_dhcp_acks_received;      /* The number of ACKs received                              */ 
    ULONG           nx_dhcp_nacks_received;     /* The number of NACKs received                             */ 
    ULONG           nx_dhcp_releases_sent;      /* The number of Releases sent                              */ 
    ULONG           nx_dhcp_force_renewal_rec;  /* The number of Forced Renewal received                    */ 
    ULONG           nx_dhcp_informs_sent;       /* The number of Inform (option requests) sent              */ 
    ULONG           nx_dhcp_inform_responses;   /* The number of Inform responses                           */ 
    ULONG           nx_dhcp_timeout;            /* The current value of any timeout, in seconds             */
    ULONG           nx_dhcp_lease_time;         /* The current Lease Time in seconds                        */
    ULONG           nx_dhcp_renewal_time;       /* Renewal Time in seconds                                  */
    ULONG           nx_dhcp_rebind_time;        /* Rebind Time in seconds                                   */
    ULONG           nx_dhcp_renewal_remain_time;/* Time remaining to renew (before rebinding necessary)     */
    ULONG           nx_dhcp_rebind_remain_time; /* Time remaining to rebind (before lease expires)          */
    UCHAR           nx_dhcp_user_option;        /* User option request                                      */ 
    NX_PACKET       *nx_dhcp_packet_received;   /* Peek into receive queue for packets waiting              */
    NX_PACKET_POOL  nx_dhcp_pool;               /* The pool of UDP data packets for DHCP messages           */
    UCHAR           nx_dhcp_pool_area[NX_DHCP_PACKET_POOL_SIZE];
    NX_UDP_SOCKET   nx_dhcp_socket;             /* The Socket used for DHCP messages                        */
    TX_THREAD       nx_dhcp_thread;             /* The DHCP processing thread                               */
    TX_MUTEX        nx_dhcp_mutex;              /* The DHCP mutex for protecting access                     */ 
    UCHAR           nx_dhcp_thread_stack[NX_DHCP_THREAD_STACK_SIZE];

#ifdef NX_DHCP_CLIENT_RESTORE_STATE
    struct NX_DHCP_CLIENT_RECORD_STRUCT nx_dhcp_client_record; /* A subset of the Client data to restore the DHCP Client state. */
#endif

    /* Define the callback function for DHCP state change notification. If specified
       by the application, this function is called whenever a state change occurs for
       the DHCP associated with this IP instance.  */
    VOID (*nx_dhcp_state_change_callback)(struct NX_DHCP_STRUCT *dhcp_ptr, UCHAR new_state);

    /* This pointer is reserved for application specific use.  */
    void            *nx_dhcp_reserved_ptr;

} NX_DHCP;


#ifndef NX_DHCP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map DHCP API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_dhcp_create                  _nx_dhcp_create
#define nx_dhcp_clear_broadcast_flag    _nxe_dhcp_clear_broadcast_flag
#define nx_dhcp_delete                  _nx_dhcp_delete
#define nx_dhcp_decline                 _nx_dhcp_decline
#define nx_dhcp_force_renew             _nx_dhcp_force_renew
#define nx_dhcp_reinitialize            _nx_dhcp_reinitialize
#define nx_dhcp_release                 _nx_dhcp_release
#define nx_dhcp_request_client_ip       _nx_dhcp_request_client_ip
#define nx_dhcp_send_request            _nx_dhcp_send_request
#define nx_dhcp_start                   _nx_dhcp_start
#define nx_dhcp_state_change_notify     _nx_dhcp_state_change_notify
#define nx_dhcp_stop                    _nx_dhcp_stop
#define nx_dhcp_user_option_retrieve    _nx_dhcp_user_option_retrieve
#define nx_dhcp_user_option_convert     _nx_dhcp_user_option_convert
#ifdef MULTI_HOME_NETX
#define nx_dhcp_set_interface_index     _nx_dhcp_set_interface_index
#ifdef NX_DHCP_CLIENT_RESTORE_STATE
#define nx_dhcp_resume                          _nx_dhcp_resume
#define nx_dhcp_suspend                         _nx_dhcp_suspend
#define nx_dhcp_client_get_record               _nx_dhcp_client_get_record
#define nx_dhcp_client_restore_record           _nx_dhcp_client_restore_record
#define nx_dhcp_client_update_time_remaining    _nx_dhcp_client_update_time_remaining
#endif /* NX_DHCP_CLIENT_RESTORE_STATE */

#endif

#else

/* Services with error checking.  */

#define nx_dhcp_create                  _nxe_dhcp_create
#define nx_dhcp_clear_broadcast_flag    _nxe_dhcp_clear_broadcast_flag
#define nx_dhcp_delete                  _nxe_dhcp_delete
#define nx_dhcp_decline                 _nxe_dhcp_decline
#define nx_dhcp_force_renew             _nxe_dhcp_force_renew
#define nx_dhcp_request_client_ip       _nxe_dhcp_request_client_ip
#define nx_dhcp_reinitialize            _nxe_dhcp_reinitialize
#define nx_dhcp_release                 _nxe_dhcp_release
#define nx_dhcp_send_request            _nxe_dhcp_send_request
#define nx_dhcp_start                   _nxe_dhcp_start
#define nx_dhcp_state_change_notify     _nxe_dhcp_state_change_notify
#define nx_dhcp_stop                    _nxe_dhcp_stop
#define nx_dhcp_user_option_retrieve    _nxe_dhcp_user_option_retrieve
#define nx_dhcp_user_option_convert     _nxe_dhcp_user_option_convert
#ifdef MULTI_HOME_NETX
#define nx_dhcp_set_interface_index     _nxe_dhcp_set_interface_index
#ifdef NX_DHCP_CLIENT_RESTORE_STATE
#define nx_dhcp_resume                          _nxe_dhcp_resume
#define nx_dhcp_suspend                         _nxe_dhcp_suspend
#define nx_dhcp_client_get_record               _nxe_dhcp_client_get_record
#define nx_dhcp_client_restore_record           _nxe_dhcp_client_restore_record
#define nx_dhcp_client_update_time_remaining    _nxe_dhcp_client_update_time_remaining
#endif /* NX_DHCP_CLIENT_RESTORE_STATE */
#endif    

#endif  /* NX_DISABLE_ERROR_CHECKING */

/* Define the prototypes accessible to the application software.  */

UINT        nx_dhcp_create(NX_DHCP *dhcp_ptr, NX_IP *ip_ptr, CHAR *name_ptr);
UINT        nx_dhcp_clear_broadcast_flag(NX_DHCP *dhcp_ptr, UINT clear_flag);
UINT        nx_dhcp_delete(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_decline(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_force_renew(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_reinitialize(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_release(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_send_request(NX_DHCP *dhcp_ptr, UINT dhcp_message_type);
UINT        nx_dhcp_start(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_request_client_ip(NX_DHCP *dhcp_ptr, ULONG client_ip_address, UINT skip_discover_message);
UINT        nx_dhcp_state_change_notify(NX_DHCP *dhcp_ptr, VOID (*dhcp_state_change_notify)(NX_DHCP *dhcp_ptr, UCHAR new_state));
UINT        nx_dhcp_stop(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_user_option_retrieve(NX_DHCP *dhcp_ptr, UINT request_option, UCHAR *destination_ptr, UINT *destination_size);
ULONG       nx_dhcp_user_option_convert(UCHAR *source_ptr);
ULONG       nx_dhcp_user_option_convert(UCHAR *source_ptr);
#ifdef MULTI_HOME_NETX
UINT        nx_dhcp_set_interface_index(NX_DHCP *dhcp_ptr, UINT interface_index);
#endif
#ifdef NX_DHCP_CLIENT_RESTORE_STATE
UINT        nx_dhcp_client_get_record(NX_DHCP *dhcp_ptr, NX_DHCP_CLIENT_RECORD *record_ptr);               
UINT        nx_dhcp_client_restore_record(NX_DHCP *dhcp_ptr, NX_DHCP_CLIENT_RECORD *record_ptr, ULONG time_elapsed);           
UINT        nx_dhcp_client_update_time_remaining(NX_DHCP *dhcp_ptr, ULONG time_elapsed);
UINT        nx_dhcp_resume(NX_DHCP *dhcp_ptr);
UINT        nx_dhcp_suspend(NX_DHCP *dhcp_ptr);
#endif /* NX_DHCP_CLIENT_RESTORE_STATE */

#else

/* DHCP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_dhcp_create(NX_DHCP *dhcp_ptr, NX_IP *ip_ptr, CHAR *name_ptr);
UINT        _nx_dhcp_create(NX_DHCP *dhcp_ptr, NX_IP *ip_ptr, CHAR *name_ptr);
UINT        _nxe_dhcp_clear_broadcast_flag(NX_DHCP *dhcp_ptr, UINT clear_flag);
UINT        _nx_dhcp_clear_broadcast_flag(NX_DHCP *dhcp_ptr, UINT clear_flag);
UINT        _nxe_dhcp_delete(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_delete(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_decline(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_decline(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_force_renew(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_force_renew(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_reinitialize(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_reinitialize(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_release(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_release(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_request_client_ip(NX_DHCP *dhcp_ptr, ULONG client_ip_address, UINT skip_discover_message);
UINT        _nx_dhcp_request_client_ip(NX_DHCP *dhcp_ptr, ULONG client_ip_address, UINT skip_discover_message);
UINT        _nxe_dhcp_send_request(NX_DHCP *dhcp_ptr, UINT dhcp_message_type);
UINT        _nx_dhcp_send_request(NX_DHCP *dhcp_ptr, UINT dhcp_message_type);
UINT        _nxe_dhcp_start(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_start(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_state_change_notify(NX_DHCP *dhcp_ptr,  VOID (*dhcp_state_change_notify)(NX_DHCP *dhcp_ptr, UCHAR new_state));
UINT        _nx_dhcp_state_change_notify(NX_DHCP *dhcp_ptr, VOID (*dhcp_state_change_notify)(NX_DHCP *dhcp_ptr, UCHAR new_state));
UINT        _nxe_dhcp_stop(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_stop(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_user_option_retrieve(NX_DHCP *dhcp_ptr, UINT request_option, UCHAR *destination_ptr, UINT *destination_size);
UINT        _nx_dhcp_user_option_retrieve(NX_DHCP *dhcp_ptr, UINT request_option, UCHAR *destination_ptr, UINT *destination_size);
ULONG       _nxe_dhcp_user_option_convert(UCHAR *source_ptr);
ULONG       _nx_dhcp_user_option_convert(UCHAR *source_ptr);
#ifdef MULTI_HOME_NETX
UINT        _nxe_dhcp_set_interface_index(NX_DHCP *dhcp_ptr, UINT interface_index);
UINT        _nx_dhcp_set_interface_index(NX_DHCP *dhcp_ptr, UINT interface_index);
#endif
#ifdef NX_DHCP_CLIENT_RESTORE_STATE
UINT        _nxe_dhcp_client_get_record(NX_DHCP *dhcp_ptr, NX_DHCP_CLIENT_RECORD *record_ptr);               
UINT        _nx_dhcp_client_get_record(NX_DHCP *dhcp_ptr, NX_DHCP_CLIENT_RECORD *record_ptr);               
UINT        _nxe_dhcp_client_restore_record(NX_DHCP *dhcp_ptr, NX_DHCP_CLIENT_RECORD *record_ptr, ULONG time_elapsed);           
UINT        _nx_dhcp_client_restore_record(NX_DHCP *dhcp_ptr, NX_DHCP_CLIENT_RECORD *record_ptr, ULONG time_elapsed);           
UINT        _nxe_dhcp_client_update_time_remaining(NX_DHCP *dhcp_ptr, ULONG time_elapsed);
UINT        _nx_dhcp_client_update_time_remaining(NX_DHCP *dhcp_ptr, ULONG time_elapsed);
UINT        _nxe_dhcp_resume(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_resume(NX_DHCP *dhcp_ptr);
UINT        _nxe_dhcp_suspend(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_suspend(NX_DHCP *dhcp_ptr);
#endif /* NX_DHCP_CLIENT_RESTORE_STATE */

#endif   /* NX_DHCP_SOURCE_CODE */

VOID        _nx_dhcp_thread_entry(ULONG ip_instance);
VOID        _nx_dhcp_process(NX_DHCP *dhcp_ptr);
UINT        _nx_dhcp_get_response(NX_DHCP *dhcp_ptr, ULONG wait_option, NX_PACKET **packet_ptr, UINT *timed_out);
VOID        _nx_dhcp_extract_information(NX_DHCP *dhcp_ptr, UCHAR *dhcp_message, UINT length);
UINT        _nx_dhcp_get_option_value(UCHAR *bootp_message, UINT option, ULONG *value, UINT length);
UINT        _nx_dhcp_add_option_value(UCHAR *bootp_message, UINT option, UINT size, ULONG value);
UINT        _nx_dhcp_add_option_string(UCHAR *bootp_message, UINT option, UINT size, UCHAR *value);
ULONG       _nx_dhcp_update_timeout(ULONG timeout);
ULONG       _nx_dhcp_update_renewal_timeout(ULONG timeout);
UCHAR       *_nx_dhcp_search_buffer(UCHAR *bootp_message, UINT option, UINT length);
ULONG       _nx_dhcp_get_data(UCHAR *data, UINT size);
VOID        _nx_dhcp_store_data(UCHAR *data, UINT size, ULONG value);
VOID        _nx_dhcp_move_string(UCHAR *dest, UCHAR *source, UINT size);
UINT        _nx_dhcp_ip_address_set(NX_DHCP *dhcp_ptr, ULONG ip_address, ULONG network_mask);
UINT        _nx_dhcp_send_request_internal(NX_DHCP *dhcp_ptr, UINT dhcp_message_type);
UINT        _nx_dhcp_client_send_arp_probe(NX_DHCP *dhcp_ptr, ULONG ip_address_to_probe, UINT timeout, UINT *is_unique);
ULONG       _nx_dhcp_add_randomize(ULONG timeout);
#ifdef NX_DHCP_CLIENT_RESTORE_STATE
UINT        _nx_dhcp_client_create_record(NX_DHCP *dhcp_ptr);
#endif /* NX_DHCP_CLIENT_RESTORE_STATE */


/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
}
#endif  /* __cplusplus */

#endif  /* NX_DHCP_H */

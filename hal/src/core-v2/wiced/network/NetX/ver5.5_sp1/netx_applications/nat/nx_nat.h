/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2010 by Express Logic Inc.               */
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
/** NetX NAT Component                                                    */
/**                                                                       */
/**   Network Address Translation Protocol (NAT)                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_nat.h                                            PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Network Address Translation Protocol     */
/*    (NAT) component, including all data types and external references.  */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, and nx_port.h,    */
/*    have already been included.                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  01-15-2008     Janet Christiansen          Initial Version 5.0        */
/*  01-28-2010     Janet Christiansen          Modified comment(s),       */ 
/*                                               resulting in version 5.1 */ 
/*  04-01-2010     Janet Christiansen          Modified comment(s),       */
/*                                               resulting in version 5.2 */
/*                                                                        */
/**************************************************************************/

#ifndef  NX_NAT_H 
#define  NX_NAT_H 


#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif


#include "nx_api.h"
#include "nx_ip.h"


/* Thread ID for identifying as an NAT device.  */

#define NX_NAT_ID           0x4E4154UL

/* Define protocol types. This may also be defined by the driver, but
   occasionally NAT needs to know to, so we define it here. */

#define NX_ETHERNET_IP      0x0800
#define NX_ETHERNET_ARP     0x0806
#define NX_ETHERNET_RARP    0x8035



/* Conversion between seconds and timer ticks. See tx_initialize_low_level.<asm> 
   for timer tick resolution before altering! */ 

#ifndef NX_NAT_MILLISECONDS_PER_TICK
#define NX_NAT_MILLISECONDS_PER_TICK           10
#endif


#ifndef NX_NAT_TICKS_PER_SECOND
#define NX_NAT_TICKS_PER_SECOND                100
#endif

    /*  NAT Debug levels in decreased filtering order:  

    NONE:       No events reported;
    SEVERE:     Report only events requiring session or server to stop operation.
    MODERATE:   Report events possibly preventing successful mail transaction
    ALL:        All events reported  */

#define NX_NAT_DEBUG_NONE      (0)
#define NX_NAT_DEBUG_LOG       (1)
#define NX_NAT_DEBUG_SEVERE    (2)
#define NX_NAT_DEBUG_MODERATE  (3)
#define NX_NAT_DEBUG_ALL       (4)


/* Internal error processing codes. */

#define NX_NAT_NON_PACKET_ERROR_CONSTANT    0xD00

#define NX_NAT_MEMORY_ERROR                 (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x01)       /* Memory handling failure   */
#define NX_NAT_PARAM_ERROR                  (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x02)       /* Invalid parameter received by a NAT service */
#define NX_NAT_INVALID_PROTOCOL             (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x03)       /* Invalid network protocol specified for translation table entry.  */ 
#define NX_NAT_NO_GLOBAL_IP_AVAILABLE       (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x04)       /* NAT unable to provide a global IP address for an outbound packet.) */  
#define NX_NAT_CREATE_ARP_ENTRY_ERROR       (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x05)       /* Error creating ARP entry for outbound packet destination. */  
#define NX_NAT_NO_ARP_MAPPING_ERROR         (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x06)       /* NAT's ARP packet queue is full and the outbound packet cannot be sent. */  
#define NX_NAT_NO_GATEWAY_ERROR             (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x07)       /* No Gateway defined for an outbound packet. */  
#define NX_NAT_OVERLAPPING_SUBNET_ERROR     (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x08)       /* Overlap between NAT device private and global subnets. */  
#define NX_NAT_BAD_PORT_COMMAND_SYNTAX      (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x0A)       /* Improperly formatted PORT command. */ 
#define NX_NAT_NO_FREE_PORT_AVAILABLE       (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x0B)       /* NAT unable to provide a unique public source port for outbound packet. */  
#define NX_NAT_NO_TRANSLATION_TABLE         (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x0C)       /* NAT translation table pointer set to NULL (no translation table?) */  
#define NX_NAT_TRANSLATION_TABLE_FULL       (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x0D)       /* NAT translation table currently is full. */
#define NX_NAT_INVALID_TABLE_ENTRY          (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x0E)       /* Invalid table entry submitted for translation table (e.g. destination broadcast address). */
#define NX_NAT_NO_PRELOAD_AFTER_NAT_START   (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x0F)       /* NAT table is not accessible for preloading (static) entries (e.g. NAT is running) */  
#define NX_NAT_NOT_FOUND_IN_TRANS_TABLE     (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x10)       /* Packet IP address has no matching entry in translation table. */
#define NX_NAT_CONVERSION_OVERFLOW_ERROR    (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x13)       /* Number is too large for conversion to ASCII. */  
#define NX_NAT_CONVERSION_BUFFER_TOO_SMALL  (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x14)       /* Buffer to hold ascii conversion of number is too small. */
#define NX_NAT_CANNOT_FRAGMENT_ERROR        (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x15)       /* NAT received a packet with the DONT FRAGMENT bit that must be fragmented for NAT to process. */
#define NX_NAT_NO_FRAGMENT_ERROR            (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x16)       /* NAT received a packet that must be fragmented but NAT is not configured to fragment packets. */
#define NX_NAT_WAITING_FOR_MORE_FRAGMENTS   (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x17)       /* NAT packet fragment queue is incomplete (not really an error). */
#define NX_NAT_FRAGMENT_QUEUE_NOT_FOUND     (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x18)       /* Packet's fragment queue cannot be found. */
#define NX_NAT_INVALID_NAT_CONFIGURATION    (NX_NAT_NON_PACKET_ERROR_CONSTANT | 0x19)       /* Invalid NAT configuration, e.g. IP address overloading set without a get_available_port callback defined. */

#define NX_NAT_PACKET_ERROR_CONSTANT         0xE00

#define NX_NAT_INVALID_IP_HEADER            (NX_NAT_PACKET_ERROR_CONSTANT | 0x01)            /* Invalid IP header. */  
#define NX_NAT_INVALID_ICMP_ERROR_PACKET    (NX_NAT_PACKET_ERROR_CONSTANT | 0x02)            /* ICMP error packet payload is bogus e.g. incomplete IP header etc. */  
#define NX_NAT_BAD_ICMP_PACKET_CHECKSUM     (NX_NAT_PACKET_ERROR_CONSTANT | 0x03)            /* IP checksum for ICMP error message packet fails to validate. */  
#define NX_NAT_PACKET_PAYLOAD_EXCEEDED      (NX_NAT_PACKET_ERROR_CONSTANT | 0x04)            /* Packet payload exceeded. */ 
#define NX_NAT_BAD_ICMP_CHECKSUM            (NX_NAT_PACKET_ERROR_CONSTANT | 0x05)            /* IP checksum for ICMP packet fails to validate. */  
#define NX_NAT_BAD_TCP_CHECKSUM             (NX_NAT_PACKET_ERROR_CONSTANT | 0x06)            /* IP checksum for TCP packet fails to validate. */  
#define NX_NAT_BAD_UDP_CHECKSUM             (NX_NAT_PACKET_ERROR_CONSTANT | 0x07)            /* IP checksum for UDP packet fails to validate. */  
#define NX_NAT_ZERO_UDP_CHECKSUM            (NX_NAT_PACKET_ERROR_CONSTANT | 0x08)            /* UDP header checksum is zero but NAT not is configured to accept packets with zero UDP checksum. */  
#define NX_NAT_MALFORMED_PACKET_FRAGMENT    (NX_NAT_PACKET_ERROR_CONSTANT | 0x09)            /* Packet fragment is malformed. */


/* ICMP error codes (not included in NetX but specified by ICMP error codes). */

#define NX_ICMP_NAT_MISSING_REQUIRED_OPTION         1
#define NX_ICMP_NAT_BAD_PACKET_LENGTH_CODE          2
#define NX_ICMP_NAT_PORT_UNREACHABLE                3


typedef enum NX_NAT_TRANSLATION_TABLE_ENTRY_ENUM
{

    NX_NAT_STATIC_ENTRY = 1,           /* Static IP address assignment. */
    NX_NAT_DYNAMIC_ENTRY               /* Dynamic IP address assignment. */

} NX_NAT_TRANSLATION_TABLE_ENTRY;

/* NetX NAT translation entry's transaction status levels. */

/* Define IP event flags.  These events are processed by the IP thread. */

#define NX_NAT_ALL_EVENTS       0xFFFFFFFFUL    /* All NAT router event flags */
#define NX_NAT_PERIODIC_EVENT   0x00000008UL    /* NAT router timer has expired. */


/* Define packet type based on direction (inbound, outbound, local). */

#define NX_NAT_INBOUND_PACKET               0x00000001UL    /* Inbound packet with local host destination on private network */
#define NX_NAT_OUTBOUND_PACKET              0x00000002UL    /* Outbound packet with external host destination on external network */ 
#define NX_NAT_PACKET_CONSUMED_BY_NAT       0x00000004UL    /* NAT device forwarded the packet to another host (e.g. packet was not intended for NAT itself) */


/* Maximum buffer size for storing an IP address in ASCII. */
#define NX_NAT_MAX_IP_ADDRESS_IN_ASCII              15

/* Maximum buffer size for storing an FTP Port 
   number command in ASCII. */
#define NX_NAT_MAX_FTP_PORTNUMBER_IN_ASCII          11


/* Start of NAT configurable options */

/* Define the mask to identify hosts on NAT's global subnet. */

#ifndef NX_NAT_GLOBAL_NETMASK
#define NX_NAT_GLOBAL_NETMASK   0xFFFFFF00UL
#endif


/* Define the mask to identify hosts on NAT's private subnet. */

#ifndef NX_NAT_PRIVATE_NETMASK
#define NX_NAT_PRIVATE_NETMASK  0xFFFFFF00UL
#endif


/* Set the event reporting/debug output for the NetX NAT */

#ifndef NX_NAT_DEBUG
#define NX_NAT_DEBUG                         NX_NAT_DEBUG_NONE
#endif

/* Set debugging level for NetX NAT */

/* Scheme for filtering messages during program execution.

    printf() itself may need to be defined for the specific
    processor that is running the application and communication. */

#if ( NX_NAT_DEBUG == NX_NAT_DEBUG_NONE )
#define NX_NAT_EVENT_LOG(debug_level, msg)
#else /* NX_NAT_DEBUG */

#define NX_NAT_EVENT_LOG(debug_level, msg)                          \
{                                                                   \
TX_INTERRUPT_SAVE_AREA                                              \
UINT level = (UINT)debug_level;                                     \
                                                                    \
TX_DISABLE                                                          \
    if (level <= NX_NAT_DEBUG_ALL && NX_NAT_DEBUG == NX_NAT_DEBUG_ALL)                        \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level <= NX_NAT_DEBUG_MODERATE && NX_NAT_DEBUG == NX_NAT_DEBUG_MODERATE)         \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == NX_NAT_DEBUG_SEVERE && NX_NAT_DEBUG == NX_NAT_DEBUG_SEVERE)             \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == NX_NAT_DEBUG_LOG)                                          \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
TX_RESTORE                                                          \
}
#endif /* NX_NAT_DEBUG */

/* Configure NAT thread and stack parameters. */

/* Set NAT thread priority. */

#ifndef NX_NAT_THREAD_PRIORITY 
#define NX_NAT_THREAD_PRIORITY                      2
#endif


/* Set the NAT thread stack size. */

#ifndef NX_NAT_THREAD_STACK_SIZE        
#define NX_NAT_THREAD_STACK_SIZE                    2048
#endif


/* Set NAT thread time slice (how long it runs before threads of equal priority may run). */

#ifndef NX_NAT_THREAD_TIME_SLICE
#define NX_NAT_THREAD_TIME_SLICE                    TX_NO_TIME_SLICE
#endif


/* Set NAT preemption threshold. */

#ifndef NX_NAT_PREEMPTION_THRESHOLD
#define NX_NAT_PREEMPTION_THRESHOLD                 NX_NAT_THREAD_PRIORITY  
#endif


/* Configure NAT memory resources. */

/* Set NAT byte pool size. */

#ifndef NX_NAT_BYTE_POOL_SIZE
#define NX_NAT_BYTE_POOL_SIZE                       (1024 * 4)
#endif


/* Set NAT byte pool name. */

#ifndef NX_NAT_BYTE_POOL_NAME
#define NX_NAT_BYTE_POOL_NAME                       "NAT Bytepool"
#endif


/* Set NAT byte pool mutex name. */

#ifndef NX_NAT_BYTE_POOL_MUTEX_NAME
#define NX_NAT_BYTE_POOL_MUTEX_NAME                 "NAT Bytepool Mutex" 
#endif


/* Set NAT byte pool mutex timeout. */

#ifndef NX_NAT_BYTE_POOL_MUTEX_WAIT
#define NX_NAT_BYTE_POOL_MUTEX_WAIT                 (5 * NX_NAT_TICKS_PER_SECOND)
#endif


/* Set NAT translation table options. */

/* Define the expiration time out for the NAT translation table entry timer. 
   With each timer expiration, translation table entries' time_remaining parameter
   are decremented by this interval.  When a translation table entry time remaining reaches zero,
   that translation table entry timeout has expired and will be removed from the table. */

#ifndef NX_NAT_TIMER_TIMEOUT_INTERVAL
#define NX_NAT_TIMER_TIMEOUT_INTERVAL               (10 * NX_NAT_TICKS_PER_SECOND)
#endif


/* Set the default expiration timeout (sec) for translation table entries. */

#ifndef NX_NAT_TABLE_ENTRY_RESPONSE_TIMEOUT
#define NX_NAT_TABLE_ENTRY_RESPONSE_TIMEOUT         (60 * NX_NAT_TICKS_PER_SECOND)
#endif


/* Set the timeout (sec) for obtaining an exclusive lock on the translation table. */

#ifndef NX_NAT_TABLE_MUTEX_WAIT
#define NX_NAT_TABLE_MUTEX_WAIT                     (20 * NX_NAT_TICKS_PER_SECOND)
#endif
      

/* Set the maximum number of entries the NAT translation table can hold. The only practical
   limit is the number of available IP addresses if IP address overloading is not enabled,
   and the overhead for searching the entire table. */

#ifndef NX_NAT_TRANSLATION_TABLE_MAX_ENTRIES
#define NX_NAT_TRANSLATION_TABLE_MAX_ENTRIES        1000
#endif

/* Configure the NAT network parameters. */

/* Set NetX IP packet pool packet size. This should be less than the Maximum Transmit Unit (MTU) of
   the driver (allow enough room for the Ethernet header plus padding bytes for frame alignment).  */

#ifndef NX_NAT_PACKET_SIZE   
#define NX_NAT_PACKET_SIZE                          1500
#endif


/* Set the size of the NAT IP packet pool. NAT only uses this packet pool to send out
   its own packets e.g. ICMP error messages.  */

#ifndef NX_NAT_PACKET_POOL_SIZE       
#define NX_NAT_PACKET_POOL_SIZE                     (NX_NAT_PACKET_SIZE * 10)
#endif    


/* Set the timeout for allocating a packet from the NAT packet pool. */

#ifndef NX_NAT_PACKET_ALLOCATE_TIMEOUT
#define NX_NAT_PACKET_ALLOCATE_TIMEOUT              NX_NO_WAIT
#endif


/* Set NetX IP helper thread stack size. */

#ifndef NX_NAT_IP_THREAD_STACK_SIZE   
#define NX_NAT_IP_THREAD_STACK_SIZE                 2048
#endif


/* Set the server IP thread priority */

#ifndef NX_NAT_IP_THREAD_PRIORITY
#define NX_NAT_IP_THREAD_PRIORITY                   2
#endif


/* Set ARP cache size of a NAT ip instance. */

#ifndef NX_NAT_ARP_CACHE_SIZE
#define NX_NAT_ARP_CACHE_SIZE                       1040 
#endif


/* Configure NAT to forward broadcast packets from the external network. */

#ifndef NX_NAT_ALLOW_INBOUND_BROADCAST_PACKETS
#define NX_NAT_ALLOW_INBOUND_BROADCAST_PACKETS      NX_FALSE
#endif


/* Configure NAT to forward broadcast packets from the private network. */

#ifndef NX_NAT_ALLOW_OUTBOUND_BROADCAST_PACKETS
#define NX_NAT_ALLOW_OUTBOUND_BROADCAST_PACKETS     NX_TRUE
#endif


/* Configure NAT to refresh a translation table entry time out on receiving a packet matching
   that table entry.  Note that this may leave the host vulnerable to Denial of Service attacks. */

#ifndef NX_NAT_REFRESH_TIMER_ON_INBOUND_PACKETS
#define NX_NAT_REFRESH_TIMER_ON_INBOUND_PACKETS     NX_TRUE
#endif


/* Configure NAT to forward packet fragments (ok if received out of order). */

#ifndef NX_NAT_ENABLE_FRAGMENT_RECEIVE
#define NX_NAT_ENABLE_FRAGMENT_RECEIVE              NX_TRUE
#endif


/* Configure NAT to fragment packets whose payload exceeds the driver MTU. */

#ifndef NX_NAT_ENABLE_FRAGMENTATION
#define NX_NAT_ENABLE_FRAGMENTATION                 NX_TRUE
#endif

/* Configure the timeout (sec) for NAT to store packet fragments while waiting to receive 
   all fragments of a fragmented packet datagram.. */

#ifndef NX_NAT_PACKET_QUEUE_TIMEOUT
#define NX_NAT_PACKET_QUEUE_TIMEOUT                 (120 * NX_NAT_TICKS_PER_SECOND)
#endif

/* Configure NAT to disable displaying the translation table. */

#define NX_NAT_DISABLE_TRANSLATION_TABLE_INFO


/* Configure NAT to disable full checksum computation of the following: */

    /* All IP packet checksum */
#define NX_NAT_DISABLE_WHOLE_IP_CHECKSUM

    /* Outbound TCP packet checksum */
#define NX_NAT_DISABLE_WHOLE_TCP_TX_CHECKSUM

    /* Inbound TCP packet checksum */
#define NX_NAT_DISABLE_WHOLE_TCP_RX_CHECKSUM
 
    /* Outbound UDP packet checksum */
#define NX_NAT_DISABLE_WHOLE_UDP_TX_CHECKSUM

    /* Inbound UDP packet checksum */
#define NX_NAT_DISABLE_WHOLE_UDP_RX_CHECKSUM

    /* Outbound ICMP packet checksum */
#define NX_NAT_DISABLE_WHOLE_ICMP_TX_CHECKSUM

    /* Inbound ICMP packet checksum */
#define NX_NAT_DISABLE_WHOLE_ICMP_RX_CHECKSUM


/* Configure protocol specific options for NAT. */

/* Set the expected FTP server port for an FTP control connection. */

#ifndef NX_NAT_FTP_CONTROL_PORT
#define NX_NAT_FTP_CONTROL_PORT                     21
#endif


/* Set the expected FTP server port for an FTP data connection. */

#ifndef NX_NAT_FTP_DATA_PORT
#define NX_NAT_FTP_DATA_PORT                        20
#endif


/* Set a timeout in seconds for binding a port number with a TCP socket. */

#ifndef NX_NAT_TCP_PORT_BIND_TIMEOUT
#define NX_NAT_TCP_PORT_BIND_TIMEOUT                (5 * NX_NAT_TICKS_PER_SECOND)
#endif


/* Configure NAT to allow UDP packets with zero checksum in the 
   UDP protocol header (not recommended by RFCs for NAT). */

#ifndef NX_NAT_ENABLE_ZERO_UDP_CHECKSUM
#define NX_NAT_ENABLE_ZERO_UDP_CHECKSUM             NX_TRUE;
#endif


/* Set the maximum number of packets in NAT's UDP receive queue depth. */

#ifndef NX_NAT_MAX_UDP_RX_QUEUE
#define NX_NAT_MAX_UDP_RX_QUEUE                     5
#endif


/* Set a timeout (sec) for waiting to bind a port number to a UDP socket. */

#ifndef NX_NAT_UDP_PORT_BIND_TIMEOUT
#define NX_NAT_UDP_PORT_BIND_TIMEOUT                100
#endif


/* Set the maximum size of the ICMP message data in ICMP error message packets.
   This should be the lesser of the driver MTU and RFC recommended
   576 bytes.  It should large enough to include at least the IP header plus 64 bits (i.e. 8 bytes
   of the protocol header of the packet generating the ICMP error message. */

#ifndef NX_NAT_MAX_ICMP_MESSAGE_SIZE
#define NX_NAT_MAX_ICMP_MESSAGE_SIZE                200
#endif


/* Set the minimum ICMP query identifier for assigning to outbound ICMP packets
   on NAT devices configured for port overloading (sharing a single global IP 
   address). Note this number must be high enough not to exceed with the local host 
   ICMP packet query IDs. */

#ifndef NX_NAT_START_ICMP_QUERY_ID
#define NX_NAT_START_ICMP_QUERY_ID                  1000
#endif


/* Set the maximum ICMP query identifier for assigning to outbound ICMP packets. */

#ifndef NX_NAT_END_ICMP_QUERY_ID
#define NX_NAT_END_ICMP_QUERY_ID                    (NX_NAT_START_ICMP_QUERY_ID + 100)
#endif




/* Define the NAT translation table entry structure.  */

typedef struct NX_NAT_TRANSLATION_ENTRY_STRUCT
{

    struct NX_NAT_TRANSLATION_TABLE_STRUCT  *nat_table_ptr;                 /* Pointer to the entry's translation table. */
    struct NX_NAT_TRANSLATION_ENTRY_STRUCT  *next_entry_ptr;                /* Pointer to the next translation entry in table */
    UINT                                    memory_dynamic_allocated;       /* Indicate if memory was dynamically allocated for this entry. */
    UINT                                    protocol;                       /* Packet's network sub protocol (TCP, UDP etc). */
    UINT                                    translation_type;               /* Translation type (static or dynamic).  */
    ULONG                                   global_inside_ip_address;       /* Outbound packet's global IP address. */
    ULONG                                   private_inside_ip_address;      /* Packet's private IP address on the local (private) network. */
    ULONG                                   external_ip_address;            /* IP address of an external host sending/receiving packets through NAT. */
    UINT                                    external_port;                  /* Source port of an external host sending/receiving packets through NAT. */
    UINT                                    private_inside_port;            /* Private port of a packet to/from a local host. */
    UINT                                    global_inside_port;             /* Global port of a packet to/from a local host connecting to an external host. */
    UINT                                    response_timeout;               /* Expiration timeout for the table entry */
    ULONG                                   response_time_remaining;        /* Time remaining on table entry timeout. */
    UINT                                    inbound_packet_initiated;       /* Indicates a connection initiated by an external host sending an inbound packet. */
    INT                                     sequence_number_delta;          /* Adjustment to a TCP packet ACK/SEQ field when a previous TCP packet's length is changed (e.g. as a result of NAT address translation). */
    ULONG                                   previous_outbound_ACK_number;   /* Acknowledgment number of a previous outgoing TCP packet. */
    ULONG                                   previous_outbound_Seq_number;   /* Sequence number for a previous outgoing TCP packet. */
    ULONG                                   fragment_id;                    /* Identifies the entry as being assigned to a fragmented packet datagram. */
    NX_TCP_SOCKET                           *tcp_socket_ptr;                /* Pointer to TCP socket associated with entry's source port number. */
    NX_UDP_SOCKET                           *udp_socket_ptr;                /* Pointer to UDP socket associated with entry's source port number. */

} NX_NAT_TRANSLATION_ENTRY;


/* Define the NAT translation table structure.  */

typedef struct NX_NAT_TRANSLATION_TABLE_STRUCT
{

    struct NX_NAT_DEVICE_STRUCT             *nat_ptr;                       /* Pointer to NAT instance. */
    UINT                                    table_entries;                  /* Number of table entries. */
    NX_NAT_TRANSLATION_ENTRY                *start_table_entry_ptr;         /* Pointer to first translation entry in table. */
    NX_NAT_TRANSLATION_ENTRY                *end_table_entry_ptr;           /* Pointer to last translation entry in table. */
    TX_MUTEX                                table_mutex;                    /* Pointer to the translation table mutex. */
    UINT                                    table_mutex_timeout;            /* Timeout to obtain translation table mutex. */
    UINT                                    nx_nat_table_entry_is_live;     /* Indicates an entry in the table is in use ('live'). */
    UINT                                    dynamic_translation_ok;         /* NAT table is configured to add or modify IP address translation while NAT is running. */
    UINT                                    overload_ip_address_enabled;    /* Indicates if IP address overloading (e.g. NAPT) is allowed. */

} NX_NAT_TRANSLATION_TABLE;


/* Define the structure for creating the list of NAT reserved IP addresses. */

typedef struct NX_NAT_RESERVED_IP_ITEM_STRUCT
{

    ULONG                                  nx_nat_ip_address;             /* Reserved (static) global IP address for NAT translation */
    struct NX_NAT_RESERVED_IP_ITEM_STRUCT  *next_ptr;                     /* Pointer to the next reserved IP address. */

}NX_NAT_RESERVED_IP_ITEM;


/* Define the structure for tpacket fragments stored in NAT's packet queues. */

typedef struct NX_NAT_PACKET_FRAGMENT_STRUCT
{

    ULONG                                       fragment_id;                 /* Fragment ID (IP header identifier) of all fragments in a queue's packets. */
    NX_PACKET                                   *packet_ptr;                 /* Pointer to the packet formed from the queue fragments. */
    struct NX_NAT_PACKET_FRAGMENT_STRUCT        *next_packet_fragment_ptr;   /* Pointer to the next packet fragment. */
    struct NX_NAT_PACKET_FRAGMENT_QUEUE_STRUCT  *fragment_queue_ptr;         /* Pointer to a packet fragment's queue. */

} NX_NAT_PACKET_FRAGMENT;


/* Define the structure for a queue to store received packet fragments.  Packet fragments can 
   be received out of order. */

typedef struct NX_NAT_PACKET_FRAGMENT_QUEUE_STRUCT
{

    ULONG                                       fragment_id;                  /* Fragment ID of all fragments in a queue's packets. */
    ULONG                                       protocol;                     /* Packet's sub protocol (TCP, UDP, ICMP etc). */
    NX_NAT_PACKET_FRAGMENT                      *start_packet_fragment_ptr;   /* Pointer to first packet fragment in a packet queue. */
    NX_NAT_PACKET_FRAGMENT                      *end_packet_fragment_ptr;     /* Pointer to the last fragment in a packet queue. */
    UINT                                        allocated_fragments_in_queue; /* Number of packet fragments with allocated packets in the queue. */
    struct NX_NAT_PACKET_FRAGMENT_QUEUE_STRUCT  *next_queue_ptr;              /* Pointer to next packet fragment queue. */
    ULONG                                       queue_time_remaining;         /* Time remaining before queue timeout expires. */

} NX_NAT_PACKET_FRAGMENT_QUEUE;


/* Define the NAT device structure.  */

typedef struct NX_NAT_DEVICE_STRUCT
{
    ULONG                                  nx_nat_id;                       /* NAT Server thread ID  */
    NX_IP                                  *nat_private_ip_ptr;             /* IP instance for NAT's private network. */
    NX_PACKET_POOL                         *private_packet_pool_owner;      /* Pointer to packet pool for private network traffic. */
    NX_IP                                  *nat_global_ip_ptr;              /* IP instance for the global network. */
    NX_PACKET_POOL                         *global_packet_pool_owner;       /* Pointer to packet pool for external network traffic. */
    ULONG                                  nat_global_gateway_address;      /* Gateway for out of the global network packets. */
    NX_NAT_TRANSLATION_TABLE               *nat_table_ptr;                  /* Pointer to the translation table. */
    TX_BYTE_POOL                           *bytepool_ptr;                   /* Pointer to NAT's byte pool. */
    TX_MUTEX                               *bytepool_mutex_ptr;             /* Pointer to NAT's byte pool mutex. */
    UINT                                   bytepool_mutex_timeout;          /* NAT's byte pool mutex timeout. */
    TX_THREAD                              nat_thread;                      /* NAT ThreadX thread */
    TX_EVENT_FLAGS_GROUP                   nat_ip_events;                   /* Flag group for NAT packet processing events. */
    TX_MUTEX                               flag_group_mutex;                /* Pointer to the NAT flag group mutex */
    TX_TIMER                               nx_nat_table_timer;              /* Translation table entry update timer. */
    ULONG                                  nx_nat_table_timer_timeout;      /* Time (sec) interval when update timer thread tast runs. */
    UINT                                   nx_nat_thread_started;           /* Indicate NAT is running; this disallows access to certain NAT resources. */
    UINT                                   icmp_errmsg_receive_enabled;     /* Permit NAT to receive ICMP error message packets from an external source. */
    UINT                                   icmp_query_respond_enabled;      /* Permit NAT to respond to ICMP query packets from an external source. */
    UINT                                   enable_receiving_packet_fragments;/* Permit NAT to accept forwarding packet fragments. */
    UINT                                   enable_packet_fragmentation;     /* Permit NAT to fragment a packet before sending out payload. */
    UINT                                   allow_zero_checksum_UDP_packets; /* Permit NAT to accept UDP packets with a zero UDP checksum. */
    UINT                                   allow_inbound_broadcast_packets; /* Configure NAT to block inbound broadcast packets. */
    UINT                                   allow_outbound_broadcast_packets;/* Configure NAT to block outbound broadcast packets. */
    UINT                                   refresh_timer_on_packet_forward; /* Configure NAT to reset translation entry timeout on receipt of matching packet. */
    ULONG                                  forwarded_packet_total;          /* Total number of packets received by NAT. */
    ULONG                                  forwarded_packets_dropped;       /* Total number of packets which cannot be forwarded. */
    ULONG                                  forwarded_packets_invalid;       /* Total number of invalid packets (e.g. truncated or corrupt headers). */
    ULONG                                  forwarded_packets_sent;          /* Total number of packets forwarded by NAT. */
    ULONG                                  forwarded_bytes_sent;            /* Total number of bytes of packet data sent by NAT. */
    NX_NAT_RESERVED_IP_ITEM                *start_reserved_ip_item_ptr;     /* First reserved IP address held by NAT. */
    NX_NAT_RESERVED_IP_ITEM                *end_reserved_ip_item_ptr;       /* Last reserved IP address held by NAT. */ 
    NX_NAT_PACKET_FRAGMENT_QUEUE           *start_fragment_queue_ptr;       /* First queue in NAT's list of active packet fragment queues. */
    NX_NAT_PACKET_FRAGMENT_QUEUE           *end_fragment_queue_ptr;         /* Last queue in NAT's list of active packet fragment queues. */
                                           
    /* Application defined services. */
    UINT                                    (*nx_nat_get_global_IP_address)(NX_NAT_TRANSLATION_TABLE *table_ptr, UINT protocol, ULONG *global_ip_address, ULONG private_inside_ip_address, ULONG destination_ip_address); 
                                                                             /* Pointer to callback for dynamically assigning a global address for a local host packet. */
    UINT                                    (*nx_nat_get_available_port)(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address,  UINT private_inside_port, UINT *assigned_port);
                                                                             /* Pointer to callback for dynamically assigning a global port for a local host packet. */
    VOID                                    (*nx_nat_tcp_filter)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *passed_filter);
                                                                             /* Pointer to callback to apply a filter to TCP packets received by NAT. */
    VOID                                    (*nx_nat_udp_filter)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *passed_filter);
                                                                             /* Pointer to callback to apply a filter to UDP packets received by NAT. */
    VOID                                    (*nx_nat_icmp_filter)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *passed_filter);
                                                                             /* Pointer to callback to apply a filter to ICMP packets received by NAT. */
    VOID                                    (*nx_nat_set_entry_timeout)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *response_timeout);
                                                                             /* Pointer to callback for setting translation table timeouts. */
} NX_NAT_DEVICE;



#ifndef     NX_NAT_SOURCE_CODE     


/* Define the system API mappings based on the error checking 
   selected by the user.   */

/* Determine if error checking is desired.  If so, map API functions
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work.
   Note: error checking is enabled by default.  */


#ifdef NX_NAT_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_nat_add_reserved_ip_list                 _nx_nat_add_reserved_ip_list
#define nx_nat_create                               _nx_nat_create
#define nx_nat_delete                               _nx_nat_delete
#define nx_nat_server_resume                        _nx_nat_server_resume
#define nx_nat_server_suspend                       _nx_nat_server_suspend
#define nx_nat_set_filters                          _nx_nat_set_filters
#define nx_nat_table_create                         _nx_nat_table_create
#define nx_nat_table_delete                         _nx_nat_table_delete
#define nx_nat_table_entry_create                   _nx_nat_table_entry_create
#define nx_nat_table_entry_preload                  _nx_nat_table_entry_preload
#define nx_nat_table_entry_delete                   _nx_nat_table_entry_delete
#define nx_nat_table_find_entry                     _nx_nat_table_find_entry
#define nx_nat_utility_display_arp_table            _nx_nat_utility_display_arp_table
#define nx_nat_utility_display_translation_table    _nx_nat_utility_display_translation_table
#define nx_nat_utility_display_bytepool_reserves    _nx_nat_utility_display_bytepool_reserves
#define nx_nat_utility_display_packetpool_reserves  _nx_nat_utility_display_packetpool_reserves
#define nx_nat_utility_get_destination_port         _nx_nat_utility_get_destination_port
#define nx_nat_utility_get_source_port              _nx_nat_utility_get_source_port

#else

/* Services with error checking.  */

#define nx_nat_add_reserved_ip_list                 _nxe_nat_add_reserved_ip_list
#define nx_nat_create                               _nxe_nat_create
#define nx_nat_delete                               _nxe_nat_delete
#define nx_nat_server_resume                        _nxe_nat_server_resume
#define nx_nat_server_suspend                       _nxe_nat_server_suspend
#define nx_nat_set_filters                          _nxe_nat_set_filters
#define nx_nat_table_create                         _nxe_nat_table_create
#define nx_nat_table_delete                         _nxe_nat_table_delete
#define nx_nat_table_entry_create                   _nxe_nat_table_entry_create
#define nx_nat_table_entry_preload                  _nxe_nat_table_entry_preload
#define nx_nat_table_entry_delete                   _nxe_nat_table_entry_delete
#define nx_nat_table_find_entry                     _nxe_nat_table_find_entry
#define nx_nat_utility_display_arp_table            _nxe_nat_utility_display_arp_table
#define nx_nat_utility_display_translation_table    _nxe_nat_utility_display_translation_table
#define nx_nat_utility_display_bytepool_reserves    _nxe_nat_utility_display_bytepool_reserves
#define nx_nat_utility_display_packetpool_reserves  _nxe_nat_utility_display_packetpool_reserves
#define nx_nat_utility_get_destination_port         _nxe_nat_utility_get_destination_port
#define nx_nat_utility_get_source_port              _nxe_nat_utility_get_source_port

#endif    /* NX_NAT_DISABLE_ERROR_CHECKING */

/* Define API services available for NAT applications. */

UINT    nx_nat_add_reserved_ip_list(NX_NAT_DEVICE *nat_ptr, ULONG reserved_ip_address, NX_NAT_RESERVED_IP_ITEM *nat_ip_item_ptr);
UINT    nx_nat_create(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr, NX_IP *nat_private_ip_ptr, NX_IP *nat_global_ip_ptr, 
                      ULONG nat_global_gateway_address, UINT icmp_query_respond_enabled, UINT icmp_errmsg_receive_enabled,
                      VOID *stack_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout, 
                      UINT (*nx_nat_get_global_IP_address)(NX_NAT_TRANSLATION_TABLE *table_ptr, UINT protocol, ULONG *global_ip_address, ULONG private_inside_ip_address, ULONG destination_ip_address), 
                      UINT (*nx_nat_get_available_port)(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address,  UINT private_inside_port, UINT *assigned_port),
                      VOID (*nx_nat_set_entry_timeout)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *response_timeout));                       
UINT    nx_nat_delete(NX_NAT_DEVICE *nat_ptr);
UINT    nx_nat_server_resume(NX_NAT_DEVICE *nat_ptr, UINT reset_packet_counts);
UINT    nx_nat_server_suspend(NX_NAT_DEVICE *nat_ptr);
UINT    nx_nat_set_filters(NX_NAT_DEVICE *nat_ptr, VOID (*nat_tcp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter),VOID (*nat_udp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter), VOID (*nat_icmp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter));
UINT    nx_nat_table_create(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr,  UINT table_mutex_timeout, UINT dynamic_translation_ok, UINT overload_ip_address_enabled);
UINT    nx_nat_table_delete(NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    nx_nat_table_entry_create(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address, ULONG private_inside_ip_address, UINT private_inside_port, UINT NAT_assigned_source_port, ULONG external_ip_address, UINT destination_port, UINT response_timeout, ULONG fragment_id, NX_TCP_SOCKET *tcp_socket_ptr, NX_UDP_SOCKET *udp_socket_ptr, UINT inbound_packet_initiated, NX_NAT_TRANSLATION_ENTRY **match_entry_ptr);
UINT    nx_nat_table_entry_preload(NX_NAT_TRANSLATION_ENTRY *nat_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr,  ULONG global_inside_ip_address, ULONG private_inside_ip_address, UINT global_inside_port, UINT protocol, UINT inbound_packet_initiated);
UINT    nx_nat_table_entry_delete(NX_NAT_TRANSLATION_ENTRY *remove_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    nx_nat_table_find_entry(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_ENTRY *entry_tomatch, NX_NAT_TRANSLATION_ENTRY **match_entry_ptr, UINT skip_inbound_init); 
UINT    nx_nat_utility_display_arp_table(NX_IP *ip_ptr);
UINT    nx_nat_utility_display_translation_table(NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    nx_nat_utility_display_bytepool_reserves(NX_NAT_DEVICE *nat_ptr);
UINT    nx_nat_utility_display_packetpool_reserves(NX_NAT_DEVICE *nat_ptr);
UINT    nx_nat_utility_get_destination_port(NX_PACKET *packet_ptr, UINT protocol, UINT *destination_port);
UINT    nx_nat_utility_get_source_port(NX_PACKET *packet_ptr, UINT protocol, UINT *source_port);


#else     /* NX_NAT_SOURCE_CODE */

/* NAT source code is being compiled, do not perform any API mapping.  */

UINT    _nx_nat_add_reserved_ip_list(NX_NAT_DEVICE *nat_ptr, ULONG reserved_ip_address, NX_NAT_RESERVED_IP_ITEM *nat_ip_item_ptr);
UINT    _nxe_nat_add_reserved_ip_list(NX_NAT_DEVICE *nat_ptr, ULONG reserved_ip_address, NX_NAT_RESERVED_IP_ITEM *nat_ip_item_ptr);
UINT    _nx_nat_create(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr, NX_IP *nat_private_ip_ptr, NX_IP *nat_global_ip_ptr, 
                       ULONG nat_global_gateway_address, UINT icmp_query_respond_enabled, UINT icmp_errmsg_receive_enabled,
                       VOID *stack_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout, 
                       UINT (*nx_nat_get_global_IP_address)(NX_NAT_TRANSLATION_TABLE *table_ptr, UINT protocol, ULONG *global_ip_address, ULONG private_inside_ip_address, ULONG destination_ip_address), 
                       UINT (*nx_nat_get_available_port)(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address,  UINT private_inside_port, UINT *assigned_port),
                       VOID (*nx_nat_set_entry_timeout)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *response_timeout));                       
UINT    _nxe_nat_create( NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr, NX_IP *nat_private_ip_ptr,  NX_IP *nat_global_ip_ptr, 
                         ULONG nat_global_gateway_address, UINT icmp_query_respond_enabled, UINT icmp_errmsg_receive_enabled,
                         VOID *stack_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                         UINT (*nx_nat_get_global_IP_address)(NX_NAT_TRANSLATION_TABLE *table_ptr, UINT protocol, ULONG *global_ip_address, ULONG private_inside_ip_address, ULONG destination_ip_address),
                         UINT (*nx_nat_get_available_port)(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address,  UINT private_inside_port, UINT *assigned_port),
                         VOID (*nx_nat_set_entry_timeout)(NX_PACKET *packet_ptr, UINT packet_direction, UINT *response_timeout));                        

UINT    _nx_nat_delete(NX_NAT_DEVICE *nat_ptr);
UINT    _nxe_nat_delete(NX_NAT_DEVICE *nat_ptr);
UINT    _nx_nat_server_resume(NX_NAT_DEVICE *nat_ptr, UINT reset_packet_counts);
UINT    _nxe_nat_server_resume(NX_NAT_DEVICE *nat_ptr, UINT reset_packet_counts);
UINT    _nx_nat_server_suspend(NX_NAT_DEVICE *nat_ptr);
UINT    _nxe_nat_server_suspend(NX_NAT_DEVICE *nat_ptr);
UINT    _nx_nat_set_filters(NX_NAT_DEVICE *nat_ptr, VOID (*nat_tcp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter), VOID (*nat_udp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter), VOID (*nat_icmp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter));
UINT    _nxe_nat_set_filters(NX_NAT_DEVICE *nat_ptr, VOID (*nat_tcp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter), VOID (*nat_udp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter), VOID (*nat_icmp_filter)(NX_PACKET *packet_ptr, UINT incoming_outgoing_direction, UINT *passed_filter));
UINT    _nx_nat_table_create(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT table_mutex_timeout, UINT dynamic_translation_ok, UINT overload_ip_address_enabled);
UINT    _nxe_nat_table_create(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT table_mutex_timeout, UINT dynamic_translation_ok, UINT overload_ip_address_enabled);
UINT    _nx_nat_table_delete(NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    _nxe_nat_table_delete(NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    _nx_nat_table_entry_create(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address, ULONG private_inside_ip_address, UINT private_inside_port, UINT NAT_assigned_source_port, ULONG external_ip_address, UINT destination_port, UINT response_timeout, ULONG fragment_id, NX_TCP_SOCKET *tcp_socket_ptr, NX_UDP_SOCKET *udp_socket_ptr, UINT inbound_packet_initiated, NX_NAT_TRANSLATION_ENTRY **match_entry_ptr);
UINT    _nxe_nat_table_entry_create(NX_NAT_TRANSLATION_TABLE *nat_table_ptr, UINT protocol, ULONG global_inside_ip_address, ULONG private_inside_ip_address, UINT private_inside_port, UINT NAT_assigned_source_port, ULONG external_ip_address, UINT destination_port, UINT response_timeout, ULONG fragment_id, NX_TCP_SOCKET *tcp_socket_ptr, NX_UDP_SOCKET *udp_socket_ptr, UINT inbound_packet_initiated, NX_NAT_TRANSLATION_ENTRY **match_entry_ptr);
UINT    _nx_nat_table_entry_preload(NX_NAT_TRANSLATION_ENTRY *nat_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr,  ULONG global_inside_ip_address, ULONG private_inside_ip_address, UINT global_inside_port, UINT protocol, UINT inbound_packet_initiated);
UINT    _nxe_nat_table_entry_preload(NX_NAT_TRANSLATION_ENTRY *nat_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr,  ULONG global_inside_ip_address, ULONG private_inside_ip_address, UINT global_inside_port, UINT protocol, UINT inbound_packet_initiated);
UINT    _nx_nat_table_entry_delete(NX_NAT_TRANSLATION_ENTRY *remove_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    _nxe_nat_table_entry_delete(NX_NAT_TRANSLATION_ENTRY *remove_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    _nx_nat_table_find_entry(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_ENTRY *entry_tomatch, NX_NAT_TRANSLATION_ENTRY **match_entry_ptr, UINT skip_inbound_init); 
UINT    _nxe_nat_table_find_entry(NX_NAT_DEVICE *nat_ptr, NX_NAT_TRANSLATION_ENTRY *entry_tomatch, NX_NAT_TRANSLATION_ENTRY **match_entry_ptr, UINT skip_inbound_init); 
UINT    _nx_nat_utility_display_arp_table(NX_IP *ip_ptr);
UINT    _nxe_nat_utility_display_arp_table(NX_IP *ip_ptr);
UINT    _nx_nat_utility_display_translation_table(NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    _nxe_nat_utility_display_translation_table(NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
UINT    _nx_nat_utility_display_bytepool_reserves(NX_NAT_DEVICE *nat_ptr);
UINT    _nxe_nat_utility_display_bytepool_reserves(NX_NAT_DEVICE *nat_ptr);
UINT    _nx_nat_utility_display_packetpool_reserves(NX_NAT_DEVICE *nat_ptr);
UINT    _nxe_nat_utility_display_packetpool_reserves(NX_NAT_DEVICE *nat_ptr);
UINT    _nx_nat_utility_get_destination_port(NX_PACKET *packet_ptr, UINT protocol, UINT *destination_port);
UINT    _nxe_nat_utility_get_destination_port(NX_PACKET *packet_ptr, UINT protocol, UINT *destination_port);
UINT    _nx_nat_utility_get_source_port(NX_PACKET *packet_ptr, UINT protocol, UINT *source_port);
UINT    _nxe_nat_utility_get_source_port(NX_PACKET *packet_ptr, UINT protocol, UINT *source_port);

#endif

/* Define internal NAT services. */

UINT    _nx_nat_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, ULONG mutex_wait_option);
UINT    _nx_nat_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, ULONG mutex_wait_option);
UINT    _nx_nat_checksum_adjustment(UCHAR *old_checksum, UCHAR *old_data, UCHAR *new_data, UINT data_adjustment_length, ULONG *adjusted_checksum);
UINT    _nx_nat_compute_udp_checksum(NX_PACKET *packet_ptr, ULONG source_address, ULONG destination_address);
UINT    _nx_nat_compute_ip_checksum(NX_PACKET *packet_ptr);
UINT    _nx_nat_handle_FTP_data_connection(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr, NX_NAT_TRANSLATION_ENTRY **entry_ptr, UINT *is_FTP_connection_attempt);
UINT    _nx_nat_icmp_error_message_process(NX_PACKET *icmp_packet_ptr, NX_NAT_DEVICE *nat_ptr, UINT packet_is_outbound);
UINT    _nx_nat_icmp_error_message_send(NX_NAT_DEVICE *nat_ptr, NX_IP *ip_ptr, NX_PACKET *returned_packet_ptr, UINT error_message_type, UINT error_message_code, ULONG unused_icmp_header_word);
UINT    _nx_nat_packet_is_icmp_error_message(NX_PACKET *packet_ptr, UINT *is_icmp_error_msg);
UINT    _nx_nat_outbound_PORT_command(NX_PACKET *packet_ptr, NX_NAT_TRANSLATION_ENTRY *entry_ptr, UINT *length_is_changed);
UINT    _nx_nat_packet_datagram_send(NX_NAT_DEVICE *nat_ptr, ULONG fragment_id, NX_IP *nat_ip_ptr, ULONG old_ip_address, NX_PACKET *packet_ptr);
UINT    _nx_nat_packet_ok_for_fragmentation(NX_IP *ip_ptr, NX_PACKET *packet_ptr);
UINT    _nx_nat_packet_queue_add_to_nat(NX_NAT_DEVICE *nat_ptr, NX_NAT_PACKET_FRAGMENT_QUEUE *fragment_queue_ptr);
UINT    _nx_nat_packet_queue_add_fragment(NX_NAT_PACKET_FRAGMENT *packet_fragment_ptr, NX_NAT_PACKET_FRAGMENT_QUEUE *fragment_queue_ptr);
UINT    _nx_nat_packet_queue_process_fragment(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr, NX_NAT_PACKET_FRAGMENT_QUEUE **fragment_queue_ptr);
UINT    _nx_nat_packet_queue_remove(NX_NAT_DEVICE *nat_ptr, NX_NAT_PACKET_FRAGMENT_QUEUE *fragment_queue_ptr);
UINT    _nx_nat_packet_release(NX_PACKET *packet_ptr);
UINT    _nx_nat_packet_send(NX_NAT_DEVICE *nat_ptr, NX_IP *nat_ip_ptr, NX_PACKET *packet_ptr, ULONG destination_ip, ULONG time_to_live, ULONG protocol, ULONG old_ip_address, ULONG fragment);
UINT    _nx_nat_port_bind(NX_NAT_DEVICE *nat_ptr, NX_IP *ip_ptr, UINT protocol, NX_TCP_SOCKET *tcp_socket_ptr, NX_UDP_SOCKET *udp_socket_ptr, UINT port);
UINT    _nx_nat_process_inbound_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr);
UINT    _nx_nat_process_inbound_TCP_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr);
UINT    _nx_nat_process_inbound_UDP_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr);
UINT    _nx_nat_process_inbound_ICMP_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr);
UINT    _nx_nat_process_outbound_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr);
UINT    _nx_nat_process_outbound_TCP_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr, ULONG fragment_id, NX_NAT_TRANSLATION_ENTRY *entry_ptr);
UINT    _nx_nat_process_outbound_UDP_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr, ULONG fragment_id, NX_NAT_TRANSLATION_ENTRY *entry_ptr);
UINT    _nx_nat_process_outbound_ICMP_packet(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr, ULONG fragment_id, NX_NAT_TRANSLATION_ENTRY *entry_ptr);
VOID    _nx_nat_process_packet(struct NX_IP_STRUCT *ip_ptr, NX_PACKET *packet_ptr, UINT *packet_consumed);
UINT    _nx_nat_process_received_packet_fragments(NX_NAT_DEVICE *nat_ptr, NX_IP *ip_ptr, NX_PACKET *packet_ptr, NX_NAT_PACKET_FRAGMENT_QUEUE **fragment_queue_ptr);
VOID    _nx_nat_server_thread_entry(ULONG info);
UINT    _nx_nat_table_entry_add(NX_NAT_TRANSLATION_ENTRY *nat_table_entry_ptr, NX_NAT_TRANSLATION_TABLE *nat_table_ptr);
VOID    _nx_nat_table_timeout_entry(ULONG info);
VOID    _nx_nat_table_set_entry_timeout(NX_NAT_DEVICE *nat_ptr, NX_PACKET *packet_ptr, UINT packet_direction, UINT *response_timeout);
UINT    _nx_nat_table_find_available_query_ID(NX_NAT_DEVICE *nat_ptr, UINT *queryID);
VOID    _nx_nat_utility_find_crlf(CHAR *buffer, UINT length, CHAR **CRLF, UINT reverse);
UINT    _nx_nat_utility_convert_IP_ULONG_to_ascii(ULONG IP_address, CHAR *numstring, UINT length);
UINT    _nx_nat_utility_convert_portnumber_ascii_to_ULONG(CHAR *buffer, UINT *number, UINT buffer_length);
UINT    _nx_nat_utility_convert_portnumber_ULONG_to_ascii(CHAR *buffer, ULONG number, UINT buffer_length);
UINT    _nx_nat_utility_convert_number_ascii(UINT number, CHAR *numstring, UINT string_length);

#ifdef  NX_NAT_ALTERNATE_UCHAR_TO_ASCII
UINT    _nx_uchar_to_ascii_convert(UCHAR number, CHAR *buffstring);
#else
#define _nx_uchar_to_ascii_convert(number, buffstring)  sprintf((buffstring),"%u",(unsigned int)(number))
#endif /* ifdef  NX_NAT_ALTERNATE_UCHAR_TO_ASCII */


/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
        }
#endif

#endif /* NX_NAT_H  */


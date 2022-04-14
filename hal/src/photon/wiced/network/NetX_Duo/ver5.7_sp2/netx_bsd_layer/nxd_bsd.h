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
/*  Express Logic, Inc.                                                   */ 
/*  11423 West Bernardo Court               info@expresslogic.com         */ 
/*  San Diego, CA  92127                    http://www.expresslogic.com   */ 
/*                                                                        */ 
/**************************************************************************/ 

/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** NetX Component                                                        */ 
/**                                                                       */
/** BSD 4.3 Socket API Compatible Interface to NetX Duo                   */ 
/**                                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/ 
/*                                                                        */ 
/*  BSD DEFINITIONS                                        RELEASE        */ 
/*                                                                        */ 
/*    nxd_bsd.h                                           PORTABLE C      */ 
/*                                                           5.3          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Express Logic, Inc.                                                 */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the constants, structures, etc... needed to       */ 
/*    implement the BSD 4.3 Socket API Compatible Interface to NetX Duo.  */
/*                                                                        */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  06-01-2010     Yuxin Zhou               Initial Version 5.0           */ 
/*  12-15-2011     Janet Christiansen       Modified comment(s),          */
/*                                            modified definition of      */
/*                                            FD_SETSIZE, FD macros,time_t*/
/*                                            to eliminate redefinition   */
/*                                            compiler errors on some     */
/*                                            environments, added support */
/*                                            for BSD extended services,  */
/*                                            modified BSD status codes,  */
/*                                            resulting in version 5.1    */
/*  01-10-2012     Janet Christiansen       Modified comment(s), and      */
/*                                            added support for new socket*/
/*                                            options send and receive    */
/*                                            timeouts, TCP receive window*/
/*                                            size, and added new         */
/*                                            internal functions for more */
/*                                            efficient processing,       */
/*                                            new configuration define    */
/*                                            NX_EXTENDED_BSD_LINGER_AND_ */
/*                                            TIMED_WAIT compile option,  */
/*                                            corrected an error in the   */
/*                                            NX_BSD_INHERIT_LISTENER_    */
/*                                            _SOCKET_SETTINGS definition,*/
/*                                            added the NX_BSD_PRINT_     */
/*                                            ERRORS option, added support*/
/*                                            for raw sockets,            */
/*                                            resulting in version 5.2    */
/*  01-31-2013     Janet Christiansen       Modified comment(s), and      */
/*                                            added support for raw       */
/*                                            sockets, multicast packets, */
/*                                            mapping multiple UDP BSD    */
/*                                            sockets per NetX Duo UDP    */
/*                                            sockets, process in timer,  */
/*                                            added configuration options */
/*                                            for TCP BSD sockets,        */
/*                                            resulting in version 5.3    */
/*                                                                        */ 
/**************************************************************************/

#ifndef NXD_BSD_H
#define NXD_BSD_H


/* Bring in the necessary NetX include file.  */
#include "nx_api.h"
#include "nx_packet.h"


/* Define the print error macro for reporting source line number.  

#define NX_BSD_PRINT_ERRORS
*/


/* 
   Define the BSD socket timeout process to execute in the timer context. 
   If this option is not defined, application needs to create stack space for the BSD timeout process
   thread, and passes the stack to the BSD layer through the bsd_initialize() call.  In this configuration,
   the BSD timeout process is all done in the BSD timeout process thread context.

   User may choose to define NX_BSD_TIMEOUT_PROCESS_IN_TIMER, so the BSD timeout process is executed in the
   ThreadX timer object context.  This configuration could eliminate an extra thread (and associated stack
   space.)  The following parameters passed into bsd_initialize are ignored:
   * bsd_thread_stack_area
   * bsd_thread_stack_size
   * bsd_thread_priority
   However system designer needs to be aware of the following if the BSD timeout process is
   executed in ThreadX timer context:
   * The amount of BSD socket processing may slow down the ThreadX timer;
   * If the timer is executed in ISR context (ThreadX library is built with TX_TIMER_PROCESS_IN_ISR),
     the BSD timeout process is now executing in the ISR context;

     By default NX_BSD_TIMEOUT_PROCESS_IN_TIMER is NOT defined. 
*/
/* #define NX_BSD_TIMEOUT_PROCESS_IN_TIMER */
                      
/* Define configuration constants for the BSD compatibility layer.  Note that these can be overridden via -D or a #define somewhere else.  */

#ifndef NX_BSD_TCP_WINDOW
#define NX_BSD_TCP_WINDOW                   65535                   /* 64k is typical window size for 100Mb ethernet.                       */
#endif 

#ifndef NX_BSD_SOCKFD_START
#define NX_BSD_SOCKFD_START                 32                      /* Logical FD starting value.                                           */ 
#endif

#ifndef NX_BSD_MAX_SOCKETS  
#define NX_BSD_MAX_SOCKETS                  32                      /* Maximum number of total sockets available in the BSD layer. Note     */
#endif                                                              /*   NOTE:  Must be multiple of 32!                                     */ 

#ifndef NX_BSD_MAX_LISTEN_BACKLOG
#define NX_BSD_MAX_LISTEN_BACKLOG           5                       /* Maximum listen backlog.                                              */ 
#endif

#ifndef NX_CPU_TICKS_PER_SECOND
#define NX_CPU_TICKS_PER_SECOND             100                     /* Number of timer ticks per second, default 10ms timer.                */  
#endif

#ifndef NX_MICROSECOND_PER_CPU_TICK
#define NX_MICROSECOND_PER_CPU_TICK         10000                   /* Number of microseconds per timer interrupt, default 10ms.            */ 
#endif
#ifndef NX_BSD_TIMEOUT
#define NX_BSD_TIMEOUT                      (20*NX_CPU_TICKS_PER_SECOND)      
                                                                    /* By default all internal NetX calls wait and block for 20 seconds.    */
#endif

/* Define configurable options for BSD extended options. */


#ifndef NX_BSD_TIMED_WAIT_TIMEOUT                                            /* Only available with NetX 5.4 and higher.                     */
#define NX_BSD_TIMED_WAIT_TIMEOUT           (60 * NX_CPU_TICKS_PER_SECOND)   /* Timeout for sockets in the Timed Wait state (socket must     */
#endif                                                                       /* not be enabled with the REUSEADDR to enter this state).      */ 

#ifndef NX_BSD_TIMER_RATE                 
#define NX_BSD_TIMER_RATE                   (1 * NX_CPU_TICKS_PER_SECOND)   /* Rate at which BSD timer runs.                                   */
#endif                                                                      

/* This determines if the secondary sockets created from the master socket inherit
  the master socket socket options and socket flags. The default is enabled.  */
#define NX_BSD_INHERIT_LISTENER_SOCKET_SETTINGS 

            
/* Define BSD events */

#define NX_BSD_RECEIVE_EVENT                ((ULONG) 0x00000001)    /* Event flag to signal a receive packet event                          */ 
#define NX_BSD_SELECT_EVENT                 ((ULONG) 0x00008000)    /* Event flag to signal a thread is waiting in select                   */ 
#define NX_BSD_ALL_EVENTS                   ((ULONG) 0xFFFFFFFF)    /* All event flag                                                       */
#define NX_BSD_CONNECT_EVENT                ((ULONG) 0x00000002)
#define NX_BSD_LINGER_EVENT                 ((ULONG) 0x00000004)    /* Event flag to signal a timed linger state has expired on a socket    */
#define NX_BSD_TIMED_WAIT_EVENT             ((ULONG) 0x00000008)    /* Event flag to signal a timed wait state has expired on a socket      */
#define NX_BSD_TIMER_EVENT                  ((ULONG) 0x00000010)    /* Event flag to singal a BSD 1 sec timer */

/* For compatibility undefine the fd_set.  Then define the FD set size.  */

#ifdef fd_set
#undef fd_set
#endif

#ifdef   FD_SETSIZE
#undef   FD_SETSIZE                                                  /* May be different in other header files e.g 64 in GNU types.h file    */
#define  FD_SETSIZE                          NX_BSD_MAX_SOCKETS      /* Number of sockets to select on - same is max sockets!                */
#else
#define  FD_SETSIZE                          NX_BSD_MAX_SOCKETS      /* Number of sockets to select on - same is max sockets!                */
#endif


/* Define some BSD protocol constants.  */

#define SOCK_STREAM                         1                       /* TCP Socket                                                          */
#define SOCK_DGRAM                          2                       /* UDP Socket                                                          */
#define SOCK_RAW                            3                       /* Raw socket                                                          */
#define IPPROTO_TCP                         6                       /* TCP Socket                                                          */
#define IPPROTO_UDP                         17                      /* TCP Socket                                                          */
#define IPPROTO_RAW                         255                     /* Raw Socket                                                          */

/* Address families.  */

#define     AF_UNSPEC                       0                       /* Unspecified.                                                         */
#define     AF_NS                           1                       /* Local to host (pipes, portals).                                      */
#define     AF_INET                         2                       /* IPv4 socket (UDP, TCP, etc)                                          */
#define     AF_INET6                        3                       /* IPv6 socket (UDP, TCP, etc)                                          */

/* Protocol families, same as address families.  */
#define     PF_INET                         AF_INET
#define     PF_INET6                        AF_INET6

#define     INADDR_ANY                      0
#define     NX_BSD_LOCAL_IF_INADDR_ANY      0xFFFFFFFF
/* Define API error codes.  */

#define NX_SOC_ERROR                       -1                       /* Failure.                                                             */
#ifndef ERROR
#define ERROR                               NX_SOC_ERROR
#endif
#define NX_SOC_OK                           0                       /* Success.                                                             */
#ifndef OK
#define OK                                  NX_SOC_OK
#endif
#define NX_BSD_BLOCK_POOL_ERROR             1
#define NX_BSD_MUTEX_ERROR                  2
#define NX_BSD_THREAD_ERROR                 4
#define NX_BSD_EVENT_ERROR                  7

#ifndef NX_PACKET_OFFSET_ERROR
#define NX_PACKET_OFFSET_ERROR              0x53
#endif

/* The Netx API does not require Host to Network conversion or vice versa. The following macro's are provided for source compatibility reasons only.  */

#ifndef htons
#define htons(a)                            a
#endif
#ifndef htonl
#define htonl(a)                            a
#endif
#ifndef ntohs
#define ntohs(a)                            a
#endif
#ifndef ntohl
#define ntohl(a)                            a
#endif

/* Define error handling macro.  */

#ifdef NX_BSD_PRINT_ERRORS
#define NX_BSD_ERROR(status, line) printf(" NX BSD debug error message:, NX status: %x source line: %i \n", status, line)
#else
#define NX_BSD_ERROR(status, line) 
#endif


/* Define file descriptor operation flags. */

/* Note: FIONREAD is hardware dependant. The default is for i386 processor. */
#ifndef FIONREAD
#define FIONREAD                            0x541B                  /* Read bytes available for the ioctl() command                         */
#endif

#define F_GETFL                             3                       /* Get file descriptors                                                 */
#define F_SETFL                             4                       /* Set a subset of file descriptors (e.g. O_NONBlOCK                    */
#define O_NONBLOCK                          0x4000                  /* Option to enable non blocking on a file (e.g. socket)                */

#ifndef FIONBIO
#define FIONBIO                             0x5421                  /* Enables socket non blocking option for the ioctl() command            */
#endif


/* Define the minimal TCP socket listen backlog value. */
#ifndef NX_BSD_TCP_LISTEN_MIN_BACKLOG
#define NX_BSD_TCP_LISTEN_MIN_BACKLOG   1
#endif

/* Define the maximum number of packets that can be queued on a UDP socket or RAW socket. */
#ifndef NX_BSD_SOCKET_QUEUE_MAX 
#define NX_BSD_SOCKET_QUEUE_MAX 5
#endif

/* Define additional BSD socket errors. */

/* From errno-base.h in /usr/include/asm-generic;  */
#define EPERM        1  /* Operation not permitted */
#define E_MIN        1  /* Minimum Socket/IO error */
#define ENOENT       2  /* No such file or directory */
#define ESRCH        3  /* No such process */
#define EINTR        4  /* Interrupted system call */
#define EIO          5  /* I/O error */
#define ENXIO        6  /* No such device or address */
#define E2BIG        7  /* Argument list too long */
#define ENOEXEC      8  /* Exec format error */
#define EBADF        9  /* Bad file number */
#define ECHILD      10  /* No child processes */
#define EAGAIN      11  /* Try again */
#define ENOMEM      12  /* Out of memory */
#define EACCES      13  /* Permission denied */
#define EFAULT      14  /* Bad address */
#define ENOTBLK     15  /* Block device required */
#define EBUSY       16  /* Device or resource busy */
#define EEXIST      17  /* File exists */
#define EXDEV       18  /* Cross-device link */
#define ENODEV      19  /* No such device */
#define ENOTDIR     20  /* Not a directory */
#define EISDIR      21  /* Is a directory */
#define EINVAL      22  /* Invalid argument */
#define ENFILE      23  /* File table overflow */
#define EMFILE      24  /* Too many open files */
#define ENOTTY      25  /* Not a typewriter */
#define ETXTBSY     26  /* Text file busy */
#define EFBIG       27  /* File too large */
#define ENOSPC      28  /* No space left on device */
#define ESPIPE      29  /* Illegal seek */
#define EROFS       30  /* Read-only file system */
#define EMLINK      31  /* Too many links */
#define EPIPE       32  /* Broken pipe */
#define EDOM        33  /* Math argument out of domain of func */
#define ERANGE      34  /* Math result not representable */

/* From errno.h in /usr/include/asm-generic;   */

#define EDEADLK         35  /* Resource deadlock would occur */
#define ENAMETOOLONG    36  /* File name too long */
#define ENOLCK          37  /* No record locks available */
#define ENOSYS          38  /* Function not implemented */
#define ENOTEMPTY       39  /* Directory not empty */
#define ELOOP           40  /* Too many symbolic links encountered */
#define EWOULDBLOCK     EAGAIN  /* Operation would block */
#define ENOMSG          42  /* No message of desired type */
#define EIDRM           43  /* Identifier removed */
#define ECHRNG          44  /* Channel number out of range */
#define EL2NSYNC        45  /* Level 2 not synchronized */
#define EL3HLT          46  /* Level 3 halted */
#define EL3RST          47  /* Level 3 reset */
#define ELNRNG          48  /* Link number out of range */
#define EUNATCH         49  /* Protocol driver not attached */
#define ENOCSI          50  /* No CSI structure available */
#define EL2HLT          51  /* Level 2 halted */
#define EBADE           52  /* Invalid exchange */
#define EBADR           53  /* Invalid request descriptor */
#define EXFULL          54  /* Exchange full */
#define ENOANO          55  /* No anode */
#define EBADRQC         56  /* Invalid request code */
#define EBADSLT         57  /* Invalid slot */
#define EDEADLOCK       EDEADLK
#define EBFONT          59  /* Bad font file format */
#define ENOSTR          60  /* Device not a stream */
#define ENODATA         61  /* No data available */
#define ETIME           62  /* Timer expired */
#define ENOSR           63  /* Out of streams resources */
#define ENONET          64  /* Machine is not on the network */
#define ENOPKG          65  /* Package not installed */
#define EREMOTE         66  /* Object is remote */
#define ENOLINK         67  /* Link has been severed */
#define EADV            68  /* Advertise error */
#define ESRMNT          69  /* Srmount error */
#define ECOMM           70  /* Communication error on send */
#define EPROTO          71  /* Protocol error */
#define EMULTIHOP       72  /* Multihop attempted */
#define EDOTDOT         73  /* RFS specific error */
#define EBADMSG         74  /* Not a data message */
#define EOVERFLOW       75  /* Value too large for defined data type */
#define ENOTUNIQ        76  /* Name not unique on network */
#define EBADFD          77  /* File descriptor in bad state */
#define EREMCHG         78  /* Remote address changed */
#define ELIBACC         79  /* Can not access a needed shared library */
#define ELIBBAD         80  /* Accessing a corrupted shared library */
#define ELIBSCN         81  /* .lib section in a.out corrupted */
#define ELIBMAX         82  /* Attempting to link in too many shared libraries */
#define ELIBEXEC        83  /* Cannot exec a shared library directly */
#define EILSEQ          84  /* Illegal byte sequence */
#define ERESTART        85  /* Interrupted system call should be restarted */
#define ESTRPIPE        86  /* Streams pipe error */
#define EUSERS          87  /* Too many users */
#define ENOTSOCK        88  /* Socket operation on non-socket */
#define EDESTADDRREQ    89  /* Destination address required */
#define EMSGSIZE        90  /* Message too long */
#define EPROTOTYPE      91  /* Protocol wrong type for socket */
#define ENOPROTOOPT     92  /* Protocol not available */
#define EPROTONOSUPPORT 93  /* Protocol not supported */
#define ESOCKTNOSUPPORT 94  /* Socket type not supported */
#define EOPNOTSUPP      95  /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT    96  /* Protocol family not supported */
#define EAFNOSUPPORT    97  /* Address family not supported by protocol */
#define EADDRINUSE      98  /* Address already in use */
#define EADDRNOTAVAIL   99  /* Cannot assign requested address */
#define ENETDOWN        100 /* Network is down */
#define ENETUNREACH     101 /* Network is unreachable */
#define ENETRESET       102 /* Network dropped connection because of reset */
#define ECONNABORTED    103 /* Software caused connection abort */
#define ECONNRESET      104 /* Connection reset by peer */
#define ENOBUFS         105 /* No buffer space available */
#define EISCONN         106 /* Transport endpoint is already connected */
#define ENOTCONN        107 /* Transport endpoint is not connected */
#define ESHUTDOWN       108 /* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS    109 /* Too many references: cannot splice */
#define ETIMEDOUT       110 /* Connection timed out */
#define ECONNREFUSED    111 /* Connection refused */
#define EHOSTDOWN       112 /* Host is down */
#define EHOSTUNREACH    113 /* No route to host */
#define EALREADY        114 /* Operation already in progress */
#define EINPROGRESS     115 /* Operation now in progress */
#define ESTALE          116 /* Stale NFS file handle */
#define EUCLEAN         117 /* Structure needs cleaning */
#define ENOTNAM         118 /* Not a XENIX named type file */
#define ENAVAIL         119 /* No XENIX semaphores available */
#define EISNAM          120 /* Is a named type file */
#define EREMOTEIO       121 /* Remote I/O error */
#define EDQUOT          122 /* Quota exceeded */
#define ENOMEDIUM       123 /* No medium found */
#define EMEDIUMTYPE     124 /* Wrong medium type */
#define ECANCELED       125 /* Operation Canceled */
#define ENOKEY          126 /* Required key not available */
#define EKEYEXPIRED     127 /* Key has expired */
#define EKEYREVOKED     128 /* Key has been revoked */
#define EKEYREJECTED    129 /* Key was rejected by service */
#define EOWNERDEAD      130 /* Owner died - for robust mutexes*/
#define ENOTRECOVERABLE 131 /* State not recoverable */
#define ERFKILL         132 /* Operation not possible due to RF-kill */


/* List of BSD sock options from socket.h in /usr/include/asm/socket.h and asm-generic/socket.h.
   Note: not all of these are implemented in NetX Duo Extended socket options.  

   The first set of socket options take the socket level (category) SOL_SOCKET. */

#define SOL_SOCKET      1   /* Define the socket option category. */
#define IPPROTO_IP      2   /* Define the IP option category.     */
#define SO_MIN          1   /* Minimum Socket option ID */
#define SO_DEBUG        1   /* Debugging information is being recorded.*/
#define SO_REUSEADDR    2   /* Enable reuse of local addresses in the time wait state */
#define SO_TYPE         3   /* Socket type */
#define SO_ERROR        4   /* Socket error status */
#define SO_DONTROUTE    5   /* Bypass normal routing */
#define SO_BROADCAST    6   /* Transmission of broadcast messages is supported.*/
#define SO_SNDBUF       7   /* Enable setting trasnmit buffer size */
#define SO_RCVBUF       8   /* Enable setting receive buffer size */
#define SO_KEEPALIVE    9   /* Connections are kept alive with periodic messages */
#define SO_OOBINLINE    10  /* Out-of-band data is transmitted in line */
#define SO_NO_CHECK     11  /* Disable UDP checksum */
#define SO_PRIORITY     12  /* Set the protocol-defined priority for all packets to be sent on this socket */
#define SO_LINGER       13  /* Socket lingers on close pending remaining send/receive packets. */
#define SO_BSDCOMPAT    14  /* Enable BSD bug-to-bug compatibility */
#define SO_REUSEPORT    15  /* Rebind a port already in use */

#ifndef SO_PASSCRED          /* Used for passing credentials. Not currently in use. */
#define SO_PASSCRED     16   /* Enable passing local user credentials  */
#define SO_PEERCRED     17   /* Obtain the process, user and group ids of the other end of the socket connection */
#define SO_RCVLOWAT     18   /* Enable receive "low water mark" */
#define SO_SNDLOWAT     19   /* Enable send "low water mark" */
#define SO_RCVTIMEO     20   /* Enable receive timeout */
#define SO_SNDTIMEO     21   /* Enable send timeout */
#endif  /* SO_PASSCRED */
#define SO_SNDBUFFORCE  22  /* Enable setting trasnmit buffer size overriding user limit (admin privelege) */
#define SO_RCVBUFFORCE  23  /* Enable setting trasnmit buffer size overriding user limit (admin privelege) */
#define SO_MAX          SO_RCVBUFFORCE /* Maximum Socket option ID      */

/*  This second set of socket options take the socket level (category) IPPROTO_IP. */

#define IP_MULTICAST_IF     27 /* Specify outgoing multicast interface */
#define IP_MULTICAST_TTL    28 /* Specify the TTL value to use for outgoing multicast packet. */
#define IP_MULTICAST_LOOP   29 /* Whether or not receive the outgoing multicast packet, loopbacloopbackk mode. */
#define IP_BLOCK_SOURCE     30 /* Block multicast from certain source. */
#define IP_UNBLOCK_SOURCE   31 /* Unblock multicast from certain source. */
#define IP_ADD_MEMBERSHIP   32 /* Join IPv4 multicast membership */
#define IP_DROP_MEMBERSHIP  33 /* Leave IPv4 multicast membership */
#define IP_HDRINCL          34 /* Raw socket IPv4 header included. */
#define IP_RAW_RX_NO_HEADER 35 /* NetX proprietary socket option that does not include 
                                  IPv4/IPv6 header (and extension headers) on received raw sockets.*/
#define IP_RAW_IPV6_HDRINCL 36 /* Transmitted buffer over IPv6 socket contains IPv6 header. */
#define IP_OPTION_MAX       IP_RAW_IPV6_HDRINCL



/*
 * User-settable options (used with setsockopt).
 */
#define TCP_NODELAY 0x01    /* don't delay send to coalesce packets     */
#define TCP_MAXSEG  0x02    /* set maximum segment size                 */
#define TCP_NOPUSH  0x04    /* don't push last block of write           */
#define TCP_NOOPT   0x08    /* don't use TCP options                    */


/* Define data types used in structure timeval.  */
#ifdef SYS_TIME_H_AVAILABLE
#include <sys/time.h>
#else /* ifdef SYS_TIME_H_AVAILABLE */

#ifndef __time_t_defined
typedef LONG       time_t;
#endif

#ifndef __suseconds_t_defined
typedef LONG       suseconds_t;
#endif /* ifndef __suseconds_t_defined */

struct timeval
{
    time_t          tv_sec;             /* Seconds      */
    suseconds_t     tv_usec;            /* Microseconds */
};

#endif /* ifdef SYS_TIME_H_AVAILABLE */

struct sockaddr
{
    USHORT          sa_family;              /* Address family (e.g. , AF_INET).                 */
    UCHAR           sa_data[14];            /* Protocol- specific address information.          */
};

struct in6_addr 
{
    union 
    {
        UCHAR _S6_u8[16];
        ULONG _S6_u32[4];
    } _S6_un;
};

#define s6_addr     _S6_un._S6_u8
#define s6_addr32   _S6_un._S6_u32

struct sockaddr_in6 
{
    USHORT          sin6_family;                 /* AF_INET6 */
    USHORT          sin6_port;                   /* Transport layer port.  */
    ULONG           sin6_flowinfo;               /* IPv6 flow information. */
    struct in6_addr sin6_addr;                   /* IPv6 address. */
    ULONG           sin6_scope_id;               /* set of interafces for a scope. */

};

/* Internet address (a structure for historical reasons).  */

struct in_addr
{
    ULONG           s_addr;             /* Internet address (32 bits).                         */        
};

#define in_addr_t   ULONG
                                 
/* Socket address, Internet style. */

struct sockaddr_in
{
    USHORT              sin_family;         /* Internet Protocol (AF_INET).                    */
    USHORT              sin_port;           /* Address port (16 bits).                         */
    struct in_addr      sin_addr;           /* Internet address (32 bits).                     */
    CHAR                sin_zero[8];        /* Not used.                                       */
};



typedef struct FD_SET_STRUCT                /* The select socket array manager.                                                             */
{ 
   INT                  fd_count;           /* How many are SET?                                                                            */
   ULONG                fd_array[(FD_SETSIZE + 31)/32]; /* Bit map of SOCKET Descriptors.                                                   */
} fd_set;



typedef struct NX_BSD_SOCKET_SUSPEND_STRUCT
{
    ULONG               nx_bsd_socket_suspend_actual_flags;
    fd_set              nx_bsd_socket_suspend_read_fd_set;
    fd_set              nx_bsd_socket_suspend_write_fd_set;
    fd_set              nx_bsd_socket_suspend_exception_fd_set;

} NX_BSD_SOCKET_SUSPEND;


struct ip_mreq 
{
    struct in_addr imr_multiaddr;     /* The IPv4 multicast address to join. */
    struct in_addr imr_interface;     /* The interface to use for this group. */
};


/* Define additional BSD data structures for supporting socket options.  */

struct sock_errno
{
    INT error;                              /* default = 0; */
};

struct linger
{
    INT l_onoff;                            /* 0 = disabled; 1 = enabled; default = 0;*/
    INT l_linger;                           /* linger time in seconds; default = 0;*/
};

struct sock_keepalive
{
    INT keepalive_enabled;                  /* 0 = disabled; 1 = enabled; default = 0;*/
};

struct sock_reuseaddr
{
    INT reuseaddr_enabled;                  /* 0 = disabled; 1 = enabled; default = 1; */
};

struct sock_winsize
{
    INT winsize;                            /* receive window size for TCP sockets   ; */
};


/* Define the BSD socket status bits. */
#define NX_BSD_SOCKET_CONNECTION_INPROGRESS      1
#define NX_BSD_SOCKET_ERROR                      (1 << 1)
#define NX_BSD_SOCKET_CONNECTED                  (1 << 2)
/* Disconnected from the stack. */
#define NX_BSD_SOCKET_DISCONNECT_FROM_STACK      (1 << 3) 
#define NX_BSD_SOCKET_SERVER_MASTER_SOCKET       (1 << 4)
#define NX_BSD_SOCKET_TX_HDR_INCLUDE             (1 << 5)
#define NX_BSD_SOCKET_RX_NO_HDR                  (1 << 6)


/* Define the internal management structure for the BSD layer.  */ 

typedef struct NX_BSD_SOCKET_STRUCT
{
    NX_TCP_SOCKET       *nx_bsd_socket_tcp_socket;
    NX_UDP_SOCKET       *nx_bsd_socket_udp_socket;
    UCHAR               nx_bsd_socket_in_use;
    UCHAR               nx_bsd_socket_client;
    UCHAR               nx_bsd_socket_listen_enabled;
    UCHAR               nx_bsd_socket_bound;
    UCHAR               nx_bsd_socket_accepting;
    UCHAR               nx_bsd_socket_secondary_server_socket;
    UCHAR               nx_bsd_socket_connection_request;
    UCHAR               nx_bsd_socket_disconnection_request;
    ULONG               nx_bsd_socket_family;
    /* Store the protocol number.  For example TCP is 6, UDP is 17. For raw socket
       it is the protocol it wishes to receive. */
    USHORT              nx_bsd_socket_protocol;
#ifdef NX_ENABLE_IP_RAW_PACKET_FILTER
    UINT                nx_bsd_raw_socket_enabled;
#endif
    TX_THREAD           *nx_bsd_socket_busy;
    INT                 nx_bsd_socket_master_socket_id;
    NX_PACKET*          nx_bsd_socket_received_packet;
    NX_PACKET*          nx_bsd_socket_received_packet_tail;
    UINT                nx_bsd_socket_received_byte_count;
    UINT                nx_bsd_socket_received_byte_count_max;
    UINT                nx_bsd_socket_received_packet_count;
    UINT                nx_bsd_socket_received_packet_count_max;
    ULONG               nx_bsd_socket_received_packet_offset;
    INT                 nx_bsd_socket_source_port;
    NXD_ADDRESS         nx_bsd_socket_source_ip_address;
    ULONG               nx_bsd_socket_local_bind_interface;
    UINT                nx_bsd_socket_local_bind_interface_index;
    NXD_ADDRESS         nx_bsd_socket_peer_ip;

    /* For TCP/UDP, the local port is the port number this socket receives on. 
       For raw socket this field is not used. */
    USHORT              nx_bsd_socket_local_port;
    USHORT              nx_bsd_socket_peer_port;
    INT                 nx_bsd_option_linger_enabled;
    INT                 nx_bsd_option_linger_time;
    UINT                nx_bsd_option_linger_time_closed; 
    UINT                nx_bsd_option_linger_start_close;   
    INT                 nx_bsd_option_reuseaddr_enabled;
    UINT                nx_bsd_socket_time_wait_remaining;
    INT                 nx_bsd_option_non_blocking_enabled;
    ULONG               nx_bsd_option_receive_timeout;  
    ULONG               nx_bsd_option_send_timeout;     
    INT                 nx_bsd_file_descriptor_flags;
    ULONG               nx_bsd_socket_status_flags;
    int                 nx_bsd_socket_error_code;

} NX_BSD_SOCKET;


/* Define the BSD function prototypes for use by the application.  */

INT  accept(INT sockID, struct sockaddr *ClientAddress, INT *addressLength);
INT  bsd_initialize(NX_IP *default_ip, NX_PACKET_POOL *default_pool, CHAR *bsd_thread_stack_area, ULONG bsd_thread_stack_size, UINT bsd_thread_priority);
UINT bsd_number_convert(UINT number, CHAR *string);
INT  bind(INT sockID, struct sockaddr *localAddress, INT addressLength);
INT  connect(INT sockID, struct sockaddr *remoteAddress, INT addressLength);
INT  getpeername(INT sockID, struct sockaddr *remoteAddress, INT *addressLength);
INT  getsockname(INT sockID, struct sockaddr *localAddress, INT *addressLength);
INT  ioctl(INT sockID, INT command, INT *result); 
in_addr_t inet_addr(const CHAR *buffer);
CHAR *inet_ntoa(struct in_addr address_to_convert);
INT  inet_aton(const CHAR *cp_arg, struct in_addr *addr);
INT  listen(INT sockID, INT backlog);
#ifdef NX_ENABLE_IP_RAW_PACKET_FILTER 
UINT nx_bsd_raw_packet_receive(NX_BSD_SOCKET *bsd_socket_ptr, NX_PACKET **packet_ptr);
UINT nx_bsd_raw_packet_info_extract(NX_PACKET *packet_ptr, NXD_ADDRESS *nxd_address, UINT *interface_index);
VOID nx_bsd_raw_receive_notify(NX_IP *ip_ptr, UINT bsd_socket_index);
#endif
UINT nx_bsd_socket_set_inherited_settings(UINT master_sock_id, UINT secondary_sock_id);
INT  recvfrom(INT sockID, CHAR *buffer, INT buffersize, INT flags,struct sockaddr *fromAddr, INT *fromAddrLen);
INT  recv(INT sockID, VOID *rcvBuffer, INT bufferLength, INT flags);
INT  sendto(INT sockID, CHAR *msg, INT msgLength, INT flags, struct sockaddr *destAddr, INT destAddrLen);
INT  send(INT sockID, const CHAR *msg, INT msgLength, INT flags);
INT  select(INT nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
INT  soc_close( INT sockID);
INT  socket(INT protocolFamily, INT type, INT protocol);
INT  fcntl(INT sock_ID, UINT flag_type, UINT f_options);
INT  getsockopt(INT sockID, INT option_level, INT option_name, void *option_value, INT *option_length);
INT  setsockopt(INT sockID, INT option_level, INT option_name, const void *option_value, INT option_length);

#undef FD_SET
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO

VOID FD_SET(INT fd, fd_set *fdset);
VOID FD_CLR(INT fd, fd_set *fdset);
INT  FD_ISSET(INT fd, fd_set *fdset);
VOID FD_ZERO(fd_set *fdset);
VOID set_errno(INT tx_errno);
int _nxd_get_errno(void);
#define errno (_nxd_get_errno())

#endif  /* NXD_BSD_H */


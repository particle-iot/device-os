/*****************************************************************************
*
*  evnt_handler.h  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

#ifndef __INCLUDE_NUTTX_WIRELESS_CC3000_EVENT_HANDLER_H
#define __INCLUDE_NUTTX_WIRELESS_CC3000_EVENT_HANDLER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/wireless/cc3000/hci.h>
#include <sys/socket.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define M_BSD_RESP_PARAMS_OFFSET(hci_event_hdr)\
  ((char *)(hci_event_hdr) + HCI_EVENT_HEADER_SIZE)

#define SOCKET_STATUS_ACTIVE       0
#define SOCKET_STATUS_INACTIVE     1

/* Init socket_active_status = 'all ones': init all sockets with
 * SOCKET_STATUS_INACTIVE. Will be changed by 'set_socket_active_status' upon
 * 'connect' and 'accept' calls
 */

#define SOCKET_STATUS_INIT_VAL  0xFFFF
#define M_IS_VALID_SD(sd) ((0 <= (sd)) && ((sd) <= 7))
#define M_IS_VALID_STATUS(status) \
  (((status) == SOCKET_STATUS_ACTIVE)||((status) == SOCKET_STATUS_INACTIVE))

#define BSD_RECV_FROM_FROMLEN_OFFSET  (4)
#define BSD_RECV_FROM_FROM_OFFSET    (16)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct _bsd_accept_return_t
{
  long             iSocketDescriptor;
  long             iStatus;
  struct sockaddr  tSocketAddress;
} tBsdReturnParams;

typedef struct _bsd_read_return_t
{
  long             iSocketDescriptor;
  long             iNumberOfBytes;
  unsigned long    uiFlags;
} tBsdReadReturnParams;

typedef struct _bsd_select_return_t
{
  long             iStatus;
  unsigned long    uiRdfd;
  unsigned long    uiWrfd;
  unsigned long    uiExfd;
} tBsdSelectRecvParams;

typedef struct _bsd_getsockopt_return_t
{
  uint8_t          ucOptValue[4];
  char             iStatus;
} tBsdGetSockOptReturnParams;

typedef struct _bsd_gethostbyname_return_t
{
  long             retVal;
  long             outputAddress;
} tBsdGethostbynameParams;

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

extern unsigned long socket_active_status;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: hci_event_handler
 *
 * Description:
 *   Parse the incoming events packets and issues corresponding event
 *   handler from global array of handlers pointers
 *
 * Input Parameters:
 *   pRetParams     incoming data buffer
 *   from           from information (in case of data received)
 *   fromlen        from information length (in case of data received)
 *
 * Returned Values:
 *   None
 *
 ****************************************************************************/

uint8_t *hci_event_handler(void *pRetParams, uint8_t *from, uint8_t *fromlen);

/****************************************************************************
 * Name: hci_unsol_event_handler
 *
 * Description:
 *   Handle unsolicited events
 *
 * Input Parameters:
 *   event_hdr   event header
 *
 * Returned Values:
 *   1 if event supported and handled; 0 if event is not supported
 *
 ****************************************************************************/

long hci_unsol_event_handler(char *event_hdr);

/****************************************************************************
 * Name: hci_unsolicited_event_handler
 *
 * Description:
 *   Parse the incoming unsolicited event packets and issues corresponding
 *   event handler.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   ESUCCESS if successful, EFAIL if an error occurred
 *
 ****************************************************************************/

long hci_unsolicited_event_handler(void);

void set_socket_active_status(long Sd, long Status);
long get_socket_active_status(long Sd);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __INCLUDE_NUTTX_WIRELESS_CC3000_EVENT_HANDLER_H

/*****************************************************************************
*
*  netapp.c  - CC3000 Host Driver Implementation.
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
#include <string.h>
#include "netapp.h"
#include "hci.h"
#include "socket.h"
#include "evnt_handler.h"
#include "nvmem.h"

#define MIN_TIMER_VAL_SECONDS      20
#define MIN_TIMER_SET(t)    if ((0 != t) && (t < MIN_TIMER_VAL_SECONDS)) \
                            { \
                                t = MIN_TIMER_VAL_SECONDS; \
                            }


#define NETAPP_DHCP_PARAMS_LEN 				(20)
#define NETAPP_SET_TIMER_PARAMS_LEN 		(20)
#define NETAPP_SET_DEBUG_LEVEL_PARAMS_LEN	(4)
#define NETAPP_PING_SEND_PARAMS_LEN			(16)


/**
 * \brief Configure MAC address
 *
 * Configure device MAC address and store it in NVMEM
 *  
 * \param[in] mac    device mac address, 6 bytes. Saved: yes 
 *       Default: 00-12-55-55-55-55
 *      
 *  
 * \return on success 0, otherwise error. 
 *  
 * \sa          
 * \note   The value of the MAC address configured through the API will be stored
 * 		 in CC3000 non volatile memory, thus preserved over resets.
 * \warning     
 */

long netapp_config_mac_adrress(unsigned char * mac)
{
   return  nvmem_set_mac_address(mac);
}


/**
 * \brief configure IP interface
 *
 * Netapp_Dhcp is used to configure the network interface, 
 * static or dynamic (DHCP).\n In order to activate DHCP mode, 
 * aucIP, aucSubnetMask, aucDefaultGateway must 
 * be 0.The default mode of CC3000 is DHCP mode.
 * Note that the configuration is saved in non volatile memory and thus
 * preserved over resets.
 *
 *  
 * \param[in] aucIP               static IP or 0 for dynamic. 
 *       Saved: yes
 * \param[in] aucSubnetMask       static MASK IP or 0 for 
 *       dynamic. Saved: yes
 * \param[in] aucDefaultGateway   static Gateway or 0 for 
 *       dynamic. Saved: yes
 * \param[in] aucDNSServer        static DNS server or dynamic 
 *       (DHCP mode must be used). Saved: yes
 *       
 *  
 * \return 
 *  
 * \sa          
 * \note If the mode is altered a reset of CC3000 device is required in order
 * to apply changes.\nAlso note that asynchronous event of DHCP_EVENT, 
 * which is generated when an IP address is allocated either by the DHCP server or due to static allocation
 * is generated only upon a connection to the AP was established\n
 * \warning     
 */
long netapp_dhcp(unsigned long *aucIP, unsigned long *aucSubnetMask,unsigned long *aucDefaultGateway, unsigned long *aucDNSServer)
{
    signed char scRet;
    unsigned char *ptr;
    unsigned char *args;

    scRet = EFAIL;
    ptr = tSLInformation.pucTxCommandBuffer;
    args = (ptr + HEADERS_SIZE_CMD);
	
    //
    // Fill in temporary command buffer
    //
	ARRAY_TO_STREAM(args,aucIP,4);
	ARRAY_TO_STREAM(args,aucSubnetMask,4);
	ARRAY_TO_STREAM(args,aucDefaultGateway,4);
	args = UINT32_TO_STREAM(args, 0);
	ARRAY_TO_STREAM(args,aucDNSServer,4);
	
    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_DHCP, ptr, NETAPP_DHCP_PARAMS_LEN);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_DHCP, &scRet);
	
    return(scRet);
}



/**
 * \brief Set new timeout values
 *  
 * Function set new timeout values for: DHCP lease timeout, ARP 
 * refresh timeout, keepalive event timeout and  socket 
 * inactivity timeout 
 * \return    On success, zero is returned. On error, -1 is 
 *            returned
 *  
 *  
 * \param[in] aucDHCP    DHCP lease time request, also impact 
 *       the DHCP renew timeout. Range: [0-0xffffffff] seconds,
 *       0 or 0xffffffff == infinity lease timeout. Resolution:
 *       10 seconds. Influence: only after reconnecting to the
 *       AP. Minimal bound value: MIN_TIMER_VAL_SECONDS - 20 
 *       seconds. The parameter is saved into the CC3000 NVMEM. 
 *	   The default value on CC3000 is 14400 seconds.
 * \param[in] aucARP    ARP refresh timeout, if ARP entry is not
 *       updated by incoming packet, the ARP entery will be
 *       deleted by the end of the timeout. Range:
 *       [0-0xffffffff] seconds, 0 == inifnity ARP timeout .
 *       Resolution: 10 seconds. Influence: on runtime.
 *       Minimal bound value: MIN_TIMER_VAL_SECONDS - 20 seconds
 *       The parameter is saved into the CC3000 NVMEM. 
 *	   The default value on CC3000 is 3600 seconds.
 * \param[in] aucKeepalive    Keepalive event sent by the end of
 *       keepalive timeout. Range: [0-0xffffffff] seconds, 0 ==
 *       inifnity Keepalive timeout. Resolution: 10 seconds.
 *       Influence: on runtime.
 *       Minimal bound value: MIN_TIMER_VAL_SECONDS - 20 seconds
 *       The parameter is saved into the CC3000 NVMEM. 
 *	   The default value on CC3000 is 10 seconds.
 * \param[in] aucInactivity    Socket inactivity timeout, socket
 *       timeout is refreshed by incoming or outgoing packet, by
 *       the end of the socket timeout the socket will be
 *       closed. Range: [0-0xffffffff] seconds, 0 == inifnity
 *       timeout. Resolution: 10 seconds. Influence: on runtime.
 *       Minimal bound value: MIN_TIMER_VAL_SECONDS - 20 seconds
 *       The parameter is saved into the CC3000 NVMEM. 
 *	   The default value on CC3000 is 60 seconds.
 * \sa          
 * \note If a parameter set to non zero value which is less than
 *        20s, it will be set automatically to 20s.
 * \warning     
 */
#ifndef CC3000_TINY_DRIVER
long 
netapp_timeout_values(unsigned long *aucDHCP, unsigned long *aucARP,unsigned long *aucKeepalive,	unsigned long *aucInactivity)
{
    signed char scRet;
    unsigned char *ptr;
    unsigned char *args;

    scRet = EFAIL;
    ptr = tSLInformation.pucTxCommandBuffer;
    args = (ptr + HEADERS_SIZE_CMD);

    //
    // Set minimal values of timers 
    //
    MIN_TIMER_SET(*aucDHCP)
    MIN_TIMER_SET(*aucARP)
    MIN_TIMER_SET(*aucKeepalive)
    MIN_TIMER_SET(*aucInactivity)

    //
    // Fill in temporary command buffer
    //
    args = UINT32_TO_STREAM(args, *aucDHCP);
	args = UINT32_TO_STREAM(args, *aucARP);
	args = UINT32_TO_STREAM(args, *aucKeepalive);
	args = UINT32_TO_STREAM(args, *aucInactivity);
	

    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_SET_TIMERS, ptr, NETAPP_SET_TIMER_PARAMS_LEN);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_SET_TIMERS, &scRet);
	
    return(scRet);
}
#endif

/**
 * \brief send ICMP ECHO_REQUEST to network hosts 
 *  
 *  ping uses the ICMP protocol's mandatory ECHO_REQUEST. 
 *
 *
 * \param[in]   ip              destination
 * \param[in]   pingAttempts    number of echo requests to send 
 * \param[in]   pingSize        send buffer size which may be up to 1400 bytes  
 * \param[in]   pingTimeout     Time to wait for a response,in 
 *       milliseconds
 *       
 *  
 * \return    On success, zero is returned. On error, -1 is 
 *            returned
 *  
 * \sa Netapp_PingReport Netapp_PingStop           
 * \note   If an operation finished
 *  successfully asynchronous ping report event will be generated. The report structure 
 *  is as defined by structure netapp_pingreport_args_t     
 * \warning  Calling this function while a previous Ping  Requests are in progress
 * will stop the previous ping request.
 */
#ifndef CC3000_TINY_DRIVER
long
netapp_ping_send(unsigned long *ip, unsigned long ulPingAttempts, unsigned long ulPingSize, unsigned long ulPingTimeout)
{
    signed char scRet;
    unsigned char *ptr, *args;

    scRet = EFAIL;
    ptr = tSLInformation.pucTxCommandBuffer;
    args = (ptr + HEADERS_SIZE_CMD);

    //
    // Fill in temporary command buffer
    //
	args = UINT32_TO_STREAM(args, *ip);
	args = UINT32_TO_STREAM(args, ulPingAttempts);
	args = UINT32_TO_STREAM(args, ulPingSize);
	args = UINT32_TO_STREAM(args, ulPingTimeout);
	   
    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_PING_SEND, ptr, NETAPP_PING_SEND_PARAMS_LEN);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_PING_SEND, &scRet);
	
    return(scRet);
}
#endif
/**
 * \brief Request for ping status. This API triggers the CC3000 
 *        to send asynchronous events:
 *        HCI_EVNT_WLAN_ASYNC_PING_REPORT. This event will carry
 *        the report structure: netapp_pingreport_args_t. This
 *        structure is filled in with ping results up till point
 *        of triggering API.\n
 *	   
 *       netapp_pingreport_args_t:\n packets_sent - echo sent,
 *       packets_received - echo reply, min_round_time - minimum
 *       round time, max_round_time - max round time,
 *       avg_round_time - average round time
 * 
 * \return void
 *  
 * \sa netapp_ping_stop netapp_ping_send
 * \note    When a ping operation is not active, the returned structure fields are 0.\n
 * \warning     
 */
#ifndef CC3000_TINY_DRIVER
void netapp_ping_report()
{
    unsigned char *ptr;
    ptr = tSLInformation.pucTxCommandBuffer;
    signed char scRet;

    scRet = EFAIL;

    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_PING_REPORT, ptr, 0);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_PING_REPORT, &scRet); 
}
#endif

/**
 * \brief Stop any ping request
 *
 
 * \return  On success, zero is returned. On error, -1 is 
 *           returned
 *  
 * \sa netapp_ping_report netapp_ping_send
 * \note        
 * \warning     
 */
#ifndef CC3000_TINY_DRIVER
long netapp_ping_stop()
{
    signed char scRet;
    unsigned char *ptr;

    scRet = EFAIL;
    ptr = tSLInformation.pucTxCommandBuffer;

    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_PING_STOP, ptr, 0);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_PING_STOP, &scRet);
	
    return(scRet);
}
#endif

/**
 * \brief Obtain the CC3000 Network interface information.
 *		Note that the information is available only after teh WLAN
 *		connection was established. Calling this function before
 *		assosiation, will cause non-defined values to be returned.
 * 
 *  
 * \param[out] ipconfig    This argument is a pointer to a 
 *       tNetappIpconfigRetArgs structure. This structure is
 *       filled in with the network interface configuration\n
 *       tNetappIpconfigRetArgs:\n aucIP - ip address,
 *       aucSubnetMask - mask, aucDefaultGateway - default
 *       gateway address, aucDHCPServer - dhcp server address,
 *       aucDNSServer - dns server address, uaMacAddr - mac
 *       address, uaSSID - connected AP ssid
 *        
 *  
 * \return    void
 *  
 * \sa          
 * \note  The function is useful for figuring out the IP Configuration of
 *		the device when DHCP is used, for figuring out the SSID of
 *		the Wireless network the device is assosiated with.
 * \warning     
 */
#ifndef CC3000_TINY_DRIVER
void netapp_ipconfig( tNetappIpconfigRetArgs * ipconfig )
{
    
    unsigned char *ptr;

     ptr = tSLInformation.pucTxCommandBuffer;

    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_IPCONFIG, ptr, 0);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_IPCONFIG, ipconfig );

}
#else
void netapp_ipconfig( tNetappIpconfigRetArgs * ipconfig )
{

}
#endif
/**
 * \brief Flushes ARP table
 *  
 * Function flush the ARP table
 *  
 * \return    On success, zero is returned. On error, -1 is 
 *           returned
 *  
 * \sa          
 * \note        
 * \warning     
 */
#ifndef CC3000_TINY_DRIVER
long netapp_arp_flush(void)
{
	signed char scRet;
    unsigned char *ptr;

    scRet = EFAIL;
    ptr = tSLInformation.pucTxCommandBuffer;

    //
    // Initiate a HCI command
    //
    hci_command_send(HCI_NETAPP_ARP_FLUSH, ptr, 0);

	//
	// Wait for command complete event
	//
	SimpleLinkWaitEvent(HCI_NETAPP_ARP_FLUSH, &scRet);
	
    return(scRet);
}
#endif

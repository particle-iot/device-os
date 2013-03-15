/*****************************************************************************
*
*  netapp.h  - CC3000 Host Driver Implementation.
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
#ifndef __NETAPP_H__
#define	__NETAPP_H__


//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef	__cplusplus
extern "C" {
#endif

//*****************************************************************************
//
//! \addtogroup netapp_api
//! @{
//
//*****************************************************************************

typedef struct _netapp_dhcp_ret_args_t
{
    unsigned char aucIP[4];
	unsigned char aucSubnetMask[4];
	unsigned char aucDefaultGateway[4];
	unsigned char aucDHCPServer[4];
	unsigned char aucDNSServer[4];
}tNetappDhcpParams;

typedef struct _netapp_ipconfig_ret_args_t
{
    unsigned char aucIP[4];
	unsigned char aucSubnetMask[4];
	unsigned char aucDefaultGateway[4];
	unsigned char aucDHCPServer[4];
	unsigned char aucDNSServer[4];
	unsigned char uaMacAddr[6];
	unsigned char uaSSID[32];
}tNetappIpconfigRetArgs;


/*Ping send report parameters*/
typedef struct _netapp_pingreport_args
{
	unsigned long packets_sent;
	unsigned long packets_received;
	unsigned long min_round_time;
	unsigned long max_round_time;
	unsigned long avg_round_time;
} netapp_pingreport_args_t;


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

extern long  netapp_config_mac_adrress( unsigned char *mac );



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
extern 	long netapp_dhcp(unsigned long *aucIP, unsigned long *aucSubnetMask,unsigned long *aucDefaultGateway, unsigned long *aucDNSServer);

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
 * \sa netapp_ping_report netapp_Ping_stop
 * \note   If an operation finished
 *  successfully asynchronous ping report event will be generated. The report structure 
 *  is as defined by structure netapp_pingreport_args_t     
 * \warning  Calling this function while a previous Ping  Requests are in progress
 * will stop the previous ping request.
 */
 #ifndef CC3000_TINY_DRIVER
extern long netapp_ping_send(unsigned long *ip, unsigned long ulPingAttempts, unsigned long ulPingSize, unsigned long ulPingTimeout);
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
extern void netapp_ping_report();
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
extern long netapp_ping_stop();
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

extern void netapp_ipconfig( tNetappIpconfigRetArgs * ipconfig );


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
extern long netapp_arp_flush();
#endif

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
extern long netapp_timeout_values(unsigned long *aucDHCP, unsigned long *aucARP,unsigned long *aucKeepalive,	unsigned long *aucInactivity);
#endif


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************



//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef	__cplusplus
}
#endif // __cplusplus

#endif	// __NETAPP_H__


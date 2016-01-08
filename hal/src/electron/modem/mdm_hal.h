/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#pragma once

//#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "pipe_hal.h"
#include "electronserialpipe_hal.h"
#include "pinmap_hal.h"
#include "system_tick_hal.h"
#include "enums_hal.h"

/* Include for debug capabilty */
#define MDM_DEBUG

#define USE_USART3_HARDWARE_FLOW_CONTROL_RTS_CTS 1

/** basic modem parser class
*/
class MDMParser
{
public:
    //! Constructor
    MDMParser(void);
    //! get static instance
    static MDMParser* getInstance() { return inst; };

    /* Used to cancel all operations */
    void cancel(void);

    /* User to resume all operations */
    void resume(void);

    /** Combined Init, checkNetStatus, join suitable for simple applications
        \param simpin a optional pin of the SIM card
        \param apn  the of the network provider e.g. "internet" or "apn.provider.com"
        \param username is the user name text string for the authentication phase
        \param password is the password text string for the authentication phase
        \param auth is the authentication mode (CHAP,PAP,NONE or DETECT)
        \return true if successful, false otherwise
    */
    bool connect(const char* simpin = NULL,
            const char* apn = "spark.telefonica.com", const char* username = NULL,
            const char* password = NULL, Auth auth = AUTH_DETECT);

    /**
     * Used to issue a hardware reset of the modem
     */
    void reset(void);

    /**
     * powerOn Initialize the modem and SIM card
     * \param simpin a optional pin of the SIM card
     * \return true if successful, false otherwise
     */
    bool powerOn(const char* simpin = NULL);

    /** init (Attach) the MT to the GPRS service.
        \param status an optional struture to with device information
        \return true if successful, false otherwise
    */
    bool init(DevStatus* status = NULL);

    /** get the current device status
        \param strocture holding the device information.
    */
    void getDevStatus(DevStatus* dev) { memcpy(dev, &_dev, sizeof(DevStatus)); }

    const DevStatus* getDevStatus() { return &_dev; }

    /** register to the network
        \param status an optional structure to with network information
        \param timeout_ms -1 blocking, else non blocking timeout in ms
        \return true if successful and connected to network, false otherwise
    */
    bool registerNet(NetStatus* status = NULL, system_tick_t timeout_ms = 300000);

    /** check if the network is available
        \param status an optional structure to with network information
        \return true if successful and connected to network, false otherwise
    */
    bool checkNetStatus(NetStatus* status = NULL);

    /** checks the signal strength
        \param status an optional structure that will have current network information
               and updated RSSI and QUAL values.
        \return true if successful, false otherwise
    */
    bool getSignalStrength(NetStatus &status);

    /** Power off the MT, This function has to be called prior to
        switching off the supply.
        \return true if successfully, false otherwise
    */
    bool powerOff(void);

    /** Setup the PDP context
    */
    bool pdp(const char* apn = "spark.telefonica.com");

    // ----------------------------------------------------------------
    // Data Connection (GPRS)
    // ----------------------------------------------------------------

    /** register (Attach) the MT to the GPRS service.
        \param apn  the of the network provider e.g. "internet" or "apn.provider.com"
        \param username is the user name text string for the authentication phase
        \param password is the password text string for the authentication phase
        \param auth is the authentication mode (CHAP,PAP,NONE or DETECT)
        \return the ip that is assigned
    */
    MDM_IP join(const char* apn = "spark.telefonica.com", const char* username = NULL,
                       const char* password = NULL, Auth auth = AUTH_DETECT);

    /** deregister (detach) the MT from the GPRS service.
        \return true if successful, false otherwise
    */
    bool disconnect(void);

    bool reconnect(void);

    /** Detach the MT from the GPRS service.
        \return true if successful, false otherwise
    */
    bool detach(void);

    /** Translates a domain name to an IP address
        \param host the domain name to translate e.g. "u-blox.com"
        \return the IP if successful, 0 otherwise
    */
    MDM_IP gethostbyname(const char* host);

    /** get the current assigned IP address
        \return the ip that is assigned
    */
    MDM_IP getIpAddress(void) { return _ip; }

    // ----------------------------------------------------------------
    // Sockets
    // ----------------------------------------------------------------

    /** Create a socket for a ip protocol (and optionaly bind)
        \param ipproto the protocol (UDP or TCP)
        \param port in case of UDP, this optional port where it is bind
        \return the socket handle if successful or SOCKET_ERROR on failure
    */
    int socketSocket(IpProtocol ipproto, int port = -1);

    /** make a socket connection
        \param socket the socket handle
        \param host the domain name to connect e.g. "u-blox.com"
        \param port the port to connect
        \return true if successfully, false otherwise
    */
    bool socketConnect(int socket, const char* host, int port);

    bool socketConnect(int socket, const MDM_IP& ip, int port);

    /** make a socket connection
        \param socket the socket handle
        \return true if connected, false otherwise
    */
    bool socketIsConnected(int socket);

    /** Get the number of bytes pending for reading for this socket
        \param socket the socket handle
        \param timeout_ms -1 blocking, else non blocking timeout in ms
        \return 0 if successful or SOCKET_ERROR on failure
    */
    bool socketSetBlocking(int socket, system_tick_t timeout_ms);

    /** Write socket data
        \param socket the socket handle
        \param buf the buffer to write
        \param len the size of the buffer to write
        \return the size written or SOCKET_ERROR on failure
    */
    int socketSend(int socket, const char * buf, int len);

    /** Write socket data to a IP
        \param socket the socket handle
        \param ip the ip to send to
        \param port the port to send to
        \param buf the buffer to write
        \param len the size of the buffer to write
        \return the size written or SOCKET_ERROR on failure
    */
    int socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len);

    /** Get the number of bytes pending for reading for this socket
        \param socket the socket handle
        \return the number of bytes pending or SOCKET_ERROR on failure
    */
    int socketReadable(int socket);

    /** Read this socket
        \param socket the socket handle
        \param buf the buffer to read into
        \param len the size of the buffer to read into
        \return the number of bytes read or SOCKET_ERROR on failure
    */
    int socketRecv(int socket, char* buf, int len);

    /** Read from this socket
        \param socket the socket handle
        \param ip the ip of host where the data originates from
        \param port the port where the data originates from
        \param buf the buffer to read into
        \param len the size of the buffer to read into
        \return the number of bytes read or SOCKET_ERROR on failure
    */
    int socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len);

    /** Close a connectied socket (that was connected with #socketConnect)
        \param socket the socket handle
        \return true if successfully, false otherwise
    */
    bool socketClose(int socket);

    /** Free the socket (that was allocated before by #socketSocket)
        \param socket the socket handle
        \return true if successfully, false otherwise
    */
    bool socketFree(int socket);

    // ----------------------------------------------------------------
    // SMS Short Message Service
    // ----------------------------------------------------------------

    /** count the number of sms in the device and optionally return a
        list with indexes from the storage locations in the device.
        \param stat what type of messages you can use use
                    "REC UNREAD", "REC READ", "STO UNSENT", "STO SENT", "ALL"
        \param ix   list where to save the storage positions
        \param num  number of elements in the list
        \return the number of messages, this can be bigger than num, -1 on failure
    */
    int smsList(const char* stat = "ALL", int* ix = NULL, int num = 0);

    /** Read a Message from a storage position
        \param ix the storage position to read
        \param num the originator address (~16 chars)
        \param buf a buffer where to save the sm
        \param len the length of the sm
        \return true if successful, false otherwise
    */
    bool smsRead(int ix, char* num, char* buf, int len);

    /** Send a message to a recipient
        \param ix the storage position to delete
        \return true if successful, false otherwise
    */
    bool smsDelete(int ix);

    /** Send a message to a recipient
        \param num the phone number of the recipient
        \param buf the content of the message to sent
        \return true if successful, false otherwise
    */
    bool smsSend(const char* num, const char* buf);

    // ----------------------------------------------------------------
    // USSD Unstructured Supplementary Service Data
    // ----------------------------------------------------------------

    /** Read a Message from a storage position
        \param cmd the ussd command to send e.g "*#06#"
        \param buf a buffer where to save the reply
        \return true if successful, false otherwise
    */
    bool ussdCommand(const char* cmd, char* buf);

    // ----------------------------------------------------------------
    // FILE
    // ----------------------------------------------------------------

    /** Delete a file in the local file system
        \param filename the name of the file
        \return true if successful, false otherwise
    */
    bool delFile(const char* filename);

    /** Write some data to a file in the local file system
        \param filename the name of the file
        \param buf the data to write
        \param len the size of the data to write
        \return the number of bytes written
    */
    int writeFile(const char* filename, const char* buf, int len);

    /** REad a file from the local file system
        \param filename the name of the file
        \param buf a buffer to hold the data
        \param len the size to read
        \return the number of bytes read
    */
    int readFile(const char* filename, char* buf, int len);

    // ----------------------------------------------------------------
    // DEBUG/DUMP status to DEBUG output
    // ----------------------------------------------------------------

    /*! Set the debug level
        \param level -1 = OFF, 0 = ERROR, 1 = INFO(default), 2 = TRACE, 3 = ATCMD,TEST
        \return true if successful, false not possible
    */
    bool setDebug(int level);

    /** dump the device status to DEBUG output
    */
    void dumpDevStatus(DevStatus *status);

    /** dump the network status to DEBUG output
    */
    void dumpNetStatus(NetStatus *status);

    /** dump the ip address to DEBUG output
    */
    void dumpIp(MDM_IP ip);

    //----------------------------------------------------------------
    // Parsing
    //----------------------------------------------------------------

    /** Get a line from the physical interface. This function need
        to be implemented in a inherited class. Usually just calls
        #_getLine on the rx buffer pipe.

        \param buf the buffer to store it
        \param buf size of the buffer
        \return type and length if something was found,
                WAIT if not enough data is available
                NOT_FOUND if nothing was found
    */
    virtual int getLine(char* buf, int len) = 0;

    /* clear the pending input data
    */
    virtual void purge(void) = 0;

    /** Write data to the device
        \param buf the buffer to write
        \param buf size of the buffer to write
        \return bytes written
    */
    virtual int send(const char* buf, int len);

    /** Write formated date to the physical interface (printf style)
        \param fmt the format string
        \param .. variable arguments to be formated
        \return bytes written
    */
    int sendFormated(const char* format, ...);

    /** callback function for #waitFinalResp with void* as argument
        \param type the #getLine response
        \param buf the parsed line
        \param len the size of the parsed line
        \param param the optional argument passed to #waitFinalResp
        \return WAIT if processing should continue,
                any other value aborts #waitFinalResp and this retunr value retuned
    */
    typedef int (*_CALLBACKPTR)(int type, const char* buf, int len, void* param);

    /** Wait for a final respons
        \param cb the optional callback function
        \param param the optional callback function parameter
        \param timeout_ms the timeout to wait (See Estimated command
               response time of AT manual)
    */
    int waitFinalResp(_CALLBACKPTR cb = NULL,
                      void* param = NULL,
                      system_tick_t timeout_ms = 10000);

    /** template version of #waitFinalResp when using callbacks,
        This template will allow the compiler to do type cheking but
        internally symply casts the arguments and call the (void*)
        version of #waitFinalResp.
        \sa waitFinalResp
    */
    template<class T>
    inline int waitFinalResp(int (*cb)(int type, const char* buf, int len, T* param),
                    T* param,
                    system_tick_t timeout_ms = 10000)
    {
        return waitFinalResp((_CALLBACKPTR)cb, (void*)param, timeout_ms);
    }

protected:
    /** Write bytes to the physical interface. This function should be
        implemented in a inherited class.
        \param buf the buffer to write
        \param buf size of the buffer to write
        \return bytes written
    */
    virtual int _send(const void* buf, int len) = 0;

    /** Helper: Parse a line from the receiving buffered pipe
        \param pipe the receiving buffer pipe
        \param buf the parsed line
        \param len the size of the parsed line
        \return type and length if something was found,
                WAIT if not enough data is available
                NOT_FOUND if nothing was found
    */
    static int _getLine(Pipe<char>* pipe, char* buffer, int length);

    /** Helper: Parse a match from the pipe
        \param pipe the buffered pipe
        \param number of bytes to parse at maximum,
        \param sta the starting string, NULL if none
        \param end the terminating string, NULL if none
        \return size of parsed match
    */
    static int _parseMatch(Pipe<char>* pipe, int len, const char* sta, const char* end);

    /** Helper: Parse a match from the pipe
        \param pipe the buffered pipe
        \param number of bytes to parse at maximum,
        \param fmt the formating string (%d any number, %c any char of last %d len)
        \return size of parsed match
    */
    static int _parseFormated(Pipe<char>* pipe, int len, const char* fmt);

protected:
    // for rtos over riding by useing Rtos<MDMxx>
    //! override the lock in a rtos system
    virtual void lock(void)        { }
    //! override the unlock in a rtos system
    virtual void unlock(void)      { }
protected:
    // parsing callbacks for different AT commands and their parameter arguments
    static int _cbString(int type, const char* buf, int len, char* str);
    static int _cbInt(int type, const char* buf, int len, int* val);
    // device
    static int _cbATI(int type, const char* buf, int len, Dev* dev);
    static int _cbCPIN(int type, const char* buf, int len, Sim* sim);
    static int _cbCCID(int type, const char* buf, int len, char* ccid);
    // network
    static int _cbCSQ(int type, const char* buf, int len, NetStatus* status);
    static int _cbCOPS(int type, const char* buf, int len, NetStatus* status);
    static int _cbCNUM(int type, const char* buf, int len, char* num);
    static int _cbUACTIND(int type, const char* buf, int len, int* i);
    static int _cbUDOPN(int type, const char* buf, int len, char* mccmnc);
    // sockets
    static int _cbCMIP(int type, const char* buf, int len, MDM_IP* ip);
    static int _cbUPSND(int type, const char* buf, int len, int* act);
    static int _cbUPSND(int type, const char* buf, int len, MDM_IP* ip);
    static int _cbUDNSRN(int type, const char* buf, int len, MDM_IP* ip);
    static int _cbUSOCR(int type, const char* buf, int len, int* handle);
    static int _cbUSOCTL(int type, const char* buf, int len, int* handle);
    typedef struct { char* buf; int len; } USORDparam;
    static int _cbUSORD(int type, const char* buf, int len, USORDparam* param);
    typedef struct { char* buf; MDM_IP ip; int port; int len; } USORFparam;
    static int _cbUSORF(int type, const char* buf, int len, USORFparam* param);
    typedef struct { char* buf; char* num; } CMGRparam;
    static int _cbCUSD(int type, const char* buf, int len, char* resp);
    // sms
    typedef struct { int* ix; int num; } CMGLparam;
    static int _cbCMGL(int type, const char* buf, int len, CMGLparam* param);
    static int _cbCMGR(int type, const char* buf, int len, CMGRparam* param);
    // file
    typedef struct { const char* filename; char* buf; int sz; int len; } URDFILEparam;
    static int _cbUDELFILE(int type, const char* buf, int len, void*);
    static int _cbURDFILE(int type, const char* buf, int len, URDFILEparam* param);
    // variables
    DevStatus   _dev; //!< collected device information
    NetStatus   _net; //!< collected network information
    MDM_IP       _ip;  //!< assigned ip address
    // management struture for sockets
    typedef struct {
        int handle;
        system_tick_t timeout_ms;
        volatile bool connected;
        volatile int pending;
        volatile bool open;
    } SockCtrl;
    // LISA-C has 6 TCP and 6 UDP sockets
    // LISA-U and SARA-G have 7 sockets
    SockCtrl _sockets[7];
    int _findSocket(int handle = MDM_SOCKET_ERROR/* = CREATE*/);
    int _socketCloseHandleIfOpen(int socket);
    int _socketCloseUnusedHandles(void);
    int _socketSocket(int socket, IpProtocol ipproto, int port);
    bool _socketFree(int socket);
    bool _powerOn(void);
    static MDMParser* inst;
    bool _init;
    bool _pwr;
    bool _activated;
    bool _attached;
    bool _attached_urc;
    volatile bool _cancel_all_operations;
#ifdef MDM_DEBUG
    int _debugLevel;
    system_tick_t _debugTime;
    void _debugPrint(int level, const char* color, const char* format, ...);
#endif
};

// -----------------------------------------------------------------------

/** modem class which uses USART3
    as physical interface.
*/
class MDMElectronSerial : public ElectronSerialPipe, public MDMParser
{
public:
    /** Constructor
        \param rxSize the size of the serial rx buffer
        \param txSize the size of the serial tx buffer
    */
    MDMElectronSerial( int rxSize = 1024, int txSize = 1024 );
    //! Destructor
    virtual ~MDMElectronSerial(void);

    /** Get a line from the physical interface.
        \param buf the buffer to store it
        \param buf size of the buffer
        \return type and length if something was found,
                WAIT if not enough data is available
                NOT_FOUND if nothing was found
    */
    virtual int getLine(char* buffer, int length);

    /* clear the pending input data */
    virtual void purge(void)
    {
        while (readable())
            getc();
    }
protected:
    /** Write bytes to the physical interface.
        \param buf the buffer to write
        \param buf size of the buffer to write
        \return bytes written
    */
    virtual int _send(const void* buf, int len);
};

/* Instance of MDMElectronSerial for use in HAL_USART3_Handler */
extern MDMElectronSerial electronMDM;

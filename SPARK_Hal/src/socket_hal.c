#include "socket.h"


long socket_connect(long sd, const sockaddr *addr, long addrlen)
{
    return connect(sd, addr, addrlen);
}

void socket_reset_blocking_call() 
{
    //Work around to exit the blocking nature of socket calls
    tSLInformation.usEventOrDataReceived = 1;
    tSLInformation.usRxEventOpcode = 0;
    tSLInformation.usRxDataPending = 0;
}

int socket_receive(socket_handle_t sd, void* buffer, int len, system_tick_t _timeout)
{
  timeval timeout;
  _types_fd_set_cc3000 readSet;
  int bytes_received = 0;
  int num_fds_ready = 0;

  // reset the fd_set structure
  FD_ZERO(&readSet);
  FD_SET(sparkSocket, &readSet);

  // tell select to timeout after the minimum 5000 microseconds
  // defined in the SimpleLink API as SELECT_TIMEOUT_MIN_MICRO_SECONDS
  timeout.tv_sec = _timeout/1000;    
  timeout.tv_usec = (_timeout%1000)*1000;
  if (timeout.tv_sec==0 && timeout.tv_usec<5000)
      timeout.tv_usec = 5000;
  
  num_fds_ready = select(sparkSocket + 1, &readSet, NULL, NULL, &timeout);

  if (0 < num_fds_ready)
  {
    if (FD_ISSET(sparkSocket, &readSet))
    {
      // recv returns negative numbers on error
      bytes_received = recv(sparkSocket, buf, buflen, 0);
      DEBUG("bytes_received %d",bytes_received);
    }
  }
  else if (0 > num_fds_ready)
  {
    // error from select
    DEBUG("select Error %d",num_fds_ready);
    return num_fds_ready;
  }
  return bytes_received;
}

int32_t socket_create_nonblocking_server(socket_handle_t sock) {
    long optval = SOCK_ON;
    int32 retVal;
    (retVal=setsockopt(sock, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, &optval, sizeof(optval)) >= 0) &&
    (retVal=bind(sock, (sockaddr*)&tServerAddr, sizeof(tServerAddr)) >= 0) &&
    (retVal=listen(sock, 0));
    return retVal;
}

int32_t socket_receivefrom(socket_handle_t sock, void* buffer, uint32_t bufLen, sockaddr* addr, socklen_t addrsize) {
    _types_fd_set_cc3000 readSet;
    timeval timeout;

    FD_ZERO(&readSet);
    FD_SET(_sock, &readSet);

    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    int ret = -1;
    if (select(sock + 1, &readSet, NULL, NULL, &timeout) > 0)
    {
        if (FD_ISSET(sock, &readSet))
        {
            ret = socket_recvfrom(sock, buffer, bufLen, 0, addr, &addrsize);
        }
    }
    return ret;
}

int32_t socket_bind(socket_handle_t sock, uint16_t port) {
           
    sockaddr tUDPAddr;
    memset(&tUDPAddr, 0, sizeof(tUDPAddr));
    tUDPAddr.sa_family = AF_INET;
    tUDPAddr.sa_data[0] = (_port & 0xFF00) >> 8;
    tUDPAddr.sa_data[1] = (_port & 0x00FF);

    return bind(sock, &tUDPAddr, sizoef(tUDBAddr));
}

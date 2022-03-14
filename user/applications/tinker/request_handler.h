#pragma once

#include "system_control.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_json.h"

namespace particle {

// Handler for fqc requests
class RequestHandler {
public:
    RequestHandler();
    ~RequestHandler();

    int init();
    void destroy();

    void process(ctrl_request* ctrlReq);

    static RequestHandler* instance();

private:
    class Request;

    bool inited_;
    uint8_t tcpServer[4];
    int tcpPort;
    TCPClient tcpClient;

    int request(ctrl_request* ctrlReq);
    int request(Request* req);
    const auto passResponse(bool success);
    const auto tcpErrorResponse(int tcpError);
    const auto wifiScanNetworksResponse(Vector<WiFiAccessPoint> * accessPoints);
    void parseIpAndPort(spark::JSONValue parameters);
    int sendTCPMessage(const char * tx_data, char * rx_data_buffer, int rx_data_buffer_length, int response_poll_ms = 5000);
    
    int writeSerial1(Request* req);
    int useAntenna(Request* req);
    int bleScan(Request* req);
    int ioTest(Request* req);
    int wifiNetcat(Request* req);
    int wifiScanNetworks(Request* req);
};

} // namespace particle

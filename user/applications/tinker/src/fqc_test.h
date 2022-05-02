#pragma once

#include "system_control.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_json.h"

namespace particle {

using namespace spark;

// Handler for fqc requests
class FqcTest {
public:
    FqcTest();
    ~FqcTest();

    bool process(JSONValue test);
    static FqcTest* instance();

    char * reply();
    size_t replySize();

private:
    void initWriter();

    uint8_t tcpServer[4];
    int tcpPort;
    JSONBufferWriter writer; 
    char json_response_buffer[2048];
    TCPClient tcpClient;
    bool inited_;
    
    bool passResponse(bool success);
    bool tcpErrorResponse(int tcpError);

    void parseIpAndPort(JSONValue parameters);
    int sendTCPMessage(const char * tx_data, char * rx_data_buffer, int rx_data_buffer_length, int response_poll_ms = 5000);
    
    bool writeSerial1(JSONValue req);
    bool useAntenna(JSONValue req);
    bool bleScan(JSONValue req);
    bool ioTest(JSONValue req);
    bool wifiNetcat(JSONValue req);
    bool wifiScanNetworks(JSONValue req);
};

} // namespace particle

#pragma once

#ifdef ENABLE_FQC_FUNCTIONALITY

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

    uint32_t gnssFixQuality_ = 0;
    String gnssNmeaOutput_;

private:
    static const uint32_t GNSS_POLL_TIMEOUT_DEFAULT_MS = 30000;

    void initWriter();

    uint8_t tcpServer_[4];
    int tcpPort_;
    JSONBufferWriter writer_; 
    char json_response_buffer_[2048];
    TCPClient tcpClient_;
    bool inited_;

    Thread* gnssThread_;
    uint32_t gnssPollTimeoutMs_;
    std::atomic_bool gnssEnableSearch_;
    uint32_t gnssTimeToFix_;
    
    bool passResponse(bool success, String message = String(), int errorCode = 0);
    bool tcpErrorResponse(int tcpError);

    static void gnssLoop(void* arg);

    void parseIpAndPort(JSONValue parameters);
    int sendTCPMessage(const char * tx_data, char * rx_data_buffer, int rx_data_buffer_length, int response_poll_ms = 5000);
    
    bool writeSerial1(JSONValue req);
    bool useAntenna(JSONValue req);
    bool bleScan(JSONValue req);
    bool ioTest(JSONValue req);
    bool wifiNetcat(JSONValue req);
    bool wifiScanNetworks(JSONValue req);
    bool gnssTest(JSONValue req);
};

} // namespace particle

#endif // ENABLE_FQC_FUNCTIONALITY

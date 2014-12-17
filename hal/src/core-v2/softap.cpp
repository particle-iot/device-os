
#include <cstdlib>
#include <algorithm>
#include "wiced.h"
#ifdef HAL_SOFTAP_HTTPSERVER
#include "http_server.h"
#endif
#include "dns_redirect.h"
#include "wiced_security.h"
#include "jsmn.h"
#include "softap.h"

/**
 * Abstraction of an input stream.
 */
struct Reader {
    typedef int (*callback_t)(Reader* stream, uint8_t *buf, size_t count);

    callback_t callback;
    size_t bytes_left;
    void* state;

    inline int read(uint8_t* buf, size_t length) {
        int result = -1;
        if (bytes_left) {
            result = callback(this, buf, std::min(length, bytes_left));
            if (result>0)
                bytes_left -= result;
        }
        return result;
    }

    char* fetch() {
        char* buf = (char*)malloc(bytes_left);
        if (buf) {
            read((uint8_t*)buf, bytes_left);
            bytes_left = 0;
        }
        return buf;
    }
};

/**
 * Abstraction of an output stream.
 */
struct Writer {
    typedef void (*callback_t)(Writer* stream, const uint8_t *buf, size_t count);

    callback_t callback;
    void* state;

    inline void write(const uint8_t* buf, size_t length) {
        callback(this, buf, length);
    }

    void write(const char* s) {
        write((const uint8_t*)s, strlen(s));
    }

};

/**
 * A command that consumes data from a reader and produces a result to a writer.
 */
class Command {

public:
    /**
     * Executes this command.
     * @param reader    The data supplied to the command.
     * @param writer    The writer that receives the result of the command.
     * @return  A command response code. By convention 0 indicates success.
     */
    virtual int execute(Reader& reader, Writer& writer)=0;
};

/**
 * Base class for commands whose requests and responses are encoded using
 * protobuf. Subclasses must ensure the req_ and resp_ members are set to
 * the nanopb fields and data used to decode the request and encode
 * the response.
 */
class JSONCommand : public Command {

protected:

    jsmntok_t * json_tokenise(char *js)
    {
        jsmn_parser parser;
        jsmn_init(&parser);

        unsigned int n = 64;
        jsmntok_t* tokens = (jsmntok_t*)malloc(sizeof(jsmntok_t) * n);
        int ret = jsmn_parse(&parser, js, strlen(js), tokens, n);
        while (ret==JSMN_ERROR_NOMEM)
        {
            n = n * 2 + 1;
            tokens = (jsmntok_t*)realloc(tokens, sizeof(jsmntok_t) * n);
            ret = jsmn_parse(&parser, js, strlen(js), tokens, n);
        }
        return tokens;
    }

    bool json_token_streq(const char *js, jsmntok_t *t, const char *s)
    {
        return (strncmp(js + t->start, s, t->end - t->start) == 0
                && strlen(s) == (size_t) (t->end - t->start));
    }

    char* json_token_tostr(char *js, jsmntok_t *t)
    {
        js[t->end] = '\0';
        return js + t->start;
    }

    void write_result_code(Writer& writer, int result) {
        write_char(writer, '{');
        write_json_int(writer, "r", result);
        write_char(writer, '}');
    }

public:

    virtual int execute(Reader& reader, Writer& writer)
    {
        int result = parse_request(reader);
        if (!result) {
            result = process();
            produce_response(writer, result);
        }
        return result;
    }

    virtual int parse_request(Reader& reader) { return 0; }

    virtual void produce_response(Writer& writer, int result) {
        write_result_code(writer, result);
    }

    /**
     * The core processing for this command. Consumes the request object and produces
     * the response.
     * @return The command result code to return from execute().
     */
    virtual int process() { return 0; }

    void write_char(Writer& w, char c) {
        w.write((uint8_t*)&c, 1);
    }

    void write_quoted_string(Writer& out, const char* s) {
        write_char(out, '"');
        out.write(s);
        write_char(out, '"');
    }

    void write_json_string(Writer& out, const char* name, const char* value) {
        write_quoted_string(out, name);
        write_char(out, ':');
        write_quoted_string(out, value);
    }

    char* int_to_ascii(int val, char* buf, int i) {
        buf[--i] = 0;
        bool negative = val < 0;
        if (negative) {
            val = -val;
        }
        buf[--i] = (val % 10) + '0';
        val /= 10;
        for(; val && i ; val /= 10) {
            buf[--i] = (val % 10) + '0';
        }
        if (negative)
            buf[--i] = '-';
        return &buf[i];
    }

    void write_json_int(Writer& out, const char* name, int value) {
        char buf[20];
        write_quoted_string(out, name);
        write_char(out, ':');
        out.write(int_to_ascii(value, buf, 20));
    }
};

class VersionCommand : public JSONCommand {

public:
    void produce_response(Writer& out, int result) {
        out.write("{\"v\":1}");
    }
};

struct ScanEntry {
    char ssid[33];
    int32_t rssi;
    int32_t security;
    int32_t channel;
    int32_t max_data_rate;
};

class ScanAPCommand : public JSONCommand {
    wiced_queue_t queue_;

    // these can be quite large, so allocate on the heap
    ScanEntry read_;
    ScanEntry write_;

public:
    ScanAPCommand()
    {
    }

    /**
     * Initializes the scan queue, and start a scan using the WICED wifi scan api.
     *
     */
    int process() {
        int result = wiced_rtos_init_queue(&queue_, NULL, sizeof(ScanEntry), 10);
        if (!result)
            result = wiced_wifi_scan_networks(scan_handler, this);
        return result;
    }

    void produce_response(Writer& out, int result) {
        out.write("{\"scans\":[");
        bool first = true;
        ScanEntry& entry = read_;
        while (!wiced_rtos_pop_from_queue(&queue_, &entry, WICED_NEVER_TIMEOUT)) {
            if (!*entry.ssid)
                break;
            if (first)
                first = false;
            else
                write_char(out, ',');
            write_char(out, '{');
            write_json_string(out, "ssid", entry.ssid);
            write_char(out, ',');
            write_json_int(out, "rssi", entry.rssi);
            write_char(out, ',');
            write_json_int(out, "sec", entry.security);
            write_char(out, ',');
            write_json_int(out, "ch", entry.channel);
            write_char(out, ',');
            write_json_int(out, "mdr", entry.max_data_rate);
            write_char(out, '}');
        }
        out.write("]}");
        wiced_rtos_deinit_queue(&queue_);
    }

    static wiced_result_t scan_handler(wiced_scan_handler_result_t* malloced_scan_result)
    {
        ScanAPCommand& cmd = *(ScanAPCommand*)malloced_scan_result->user_data;
        malloc_transfer_to_curr_thread( malloced_scan_result );
        ScanEntry& entry = cmd.write_;
        if (malloced_scan_result->status == WICED_SCAN_INCOMPLETE)
        {
            wiced_scan_result_t& ap_details = malloced_scan_result->ap_details;
            memcpy(entry.ssid, ap_details.SSID.value, ap_details.SSID.length);
            entry.ssid[ap_details.SSID.length] = 0;
            entry.rssi = ap_details.signal_strength;
            entry.security = ap_details.security;
            entry.channel = ap_details.channel;
            entry.max_data_rate = ap_details.max_data_rate;
        }
        else
        {
            *entry.ssid = 0;    // end of stream sentinel
        }
        wiced_rtos_push_to_queue(&cmd.queue_, &entry, WICED_WAIT_FOREVER);
        free(malloced_scan_result);
        return WICED_SUCCESS;
    }
};

struct ConfigureAP {
    int32_t index;
    char ssid[33];
    char passcode[65];
    int32_t security;
    int32_t channel;
};

#define JSON_DEBUG(x)

/**
 * Connects to an access point using the given credentials and signals
 * soft-ap process complete.
 */
class ConfigureAPCommand : public JSONCommand {
    ConfigureAP configureAP;

    static const char* KEYS[5];
    static const int OFFSET[];
    static const jsmntype_t TYPE[];

    int save_credentials() {
        // Write received credentials into DCT
        struct
        {
            wiced_bool_t             device_configured;
            wiced_config_ap_entry_t  ap_entry;
        } config;

        WPRINT_APP_INFO( ( "saving AP credentials:\n" ) );
        WPRINT_APP_INFO( ( "index: %d\n", (int)configureAP.index ) );
        WPRINT_APP_INFO( ( "ssid: %s\n", configureAP.ssid ) );
        WPRINT_APP_INFO( ( "passcode: %s\n", configureAP.passcode ) );
        WPRINT_APP_INFO( ( "security: %d\n", (int)configureAP.security ) );
        WPRINT_APP_INFO( ( "channel: %d\n", (int)configureAP.channel ) );

        memset(&config, 0, sizeof(config));
        wiced_ap_info_t& details = config.ap_entry.details;
        memcpy(&details.SSID.value, configureAP.ssid, strlen(configureAP.ssid));
        details.SSID.length = strlen(configureAP.ssid);
        details.security = (wiced_security_t)configureAP.security;
        details.channel = configureAP.channel;
        memcpy(&config.ap_entry.security_key, configureAP.passcode, sizeof(config.ap_entry.security_key));
        config.ap_entry.security_key_length = strlen(configureAP.passcode);
        config.device_configured = WICED_TRUE;
        int result = wiced_dct_write( &config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(config) );
        return result;
    }

public:
    ConfigureAPCommand() {}


    int parse_request(Reader& reader) {

        char* js = reader.fetch();
        jsmntok_t *tokens = json_tokenise(js);

        enum parse_state { START, KEY, VALUE, SKIP, STOP };

        parse_state state = START;
        jsmntype_t expected_type = JSMN_OBJECT;
        void* data = NULL;
        int result = 0;
        int key = -1;
        memset(&configureAP, 0, sizeof(configureAP));

        for (size_t i = 0, j = 1; j > 0; i++, j--)
        {
            jsmntok_t *t = &tokens[i];
            if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
                j += t->size * 2;

            switch (state)
            {
                case START:
                    state = KEY;
                    break;

                case KEY:
                    state = VALUE;
                    data = NULL;
                    key = -1;
                    for (size_t i = 0; i < sizeof(KEYS)/sizeof(KEYS[0]); i++)
                    {
                        if (json_token_streq(js, t, KEYS[i]))
                        {
                            expected_type = TYPE[i];
                            data = ((uint8_t*)&configureAP)+OFFSET[i];
                            key = i;
                            JSON_DEBUG( ( "key: %s %d %d\n", KEYS[i], i, (int)expected_type ) );
                        }
                    }
                    if (key==-1)
                        JSON_DEBUG( ( "unknown key: %s\n", json_token_tostr(js, t) ) );
                    break;

                case VALUE:
                    if (t->type != expected_type) {
                        result = -1;
                        JSON_DEBUG( ( "type mismatch\n" ) );
                    }
                    else {
                        if (!data) {
                            JSON_DEBUG( ( "no data\n" ) );
                            break;
                        }

                        char *str = json_token_tostr(js, t);
                        if ((key==2 || key==1) && t->type==JSMN_STRING) {
                            strncpy((char*)data, str, key==1 ? sizeof(ConfigureAP::ssid) : sizeof(ConfigureAP::passcode));
                            JSON_DEBUG( ( "copied value %s\n", (char*)str ) );
                        }
                        else {
                            int32_t value = atoi(str);
                            *((int32_t*)data) = value;
                            JSON_DEBUG( ( "copied number %s (%d)\n", (char*)str, (int)value ) );
                        }
                        data = NULL;
                    }
                    state = KEY;
                    break;

                case STOP: // Just consume the tokens
                    break;

                default:
                    result = -1;
            }
        }
        free(js);
        return result;
    }

    /**
     * Write the requested AP details to DCT
     * @return
     */
    int process() {
        return save_credentials();
    }

};

const char* ConfigureAPCommand::KEYS[5] = {"idx","ssid","pwd","ch","sec" };
const int ConfigureAPCommand::OFFSET[] = {
                            offsetof(ConfigureAP, index),
                            offsetof(ConfigureAP, ssid),
                            offsetof(ConfigureAP, passcode),
                            offsetof(ConfigureAP, channel),
                            offsetof(ConfigureAP, security)
};
const jsmntype_t ConfigureAPCommand::TYPE[] =  { JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_PRIMITIVE, JSMN_PRIMITIVE };


class ConnectAPCommand : public JSONCommand {
    int index;
    wiced_semaphore_t* signal_complete_;
    void (*softap_complete_)();
    int responseCode;
public:
    ConnectAPCommand(wiced_semaphore_t* signal_complete, void (*softap_complete)()) {
        signal_complete_ = signal_complete;
        softap_complete_ = softap_complete;
    }

    // todo - parse index

    int process() {
        int result = 1;
        if (signal_complete_!=NULL) {
            wiced_rtos_set_semaphore(signal_complete_);
            result = 0;
        }
        return result;
    }

    void produce_response(Writer& writer, int result) {
        write_result_code(writer, result);
        if (softap_complete_)
            softap_complete_();
    }
};

class DeviceIDCommand : public JSONCommand {

    char device_id[25];

public:
    DeviceIDCommand() {}

    static inline char* concat_nibble(char* p, uint8_t nibble)
    {
        char hex_digit = nibble + 48;
        if (57 < hex_digit)
            hex_digit += 39;
        *p++ = hex_digit;
        return p;
    }

    static void bytes2hex(const uint8_t* buf, unsigned len, char* out)
    {
        unsigned i;
        for (i = 0; i < len; ++i)
        {
            concat_nibble(out, (buf[i] >> 4));
            out++;
            concat_nibble(out, (buf[i] & 0xF));
            out++;
        }
    }

    static void get_device_id(char buffer[25]) {
        bytes2hex((const uint8_t*)0x1FFF7A10, 12, buffer);
        buffer[24] = 0;
    }

    int process() {
        memset(device_id, 0, sizeof(device_id));
        get_device_id(device_id);
        return 0;
    }

    void produce_response(Writer& writer, int result) {
        write_char(writer, '{');
        write_json_string(writer, "id", device_id);
        write_char(writer, '}');
    }
};

struct ProvisionKeys {
    uint32_t salt;
    bool force;
};

class ProvisionKeysCommand : public JSONCommand
{
    ProvisionKeys request;

    rsa_context ctx;

    static int32_t rng_func(void* rng_state)
    {
        return( (int32_t)rand() );
    }

public:

    ProvisionKeysCommand() {}

    int process() {
        /*
        rsa_init(&ctx, RSA_PKCS_V15, 0, rng_func, NULL);
        int result = rsa_gen_key(&ctx, 1024, 65537);
        rsa_free(&ctx);
        return result;
         * */
        return 0;
        // todo - to create a DER format, encode the values from the rsa_context structure
        // according to the RSA DER format. https://polarssl.org/kb/cryptography/asn1-key-structures-in-der-and-pem
    }
};

struct AllSoftAPCommands {
    VersionCommand version;
    DeviceIDCommand deviceID;
    ScanAPCommand scanAP;
    ConfigureAPCommand configureAP;
    ConnectAPCommand connectAP;
    ProvisionKeysCommand provisionKeys;

    AllSoftAPCommands(wiced_semaphore_t* complete, void (*softap_complete)()) :
        connectAP(complete, softap_complete) {}
};

extern "C" wiced_ip_setting_t device_init_ip_settings;

/**
 * Manages the soft access point.
 */
class SoftAPController {
    wiced_semaphore_t complete;
    dns_redirector_t dns_redirector;

    void setup_soft_ap_credentials() {
        wiced_config_soft_ap_t expected;
        memset(&expected, 0, sizeof(expected));
        DeviceIDCommand::get_device_id((char*)expected.SSID.value);
        memcpy(expected.SSID.value, "photon-", 7);
        expected.SSID.length = strlen((char*)expected.SSID.value);
        expected.channel = 11;
        expected.details_valid = WICED_TRUE;

        wiced_config_soft_ap_t* soft_ap;
        wiced_result_t result = wiced_dct_read_lock( (void**) &soft_ap, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, OFFSETOF(platform_dct_wifi_config_t, soft_ap_settings), sizeof(wiced_config_soft_ap_t) );
        if (result == WICED_SUCCESS)
        {
            if (memcmp(&expected, soft_ap, sizeof(expected))) {
                wiced_dct_write(&expected, DCT_WIFI_CONFIG_SECTION, OFFSETOF(platform_dct_wifi_config_t, soft_ap_settings), sizeof(wiced_config_soft_ap_t));
            }
            wiced_dct_read_unlock( soft_ap, WICED_FALSE );
        }
    }

public:

    wiced_semaphore_t& complete_semaphore() {
        return complete;
    }

    void start() {
        wiced_rtos_init_semaphore(&complete);
        setup_soft_ap_credentials();
        wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &device_init_ip_settings );
        wiced_dns_redirector_start( &dns_redirector, WICED_AP_INTERFACE );
    }

    void waitForComplete() {
        wiced_rtos_get_semaphore(&complete, WICED_WAIT_FOREVER);
    }

    void signalComplete() {
        wiced_rtos_set_semaphore(&complete);
    }

    void stop() {
        signalComplete();

        /* Cleanup DNS server */
        wiced_dns_redirector_stop(&dns_redirector);

        /* Turn off AP */
        wiced_network_down( WICED_AP_INTERFACE );

        wiced_rtos_deinit_semaphore(&complete);
    }
};

class Dispatcher {
public:
    void start() {};
    void stop() {};
};

static void tcp_write(Writer* w, const uint8_t *buf, size_t count) {
    wiced_tcp_stream_t* tcp_stream = (wiced_tcp_stream_t*)w->state;
    wiced_tcp_stream_write(tcp_stream, buf, count);
}

static int tcp_read(Reader* r, uint8_t *buf, size_t count) {
    wiced_tcp_stream_t* tcp_stream = (wiced_tcp_stream_t*)r->state;
    int result = wiced_tcp_stream_read(tcp_stream, buf, count, WICED_NEVER_TIMEOUT);
    return result==WICED_SUCCESS ? count : 0;
}

static void tcp_stream_writer(Writer& w, wiced_tcp_stream_t* stream) {
    w.callback = tcp_write;
    w.state = stream;
}

static void tcp_stream_reader(Reader& r, wiced_tcp_stream_t* stream) {
    r.bytes_left = SIZE_MAX;
    r.callback = tcp_read;
    r.state = stream;
}

int read_from_buffer(Reader* r, uint8_t* target, size_t length) {
    memcpy(target, r->state, length);
    return length;
}

void reader_from_buffer(Reader* r, const uint8_t* buffer, size_t length) {
    r->bytes_left = length;
    r->callback = read_from_buffer;
    r->state = (void*)buffer;
}


#ifdef HAL_SOFTAP_HTTPSERVER
extern "C" wiced_http_page_t soft_ap_http_pages[];

class HTTPDispatcher {
    wiced_http_server_t server;

    wiced_http_page_t page[6];

    void setCommand(unsigned index, Command& cmd) {
        page[index].url_content.dynamic_data.generator = handle_command;
        page[index].url_content.dynamic_data.arg = &cmd;
    }

public:
    HTTPDispatcher(AllSoftAPCommands& commands) {
        memcpy(page, soft_ap_http_pages, sizeof(page));
        setCommand(2, commands.version);
        setCommand(3, commands.deviceID);
        setCommand(4, commands.scanAP);
        setCommand(5, commands.configureAP);
        setCommand(6, commands.connectAP);
        setCommand(7, commands.provisionKeys);
    }

    void start() {
        wiced_http_server_start(&server, 80, page, WICED_AP_INTERFACE );
    }

    void stop() {
        wiced_http_server_stop(&server);
    }

    static int32_t handle_command(const char* url, wiced_tcp_stream_t* stream, void* arg, wiced_http_message_body_t* http_data) {
        Command* cmd = (Command*)arg;
        Reader r;
        reader_from_buffer(&r, (uint8_t*)http_data->data, http_data->message_data_length);

        Writer w;
        tcp_stream_writer(w, stream);
        int result = cmd->execute(r, w);
        return result;
    }

};
#endif

/**
 * Parses a very simple protocol for sending command requests over a stream
 * and dispatches the commands to a AllSoftAPCommands instance.
 */
class SimpleProtocolDispatcher
{
    AllSoftAPCommands& commands_;

    char readChar(Reader& reader) {
        uint8_t c = 0;
        reader.read(&c, 1);
        return (char)c;
    }

    Command* commandForName(const char* name) {
        Command* cmd = NULL;
        if (name) {
            // todo - create an array of commands and names and do a scan!
            if (!strcmp("version", name))
                cmd = &commands_.version;
            else if (!strcmp("device-id", name))
                cmd = &commands_.deviceID;
            else if (!strcmp("scan-ap", name))
                cmd = &commands_.scanAP;
            else if (!strcmp("configure-ap", name))
                cmd = &commands_.configureAP;
            else if (!strcmp("connect-ap", name))
                cmd = &commands_.connectAP;
            else if (!strcmp("provision-keys", name))
                cmd = &commands_.provisionKeys;
        }
        return cmd;
    }

public:
    SimpleProtocolDispatcher(AllSoftAPCommands& commands) : commands_(commands) {}

    int handle(Reader& reader, Writer& writer) {
        char name[30];
        int requestLength = 0;
        int idx = 0;
        int result = -1;

        while (idx<30) {
            char c=readChar(reader);
            if (!c || c=='\n')
                break;
            name[idx++] = c;
        }
        name[idx] = 0;
        WPRINT_APP_INFO( ( "Fetched name '%s'\n", name ) );

        for (;;) {
            char c=readChar(reader);
            if (c=='\n')
                break;
            requestLength = requestLength * 10 + c-'0';
        }

        WPRINT_APP_INFO( ( "Request length %d\n", requestLength) );

        // todo - keep reading until a \n\n is encountered.
        bool seenNewline = true;
        for (;;) {
            char c=readChar(reader);
            if (c=='\n') {
                if (seenNewline)
                    break;
                else
                    seenNewline = true;
            }
            else
                seenNewline = false;
        }

        Command* cmd = commandForName(name);
        if (cmd) {
            WPRINT_APP_INFO( ( "invoking command %s\n", name ) );
            reader.bytes_left = requestLength;
            // allow for some protocol preamble before the payload
            writer.write("\n\n");
            result = cmd->execute(reader, writer);
            WPRINT_APP_INFO( ( "invoking command %s done\n", name ) );
        }
        else {
            WPRINT_APP_INFO( ( "Unknown command '%s'\n", name ) );
        }
        return result;
    }
};


class SerialDispatcher {
    uint8_t             rx_data[64];
    wiced_uart_config_t uart_config;
    wiced_ring_buffer_t rx_buffer;

public:
    SerialDispatcher(SimpleProtocolDispatcher& dispatcher) {}

    void start() {
#if 0
        uart_config =
        {
            .baud_rate    = 115200,
            .data_width   = DATA_WIDTH_8BIT,
            .parity       = NO_PARITY,
            .stop_bits    = STOP_BITS_1,
            .flow_control = FLOW_CONTROL_DISABLED,
        };

        ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );
        wiced_uart_init( STDIO_UART, &uart_config, &rx_buffer );
#endif
    }

    void stop() {

    }
};

/**
 * Used to transfer the callback info into the tcp server thread, to avoid blocking
 * the network thread.
 */
struct socket_message_t
{
    enum event
    {
        message,
        connect,
        disconnect,
    };

    wiced_tcp_socket_t*  socket;
    event event_type;
};


/**
 * A threaded TCP server that delegates to the SimpleProtocolDispatcher
 */
class TCPServerDispatcher
{
    SimpleProtocolDispatcher&   dispatcher_;
    wiced_interface_t           iface_;
    volatile bool               quit_;
    wiced_thread_t              thread_;
    wiced_queue_t               queue_;
    wiced_tcp_server_t          server_;

    /**
     * The callbacks from the tcp_server don't take a data argument, so we're forced to
     * resort to globals. No biggie - we don't need more than one tcp server.
     */
    static TCPServerDispatcher*  static_server;

    /**
     * Creates a stream for the client socket, wraps this in a reader/writer
     * and delegates to the dispatcher to handle the incoming command and produce the
     * result.
     * @param client
     */
    void handle_client(wiced_tcp_socket_t& client) {
        wiced_tcp_stream_t stream;
        if (!wiced_tcp_stream_init(&stream, &client)) {
            Reader r; Writer w;
            tcp_stream_writer(w, &stream);
            tcp_stream_reader(r, &stream);
            dispatcher_.handle(r, w);
            wiced_tcp_stream_flush(&stream);
            wiced_tcp_stream_deinit(&stream);
            wiced_tcp_server_disconnect_socket(&server_, &client);
        }
    }

    static wiced_result_t connect_callback(void* socket) {
        return post_event(socket_message_t::connect, socket);
    }

    static wiced_result_t disconnect_callback(void* socket) {
        return post_event(socket_message_t::disconnect, socket);
    }

    static wiced_result_t receive_callback(void* socket) {
        return post_event(socket_message_t::message, socket);
    }

    static wiced_result_t post_event(socket_message_t::event event_type, void* socket)
    {
        socket_message_t message;
        message.socket = (wiced_tcp_socket_t*)socket;
        message.event_type = event_type;
        if (static_server)
            wiced_rtos_push_to_queue(&static_server->queue_, &message, WICED_NO_WAIT);
        return WICED_SUCCESS;
    }

public:

    TCPServerDispatcher(SimpleProtocolDispatcher& dispatcher, wiced_interface_t iface)
        : dispatcher_(dispatcher), iface_(iface), quit_(false)
    {
        memset(&thread_, 0, sizeof(thread_));
    }

    void start()
    {
        static_server = this;
        if (wiced_rtos_init_queue(&queue_, NULL, sizeof(socket_message_t), 10))
            return;

        if (wiced_tcp_server_start(&server_, iface_, 5609, connect_callback, receive_callback, disconnect_callback))
            return;

        wiced_rtos_create_thread(&thread_, WICED_DEFAULT_LIBRARY_PRIORITY, "tcp server", tcp_server_thread, 1024*6, this);
    }

    void stop() {
        quit_ = true;
        wiced_tcp_server_stop(&server_);
        wiced_rtos_deinit_queue(&queue_);

        if ( wiced_rtos_is_current_thread( &thread_ ) != WICED_SUCCESS )
        {
            wiced_rtos_thread_force_awake( &thread_ );
            wiced_rtos_thread_join( &thread_);
            wiced_rtos_delete_thread( &thread_ );
        }
        static_server = NULL;
        WPRINT_APP_INFO( ( "TCP client done\n" ) );
    }

    void run() {

        while (!quit_)
        {
            socket_message_t event;
            if (wiced_rtos_pop_from_queue(&queue_, &event, WICED_NEVER_TIMEOUT))
                continue;

            switch(event.event_type)
            {
              case socket_message_t::disconnect:
                  wiced_tcp_server_disconnect_socket(&server_, event.socket);
                  break;

              case socket_message_t::connect:
                  wiced_tcp_server_accept(&server_, event.socket);
                  break;

              case socket_message_t::message:
                  handle_client(*event.socket);
              default:
                  break;
              }
        }
        WPRINT_APP_INFO( ( "TCP server exiting\n" ) );
    }

    /**
     * Function that is main entry point for the tcpserver thread. Just bounces the call
     * back to the run() method.
     */
    static void tcp_server_thread(uint32_t value) {
        TCPServerDispatcher* dispatcher = (TCPServerDispatcher*)value;
        dispatcher->run();
        WICED_END_OF_CURRENT_THREAD( );
    }
};

TCPServerDispatcher*  TCPServerDispatcher::static_server = NULL;

/**
 * The SoftAP setup application. This co-ordinates the various dispatchers and the soft AP.
 */
class SoftAPApplication
{
    SoftAPController softAP;
    AllSoftAPCommands commands;
#ifdef HAL_SOFTAP_HTTPSERVER    
    HTTPDispatcher http;
#endif    
    SimpleProtocolDispatcher simpleProtocol;
    TCPServerDispatcher tcpServer;
    SerialDispatcher serial;

public:
    SoftAPApplication(void (*complete_callback)()) :
            commands(&softAP.complete_semaphore(), complete_callback),
#ifdef HAL_SOFTAP_HTTPSERVER                
            http(commands),
#endif                
            simpleProtocol(commands),
            tcpServer(simpleProtocol, WICED_AP_INTERFACE),
            serial(simpleProtocol)
    {
        softAP.start();
        serial.start();
        tcpServer.start();
#ifdef HAL_SOFTAP_HTTPSERVER
        http.start();
#endif
    }

    ~SoftAPApplication()
    {
#ifdef HAL_SOFTAP_HTTPSERVER
        http.stop();
#endif        
        tcpServer.stop();
        serial.stop();
        softAP.stop();
    }
};

softap_handle softap_start(softap_config* config) {
    SoftAPApplication* app = new SoftAPApplication(config->softap_complete);
    return app;
}

void softap_stop(softap_handle handle) {
    SoftAPApplication* app = (SoftAPApplication*)handle;
    delete app;
}

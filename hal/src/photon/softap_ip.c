
#include "wiced.h"
#include "http_server.h"

// This has to be compiled as C since it doesn't compile as C++ due to non-trivial assignment

const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

const char SOFT_AP_MSG[] = "Soft AP Setup";
START_OF_HTTP_PAGE_DATABASE(soft_ap_http_pages)
    ROOT_HTTP_PAGE_REDIRECT("/hello"),
    { "/hello", "text/plain", WICED_STATIC_URL_CONTENT, .url_content.static_data = {SOFT_AP_MSG, sizeof(SOFT_AP_MSG) }},
    { "/version", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/device-id", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/scan-ap", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/configure-ap", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/connect-ap", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/public-key", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/set", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
END_OF_HTTP_PAGE_DATABASE();



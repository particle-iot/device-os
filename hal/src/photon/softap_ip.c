
#include "wiced.h"
#include "http_server.h"
#include "softap_pages.h"

// This has to be compiled as C since it doesn't compile as C++ due to non-trivial assignment

const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

const char SOFT_AP_MSG[] = "Soft AP Setup";
START_OF_HTTP_PAGE_DATABASE(soft_ap_http_pages)
    ROOT_HTTP_PAGE_REDIRECT("/index"),
    { "/hello", "text/html", WICED_STATIC_URL_CONTENT, .url_content.static_data = {SOFT_AP_MSG, sizeof(SOFT_AP_MSG) }},
    { "/version", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/device-id", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/scan-ap", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/configure-ap", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/connect-ap", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/public-key", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/set", "application/octet-stream", WICED_RAW_DYNAMIC_URL_CONTENT },
    { "/index.html", "text/html", WICED_STATIC_URL_CONTENT, .url_content.static_data = {index_html, sizeof(index_html) - 1 }},
    { "/rsa-utils/rsa.js", "application/javascript", WICED_STATIC_URL_CONTENT, .url_content.static_data = {rsa_js, sizeof(rsa_js) - 1 }},
    { "/style.css", "text/css", WICED_STATIC_URL_CONTENT, .url_content.static_data = {style_css, sizeof(style_css) - 1 }},
    { "/rsa-utils/rng.js", "application/javascript", WICED_STATIC_URL_CONTENT, .url_content.static_data = {rng_js, sizeof(rng_js) - 1 }},
    { "/rsa-utils/jsbn_2.js", "application/javascript", WICED_STATIC_URL_CONTENT, .url_content.static_data = {jsbn_2_js, sizeof(jsbn_2_js) - 1 }},
    { "/rsa-utils/jsbn_1.js", "application/javascript", WICED_STATIC_URL_CONTENT, .url_content.static_data = {jsbn_1_js, sizeof(jsbn_1_js) - 1 }},
    { "/script.js", "application/javascript", WICED_STATIC_URL_CONTENT, .url_content.static_data = {script_js, sizeof(script_js) - 1 }},
    { "/rsa-utils/prng4.js", "application/javascript", WICED_STATIC_URL_CONTENT, .url_content.static_data = {prng4_js, sizeof(prng4_js) - 1 }},
END_OF_HTTP_PAGE_DATABASE();



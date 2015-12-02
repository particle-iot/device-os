
#include "wiced.h"

wiced_result_t wiced_tcp_start_tls( wiced_tcp_socket_t* socket, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_close_notify( wiced_tcp_socket_t* socket )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_deinit_context( wiced_tls_simple_context_t* tls_context )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_calculate_overhead( wiced_tls_context_t* context, uint16_t content_length, uint16_t* header, uint16_t* footer )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_encrypt_packet( wiced_tls_context_t* context, wiced_packet_t* packet )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_receive_packet( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    return WICED_SUCCESS;
}

wiced_result_t wiced_tls_reset_context( wiced_tls_simple_context_t* tls_context )
{
    return WICED_SUCCESS;
}

void host_network_process_eapol_data( /*@only@*/ wiced_buffer_t buffer, wwd_interface_t interface )
{
}

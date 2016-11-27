#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t size;
    void (*notify_connected)(); // HAL_NET_notify_connected()
    void (*notify_disconnected)(); // HAL_NET_notify_disconnected()
    void (*notify_dhcp)(bool dhcp); // HAL_NET_notify_dhcp()
    void (*notify_can_shutdown)(); // HAL_NET_notify_can_shutdown()
} HAL_NET_Callbacks;

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInuS);

/**
 * Sets notification callbacks. This function is used when HAL implementation cannot be linked
 * with HAL_NET_notify_*() functions directly.
 */
void HAL_NET_SetCallbacks(const HAL_NET_Callbacks* callbacks, void* reserved);

/**
 * Notification that the wifi network has been connected to.
 */
void HAL_NET_notify_connected();
void HAL_NET_notify_disconnected();

/**
 * Notification that an IP address has been received via DHCP.
 * todo - what with the case of static IP config?
 */
void HAL_NET_notify_dhcp(bool dhcp);

void HAL_NET_notify_can_shutdown();


#ifdef __cplusplus
}
#endif

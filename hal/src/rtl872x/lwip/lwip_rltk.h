#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "wifi_constants.h"

struct eth_drv_sg {
    unsigned int	buf;
    unsigned int 	len;
};

#define MAX_ETH_DRV_SG	32
#define MAX_ETH_MSG	1500

extern struct sk_buff * rltk_wlan_get_recv_skb(int idx);
extern struct sk_buff * rltk_wlan_alloc_skb(unsigned int total_len);
extern void rltk_wlan_send_skb(int idx, struct sk_buff *skb);
extern int netif_get_idx(struct netif *pnetif);
extern unsigned char rltk_wlan_check_isup(int idx);
extern void rltk_wlan_tx_inc(int idx);
extern void rltk_wlan_tx_dec(int idx);
extern unsigned char rltk_wlan_running(unsigned char idx); // interface is up. 0: interface is down

void netif_pre_sleep_processing(void);
void rltk_wlan_recv(int idx, struct eth_drv_sg *sg_list, int sg_len);
int rltk_wlan_send(int idx, struct eth_drv_sg *sg_list, int sg_len, int total_len);
unsigned char is_promisc_enabled(void);
void promisc_deinit(void *padapter);
int promisc_recv_func(void *padapter, void *rframe);
void eap_autoreconnect_hdl(uint8_t method_id);
int get_eap_method(void);
int get_eap_phase(void);
void netif_post_sleep_processing(void);
int netif_get_idx(struct netif* pnetif);
int promisc_set(rtw_rcr_level_t enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used);

#ifdef __cplusplus
}
#endif

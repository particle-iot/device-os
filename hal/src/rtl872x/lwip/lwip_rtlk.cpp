#include "lwip_rltk.h"
#include "logging.h"
LOG_SOURCE_CATEGORY("net.lwip_rltk");

extern "C" {

#include "skbuff.h"

// Some Realtek SDK-specific variables/functions in order not to compile in a bunch of unused sources
rtw_mode_t wifi_mode = RTW_MODE_STA;

extern void save_and_cli(void);
extern void restore_flags(void);
// uint8_t roaming_awake_rssi_range = 0;
// uint8_t roaming_normal_rssi_range = 0;
// unsigned char roaming_type_flag = 0;

}

unsigned char is_promisc_enabled(void) {
    LOG(INFO, "is_promisc_enabled");
    return 0;
}

int promisc_set(rtw_rcr_level_t enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used) {
    LOG(INFO, "promisc_set TODO");
    return -1;
}

void promisc_deinit(void *padapter) {
    LOG(INFO, "promisc_deinit TODO");
}

int promisc_recv_func(void *padapter, void *rframe) {
    LOG(INFO, "promisc_recv_func TODO");
    return 0;
}

void eap_autoreconnect_hdl(uint8_t method_id) {
    LOG(INFO, "eap_autoreconnect_hdl");
}

int get_eap_phase(void) {
    LOG(INFO, "get_eap_phase");
    return 0;
}

int get_eap_method(void) {
    LOG(INFO, "get_eap_method");
    return 0;
}

void netif_post_sleep_processing(void) {
    LOG(INFO, "netif_post_sleep_processing TODO");
}

void netif_pre_sleep_processing(void) {
    LOG(INFO, "netif_pre_sleep_processing TODO");
}

int netif_get_idx(struct netif* pnetif) {
    (void)pnetif;
    // FIXME: 0 for STA, 1 for AP
    return 0;
}

/**
 *      rltk_wlan_recv - indicate packets to LWIP. Called by ethernetif_recv().
 *      @idx: netif index
 *      @sg_list: data buffer list
 *      @sg_len: size of each data buffer
 *
 *      Return Value: None
 */
void rltk_wlan_recv(int idx, struct eth_drv_sg *sg_list, int sg_len)
{
    struct eth_drv_sg *last_sg;
    struct sk_buff *skb;

    // LOG(TRACE, "%s is called", __FUNCTION__);
    if(idx == -1){
        LOG(ERROR, "skb is NULL");
        return;
    }
    skb = rltk_wlan_get_recv_skb(idx);
    if (skb == nullptr) {
        LOG(ERROR, "No pending rx skb");
    }

    for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
        if (sg_list->buf != 0) {
            memcpy((void *)(sg_list->buf), skb->data, sg_list->len);
            skb_pull(skb, sg_list->len);
        }
    }
}

/**
 *      rltk_wlan_send - send IP packets to WLAN. Called by low_level_output().
 *      @idx: netif index
 *      @sg_list: data buffer list
 *      @sg_len: size of each data buffer
 *      @total_len: total data len
 *
 *      Return Value: None
 */
int rltk_wlan_send(int idx, struct eth_drv_sg *sg_list, int sg_len, int total_len)
{
    struct eth_drv_sg *last_sg;
    struct sk_buff *skb = NULL;
    int ret = 0;

    // WIFI_MONITOR_TIMER_START(wifi_time_test.wlan_send_time);
    if(idx == -1){
        LOG(ERROR, "netif is DOWN");
        return -1;
    }
    // LOG(TRACE, "%s is called idx=%d", __FUNCTION__, idx);

    save_and_cli();
    rltk_wlan_tx_inc(idx);
    restore_flags();

    // WIFI_MONITOR_TIMER_START(wifi_time_test.wlan_send_time1);
    skb = rltk_wlan_alloc_skb(total_len);
    // WIFI_MONITOR_TIMER_END(wifi_time_test.wlan_send_time1, total_len);
    if (skb == NULL) {
        LOG(ERROR, "rltk_wlan_alloc_skb() for data len=%d failed!", total_len);
        ret = -1;
        goto exit;
    }
    // WIFI_MONITOR_TIMER_START(wifi_time_test.wlan_send_time2);
    for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
        memcpy(skb->tail, (void *)(sg_list->buf), sg_list->len);
        skb_put(skb,  sg_list->len);
        // LOG(INFO, "SKB put: %d", sg_list->len);
    }
    // WIFI_MONITOR_TIMER_END(wifi_time_test.wlan_send_time2, total_len);

    // WIFI_MONITOR_TIMER_START(wifi_time_test.wlan_send_skb_time);
    rltk_wlan_send_skb(idx, skb);
    // WIFI_MONITOR_TIMER_END(wifi_time_test.wlan_send_skb_time, total_len);
    // WIFI_MONITOR_TIMER_END(wifi_time_test.wlan_send_time, total_len);

exit:
    save_and_cli();
    rltk_wlan_tx_dec(idx);
    restore_flags();
    return ret;
}

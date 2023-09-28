#ifndef _APP_HFP_H_
#define _APP_HFP_H_

/**********************************************************************
 *
 * echo erase part
 *
 ***********************************************************************/
typedef  struct _app_echo_cfg_s
{
    uint8_t   used;
    uint8_t   hfp_a_s;
    uint8_t   hfp_a_n;
    uint16_t  hfp_noise_interval;
    uint16_t  hold_time;
    uint8_t   hfp_a_cspn_tx;
    uint16_t   st;
    uint16_t   ft;
    uint8_t   hfp_a_cptr;
    uint8_t   hfp_hyt_rx;
    uint8_t   hfp_hyt_tx;
}__PACKED_POST__ app_echo_cfg_t;

enum {
    ECHO_HFP_ICOTL_ENABLE = 0,
    ECHO_HFP_IOCTL_AS,
    ECHO_HFP_IOCTL_AN,
    ECHO_HFP_IOCTL_NOISEINTER,
    ECHO_HFP_IOCTL_HOLD,
    ECHO_HFP_IOCTL_CSPNTX,
    ECHO_HFP_IOCTL_CSPNRX,
    ECHO_HFP_IOCTL_CSPNIDLE,
    ECHO_HFP_IOCTL_CPTR,
    ECHO_HFP_IOCTL_HYTRX,
    ECHO_HFP_IOCTL_HYTTX,
    ECHO_HFP_ADJUST_DAC_GAIN,
    ECHO_HFP_ADJUST_MIC_GAIN,
    ECHO_HFP_DEBUG_ENABLE
};

void app_hfp_echo_erase_init(void);
void app_hfp_echo_fill_rxbuff(uint8_t *buff, uint16_t len);
void app_hfp_echo_fill_txbuff(uint8_t *buff, uint16_t len);
void app_hfp_echo_erase(void);
int app_hfp_echo_ioctl(uint8_t oper, uint32_t arg);
void app_hfp_echo_cfg_ptr_set(app_echo_cfg_t * ptr);

#ifdef BEKEN_DEBUG
void app_hfp_debug_show(void);
#endif

void app_hfp_agc_init(uint8_t agc_max);

#endif

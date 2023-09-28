/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */

#ifndef _BT_SBC_H_
#define _BT_SBC_H_

#include <jos.h>
#include <jos/jos_mbuf.h>

#define SBC_BUF_LENGTH          1024 /* encoded size */
#define PCM_BUF_LENGTH          512  /* decoded size */

result_t bt_sbc_init(void);
void bt_sbc_uninit(void);

typedef void (*pcm_input_cb_t)(void *pcm_ctx, 
                                  void *pcm_buf,
                                  uint32_t pcm_len);

result_t bt_sbc_input(struct mbuf *m,  uint8_t padding,
                          pcm_input_cb_t pcm_input, void *pcm_ctx);
#endif

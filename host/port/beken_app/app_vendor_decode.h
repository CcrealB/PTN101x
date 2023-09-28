#ifndef _APP_VENDOR_H_
#define _APP_VENDOR_H_

#include "sys_config.h"
#include "jos_mbuf.h"

//#define A2DP_VENDOR_DECODE

//#define VENDOR_DECODE_TEST // for test vendor decode flow by SBC source


// for a2dp vendor decode edit code
#ifdef A2DP_VENDOR_DECODE
void vendor_target_deinit( void );
void vendor_mem_free( void );
void RAM_CODE vendor_fill_encode_buffer( struct mbuf *m, int len, int frames );
void vendor_do_decode( void );
#endif //A2DP_VENDOR_DECODE

#ifdef __cplusplus
}
#endif

#endif /* _APP_VENDOR_H */

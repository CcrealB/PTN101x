#include <jos.h>
#include <bt_a2dp_types.h>
#include "bt_a2dp_dev.h"

#ifdef A2DP_VENDOR_DECODE
void bt_a2dp_set_configure_cb_vendor_param(const bt_a2dp_codec_t *codec, uint32_t *bps, uint32_t *freq, uint32_t *channel);
result_t bt_a2dp_sink_get_default_vendor_function(bt_a2dp_vendor_t *ladc);
result_t bt_a2dp_vendor_stream_input(stream_softc_t *st, struct mbuf *m);

void bt_vendor_init(void);
void bt_vendor_uninit(void);
#endif //A2DP_VENDOR_DECODE

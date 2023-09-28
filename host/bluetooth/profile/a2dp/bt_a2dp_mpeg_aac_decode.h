#include <jos.h>
#include <bt_a2dp_types.h>
#include "config.h"
#include "bt_a2dp_dev.h"

#ifdef A2DP_MPEG_AAC_DECODE

#define AAC_SAMPLES_PER_FRAME           (1024)
#define AAC_FRAME_BUFFER_MAX_SIZE       (1024)
#define AAC_FRAME_BUFFER_MAX_NODE       (20)

typedef struct _RingBufferNodeContext
{
    uint8_t* address;   /**< memory base address            */
    uint32_t node_len;  /**< length per node buffer in byte */
    uint32_t node_num;  /**< total nodes                    */
    uint32_t rp;        /**< write point in node            */
    uint32_t wp;        /**< read point in node             */
}RingBufferNodeContext;

extern RingBufferNodeContext aac_frame_nodes;

uint32_t ring_buffer_node_get_fill_nodes(RingBufferNodeContext* rbn);

void bt_a2dp_aac_node_buffer_init(void);
void bt_a2dp_set_configure_cb_aac_param(const bt_a2dp_codec_t *codec, uint32_t *bps, uint32_t *freq, uint32_t *channel);
result_t bt_a2dp_sink_get_default_aac_function(bt_a2dp_mpeg_aac_t *aac);
result_t bt_a2dp_aac_stream_start(uint32_t sample_rate, uint32_t channels);
result_t bt_a2dp_aac_stream_sync(uint32_t type);
result_t bt_a2dp_aac_stream_suspend(void);
result_t bt_a2dp_aac_stream_input(stream_softc_t *st, struct mbuf *m);
void bt_a2dp_aac_stream_decode(void);

#endif

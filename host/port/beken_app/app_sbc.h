#ifndef _APP_SBC_H_
#define _APP_SBC_H_

#include "driver_beken_includes.h"
#include "jheaders.h"
#include "sbc_encoder.h"
#include "sbc_decoder.h"
#include "app_vendor_decode.h" // for a2dp vendor decode

#define SBC_ERR_FRM_PROC			0
#define SBC_FRAME_LENGTH_MAX        120 /* shall not be changed. */
#ifdef CONFIG_APP_AEC
#define SBC_FIRST_TRY_TIME          120
#define SBC_MEM_POOL_MAX_NODE       153
#define SBC_ENCODE_BUFFER_LEN       (SBC_MEM_POOL_MAX_NODE * SBC_FRAME_LENGTH_MAX)
#else
#define SBC_FIRST_TRY_TIME          96
#define SBC_MEM_POOL_MAX_NODE       128
#define SBC_ENCODE_BUFFER_LEN       (SBC_MEM_POOL_MAX_NODE * SBC_FRAME_LENGTH_MAX)
#if A2DP_ROLE_SOURCE_CODE            
#else
    uint8_t sbc_encode_buff[SBC_ENCODE_BUFFER_LEN];
#endif
#endif

#define SBC_GAME_MODE_FIRST_TRY    50 //game mode sbc nodes level high threshold
#define SBC_GAME_MODE_LEVEL_LOW    30 //game mode sbc nodes level low threshold

#define AUD_FLAG_IS_MAC_WIN_BOOK  (1UL<<0)
#define AUD_FLAG_GAME_MODE        (1UL<<1)

extern void set_aud_mode_flag(uint32_t flag);
extern void unset_aud_mode_flag(uint32_t flag);
extern uint32_t get_aud_mode_flag(uint32_t flag);

#define SBC_DEBUG

typedef struct _sbc_mem_node_s
{
    uint8_t *data_ptr;
#ifdef A2DP_VENDOR_DECODE // for a2dp vendor decode
    uint16_t data_len;
#endif
    struct _sbc_mem_node_s *next;
}sbc_mem_node_t;


typedef struct _sbc_mem_list_s
{
    sbc_mem_node_t *head;
    sbc_mem_node_t *tail;
}sbc_mem_list_t;

typedef struct _sbc_mem_pool_s
{
    sbc_mem_list_t freelist;
    sbc_mem_list_t uselist;
    int  node_num;
}sbc_mem_pool_t;

typedef struct _app_sbc_s
{
	uint8_t *             sbc_encode_buffer;
    uint8_t *             sbc_output;

    sbc_mem_pool_t      sbc_mem_pool;

    uint32_t              freq;
    uint16_t              timer_cnt;
    uint8_t               sbc_target_initial;
    uint8_t               sbc_first_try;
    uint8_t               sbc_clk_mode;
    uint8_t               use_count;
    uint16_t				decode_cnt;
    uint16_t              sbc_ecout;    
#if A2DP_ROLE_SOURCE_CODE
    int8_t    src_pkt_num;

    uint32_t	in_frame_len;
    uint32_t	out_frame_len;
    uint16_t	payload_len;
    uint16_t	frames_per_packet;
    uint16_t	sequence;
    uint32_t	samples;
    uint8_t	*sbc_output_buffer;
#endif
}app_sbc_t;

#if A2DP_ROLE_SOURCE_CODE
typedef struct rtp_header
{
	unsigned cc:4;
	unsigned x:1;
	unsigned p:1;
	unsigned v:2;

	unsigned pt:7;
	unsigned m:1;

	uint16_t sequence_number;
	uint32_t timestamp;
	uint32_t ssrc;
}rtp_header_t;

typedef struct media_packet_sbc
{
	rtp_header_t hdr;
	uint8_t  frame_count;
	uint8_t  data[1];
}media_packet_sbc_t;

#endif

extern uint8_t get_flag_sbc_buffer_play(void);
extern void set_flag_sbc_buffer_play(uint8_t flag);

/******************
 * beken defination
 */
void sbc_target_init(void);
void sbc_mem_free(void);
void sbc_target_deinit( void );
void sbc_fill_encode_buffer( struct mbuf *m, int len, int frames );
void sbc_do_decode( void );
void sbc_stream_start_init(  uint32_t freq  );
void sbc_mem_pool_init( int framelen );
void sbc_mem_pool_deinit( void );
void sbc_target_init_malloc_buff( void );//add by zjw
void sbc_target_deinit_jfree_buff(void);
void sbc_init_adjust_param(void);
void sbc_discard_uselist_node(void);
uint32_t sbc_buf_get_use_count(void);
void sbc_buf_increase_use_count(void);
void sbc_buf_decrease_use_count(void);
uint16_t sbc_buf_get_node_count(void);
uint8_t sbc_node_buff_monitor(void);
void sbc_dac_clk_tracking(uint32_t step);
void *sbc_get_sbc_ptr(void);
void sbc_first_play_init(uint8_t enable,uint8_t cnt);
#ifdef CONFIG_OPTIMIZING_SBC
#define opt_sbc_volume_index_max 9
#define opt_sbc_volume_steps (opt_sbc_volume_index_max+1)
#define aud_rssi_sum_num (128*200)
#define aud_rssi_scale_max (150)
#define aud_rssi_scale_min (50)

typedef   struct __app_detect_aud_rssi_s
{
	uint16_t k;
	uint16_t avg;
	uint32_t sum;
	uint8_t sbc_gain_index;
	int8_t ang_adj;
	int8_t adjust_cnt;
}__PACKED_POST__ app_detect_aud_rssi_t;
#endif

#ifdef CONFIG_SBC_PROMPT
extern SbcDecoderContext* wav_sbc_decoder;
void app_wave_file_sbc_fill(void);
#endif

void optimize_sbc_init(void);
int8_t optimize_sbc_get_volume_adj(void);

extern SbcEncoderContext* sbc_encoder;
extern SbcDecoderContext* sbc_decoder;

#ifdef __cplusplus
}
#endif

#endif /* __SBC_H */

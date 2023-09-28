
#include <string.h>
#include "app_vendor_decode.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

#ifdef A2DP_VENDOR_DECODE

typedef struct _app_vendor_s
{
	uint8_t	 	vendor_target_initial;
	uint8_t		*output;

	uint16_t	frame_num;

}app_vendor_t;

typedef struct _vendor_dec_info_t
{
	uint32_t	dec_result;
	void		*out_buf;
} vendor_dec_info_t;

static app_vendor_t  app_vendor={0};
static uint8_t	vendor_decode_init = 0;

volatile uint8_t flag_vendor_enable = 0;
volatile uint8_t flag_vendor_buffer_play = 0;


//** edit function **//
void vendor_target_deinit( void )
{
	app_vendor.vendor_target_initial=0;

}

void vendor_mem_free( void )
{
	app_vendor.vendor_target_initial=0;

}

extern uint32_t _sbcmem_begin;
void vendor_target_init_malloc_buff(void)
{
	app_vendor.output = (uint8_t *)((uint32_t)&_sbcmem_begin);

}

// refer sbc_fill_encode_buffer() in app_sbc.c
void RAM_CODE vendor_fill_encode_buffer( struct mbuf *m, int len, int frames )
{

	if(app_vendor.vendor_target_initial==0) {
		//*** for a2dp vendor decode edit code #04 ***//
		// allocate decode input buffer & output buffer
		vendor_target_init_malloc_buff();
		app_vendor.vendor_target_initial=1;




	}

	//*** for a2dp vendor decode edit code #05 ***//
	// regroup mbuf to decoder buffer
    // jos_mbuf.h support mbuf function
	// can use m_copydata() copy mbuf data to input buffer
    // refer sbc_fill_encode_buffer() in app_sbc.c


	app_vendor.frame_num += frames;

}

/*void vendor_decode(vendor_dec_info_t *dec_info)
{


}*/

void vendor_decode( vendor_dec_info_t *dec_info )
{

	// check DSP decoder initial
    if(vendor_decode_init == 0) {
    	//*** for a2dp vendor decode edit code #09 ***//
		// initial decoder
	#ifdef VENDOR_DECODE_TEST
    	os_printf("%s, initial decoder\r\n", __func__);
	#endif


    	// if initial decoder is finish
    	vendor_decode_init = 1;
    }

    if(vendor_decode_init == 1) {

    	//*** for a2dp vendor decode edit code #10 ***//
    	// start to decode








    	// check software mute
		if(app_bt_flag2_get(APP_FLAG2_SW_MUTE))
		{
			// clear decoder output data to mute
			memset(app_vendor.output,0,512);
		}


		if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
		{
			if(get_bt_dev_priv_work_flag())
			{
				set_aud_fade_in_out_state(AUD_FADE_OUT);
			}
			else if(app_bt_flag1_get(APP_FLAG_HFP_OUTGOING) || app_bt_flag2_get(APP_FLAG2_HFP_INCOMING))
			{
				// clear decoder output data to mute
				memset(app_vendor.output,0,512);
			}

			if(!bt_audio_ch_busy ()) {
				//*** for a2dp vendor decode edit code #11 ***//
				// fill PCM data to audio play buffer
				aud_dac_fill_buffer(app_vendor.output, 512);
			}
		}
    }
}


void RAM_CODE vendor_do_decode( void )
{
	int decode_frms, i;
	vendor_dec_info_t dec_info={0};

	// check vendor target initial
	if(app_vendor.vendor_target_initial == 0) {
		return;
	}


	if(hfp_has_sco())  //for the bug:the sbc still has remained data to fill in buff while sco has connection
	{
		//*** for a2dp vendor decode edit code #06 ***//
		// clear & free decode input buffer data
		// refer sbc_do_decode() in app_sbc.c
		return;
	}

	//  a2dp vendor decode buffer cache, 	avoid "POP"
    if(!flag_vendor_buffer_play)
    {
        if(app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
        {
    		//*** for a2dp vendor decode edit code #07 ***//
    		// clear & free decode input buffer data
    		// refer sbc_do_decode() in app_sbc.c
        	return;
        }
		
        flag_vendor_buffer_play = 1;
    }

    // get audio play free buffer
    decode_frms = (aud_dac_get_free_buffer_size()+1)/512;
#ifdef VENDOR_DECODE_TEST
    os_printf("%s, decode_frms=%d\r\n", __func__, decode_frms);
#endif

    if(decode_frms > 0)
    {
        for( i = 0; i < decode_frms; i++ )
		{
        	// check input data empty
        	if(app_vendor.frame_num == 0) {
        		break;
        	}


        	CLEAR_PWDOWN_TICK;

    		//*** for a2dp vendor decode edit code #08 ***//
        	// provide decoder information & input buffer to do decode
        	dec_info.dec_result = 0;
        	dec_info.out_buf 	= app_vendor.output;

        	vendor_decode(&dec_info);



    		//*** for a2dp vendor decode edit code #12 ***//
        	// free input data, if input buffer is mallocate memory
        	app_vendor.frame_num--;


		}
    }
    else
    {



    }
}

#endif //A2DP_VENDOR_DECODE

// EOF

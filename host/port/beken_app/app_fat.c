#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "diskio.h"
#include "bkreg.h"
#include "app_fat.h"
#include "playmode.h"
#include "app_player.h"

#define SZ_AUD_UNIT 0x800


/** @brief file index proc in dir traverse, to get audio file num.
 * @param fname file name in.
 * @param p_cnt counter of audio file.
 * */
static int FileIdxProc_get_file_num(void *fname, void *p_cnt, void *null)
{
    int *p = (int *)p_cnt;
    os_printf("%s is a FILE\r\n", (char *)fname);
    if (is_audio_file((char *)fname) >= F_TYP_MIN_SUPPORTED)
        p[0]++;

    return (p[0]);
}

#if 0
// 按音频文件索引打开文件，
static int open_audio_file_by_index(void *p1, void *p2, void *p3)
{
    _t_cnt_info *p = (_t_cnt_info *)p2;
    FIL *fp = (FIL *)p3;
    FRESULT fr;
    if (is_audio_file((char *)p1) >= F_TYP_MIN_SUPPORTED)
        p->curIndex++;
    if (p->curIndex > p->total)
        return (-1);
    if (p->curIndex == p->tarInd)
    { // found target file
        // os_printf((char*)p1);
        // os_printf(":");

        fr = f_open(fp, (char *)p1, FA_READ);
        if (fr == FR_OK)
        {
            return (1); //
        }
    }
    return (0);
}
#endif

/** @brief file index proc in dir traverse, to find target file_index and get abs file name.
 * @param fn_in file name in.
 * @param scanInfo traversal information, include total file num, current file index, trarget file index.
 * @param pp_fn_get pointer of target file name addr.(file path is included in file name, or abs file name)
 * @return 1: file index matched, -1:error file index, 0:index not matched
 * */
static int FileIdxProc_get_fname(void *fn_in, void *scanInfo, void *pp_fn_get)
{
    _t_cnt_info *p = (_t_cnt_info *)scanInfo;
    MYSTRING *pstr = (MYSTRING *)pp_fn_get;
    //*pstr=NULL;
    if (is_audio_file((char *)fn_in) >= F_TYP_MIN_SUPPORTED)
        p->curIndex++;
    if (p->curIndex >= p->total)
    {
        return (-1);
    }
    if (p->curIndex == p->tarInd)
    {
        char *fn = (char *)jmalloc(strlen((char *)fn_in) + 1, 1);
        strcpy(fn, (char *)fn_in);
        *pstr = fn;
        return (1);
    }
    return (0);
}

// 获取音频文件的头信息（.wav）
int get_wav_file_hdr_info(void *fp, void *fmt, BYTE **dp, unsigned int *dat0_len, unsigned int *dat_total_len)
{
    SDCARD_S si;
    sdcard_get_card_info(&si);

    FIL *pfp = (FIL *)fp;
    BYTE *filBuf = (BYTE *)jmalloc(si.block_size, 1);
    UINT br;
    unsigned int tmp;
    FRESULT fr;
    int r = 1;
    if (filBuf == NULL)
    {
        r = (-1);
        goto ret1;
    } // mem not enough
    fr = f_read(pfp, filBuf, si.block_size, &br);
    if (fr != FR_OK)
    {
        r = -2;
        goto ret1;
    }
    if (memcmp(filBuf, "RIFF", 4) != 0)
    {
        r = (-3);
        goto ret1;
    }
    if (memcmp(&filBuf[8], "WAVE", 4) != 0)
    {
        r = -4;
        goto ret1;
    }
    memcpy(&tmp, &filBuf[4], 4);
    if ((tmp + 8) != pfp->obj.objsize)
    {
        r = -5;
        goto ret1;
    }
    BYTE *pChk = &filBuf[12];
    while (1)
    {
        memcpy(&tmp, &pChk[4], 4);
        if (memcmp(pChk, "fmt ", 4) == 0)
        {
            memcpy(fmt, &pChk[8], tmp);
        }
        else if (memcmp(pChk, "data", 4) == 0)
        {
            BYTE *dat0 = (BYTE *)jmalloc(si.block_size - (&pChk[8] - filBuf), 1);
            if (dat0 == NULL)
            {
                r = -1;
                goto ret1;
            }
            memcpy(dat0, &pChk[8], si.block_size - (&pChk[8] - filBuf));
            *dp = dat0;
            *dat0_len = si.block_size - (&pChk[8] - filBuf);
            *dat_total_len = tmp - *dat0_len;
            break;
        }
        pChk += (tmp + 8);
        if ((pChk - filBuf) > si.block_size)
        {
            if ((pChk - filBuf) >= pfp->obj.objsize)
            {
                r = -6;
                goto ret1;
            } // can't find "data" chunk
            f_lseek(pfp, pChk - filBuf);
            fr = f_read(pfp, filBuf, si.block_size, &br);
            if (fr != FR_OK)
            {
                r = -2;
                goto ret1;
            }
            pChk = filBuf;
        }
    }
ret1:
    if (filBuf)
    {
        jfree(filBuf);
    }
    return (r);
}

// convert pcm fmt to 32bit2ch
void aud_cvt_to_pcm32_stereo(void *pcmo, int sz, void *pcmi, void *fmt)
{
    _t_wave_hdr_format *pfmt = (_t_wave_hdr_format *)fmt;
    BYTE *po = (BYTE *)pcmo;
    BYTE *pi = (BYTE *)pcmi;
    int i, j, c;
    c = pfmt->wBitsPerSample / 8;
    int cnt = sz / (pfmt->wBlockAlign);
    // if(pfmt->wBitsPerSample==8)po=&po[2];
    // if(pfmt->wBitsPerSample==16)po=&po[1];
    for (i = 0; i < cnt; i++)
    {
        for (j = 0; j < pfmt->wChn; j++)
        {
            memcpy(po, pi, c);
            po += 2;
            memcpy(po, pi, c);
            po += 2;
            pi += c;
        }
    }
}

void play_wav_by_fp(void *fp)
{
#if 0
	BYTE*dat=NULL;//(BYTE*)jmalloc(SZ_AUD_UNIT	,1);
	BYTE*dat0=NULL;
	unsigned int dat0_len;
	unsigned int dat_total_len,br;
	_t_wave_hdr_format fmt;
	FRESULT fr;
	BYTE*buf2=NULL;
	int l2;
	int r=get_wav_file_hdr_info(fp, &fmt,&dat0, &dat0_len, &dat_total_len);
	if(r<1){
		os_printf("get wav header FAIL,error code=%d\r\n",r);
		goto ret1;
	}

	if((dat0_len==0)||(dat_total_len==0)){
		os_printf("No data to play\r\n");
		goto ret1;
	}
	if(fmt.wBitsPerSample&0x07){
		os_printf("Can't support non-byte-align wav\r\n");
		goto ret1;
	}
	if((fmt.wBitsPerSample<16)||(fmt.wBitsPerSample>32)){
		os_printf("Can't support BR<16 or BR>32 wav\r\n");
		goto ret1;
	}

	os_printf("wav dat total size =%d\r\n",dat_total_len);

	print("wav first pkt",dat0,dat0_len);
	aud_dac_close();
	aud_dac_config(fmt.dwSampleRate, fmt.wChn, fmt.wBitsPerSample);
	int l=aud_dac_get_free_buffer_size();
	int l0=l,m,n,i;
	os_printf("aud dac buffer size =%d\r\n",l);

	dat=(BYTE*)jmalloc(SZ_AUD_UNIT,1);
	if(dat==NULL)goto ret1;

	l2=SZ_AUD_UNIT/fmt.wBlockAlign*8;
	buf2=(BYTE*)jmalloc(l2,1);
	if(buf2==NULL)goto ret1;
	aud_cvt_to_pcm32_stereo(buf2, dat0_len, dat0, &fmt);
	aud_dac_fill_buffer(buf2, dat0_len/fmt.wBlockAlign*8);

	l0=aud_dac_get_free_buffer_size();
	os_printf("aud dac buffer size =%d\r\n",l0);
	m=l0/l2;
	n=l0%l2;

	for(i=0;i<m;i++){
		fr=f_read(fp, dat, SZ_AUD_UNIT	, &br);
		memset(buf2,0,l2);
		aud_cvt_to_pcm32_stereo(buf2, SZ_AUD_UNIT, dat, &fmt);
		aud_dac_fill_buffer(buf2, l2);
	}
	if(n){
		fr=f_read(fp, dat, n/8*fmt.wBlockAlign, &br);
		memset(buf2,0,l2);
		aud_cvt_to_pcm32_stereo(buf2, br, dat, &fmt);
		aud_dac_fill_buffer(buf2, n);
	}
	extern uint64_t os_get_tick_counter();
	uint64_t t0=os_get_tick_counter();
	uint32_t t_wav=((dat_total_len+dat0_len)*100)/(fmt.wBlockAlign*fmt.dwSampleRate);
	aud_dac_open();
	dat_total_len-=l;
	m=dat_total_len/SZ_AUD_UNIT;
	n=dat_total_len%SZ_AUD_UNIT;
	for(i=0;i<m;i++){
		gpio_output(35, 1);
		fr=f_read(fp, dat, SZ_AUD_UNIT	, &br);
		gpio_output(35, 0);
		if(fr!=FR_OK){
			os_printf("f_read FAILs\r\n");
			goto ret1;
		}
		memset(buf2,0,l2);
		aud_cvt_to_pcm32_stereo(buf2, SZ_AUD_UNIT, dat, &fmt);
		while(aud_dac_get_free_buffer_size()<l2);
		gpio_output(36, 1);
		aud_dac_fill_buffer(buf2, l2);
		gpio_output(36, 0);
		timer_clear_watch_dog();
	}
	if(n){
		gpio_output(35, 1);
		fr=f_read(fp, dat, n, &br);
		gpio_output(35, 0);
		if(fr!=FR_OK){
			os_printf("f_read FAILs\r\n");
			goto ret1;
		}
		memset(buf2,0,l2);
		aud_cvt_to_pcm32_stereo(buf2, br, dat, &fmt);
		while(aud_dac_get_free_buffer_size()<n);
		aud_dac_fill_buffer(buf2, n/fmt.wBlockAlign*8);
	}
	os_printf("player waiting for end....\r\n");

	while(1){
		if(os_get_tick_counter()-t0>t_wav)break;
		timer_clear_watch_dog();
	}
ret1:
	f_close(fp);
ret2:
	os_printf("play over\r\n");
	aud_dac_close();
	jfree(buf2);
	jfree(dat);
	jfree(dat0);
#endif
}
// 播放音频文件
void play_wav_file(void *fn)
{
    FRESULT fr;
    FIL p_fil;
    fr = f_open(&p_fil, (char *)fn, FA_READ);
    if (fr != FR_OK)
    {
        return;
    }
    play_wav_by_fp(&p_fil);
}

/** @brief Traverse dir and the sub dir
 * @param dir_name the dir name to traverse, if it's root dir, then input ""
 * @param sub_dir_en traversal sub dir enable
 * @param cbk the call back func, run the cbk when the item is a file.
 * @param p1 the first param of cbk.
 * @param p2 the second param of cbk.
 * */
int Traversal_Dir(char *dir_name, uint8_t sub_dir_en, void *cbk, void *p1, void *p2)
{
    //	os_printf("Enter: %s %s\r\n",__FUNCTION__,__LINE__);
    FRESULT fr;
    FILINFO *_file = (FILINFO *)jmalloc(sizeof(FILINFO), 1);
    DIR *_dir = (DIR *)jmalloc(sizeof(DIR), 1);
    int r;
    if (_file == NULL)
    {
        r = TD_ERRCODE_Mem;
        goto ret1;
    }
    if (_dir == NULL)
    {
        r = TD_ERRCODE_Mem;
        goto ret1;
    }
    fr = f_opendir(_dir, dir_name);
    if (fr != FR_OK)
    {
        r = TD_ERRCODE_DirOpen;
        goto ret1;
    }
    while (1)
    {
        fr = f_readdir(_dir, _file);
        if (fr == FR_OK)
        {
            if (strlen(_file->fname) < 1)
            {
                r = TD_ERRCODE_SUCCESS;
                goto ret1;
            } // complete
            char *newpath = (char *)jmalloc(strlen(dir_name) + 1 + strlen(_file->fname) + 1, 1);
            if (newpath == NULL)
            {
                r = TD_ERRCODE_Mem;
                goto ret1;
            }
            strcpy(newpath, dir_name);
            strcat(newpath, "/");
            strcat(newpath, _file->fname);
            if(!(_file->fattrib & AM_HID))
            {
                if (_file->fattrib & AM_DIR)
                { // is DIR
                    if(sub_dir_en)
                    {
                        r = Traversal_Dir(newpath, 1, cbk, p1, p2);
                        if (r < 0)
                        {
                            jfree(newpath);
                            goto ret1;
                        }
                    }
                }
                else// if(_file->fattrib & AM_ARC)
                { // is FILE
                    // cbk
                    CBK_TRAVERSAL_FATFS pcbk = (CBK_TRAVERSAL_FATFS)cbk;
                    // os_printf("cbk=%.8x fun=%.8x\r\n",pcbk,FileIdxProc_get_file_num);
                    pcbk(newpath, p1, p2);
                }
            }
            jfree(newpath);
            newpath = NULL;
        }
        else
        {
            r = TD_ERRCODE_DirRead;
            goto ret1;
        }
    }
ret1:
    f_closedir(_dir);
    //	os_printf("End: %s %s\r\n",__FUNCTION__,__LINE__);
    if (_file)
        jfree(_file);
    if (_dir)
        jfree(_dir);

    return (r);
}

char *get_media_file_name_by_index(int idx, int total)
{
    _t_cnt_info scanInfo;
    MYSTRING fn;
    memset(&scanInfo, 0, sizeof(scanInfo));
    scanInfo.total = total;
    scanInfo.tarInd = idx;
    scanInfo.curIndex = 0;
    Traversal_Dir("", 1, FileIdxProc_get_fname, &scanInfo, &fn);
    return (fn);
}

int get_media_file_counter(void)
{
    int cnt = 0;
    Traversal_Dir("", 1, FileIdxProc_get_file_num, &cnt, NULL);
    return (cnt);
}

void play_mp3_file_by_fp(void *fp)
{
#if 0
	BYTE*dec=NULL;//(BYTE*)jmalloc(SZ_AUD_UNIT	,1);
	BYTE*dat=NULL;//(BYTE*)jmalloc(SZ_AUD_UNIT	,1);
	unsigned int dat0_len;
	unsigned int dat_total_len,br;
	MP3FrameInfo mp3FrameInfo;
	_t_wave_hdr_format fmt;
	FRESULT fr;
	BYTE*buf2=NULL;
	int l2,r;

	HMP3Decoder *hmp3=MP3InitDecoder();
	if(NULL == hmp3){
		LOG_I(MP3,"hMP3Decoder_malloc_fail\r\n");
		memory_usage_show();
		goto ret3;
	}
	MP3DecInfo*pmp3=(MP3DecInfo*)hmp3;
	pmp3->decode_state=MP3_DECODE_FIND_ID3_INFO;
	dec=(BYTE*)jmalloc(768*4,1);
	if(dec==NULL){
		goto ret1;
	}

	dat=(BYTE*)jmalloc(2644+4,1);
	if(dat==NULL){
		goto ret1;
	}
	MP3LinkToFil(fp);
	pmp3->mainBuf=dat;

	pmp3->mainBuf_ptr = pmp3->mainBuf + BIT_RESVOR_SIZE;
	pmp3->mainBuf_len = 0;
	pmp3->err_cnt = 0;

	r=MP3Decode(hmp3, (short*)dec, &l2);
	if(r!=ERR_MP3_NONE){
		goto ret1;
	}
	MP3GetLastFrameInfo(hmp3, &mp3FrameInfo); //获得解码

	LOG_I(MP3,"比特率%dkb/s, 原采样率%dHZ\r\n",	(mp3FrameInfo.bitrate)/1000,mp3FrameInfo.samprate);
	LOG_I(MP3,"原声道数%d, MPAG:%d,层:%d\r\n",	mp3FrameInfo.nChans,mp3FrameInfo.version,mp3FrameInfo.layer);
	LOG_I(MP3,"outputsamps:%d\r\n",mp3FrameInfo.outputSamps);
	LOG_I(MP3,"每帧数据:%d\r\n",pmp3->framesize);
	fmt.wChn=mp3FrameInfo.nChans;
	fmt.dwSampleRate=mp3FrameInfo.samprate;
	fmt.wBitsPerSample=mp3FrameInfo.bitsPerSample;
	fmt.wBlockAlign=fmt.wChn*fmt.wBitsPerSample/8;
	fmt.dwAvgBytesPerSec=fmt.dwSampleRate*fmt.wBlockAlign;
	aud_dac_config(mp3FrameInfo.samprate, mp3FrameInfo.nChans, /*mp3FrameInfo.bitsPerSample*/16);
	aud_dac_open();
	aud_dac_set_volume(AUDIO_VOLUME_MAX);

	int l=aud_dac_get_free_buffer_size();
	int l0=l,m,n,i;
	os_printf("aud dac buffer size =%d\r\n",l);
	buf2=(BYTE*)jmalloc(mp3FrameInfo.outputSamps*2,1);
	if(buf2==NULL)goto ret1;
	while(1){
		r=MP3Decode(hmp3, (short*)dec, &l2);
		if(r!=ERR_MP3_NONE)break;
//		os_printf("mp3 decode state=%d \r\n",pmp3->decode_state);
//		print("mp3 decode data",dec,l2*2);
		aud_cvt_to_pcm32_stereo(buf2, l2*2, dec, &fmt);
		while(aud_dac_get_free_buffer_size()<l2*4);
//		os_printf("pcm size=%d buf size=%d\r\n",l2*2,l);
		aud_dac_fill_buffer(buf2, l2*4);
	}
ret1:
	MP3FreeDecoder(hmp3);
ret3:
	f_close(fp);
ret2:
	os_printf("play over\r\n");
	aud_dac_close();
	if(dec)jfree(dec);
	dec=NULL;
	if(buf2)jfree(buf2);
	buf2=NULL;
	if(dat)jfree(dat);
	dat=NULL;
#endif
}

void play_mp3_file(void *fn)
{
#if 0
	BYTE*dec=NULL;//(BYTE*)jmalloc(SZ_AUD_UNIT	,1);
	BYTE*dat=NULL;//(BYTE*)jmalloc(SZ_AUD_UNIT	,1);
	unsigned int dat0_len;
	unsigned int dat_total_len,br;
	MP3FrameInfo mp3FrameInfo;
	_t_wave_hdr_format fmt;
	FRESULT fr;
	FIL p_fil;
	FIL*fp;
	BYTE*buf2=NULL;
	int l2,r;
	fp=&p_fil;
	fr=f_open(fp,(char*)fn,FA_READ);
	if(fr!=FR_OK){
		goto ret2;
	}
	LOG_I(MP3,"media file=%s\r\n",(char*)fn);

	HMP3Decoder *hmp3=MP3InitDecoder();
    if(NULL == hmp3)
    {
        LOG_I(MP3,"hMP3Decoder_malloc_fail\r\n");
        memory_usage_show();
        goto ret3;
    }
	MP3DecInfo*pmp3=(MP3DecInfo*)hmp3;
	pmp3->decode_state=MP3_DECODE_FIND_ID3_INFO;
//	dat=(BYTE*)jmalloc(READBUF_SIZE,1);
	dec=(BYTE*)jmalloc(768*4,1);
	if(dec==NULL){
		goto ret1;
	}

	dat=(BYTE*)jmalloc(2644+4,1);
	if(dat==NULL){
		goto ret1;
	}
	MP3LinkToFil(fp);
	pmp3->mainBuf=dat;

	pmp3->mainBuf_ptr = pmp3->mainBuf + BIT_RESVOR_SIZE;
	pmp3->mainBuf_len = 0;
	pmp3->err_cnt = 0;

	r=MP3Decode(hmp3, (short*)dec, &l2);
	if(r!=ERR_MP3_NONE){
		goto ret1;
	}
//	print("mp3 decode data",dec,l2);
	MP3GetLastFrameInfo(hmp3, &mp3FrameInfo); //获得解码

	LOG_I(MP3,"比特率%dkb/s, 原采样率%dHZ\r\n",
		(mp3FrameInfo.bitrate)/1000,mp3FrameInfo.samprate);
	LOG_I(MP3,"原声道数%d, MPAG:%d,层:%d\r\n",
		mp3FrameInfo.nChans,mp3FrameInfo.version,mp3FrameInfo.layer);
	LOG_I(MP3,"outputsamps:%d\r\n",mp3FrameInfo.outputSamps);
	LOG_I(MP3,"每帧数据:%d\r\n",pmp3->framesize);
	fmt.wChn=mp3FrameInfo.nChans;
	fmt.dwSampleRate=mp3FrameInfo.samprate;
	fmt.wBitsPerSample=mp3FrameInfo.bitsPerSample;
	fmt.wBlockAlign=fmt.wChn*fmt.wBitsPerSample/8;
	fmt.dwAvgBytesPerSec=fmt.dwSampleRate*fmt.wBlockAlign;
	aud_dac_config(mp3FrameInfo.samprate, mp3FrameInfo.nChans, /*mp3FrameInfo.bitsPerSample*/16);
	aud_dac_open();
	aud_dac_set_volume(AUDIO_VOLUME_MAX);

	int l=aud_dac_get_free_buffer_size();
	int l0=l,m,n,i;
	os_printf("aud dac buffer size =%d\r\n",l);
	buf2=(BYTE*)jmalloc(mp3FrameInfo.outputSamps*2,1);
	if(buf2==NULL)goto ret1;
	while(1){
		r=MP3Decode(hmp3, (short*)dec, &l2);
		if(r!=ERR_MP3_NONE)break;
//		os_printf("mp3 decode state=%d \r\n",pmp3->decode_state);
//		print("mp3 decode data",dec,l2*2);
		aud_cvt_to_pcm32_stereo(buf2, l2*2, dec, &fmt);
		while(aud_dac_get_free_buffer_size()<l2*4);
//		os_printf("pcm size=%d buf size=%d\r\n",l2*2,l);
		aud_dac_fill_buffer(buf2, l2*4);
	}
ret1:
	MP3FreeDecoder(hmp3);
ret3:
	f_close(fp);
ret2:
	os_printf("play over\r\n");
	aud_dac_close();
	if(dec)jfree(dec);
	dec=NULL;
	if(buf2)jfree(buf2);
	buf2=NULL;
	if(dat)jfree(dat);
	dat=NULL;
#endif
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

void app_fat_init(void)
{
#ifdef CONFIG_APP_SDCARD

#ifdef SDCARD_FATFS_TEST
    extern void sdcard_fatfs_test(void);
    sdcard_fatfs_test();
#endif

#ifdef CONFIG_SDCARD_DETECT
    app_sdcard_init(sd_inserted_action, sd_pull_out_action);
    #ifdef SDCARD_DETECT_IO
    if(gpio_input(SDCARD_DETECT_IO) == SDCARD_DETECT_LVL){
        app_sd_set_online(1);
        gpio_int_enable(SDCARD_DETECT_IO, (SDCARD_DETECT_LVL == 0) ? GPIO_INT_LEVEL_HIGH : GPIO_INT_LEVEL_LOW);
    }else{
        app_sd_set_online(0);
    }
    #else
    GPIO_PIN det_gpio = app_env_get_handle()->env_cfg.system_para.pins[PIN_sdDet]&0x1f;
    app_sd_set_online(gpio_input(det_gpio) == GPIO_INT_LEVEL_LOW);
    gpio_int_enable(det_gpio, GPIO_INT_LEVEL_HIGH);
    #endif

#else
    if (fat_malloc_files_buffer() != FR_OK) {
        os_printf("fat_malloc_files_buffer failed, please check\n", __func__);
        while(1);
    }
    app_sd_set_online(1);
    Media_Fs_Init(0);
    // fatfs_write_test();
    os_printf("there are %d files on disk\r\n", get_musicfile_count());

#ifdef CONFIG_SD_INSERT_MODE_SW
    system_work_mode_change_button();
// set_app_player_play_mode(APP_PLAYER_MODE_PLAY_ALL_CYCLE);
#endif
#endif

#endif
}


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

#ifdef CONFIG_SDCARD_DETECT
void sd_inserted_action(void)
{
    os_printf("SD Card inserted in\r\n");
#ifndef CONFIG_SD_AUTO_MOD_SW_DIS
	system_work_mode_set_button(SYS_WM_SDCARD_MODE);
#endif
#ifdef SDCARD_DETECT_IO
    gpio_int_enable(SDCARD_DETECT_IO, (SDCARD_DETECT_LVL == 0) ? GPIO_INT_LEVEL_HIGH : GPIO_INT_LEVEL_LOW);
#else
    GPIO_PIN det_gpio = app_env_get_handle()->env_cfg.system_para.pins[PIN_sdDet]&0x1f;
    gpio_int_enable(det_gpio, GPIO_INT_LEVEL_HIGH);
#endif
}

//call sd_pull_out_isr first then call sd_pull_out_action
void sd_pull_out_isr(uint32_t io0_31, uint32_t io32_39)
{
#ifdef SDCARD_DETECT_IO
	GPIO_PIN det_gpio = SDCARD_DETECT_IO;
#else
    GPIO_PIN det_gpio = app_env_get_handle()->env_cfg.system_para.pins[PIN_sdDet]&0x1f;
#endif
    if((det_gpio < 32) ? (io0_31 & (1 << det_gpio)) : (io32_39 & (1 << (det_gpio - 32))))
    {
        if(gpio_input(det_gpio) != SDCARD_DETECT_LVL)
        {
            os_printf("SD pull out:%d\n\n\n\n", det_gpio);
            gpio_int_disable(det_gpio);
            if(app_is_sdcard_mode()){
                set_app_player_flag_hw_detach();
            }
        }
    }
}

void sd_pull_out_action(void)
{
    os_printf("SD Card pulled out:%d\n", app_is_sdcard_mode());
    // app_sd_set_online(0);

    if(!app_is_sdcard_mode()) return;

    set_app_player_flag_hw_detach();
    Media_Fs_Uninit(0);
    // fat_free_files_buffer();
    system_work_mode_change_button();
}
#endif

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

#if 0 // write test
void fatfs_write_test(void)
{
    FRESULT res = FR_OK;
    FIL file;
    FIL* fp = &file;
    int wr_sz_get;
    char buff[] = "123456";
    os_printf("sd files write test : fatfs_write_test.txt\n");
    res = f_open(fp, "/fatfs_write_test.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if(res != FR_OK){
        os_printf("file open fail: %d\r\n", res);
    }
    res = f_write(fp, buff, sizeof(buff), (UINT*)&wr_sz_get);
    if(res != FR_OK){
        os_printf("file f_write fail: %d\r\n", res);
    }
    res = f_close(fp);
    if(res != FR_OK){
        os_printf("file f_close fail: %d\r\n", res);
    }
    os_printf("sd files write test end\n");
}
#endif


#ifdef TEST_FATFS_WRITE_SPEED
/* Pseudo random number generator, pns:!0:Initialize, 0:Get */
static uint32_t random_gen(uint32_t pns)
{
    static uint32_t lfsr;
    uint32_t n;

    if (pns) {
        lfsr = pns;
        for (n = 0; n < 32; n++) random_gen(0);
    }
    if (lfsr & 1) {
        lfsr >>= 1;
        lfsr ^= 0x80200003;
    } else {
        lfsr >>= 1;
    }
    return lfsr;
}

//测试写和读速度，并对读出内容做了校对，确保和写入一致。
void fatfs_rw_speed_test(void)
{
    #define FAT_TEST_LOG(fmt,...)  FAT_PRINTF("[FAT|SPEED TEST]"fmt, ##__VA_ARGS__)
    #define FAT_TEST_TIMEms         (30 * 1000)
    #define FAT_SINGLE_RW_SIZE      2048
    #define PRINT_OP_MIN_TIMEMS     50
    enum{
        TEST_IDLE,
        TEST_INIT,
        WRITE_OPEN,
        WRITE_ING,
        READ_OPEN,
        READ_ING,
    };
    static uint32_t time_cnt_all = 0;//ms, only rw time, loop time excluded
    uint32_t time_cnt_cur = 0;//ms, single rw time record
    static uint32_t time_cnt_min = -1;//ms, min single rw time record
    static uint32_t time_cnt_max = 0;//ms. max single rw time record
    static uint32_t op_byte_cnt = 0;

    FRESULT res = FR_OK;
    static FIL file;
    static FIL* fp = &file;
    uint8_t rw_buff[FAT_SINGLE_RW_SIZE];
    int rw_sz_get;
    static int rw_sz_set = sizeof(rw_buff);
    static int state = TEST_INIT;
    const char path_sdcard[] = "/fatfs_rw_speed_test.bin";
    const char path_udisk[] = "1:/fatfs_rw_speed_test.bin";
    uint8_t path[256];

    if(sys_time_get() < 5000) return;//test after system start

    if(state == TEST_INIT)
    {
        if(player_get_play_status()){
            app_player_play_pause_caller(0);
        }
        if(sys_time_get() > 5500){
            if(get_cur_media_type() == DISK_TYPE_SD){
                memcpy(path, path_sdcard, sizeof(path_sdcard));
            }else if(get_cur_media_type() == DISK_TYPE_UDISK){
                memcpy(path, path_udisk, sizeof(path_udisk));
            }
            state = WRITE_OPEN;
        }
    }
////////////////////////////// write
    if(state == WRITE_OPEN)
    {
        FAT_TEST_LOG(" >>>> %s -> write '%s', disk type:%d\n", __FUNCTION__, path, get_cur_media_type());
        res = f_open(fp, (const TCHAR*)path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
        if(res != FR_OK){
            FAT_TEST_LOG("file open fail: %d\n\n\n", res);
            state = TEST_IDLE;
            return;
        }
        FAT_TEST_LOG("file open @ %dms\n", sys_time_get());
        state = WRITE_ING;
    }
    else if(state == WRITE_ING)
    {
        int n;
        static uint32_t pns = 0;

        pns++;
        for (n = 0, random_gen(pns); n < rw_sz_set; n++) rw_buff[n] = (uint8_t)random_gen(0);
        time_cnt_cur = sys_time_get();
        res = f_write(fp, rw_buff, rw_sz_set, (UINT*)&rw_sz_get);
        if(res != FR_OK){
            FAT_TEST_LOG("file f_write fail: %d\n\n\n", res);
            state = TEST_IDLE;
            return;
        }
        time_cnt_cur = sys_time_get() - time_cnt_cur;
        time_cnt_all += time_cnt_cur;
        op_byte_cnt += rw_sz_get;
        if(time_cnt_cur < time_cnt_min) time_cnt_min = time_cnt_cur;
        if(time_cnt_cur > time_cnt_max) time_cnt_max = time_cnt_cur;
        if(time_cnt_cur >= PRINT_OP_MIN_TIMEMS) {
            FAT_TEST_LOG("Write@%dms, time_cnt_cur: %dms\n", sys_time_get(), time_cnt_cur);
        }
        if(time_cnt_all >= FAT_TEST_TIMEms) {
            res = f_close(fp);
            if(res != FR_OK){
                FAT_TEST_LOG("file f_close fail: %d\n\n\n", res);
                state = TEST_IDLE;
            }
            FAT_TEST_LOG("file close @ %dms\n", sys_time_get());
            int speed = (op_byte_cnt/1024) / (time_cnt_all/1000);
            FAT_TEST_LOG("write %d Byte in %dms, speed:%dKB/s\n", op_byte_cnt, time_cnt_all, speed);
            FAT_TEST_LOG("single write %d Byte, time min: %dms, max: %dms\n", FAT_SINGLE_RW_SIZE, time_cnt_min, time_cnt_max);
            FAT_TEST_LOG(" <<<< sd files write test end\n\n");
            state = READ_OPEN;
        }
    }

////////////////////////////// read
    else if(state == READ_OPEN)
    {
        FAT_TEST_LOG(" >>>> %s -> read '%s'\n", __FUNCTION__, path);
        res = f_open(fp, (const TCHAR*)path, FA_READ);
        if(res != FR_OK){
            FAT_TEST_LOG("file open fail: %d\n\n\n", res);
            state = TEST_IDLE;
            return;
        }
        FAT_TEST_LOG("file open @ %dms\n", sys_time_get());
        time_cnt_all = 0;
        time_cnt_min = -1;
        time_cnt_max = 0;
        op_byte_cnt = 0;
        state = READ_ING;
    }
    else if(state == READ_ING)
    {
        int n;
        static uint32_t pns = 0;

        pns++;
        time_cnt_cur = sys_time_get();
        res = f_read(fp, rw_buff, rw_sz_set, (UINT*)&rw_sz_get);
        if(res != FR_OK){
            FAT_TEST_LOG("file f_write fail: %d\n\n\n", res);
            state = TEST_IDLE;
            return;
        }
        for (n = 0, random_gen(pns); (n < rw_sz_get) && ((rw_buff[n] == (uint8_t)random_gen(0))); n++);
        if(n != rw_sz_get) {
            FAT_TEST_LOG(" Read data differs from the data written, n:%d, rw_sz_get:%d.\n", n, rw_sz_get);
        }
        time_cnt_cur = sys_time_get() - time_cnt_cur;
        time_cnt_all += time_cnt_cur;
        op_byte_cnt += rw_sz_get;
        if(time_cnt_cur < time_cnt_min) time_cnt_min = time_cnt_cur;
        if(time_cnt_cur > time_cnt_max) time_cnt_max = time_cnt_cur;
        if(time_cnt_cur >= PRINT_OP_MIN_TIMEMS) {
            FAT_TEST_LOG("Read@%dms, time_cnt_cur: %dms\n", sys_time_get(), time_cnt_cur);
        }
        if(rw_sz_get < rw_sz_set) {
            res = f_close(fp);
            if(res != FR_OK){
                FAT_TEST_LOG("file f_close fail: %d\n\n\n", res);
                state = TEST_IDLE;
            }
            FAT_TEST_LOG("file close @ %dms\n", sys_time_get());
            int speed = (op_byte_cnt/1024) / (time_cnt_all/1000);
            FAT_TEST_LOG("read %d Byte in %dms, speed:%dKB/s\n", op_byte_cnt, time_cnt_all, speed);
            FAT_TEST_LOG("single read %d Byte, time min: %dms, max: %dms\n", FAT_SINGLE_RW_SIZE, time_cnt_min, time_cnt_max);
            FAT_TEST_LOG(" <<<< sd files read test end\n");
            state = TEST_IDLE;
        }
    }

/*@221219
//闲鱼三星8G卡
[2022-12-19 12:25:50.190]# RECV ASCII>
[FAT|SPEED TEST] >>>> fatfs_rw_speed_test -> write 'fatfs_rw_speed_test.bin'
[FAT|SPEED TEST]file open @ 5001ms
[2022-12-19 12:26:11.304]# RECV ASCII>
[FAT|SPEED TEST]@26118ms, time_cnt_cur: 85ms
[2022-12-19 12:26:47.170]# RECV ASCII>
[FAT|SPEED TEST]file close @ 61969ms
[FAT|SPEED TEST]write 84459520 Byte in 30000ms, speed:2749KB/s
[FAT|SPEED TEST]single write time min: 0ms, max: 85ms//几次测试这个值80ms左右比较多，最低到19ms
[FAT|SPEED TEST] <<<< sd files write test end

[FAT|SPEED TEST] >>>> fatfs_rw_speed_test -> read 'fatfs_rw_speed_test.bin'
[FAT|SPEED TEST]file open @ 61982ms
[2022-12-19 12:27:35.845]# RECV ASCII>
[FAT|SPEED TEST]file close @ 110645ms
[FAT|SPEED TEST]read 85350400 Byte in 42948ms, speed:1984KB/s
[FAT|SPEED TEST]single read time min: 0ms, max: 13ms//几次测试这个值5ms~13ms
[FAT|SPEED TEST] <<<< sd files read test end
*/
}
#endif

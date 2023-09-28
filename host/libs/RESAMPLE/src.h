#include <stdint.h>

#ifndef __SRC_H__
#define __SRC_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#define MAX_CHANNELS	2

#define UTILS_AUD_SRC        1
#define UTILS_AUD_SRC244     2
#define UTILS_AUD_SRC248     4
#define UTILS_AUD_SRC_SEL    UTILS_AUD_SRC248//(UTILS_AUD_SRC244 | UTILS_AUD_SRC248)
// #define UTILS_AUD_SRC_SEL    UTILS_AUD_SRC




#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC244)

typedef enum _SRC244_ID_e
{
	SRC244_ID_NULL = 0,
	SRC244_ID_U2,      //mainly for 22k->44.1k
	SRC244_ID_U4,      //mainly for 11k->44.1k
	SRC244_ID_U147D160,//mainly for 48k->44.1k
    SRC244_ID_U147D80, //mainly for 24k->44.1k
	SRC244_ID_U441D320,//mainly for 32k->44.1k
	SRC244_ID_U441D160,//mainly for 16k->44.1k
	SRC244_ID_U441D80, //mainly for  8k->44.1k
	SRC244_ID_COUNT
}SRC244_ID_e;

uint32_t src_244_version(void);
uint8_t* src_244_verstr(void);
int32_t src_244_size(int32_t id, int32_t max_frame_size, int32_t channels);
int32_t src_244_init(void* src, int32_t id, int32_t max_frame_size, int32_t channels);
int32_t src_244_exec(void* src, void* in, uint32_t iwidth, void* out, uint32_t owidth, int32_t samples);

#define src_244_exec_16i16o(/*void*/src, /*int16_t*/in, /*int16_t*/out, /*int32_t*/samples)  src_244_exec(src, (void*)in, 16, (void*)out, 16, samples)
#define src_244_exec_16i24o(/*void*/src, /*int16_t*/in, /*int32_t*/out, /*int32_t*/samples)  src_244_exec(src, (void*)in, 16, (void*)out, 24, samples)
#define src_244_exec_24i16o(/*void*/src, /*int32_t*/in, /*int16_t*/out, /*int32_t*/samples)  src_244_exec(src, (void*)in, 24, (void*)out, 16, samples)
#define src_244_exec_24i24o(/*void*/src, /*int32_t*/in, /*int32_t*/out, /*int32_t*/samples)  src_244_exec(src, (void*)in, 24, (void*)out, 24, samples)

#endif //UTILS_AUD_SRC244





//config project_description.cmake or bstudio lib
#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC248)

typedef enum _SRC248_ID_e
{
	SRC248_ID_NULL = 0,
	SRC248_ID_U2,      //mainly for 24k->48k
	SRC248_ID_U3,      //mainly for 16k->48k
	SRC248_ID_U6,      //mainly for  8k->48k
    SRC248_ID_U3D2,	   //mainly for 32k->48k
	SRC248_ID_U160D147,//mainly for 44.100k->48k
	SRC248_ID_U320D147,//mainly for 22.050k->48k
	SRC248_ID_U640D147,//mainly for 11.025k->48k
	SRC248_ID_COUNT
}SRC248_ID_e;

uint32_t src_248_version(void);
uint8_t* src_248_verstr(void);
int32_t src_248_size(int32_t id, int32_t max_frame_size, int32_t channels);
int32_t src_248_init(void* src, int32_t id, int32_t max_frame_size, int32_t channels);
int32_t src_248_exec(void* src, void* in, uint32_t iwidth, void* out, uint32_t owidth, int32_t samples);

#define src_248_exec_16i16o(/*void*/src, /*int16_t*/in, /*int16_t*/out, /*int32_t*/samples)  src_248_exec(src, (void*)in, 16, (void*)out, 16, samples)
#define src_248_exec_16i24o(/*void*/src, /*int16_t*/in, /*int32_t*/out, /*int32_t*/samples)  src_248_exec(src, (void*)in, 16, (void*)out, 24, samples)
#define src_248_exec_24i16o(/*void*/src, /*int32_t*/in, /*int16_t*/out, /*int32_t*/samples)  src_248_exec(src, (void*)in, 24, (void*)out, 16, samples)
#define src_248_exec_24i24o(/*void*/src, /*int32_t*/in, /*int32_t*/out, /*int32_t*/samples)  src_248_exec(src, (void*)in, 24, (void*)out, 24, samples)

#endif //UTILS_AUD_SRC248







#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC)

enum
{
	SRC_ID_NULL = 0,
	SRC_ID_D2,
	SRC_ID_D3,
	SRC_ID_D6,
	SRC_ID_U2,
	SRC_ID_U3,
	SRC_ID_U6,
	SRC_ID_U160D147,//mainly for 44.1k->48k
	SRC_ID_U160D441,//mainly for 44.1k->16k
	SRC_ID_U160D882,//mainly for 44.1k-> 8k
	SRC_ID_U147D160,//mainly for 48k->44.1k
	SRC_ID_U441D160,//mainly for 16k->44.1k
	SRC_ID_U882D160,//mainly for  8k->44.1k
	SRC_ID_U320D441,//mainly for 44.1k->32k
	SRC_ID_U441D320,//mainly for 32k->44.1k
	SRC_ID_U3D2,	//mainly for 32k->48k
	SRC_ID_U2D3,	//mainly for 48k->32k
	SRC_ID_COUNT
};

uint32_t src_version(void);
uint8_t* src_verstr(void);
int32_t src_size(int32_t id, int32_t max_frame_size, int32_t channels);
int32_t src_init(void* src, int32_t id, int32_t max_frame_size, int32_t channels);
int32_t src_exec_ex(void* src, void* in, uint32_t iwidth, void* out, uint32_t owidth, int32_t samples);

#define src_exec_16i16o(/*void*/src, /*int16_t*/in, /*int16_t*/out, /*int32_t*/samples)  src_exec_ex(src, (void*)in, 16, (void*)out, 16, samples)
#define src_exec_16i24o(/*void*/src, /*int16_t*/in, /*int32_t*/out, /*int32_t*/samples)  src_exec_ex(src, (void*)in, 16, (void*)out, 24, samples)
#define src_exec_24i16o(/*void*/src, /*int32_t*/in, /*int16_t*/out, /*int32_t*/samples)  src_exec_ex(src, (void*)in, 24, (void*)out, 16, samples)
#define src_exec_24i24o(/*void*/src, /*int32_t*/in, /*int32_t*/out, /*int32_t*/samples)  src_exec_ex(src, (void*)in, 24, (void*)out, 24, samples)

#endif //UTILS_AUD_SRC

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__SRC_H__

#ifndef _PARTHUS_SYS_RAND_NUM_GEN_
#define _PARTHUS_SYS_RAND_NUM_GEN_

/***********************************************************************
 *
 * MODULE NAME:    sys_rand_num_gen.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    Function Prototypes for Random Number Generation
 * MAINTAINER:     Ivan Griffin
 * DATE:           17 February 2000
 *
 * SOURCE CONTROL: $Id: sys_rand_num_gen.h,v 1.10 2013/06/05 13:03:35 garyf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *    17.Feb.2000 -   IG       - initial version
 *
 * ISSUES:
 *    See sys/sys_rand_num_gen.c for issues.
 *    
 ***********************************************************************/

#define SYS_RAND_MAX 0xffffffff

void SYSrand_Seed_Rand(uint32_t seed);
uint32_t SYSrand_Get_Rand(void);
uint32_t SYSrand_Get_Rand_R(uint32_t *seed);

uint8_t* SYSrand_Get_Rand_128_Ex(uint8_t* buf);
uint8_t* SYSrand_Get_Rand_192_Ex(uint8_t* buf);

uint8_t* SYSrand_Get_Rand_256_Ex(uint8_t* buf);
#endif

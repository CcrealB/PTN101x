#ifndef SSP_ENGINE_HEADER
#define SSP_ENGINE_HEADER
/******************************************************************************
 * MODULE NAME:    lmp_ssp_engine.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:   
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:        
 *
 * SOURCE CONTROL: $ $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2009 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/

 void F1(uint8_t *U,uint8_t *V, uint8_t *X, uint8_t *Z,uint8_t* OutPut);
 void F2(uint8_t *W,uint8_t *N1, uint8_t *N2, uint8_t *KeyId, uint8_t *A1, uint8_t *A2,uint8_t* OutPut);
 void F3(uint8_t *W,uint8_t *N1, uint8_t *N2, uint8_t *R, uint8_t *IOcap, uint8_t *A1, uint8_t *A2,uint8_t* OutPut);
 void G(uint8_t *U,uint8_t *V, uint8_t *X, uint8_t *Y,uint8_t* OutPut);
 void F1_Invert(uint8_t *U,uint8_t *V, uint8_t *X, uint8_t *Z,uint8_t* outPut);
#endif

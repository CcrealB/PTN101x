#ifndef ECC_HEADER
#define ECC_HEADER
/******************************************************************************
 * MODULE NAME:    lmp_ecc.h
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

#include "lmp_acl_connection.h"
//#define uint32_t unsigned int
//#define uint16_t unsigned short int
//#define uint8_t unsigned char

#define MAX_DIGITS 56
#define MAX_OCTETS 28

typedef struct bigHex 
{
	uint32_t num[MAX_OCTETS/4];
	uint32_t  len;
	uint32_t  sign;
} bigHex;

typedef struct veryBigHex 
{
	uint32_t num[MAX_OCTETS/2];
	uint32_t  len;
	uint32_t  sign;
} veryBigHex;

typedef struct ECC_Point
{
	bigHex x;
	bigHex y;
} ECC_Point;

typedef struct ECC_Jacobian_Point
{
	bigHex x;
	bigHex y;
	bigHex z;
} ECC_Jacobian_Point;


void ECC_Point_Multiplication(const bigHex* pk,const ECC_Point* pPointP,t_lmp_link* p_link,BOOL blocking);
int notEqual(const bigHex *bigHexA,const bigHex *bigHexB);
__INLINE__ void GF_Point_Copy(const ECC_Point *source,ECC_Point *destination);

__INLINE__ void GF_Jacobian_Point_Copy(const ECC_Jacobian_Point *source,ECC_Jacobian_Point *destination);

void LMecc_Generate_ECC_Key(const uint8_t* secret_key, const uint8_t* public_key_x,
					   const uint8_t* public_key_y,t_lmp_link* p_link,BOOL blocking);

int LMecc_isValidSecretKey(uint8_t* secretKey);

#endif

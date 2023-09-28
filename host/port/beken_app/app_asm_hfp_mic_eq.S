#define _ASM_SRC_
#include "spr_defs.h"
//#include "app_asm_sw_eq.h"
#if 1
	.section .data

//4618=-11dB
#define TotalEQnum 4
#define EQ0 1
#define EQ0A0 0
#define EQ0A1 0
#define EQ0B0 0x100000
#define EQ0B1 0
#define EQ0B2 0

#define EQ1 1
#define EQ1A0 0
#define EQ1A1 0
#define EQ1B0 0x100000
#define EQ1B1 0
#define EQ1B2 0

#define EQ2 1
#define EQ2A0 0
#define EQ2A1 0
#define EQ2B0 0x100000
#define EQ2B1 0
#define EQ2B2 0

#define EQ3 1
#define EQ3A0 0
#define EQ3A1 0
#define EQ3B0 0x100000
#define EQ3B1 0
#define EQ3B2 0

#define mEQ(index,para) EQ##index##para
#define nEQ(index,para) -(EQ##index##para)

.globl tbl_hfp_mic_eq_coef
	tbl_hfp_mic_eq_coef:
		.word nEQ(0,A0),nEQ(0,A1),mEQ(0,B0),mEQ(0,B1),mEQ(0,B2)
		.word nEQ(1,A0),nEQ(1,A1),mEQ(1,B0),mEQ(1,B1),mEQ(1,B2)
		.word nEQ(2,A0),nEQ(2,A1),mEQ(2,B0),mEQ(2,B1),mEQ(2,B2)
		.word nEQ(3,A0),nEQ(3,A1),mEQ(3,B0),mEQ(3,B1),mEQ(3,B2)

.globl s_hfp_mic_eq_last_XY
	s_hfp_mic_eq_last_XY:
		.word 0,0,0,0 //L_eq0:X(-1),X(0),Y(-1),Y(0)
		.word 0,0,0,0 //L_eq1:X(-1),X(0),Y(-1),Y(0)
		.word 0,0,0,0 //L_eq2:X(-1),X(0),Y(-1),Y(0)
		.word 0,0,0,0 //L_eq3:X(-1),X(0),Y(-1),Y(0)
		
        .section .text

#define fseq_r_input r3
#define fseq_r_input_tmp r3
#define fseq_r_output r4
#define fseq_r_len r5
#define fseq_r_eq_times r6
#define fseq_r_gain r7
#define fseq_r_index r7
#define fseq_r_mono r8

#define fseq_r_input_Ldata r9
#define fseq_r_input_Rdata r10

#define fseq_r_coef_a0 r11
#define fseq_r_coef_a1 r12
#define fseq_r_coef_b0 r13
#define fseq_r_coef_b1 r14
#define fseq_r_coef_b2 r15
//L_X0~R_Y1
#define fseq_r_L_X0 r16
#define fseq_r_L_X1 r17
//#define fseq_r_L_X2 r18
#define fseq_r_L_Y0 r18
#define fseq_r_L_Y1 r19
#define fseq_r_R_X0 r20
#define fseq_r_R_X1 r21
//#define fseq_r_R_X2 r23
#define fseq_r_R_Y0 r22
#define fseq_r_R_Y1 r23

#define fseq_r_0x80000 r24
#define fseq_r_len_i r25
#define fseq_r_output_tmp r26
#define fseq_r_16bit_max r28
#define fseq_r_16bit_min r29
#define fseq_r_coef_tbl r30
#define fseq_r_lastXY_ptr r31

#define c_mac_rshift 20
#define c_mac_result_GT_32bit 1
//void func_sw_mic_eq((short *)input,(short *)output,int len,int eq_cnt);//len: 1=L+R=4bytes
.globl func_sw_mic_eq
func_sw_mic_eq:
	b.entri 15,(8*4) //8*4:r24~r31
	b.msw 0(r1),r24,3;
	//b.addi fseq_r_rshift,r0,c_mac_rshift
	b.addi fseq_r_16bit_max,r0,32767
	b.addi fseq_r_16bit_min,r0,-32767
	b.addi fseq_r_0x80000,r0,0x80000

	b.muli fseq_r_coef_tbl,fseq_r_index,4*5
	b.addi fseq_r_coef_tbl,fseq_r_coef_tbl,tbl_hfp_mic_eq_coef
	b.muli fseq_r_lastXY_ptr,fseq_r_index,4*4
	b.addi fseq_r_lastXY_ptr,fseq_r_lastXY_ptr,s_hfp_mic_eq_last_XY
	//b.mov fseq_r_input_tmp,fseq_r_input

	f_sweq_load_coef_mono:
		//load last XY
        b.mlwz fseq_r_L_X0,0(fseq_r_lastXY_ptr),3 //load 8 para
		//load eq0_coef
        b.mlwz fseq_r_coef_a0,0(fseq_r_coef_tbl),0 //load 2 para
        b.mlwz fseq_r_coef_b0,8(fseq_r_coef_tbl),1 //load 3 para
		b.mov fseq_r_output_tmp,fseq_r_output
		b.mov fseq_r_len_i,r0
		
		f_sweq_load_sample_mono:
		//--------main loop--------------------
			//load one sample
	        b.lwz fseq_r_input_Ldata,0(fseq_r_input_tmp) //load 1 para
			b.mtspr r0,fseq_r_0x80000,SPR_MACLO
			b.mtspr r0,r0,SPR_MACHI

			b.macs fseq_r_L_X0,fseq_r_coef_b2
			b.macs fseq_r_L_X1,fseq_r_coef_b1
			b.macs fseq_r_input_Ldata,fseq_r_coef_b0
			b.macs fseq_r_L_Y0,fseq_r_coef_a1
			b.macs fseq_r_L_Y1,fseq_r_coef_a0
			b.mov fseq_r_L_Y0,fseq_r_L_Y1
			b.mov fseq_r_L_X0,fseq_r_L_X1
			b.mov fseq_r_L_X1,fseq_r_input_Ldata


			//mac result more than 32bit:
			b.mfspr fseq_r_L_Y1,r0,SPR_MACHI
			b.mfspr fseq_r_input_Ldata,r0,SPR_MACLO
			b.srli fseq_r_input_Ldata,fseq_r_input_Ldata,c_mac_rshift
			b.slli fseq_r_L_Y1,fseq_r_L_Y1,(32-c_mac_rshift)
			b.or fseq_r_L_Y1,fseq_r_L_Y1,fseq_r_input_Ldata
			//-------------------------------------
			//-----------------------------------------------------
			b.sw 0(fseq_r_output_tmp),fseq_r_L_Y1

			b.addi fseq_r_input_tmp,fseq_r_input_tmp,4
			b.addi fseq_r_output_tmp,fseq_r_output_tmp,4
			b.addi fseq_r_len_i,fseq_r_len_i,1
			b.bne fseq_r_len_i,fseq_r_len,f_sweq_load_sample_mono

			//store last XY
	        b.msw 0(fseq_r_lastXY_ptr),fseq_r_L_X0,3 //store 8 para
			b.addi fseq_r_lastXY_ptr,fseq_r_lastXY_ptr,4*4
			b.addi fseq_r_coef_tbl,fseq_r_coef_tbl,4*5
			b.addi fseq_r_eq_times,fseq_r_eq_times,-1
			b.mov fseq_r_input_tmp,fseq_r_output //use output_ptr as input_ptr
			b.bnei fseq_r_eq_times,0,f_sweq_load_coef_mono
/*
		//b.mov fseq_r_input_tmp,fseq_r_output
		b.mov fseq_r_len_i,fseq_r_len
		f_sweq_limit_mono:
			b.lwz fseq_r_input_Ldata,0(fseq_r_input_tmp)
			//limit
			b.blts fseq_r_input_Ldata,fseq_r_16bit_max,1f
			b.mov fseq_r_input_Ldata,fseq_r_16bit_max
			b.sw 0(fseq_r_input_tmp),fseq_r_input_Ldata
			1:
			b.bgts fseq_r_input_Ldata,fseq_r_16bit_min,1f
			b.mov fseq_r_input_Ldata,fseq_r_16bit_min
			b.sw 0(fseq_r_input_tmp),fseq_r_input_Ldata
			1:
			b.addi fseq_r_input_tmp,fseq_r_input_tmp,4*2
			b.addi fseq_r_len_i,fseq_r_len_i,-1
			b.bnei fseq_r_len_i,0,f_sweq_limit_mono
// */
		b.mlwz r24,0(r1),3
		b.reti 15,(8*4)
#endif











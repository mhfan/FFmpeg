/*
 * vp6_filter blackfin asm version
 * Author: SYP(byronshy@mail.ustc.edu.cn)
 * Date: 2007.8.14
 * Copyright: GPL v2.0 or later
*/

#include "config_bfin.h"

#define SAV_R012 [SP+0]=R0; [SP+4]=R1; [SP+8]=R2


DEFUN(vp6_filter_hv4_x,mL1,(uint8_t *dst, uint8_t *src, int stride, const int16_t *weights)):
	SAV_R012;
	LINK 0;
	[--SP]=(R7:4);

	I3 = R0;		//dst
	R1 += -1;
	I0 = R1;        //src-1
	R1 += 4;
	I1 = R1;
	M3 = R1;
	R2 += -4;
	M2 = R2;		//stride
	R2 += -4;
	M1 = R2;
	M0 = 1;

	P1 = [FP+20];	//weights
	R7 = [P1++];	//weights[0],weights[1]
	R6 = [P1];		//weights[2],weights[3]
	R5.L = 64;
	R5.H = 64;
	P0 = 8;

	LSETUP(_vp6_filter_hv4_x_LB,_vp6_filter_hv4_x_LE)LC0=P0;
	_vp6_filter_hv4_x_LB:
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R0 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R1 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R0 = PACK(R3.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R1 = PACK(R3.L,R1.L) || I0-=M3;
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
        R3 = 0;
        R4 = BYTEOP3P(R1:0,R3:2)(LO);
        R0 = BYTEOP3P(R1:0,R3:2)(HI,R) || I0+=M3;
        R4 = R4 | R0;

		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2 || [I3++] = R4;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R0 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R1 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R0 = PACK(R3.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R1 = PACK(R3.L,R1.L) || I0-=M3;
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
        R3 = 0;
        R4 = BYTEOP3P(R1:0,R3:2)(LO);
        R0 = BYTEOP3P(R1:0,R3:2)(HI,R) || I0+=M3;
        R4 = R4 | R0;
		I0 += M1;
	_vp6_filter_hv4_x_LE:
		I1 += M1 || [I3++M2] = R4;

	(R7:4)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(vp6_filter_hv4_x)

///////////////////////////////////////////////////////////
DEFUN(vp6_filter_hv4_y,mL1,(uint8_t *dst, uint8_t *src, int stride, const int16_t *weights)):
	SAV_R012;
	LINK 0;
	[--SP]=(R7:4);

	I1 = R0;
	R1 = R1 - R2;
	I0 = R1;		//src-stride
	I2 = R1;
	M0 = R1;
	R1 += 4;
	I3 = R1;
	M1 = R2;
	R3 = R2 + R2;
	R3 = R3 + R2;
	M3 = R3;
	R2 += -4;
	M2 = R2;
	
	P1 = [FP+20];	//weights
	R7 = [P1++];	//weights[0],weights[1]
	R6 = [P1];		//weights[2],weights[3]
	R5.L = 64;
	R5.H = 64;
	P0 = 8;

	LSETUP(_vp6_filter_hv4_y_LB,_vp6_filter_hv4_y_LE)LC0=P0;
	_vp6_filter_hv4_y_LB:
		DISALGNEXCPT || R0 = [I2++M1] || R1 = [I3++M1];
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I2++M1] || R3 = [I3++M1];
		A0 = R0.L*R7.L, A1 = R0.H*R7.L(IS);
		R4.L = R1.L*R7.L, R4.H = R1.H*R7.L(IS);
		(R3, R2) = BYTEUNPACK R3:2 || R0 = [I2++M1] || R1 = [I3++M1];
		A0 += R2.L*R7.H, A1 += R2.H*R7.H(IS);
		R3.L = R3.L*R7.H, R3.H = R3.H*R7.H(IS);
		R4 = R4 +|+ R3;
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I2++] || R3 = [I3++];
		A0 += R0.L*R6.L, A1 += R0.H*R6.L(IS) || I2-=M3;
		R1.L = R1.L*R6.L, R1.H = R1.H*R6.L(IS) || I3-=M3;
		R4 = R4 +|+ R1;
		(R3, R2) = BYTEUNPACK R3:2;
		R3.L = R3.L*R6.H, R3.H = R3.H*R6.H(IS);
		R4 = R4 +|+ R3;
		R0 = (A0+=R2.L*R6.H), R1 = (A1+=R2.H*R6.H)(IS);
		R0 = PACK(R4.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		R1 = PACK(R4.H,R1.L) || I0-=M0;
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
		R3 = 0;
		R4 = BYTEOP3P(R1:0,R3:2)(LO);
		R0 = BYTEOP3P(R1:0,R3:2)(HI,R) || I0+=M0;
		R4 = R4 | R0;

		DISALGNEXCPT || R0 = [I2++M1] || R1 = [I3++M1];
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I2++M1] || R3 = [I3++M1];
		A0 = R0.L*R7.L, A1 = R0.H*R7.L(IS) || [I1++] = R4;
		R4.L = R1.L*R7.L, R4.H = R1.H*R7.L(IS);
		(R3, R2) = BYTEUNPACK R3:2 || R0 = [I2++M1] || R1 = [I3++M1];
		A0 += R2.L*R7.H, A1 += R2.H*R7.H(IS);
		R3.L = R3.L*R7.H, R3.H = R3.H*R7.H(IS);
		R4 = R4 +|+ R3;
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I2--] || R3 = [I3--];
		A0 += R0.L*R6.L, A1 += R0.H*R6.L(IS) || I2-=M1;
		R1.L = R1.L*R6.L, R1.H = R1.H*R6.L(IS) || I3-=M1;
		R4 = R4 +|+ R1;
		(R3, R2) = BYTEUNPACK R3:2;
		R3.L = R3.L*R6.H, R3.H = R3.H*R6.H(IS) || I2-=M1;
		R4 = R4 +|+ R3;
		R0 = (A0+=R0.L*R6.H), R1 = (A1+=R0.H*R6.H)(IS) || I3-=M1;
		R0 = PACK(R4.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		R1 = PACK(R4.H,R1.L) || I0-=M0;
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
        R3 = 0;
		R4 = BYTEOP3P(R1:0,R3:2)(LO);
		R0 = BYTEOP3P(R1:0,R3:2)(HI,R) || I0+=M0;
		R4 = R4 | R0;
	_vp6_filter_hv4_y_LE:
		[I1++M2] = R4;

	(R7:4)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(vp6_filter_hv4_y)

///////////////////////////////////////////////////////////
#if	defined(__FDPIC__) && defined(USE_L1DATA)
.section .l1.data.A,"aw",@progbits
#else
.data
#endif

filtertmp: .space 256

DEFUN(vp6_filter_diag4,mL1,(uint8_t *dst, uint8_t *src, int stride, const int16_t *h_weights,const int16_t *v_weights)):
	SAV_R012;
    LINK 0;
    [--SP]=(R7:4,P5:3);

	B0 = R0;		//dst
	R1 = R1 - R2;
	R1 += -1;
	I0 = R1;		//src-stride-1
	R1 += 4;
	I1 = R1;
	M3 = R1;
	R2 += -4;
    M2 = R2;        //stride-4
	R2 += -4;
    M1 = R2;
	M0 = 1;

	R0 = [P3+filtertmp@GOT17M4];
	B1 = R0;
	I3 = R0;
	P2 = [FP+20];	//h_weights
	P3 = [FP+24];	//v_weights

	R7 = [P2++];
	R6 = [P2];
	R5.L = 64;
	R5.H = 64;
	P0 = 11;

	LSETUP(_vp6_filter_diag4_h_LB,_vp6_filter_diag4_h_LE)LC0=P0;
    _vp6_filter_diag4_h_LB:
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R0 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R1 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R0 = PACK(R3.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R1 = PACK(R3.L,R1.L) || I0-=M3;
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
        R3 = 0;
        R4 = BYTEOP3P(R1:0,R3:2)(LO);
        R0 = BYTEOP3P(R1:0,R3:2)(HI,R) || I0+=M3;
        R4 = R4 | R0;

		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2 || [I3++] = R4;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R0 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R1 = R3 + R2;
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R0 = PACK(R3.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		DISALGNEXCPT || R2 = [I0] || R3 = [I1];
		(R3, R2) = BYTEUNPACK R3:2;
		A1 = R7.L*R2.L, A0 = R7.H*R2.H(IS) || I0+=M0;
		R3 = (A1+=R6.L*R3.L), R2 = (A0+=R6.H*R3.H)(IS) || I1+=M0;
		R3 = R3 + R2;
		R1 = PACK(R3.L,R1.L) || I0-=M3;
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
        R3 = 0;
        R4 = BYTEOP3P(R1:0,R3:2)(LO);
        R0 = BYTEOP3P(R1:0,R3:2)(HI,R) || I0+=M3;
        R4 = R4 | R0;
		I0 += M1;
	_vp6_filter_diag4_h_LE:
		I1 += M1 || [I3++] = R4;

	I3 = B0;
	I0 = B1;
	M1 = 8;
	M3 = -20;
	R7 = [P3++];
    R6 = [P3] || R0 = [I0++M1];
	P0 = 8;
	LSETUP(_vp6_filter_diag4_v_LB,_vp6_filter_diag4_v_LE)LC0=P0;
    _vp6_filter_diag4_v_LB:
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I0++M1];
		A0 = R0.L*R7.L, A1 = R0.H*R7.L(IS);
		R4.L = R1.L*R7.L, R4.H = R1.H*R7.L(IS);
		(R3, R2) = BYTEUNPACK R3:2 || R0 = [I0++M1];
		A0 += R2.L*R7.H, A1 += R2.H*R7.H(IS);
		R3.L = R3.L*R7.H, R3.H = R3.H*R7.H(IS);
		R4 = R4 +|+ R3;
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I0++M3];
		A0 += R0.L*R6.L, A1 += R0.H*R6.L(IS);
		R1.L = R1.L*R6.L, R1.H = R1.H*R6.L(IS);
		R4 = R4 +|+ R1;
		(R3, R2) = BYTEUNPACK R3:2;
		R3.L = R3.L*R6.H, R3.H = R3.H*R6.H(IS);
		R4 = R4 +|+ R3;
		R0 = (A0+=R2.L*R6.H), R1 = (A1+=R2.H*R6.H)(IS);
		R0 = PACK(R4.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		R1 = PACK(R4.H,R1.L);
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
		R3 = 0;
		R4 = BYTEOP3P(R1:0,R3:2)(LO);
		R0 = BYTEOP3P(R1:0,R3:2)(HI,R);
		R4 = R4 | R0;

		R0 = [I0++M1];
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I0++M1];
		A0 = R0.L*R7.L, A1 = R0.H*R7.L(IS) || [I3++] = R4;
		R4.L = R1.L*R7.L, R4.H = R1.H*R7.L(IS);
		(R3, R2) = BYTEUNPACK R3:2 || R0 = [I0++M1];
		A0 += R2.L*R7.H, A1 += R2.H*R7.H(IS);
		R3.L = R3.L*R7.H, R3.H = R3.H*R7.H(IS);
		R4 = R4 +|+ R3;
		(R1, R0) = BYTEUNPACK R1:0 || R2 = [I0++M3];
		A0 += R0.L*R6.L, A1 += R0.H*R6.L(IS);
		R1.L = R1.L*R6.L, R1.H = R1.H*R6.L(IS);
		R4 = R4 +|+ R1;
		(R3, R2) = BYTEUNPACK R3:2;
		R3.L = R3.L*R6.H, R3.H = R3.H*R6.H(IS);
		R4 = R4 +|+ R3;
		R0 = (A0+=R0.L*R6.H), R1 = (A1+=R0.H*R6.H)(IS);
		R0 = PACK(R4.L,R0.L);
		R0 = R0 +|+ R5;
		R0 = R0 >>> 7(V);
		R1 = PACK(R4.H,R1.L);
		R1 = R1 +|+ R5;
		R1 = R1 >>> 7(V);
		R2 = 0;
        R3 = 0;
		R4 = BYTEOP3P(R1:0,R3:2)(LO);
		R0 = BYTEOP3P(R1:0,R3:2)(HI,R);
		R4 = R4 | R0;
	_vp6_filter_diag4_v_LE:
		[I3++M2] = R4 || R0 = [I0++M1];

	(R7:4,P5:3)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(vp6_filter_diag4)

///////////////////////////////////////////////////////////
DEFUN(vp56_edge_filter,mL1,(vp56_context_t *s, uint8_t *yuv,int pix_inc, int line_inc, int t)):
	SAV_R012;
    LINK 0;
    [--SP]=(R7:4,P5:3);

	P3 = R2;		//pix_inc
	P2 = P3 << 1;	//2*pix_inc
	P1 = P3 + P2;	//3*pix_inc
	P5 = R1;		//yuv
	P4 = [FP+20];	//line_inc
	R7 = [FP+24];	//t
	P0 = 12;
	R0 = 0;

	LSETUP(_vp56_edge_filter_LB,_vp56_edge_filter_LE)LC0=P0;
	_vp56_edge_filter_LB:
		R6 = B[P5];		//yuv[0]
		P5 -= P3;	
		R5 = B[P5];		//yuv[-pix_inc]
		R4 = R6 - R5;
		R3 = R4 << 1(S);
		R4 = R4 + R3;	//3*(yuv[0]-yuv[-pix_inc])
		P5 -= P3;   
		R3 = B[P5];		//yuv[-pix2_inc]
		R4 = R3 + R4;
		P5 = P5 + P1;
		R3 = B[P5];		//yuv[pix_inc]
		R4 = R4 - R3;
		R4 += 4;
		R4 >>>= 3;

		R3 = R4 >>> 31;
		R2 = R4 ^ R3;
		R2 = R2 - R3;
		R1 = R2 - R7;
		CC = R1 < R7(IU);
		R1 = R7 << 1(S);
		R2 = R1 - R2;
		R2 = R2 + R3;
		R2 = R2 ^ R3;
		IF CC R4=R2;

		R3 = 255;
		R6 = R6 - R4;	//yuv[0]
		R6 = MAX(R6,R0);
		R6 = MIN(R6,R3);
		R5 = R5 + R4;	//yuv[-pix_inc]
		R5 = MAX(R5,R0);
		R5 = MIN(R5,R3);
		P5 -= P2;
		B[P5] = R5;
		P5 = P5 + P3;
		B[P5] = R6;
	_vp56_edge_filter_LE:
		P5 = P5 + P4;

	(R7:4,P5:3)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(vp56_edge_filter)

///////////////////////////////////////////////////////////
DEFUN(put_pixels12,mL1,(uint8_t *block, const uint8_t *pixels, int line_size, int h)):
	LINK 0;
    [--SP] = (R7:6);
    I3 = R0;        // dest
    I0 = R1;        // pixels
	R1 += 4;
	I1 = R1;
    P0 = [FP+20];   // h

    R2 += -8;
    M3 = R2;        // line_size
    R2 += -4;
    M0 = R2;

    DISALGNEXCPT                || R0 = [I0++] || R1 = [I1++];
    LSETUP(_put_pixels12_LB,_put_pixels12_LE) LC0=P0;
	_put_pixels12_LB:  
    	R6 = BYTEOP1P(R1:0,R1:0)    || R0 = [I1++];
    	R7 = BYTEOP1P(R1:0,R1:0)(R) || R1 = [I1++M0] || [I3++] = R6;
    	R6 = BYTEOP1P(R1:0,R1:0)    || R0 = [I1++] || [I3++] = R7;
	_put_pixels12_LE:  
		DISALGNEXCPT                || R1 = [I1++] || [I3++M3] = R6;

    (R7:6) = [SP++];
    UNLINK;
    RTS;
DEFUN_END(put_pixels12)

///////////////////////////////////////////////////////////
DEFUN(vp56_get_vectors_predictors,mL1,(struct vp56_get_vectors_predictors *vp56_s_t, int row, int col, vp56_frame_t ref_frame, vp56_frame_t *vp56_frame, int8_t *vp56_pos)):
    SAV_R012;
    LINK 0;
    [--SP]=(R7:4,P5:3);

    P2 = [FP+24];   //vp56_reference_frame
    P3 = [FP+28];   //vp56_candidate_predictor_pos
    P5 = R0;

	R6 = [P5];		//s->mb_width;
	R5 = [P5+4];	//s->mb_height;
	P4 = [P5+8];	//s->macroblock;
	P1 = [P5+16];	//s->vector_candidate_pos;
	P5 = [P5+12];	//s->vector_candidate;
    R0 = 0;
    [P5] = R0;
    [P5+4] = R0;
    [P5+8] = R0;
    [P5+12] = R0;
    R7 = [FP+20];   //ref_frame
    P0 = 12;
    R0 = -1;
    I2 = 0;
    M0 = 1;

    LSETUP(_vp56_get_vectors_predictors_LB,_vp56_get_vectors_predictors_LE)LC0=P0;
    _vp56_get_vectors_predictors_LB:
        R0 += 1;
        P0 = R0;
        P0 = P3 + (P0<<1);  //vp56_candidate_predictor_pos[pos]
        R4 = B[P0](X);
        R4 = R4 + R2;       //mvp.x
        CC = R4<0;
        IF CC JUMP _vp56_get_vectors_predictors_LE;
        CC = R6<=R4;
        IF CC JUMP _vp56_get_vectors_predictors_LE;
        R3 = B[P0+1](X);
        R3 = R3 + R1;       //mvp.y
        CC = R3<0;
        IF CC JUMP _vp56_get_vectors_predictors_LE;
        CC = R5<=R3;
        IF CC JUMP _vp56_get_vectors_predictors_LE;
        R3 *= R6;
        R3 = R3 + R4;       //offset
        P0 = R3;
        P0 = P0 + (P0<<1);
        P0 = P4 + (P0<<2);  //s->macroblocks[offset]
        R4 = B[P0](Z);
		I3 = P0;
        P0 = R4;
        P0 = P2 + (P0<<2);
        R4 = [P0];
		P0 = I3;
        CC = R4==R7;
        IF !CC JUMP _vp56_get_vectors_predictors_LE;
        R4 = [P0+4];        //s->macroblocks[offset].mv.x
        R3 = [P5];
        CC = R4==R3;
        R3 = [P0+8];
        IF !CC JUMP _vp56_get_vectors_predictors_LM0;
        I0 = R4;
        R4 = [P5+4];
        CC = R3==R4;
        R4 = I0;
        IF CC JUMP _vp56_get_vectors_predictors_LE;
    _vp56_get_vectors_predictors_LM0:
        CC = R4==0;
        IF !CC JUMP _vp56_get_vectors_predictors_LM1;
        CC = R3==0;
        IF CC JUMP _vp56_get_vectors_predictors_LE;
    _vp56_get_vectors_predictors_LM1:
        I0 = R4;
        R4 = I2;
        CC = R4==0;
        R4 = I0;
        IF !CC JUMP _vp56_get_vectors_predictors_LM2;
        [P5] = R4;
        [P5+4] = R3;
        I2 += M0;
        [P1] = R0;
    _vp56_get_vectors_predictors_LE:
        NOP;

    R0 = I2;
    R0 += 1;
    (R7:4,P5:3)=[SP++];
    UNLINK;
    RTS;

    _vp56_get_vectors_predictors_LM2:
        [P5+8] = R4;
        [P5+12] = R3;

    R0 = 0;
    (R7:4,P5:3)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(vp56_get_vectors_predictors)

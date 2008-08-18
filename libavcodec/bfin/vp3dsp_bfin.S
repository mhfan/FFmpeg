/*
 * vp3,vp5,vp6 idct blackfin asm version
 * Author: SYP(byronshy@mail.ustc.edu.cn)
 * Date: 2007.8.13
 * Copyright: GPL v2.0 or later
*/

#include "config_bfin.h"

#define SAV_R012 [SP+0]=R0; [SP+4]=R1; [SP+8]=R2

#if	defined(__FDPIC__) && defined(USE_L1DATA)
.section .l1.data.B,"aw",@progbits
#else
.data
#endif

.align 4;
coefs:
.short	0x7d8a;	//xC1S7
.short	0x18f8;	//xC7S1
.short	0x6a6d;	//xC3S5
.short	0x471d;	//xC5S3
.short	0x5a82;	//xC4S4
.short	0x5a82;	//xC4S4
.short	0x0;
.short	0x404;
.short	0x0;
.short	0x4;
.short	0x7641;	//xC2S6
.short	0x30fc;	//xC6S2
.short	0x0;
.short	0x0;
.short	0xff;
.short	0xff;

#if	defined(__FDPIC__) && defined(USE_L1DATA)
.section .l1.data.A,"aw",@progbits
#endif

vtmp: .space 256

#define TMP0 FP-8
#define TMP1 FP-12
#define TMP2 FP-16

DEFUN(idct_row,mL1,(int16_t *input)):
	SAV_R012;
    LINK 0;
	[--SP]=(R7:4);

	I1 = R0;
	I2 = R0;
	M0 = 8;
	M1 = -20(X);
	M3 = 12;
	I2 += M0 || R3.L = W[I1++];
	P1 = [P3+coefs@GOT17M4];
	I3 = P1;
	P0 = 8;

	LSETUP(_idct_row_LB,_idct_row_LE)LC0=P0;
	_idct_row_LB:
		I0 = I1;
		R1.L = W[I1++] || R3.H = W[I2++];
		R2.L = W[I1++] || R0.H = W[I2++];
		R0.L = W[I1++] || R2.H = W[I2++];
		R7 = R3 | R2;
		R7 = R7 | R0;
		I1 += M0 || R1.H = W[I2++];
		R7 = R7 | R1;
		CC = R7==0;
		IF CC JUMP _idct_row_LE;

		I0 -= 2 || R7 = [I3++];
		A1 = R7.L*R1.L, A0 = R7.H*R1.L(IS);
		R5 = (A1+=R7.H*R1.H), R4 = (A0-=R7.L*R1.H)(IS) || R7 = [I3++];//A,B
		A1 = R7.L*R0.L, A0 = R7.L*R0.H(IS) || [I0++] = R4;
		R1 = (A1+=R7.H*R0.H), R0 = (A0-=R7.H*R0.L)(IS) || R7 = [I3++M3] || [I0] = R5;//C,D
		R5.L = R3.L + R3.H;
		R5.H = R3.L - R3.H;
		M2 = R7;
		R5 = (A1=R7.L*R5.L), R4 = (A0=R7.L*R5.H)(IS) || R7 = [I3++M1];//E,F
		A1 = R7.L*R2.L, A0 = R7.H*R2.L(IS) || R7 = [I0--];
		R3 = (A1+=R7.H*R2.H), R2 = (A0-=R7.L*R2.H)(IS) || R6 = [I0];//G,H
		
		R7 = R7 + R1, R1 = R7 - R1;//Cd,A-C
		R6 = R6 + R0, R0 = R6 - R0;//Dd,B-D
		R1 >>>= 15;
		R0 >>>= 15;
		R0 = PACK(R0.L,R1.L);
		R1 = M2;
		R1 = (A1=R1.L*R0.L), R0 = (A0=R1.L*R0.H)(IS);//Ad,Bd

		R5 = R5 + R3, R3 = R5 - R3;//Gd,Ed
		R4 = R4 + R1, R1 = R4 - R1;//Add,Fd
		R0 = R0 + R2, R2 = R0 - R2;//Hd,Bdd

		R5 = R5 + R7, R7 = R5 - R7;//0,7
		R4 = R4 + R0, R0 = R4 - R0;//1,2
		R3 = R3 + R6, R6 = R3 - R6;//3,4
		R1 = R1 + R2, R2 = R1 - R2;//5,6

		R5 >>>= 15;
		R4 >>>= 15;
		R0 >>>= 15;
		R3 >>>= 15;
		R6 >>>= 15;
		R1 >>>= 15;
		R2 >>>= 15;
		R7 >>>= 15;

		R5 = PACK(R4.L,R5.L);
		R0 = PACK(R3.L,R0.L) || [I0++] = R5;
		R6 = PACK(R1.L,R6.L) || [I0++] = R0;
		R2 = PACK(R7.L,R2.L) || [I0++] = R6;
		[I0++] = R2;
	_idct_row_LE:
		I2 += M0 || R3.L = W[I1++];

	(R7:4)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(idct_row)

////////////////////////////////////////////////////////////////////////
DEFUN(idct_column_put,mL1,(uint8_t *dst, int stride, int16_t *input)):
	SAV_R012;
    LINK 16;
	[--SP]=(R7:4,P5:3);

	P5 = R2;
	I2 = R2;
	P4 = R0;
	I1 = R0;
	M0 = 1;
	M1 = -28(X);
	M3 = 8;
	P1 = [P3+coefs@GOT17M4];
	I3 = P1;
	P3 = R1;
	P0 = 8;
	P1 = 16;

	R0 = 8;
	R0 <<= 15;
	B2 = R0;

	LSETUP(_idct_column_put_LB,_idct_column_put_LE)LC0=P0;
	_idct_column_put_LB:
		R3.H = W[P5++P1];
		R1.L = W[P5++P1];
		R2.L = W[P5++P1];
		R0.L = W[P5++P1];
		R3.L = W[P5++P1];
		R0.H = W[P5++P1];
		R2.H = W[P5++P1];
		R1.H = W[P5++P1];
		R4 = R3.L(X);
		R7 = R2 | R0;
		R7 = R7 | R4;
		R7 = R7 | R1;
		I2 += 2;
		P5 = I2;
		CC = R7==0;
		IF CC JUMP _idct_column_put_LM;

		R7 = [I3++];
		A1 = R7.L*R1.L, A0 = R7.H*R1.L(IS);
		R5 = (A1+=R7.H*R1.H), R4 = (A0-=R7.L*R1.H)(IS) || R7 = [I3++];//A,B
		A1 = R7.L*R0.L, A0 = R7.L*R0.H(IS) || [TMP0] = R5;
		R1 = (A1+=R7.H*R0.H), R0 = (A0-=R7.H*R0.L)(IS) || R7 = [I3++] || [TMP1] = R4;//C,D
		R5.L = R3.H + R3.L;
		R5.H = R3.H - R3.L;
		R5 = (A1=R7.L*R5.L), R4 = (A0=R7.L*R5.H)(IS) || R3 = [I3++M3] || [TMP2] = R7;//E,F
		R5 = R5 + R3;
		R4 = R4 + R3(NS) || R7 = [I3++];
		A1 = R7.L*R2.L, A0 = R7.H*R2.L(IS) || R6 = [TMP1];
		R3 = (A1+=R7.H*R2.H), R2 = (A0-=R7.L*R2.H)(IS) || R7 = [TMP0];//G,H
		
		R7 = R7 + R1, R1 = R7 - R1;//Cd,A-C
		R6 = R6 + R0, R0 = R6 - R0;//Dd,B-D
		R1 >>>= 15;
		R0 >>>= 15;
		R0 = PACK(R0.L,R1.L) || R1 = [TMP2];
		R1 = (A1=R1.L*R0.L), R0 = (A0=R1.L*R0.H)(IS);//Ad,Bd

		R5 = R5 + R3, R3 = R5 - R3;//Gd,Ed
		R4 = R4 + R1, R1 = R4 - R1;//Add,Fd
		R0 = R0 + R2, R2 = R0 - R2;//Hd,Bdd

		R5 = R5 + R7, R7 = R5 - R7;//0,7
		R4 = R4 + R0, R0 = R4 - R0;//1,2
		R3 = R3 + R6, R6 = R3 - R6;//3,4
		R1 = R1 + R2, R2 = R1 - R2;//5,6

		R5 >>>= 19;
		R4 >>>= 19;
		R0 >>>= 19;
		R3 >>>= 19;
		R6 >>>= 19;
		R1 >>>= 19;
		R2 >>>= 19;
		R7 >>>= 19;

		R5 = PACK(R4.L,R5.L);//0,1
		R0 = PACK(R3.L,R0.L);//2,3
		R6 = PACK(R1.L,R6.L) || R4 = [I3++];//4,5
		R2 = PACK(R7.L,R2.L) || R3 = [I3++M1];//6,7

		R7 = MAX(R5,R4)(V);
		R5 = MIN(R7,R3)(V);
		R7 = MAX(R0,R4)(V) || B[P4] = R5;
		P4 = P4 + P3;
		R5 >>= 16;
		R0 = MIN(R7,R3)(V) || B[P4] = R5;
		P4 = P4 + P3;
		R7 = MAX(R6,R4)(V) || B[P4] = R0;
		P4 = P4 + P3;
		R0 >>= 16;
		R6 = MIN(R7,R3)(V) || B[P4] = R0;
		P4 = P4 + P3;
		R7 = MAX(R2,R4)(V) || B[P4] = R6;
		P4 = P4 + P3;
		R6 >>= 16;
		R2 = MIN(R7,R3)(V) || B[P4] = R6;
		P4 = P4 + P3;
		B[P4] = R2;
		P4 = P4 + P3;
		R2 >>= 16;
		B[P4] = R2;
		I1 += M0;
		JUMP _idct_column_put_LE;

	_idct_column_put_LM:
		R7.L = 0x5a82;
		R7 = R7.L*R3.H(IS);
		R6 = B2;
		R7 = R7 + R6;
		R7 >>>= 19;
		R6 = 128;
		R7 = R7 + R6;

		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		P4 = P4 + P3;
		B[P4] = R7;
		I1 += M0;
	_idct_column_put_LE:
		P4 = I1;

	(R7:4,P5:3)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(idct_column_put)

////////////////////////////////////////////////////////////////////////
DEFUN(idct_column_add,mL1,(uint8_t *dst, int stride, int16_t *input)):
	SAV_R012;
    LINK 16;
	[--SP]=(R7:4,P5:3);

	P5 = R2;
	I2 = R2;
	P4 = R0;
	I1 = R0;
	M0 = 1;
	M1 = -28(X);
	M3 = 8;
	P1 = [P3+coefs@GOT17M4];
	I3 = P1;
	P0 = 8;
	P1 = 16;
	P3 = R1;

	R0 = 8;
	R0 <<= 15;
	B2 = R0;

	LSETUP(_idct_column_add_LB,_idct_column_add_LE)LC0=P0;
	_idct_column_add_LB:
		R3.H = W[P5++P1];
		R1.L = W[P5++P1];
		R2.L = W[P5++P1];
		R0.L = W[P5++P1];
		R3.L = W[P5++P1];
		R0.H = W[P5++P1];
		R2.H = W[P5++P1];
		R1.H = W[P5++P1];
		R4 = R3.L(X);
		R7 = R2 | R0;
		R7 = R7 | R4;
		R7 = R7 | R1;
		I2 += 2;
		P5 = I2;
		CC = R7==0;
		IF CC JUMP _idct_column_add_LM;

		R7 = [I3++];
		A1 = R7.L*R1.L, A0 = R7.H*R1.L(IS);
		R5 = (A1+=R7.H*R1.H), R4 = (A0-=R7.L*R1.H)(IS) || R7 = [I3++];//A,B
		A1 = R7.L*R0.L, A0 = R7.L*R0.H(IS) || [TMP0] = R5;
		R1 = (A1+=R7.H*R0.H), R0 = (A0-=R7.H*R0.L)(IS) || R7 = [I3++M3] ||[TMP1] = R4;//C,D
		R5.L = R3.H + R3.L;
		R5.H = R3.H - R3.L;
		R5 = (A1=R7.L*R5.L), R4 = (A0=R7.L*R5.H)(IS) || R3 = [I3++] || [TMP2] = R7;//E,F
		R5 = R5 + R3;
		R4 = R4 + R3(NS) || R7 = [I3++];
		A1 = R7.L*R2.L, A0 = R7.H*R2.L(IS) || R6 = [TMP1];
		R3 = (A1+=R7.H*R2.H), R2 = (A0-=R7.L*R2.H)(IS) || R7 = [TMP0];//G,H
		
		R7 = R7 + R1, R1 = R7 - R1;//Cd,A-C
		R6 = R6 + R0, R0 = R6 - R0;//Dd,B-D
		R1 >>>= 15;
		R0 >>>= 15;
		R0 = PACK(R0.L,R1.L) || R1 = [TMP2];
		R1 = (A1=R1.L*R0.L), R0 = (A0=R1.L*R0.H)(IS);//Ad,Bd

		R5 = R5 + R3, R3 = R5 - R3;//Gd,Ed
		R4 = R4 + R1, R1 = R4 - R1;//Add,Fd
		R0 = R0 + R2, R2 = R0 - R2;//Hd,Bdd

		R5 = R5 + R7, R7 = R5 - R7;//0,7
		R4 = R4 + R0, R0 = R4 - R0;//1,2
		R3 = R3 + R6, R6 = R3 - R6;//3,4
		R1 = R1 + R2, R2 = R1 - R2;//5,6

		R5 >>>= 19;
		R4 >>>= 19;
		R0 >>>= 19;
		R3 >>>= 19;
		R6 >>>= 19;
		R1 >>>= 19;
		R2 >>>= 19;
		R7 >>>= 19;

		R5 = PACK(R4.L,R5.L);//0,1
        R0 = PACK(R3.L,R0.L);//2,3
        R6 = PACK(R1.L,R6.L) || R4 = [I3++];//4,5
        R2 = PACK(R7.L,R2.L) || R3 = [I3++M1];//6,7
		P2 = P4;
		
		R7 = B[P2];
		P2 = P2 + P3;
		R1 = B[P2];
		P2 = P2 + P3;
		R1 = PACK(R1.L,R7.L);
		R5 = R5 +|+ R1;
		R1 = MAX(R5,R4)(V) || R7 = B[P2];
		P2 = P2 + P3;
		R5 = MIN(R1,R3)(V) || R1 = B[P2];
		P2 = P2 + P3;
		R1 = PACK(R1.L,R7.L) || B[P4] = R5;
		P4 = P4 + P3;
		R5 >>= 16;
		R0 = R0 +|+ R1 || B[P4] = R5;
		P4 = P4 + P3;
		R1 = MAX(R0,R4)(V) || R7 = B[P2];
		P2 = P2 + P3;
		R0 = MIN(R1,R3)(V) || R1 = B[P2];
		P2 = P2 + P3;
		R1 = PACK(R1.L,R7.L) || B[P4] = R0;
		P4 = P4 + P3;
		R0 >>= 16;
		R6 = R6 +|+ R1 || B[P4] = R0;
		P4 = P4 + P3;
		R1 = MAX(R6,R4)(V) || R7 = B[P2];
		P2 = P2 + P3;
		R6 = MIN(R1,R3)(V) || R1 = B[P2];
		R1 = PACK(R1.L,R7.L) || B[P4] = R6;
		P4 = P4 + P3;
		R6 >>= 16;
		R2 = R2 +|+ R1 || B[P4] = R6;
		P4 = P4 + P3;
		R1 = MAX(R2,R4)(V);
		R2 = MIN(R1,R3)(V);
		B[P4] = R2;
		P4 = P4 + P3;
		R2 >>= 16;
		B[P4] = R2;

		I1 += M0;
		JUMP _idct_column_add_LE;

	_idct_column_add_LM:
		R3.L = 0;
		CC = R3==0;
		I1 += M0;
		IF CC JUMP _idct_column_add_LE;
		R7.L = 0x5a82;
		R7 = R7.L*R3.H(IS) || I3 -= M1;
		R6 = B2;
		R7 = R7 + R6;
		R7 >>>= 19;
		R7 = PACK(R7.L,R7.L) || R4 = [I3--];
		P2 = P4;

		R3 = B[P2] || R5 = [I3++];
		P2 = P2 + P3;
		R2 = B[P2];
		P2 = P2 + P3;
		R2 = PACK(R2.L,R3.L) || R1 = B[P2];
		P2 = P2 + P3;
		R2 = R7 +|+ R2 || R0 = B[P2];
		P2 = P2 + P3;
		R6 = MAX(R2,R5)(V) || I3 += M1;
		R2 = MIN(R6,R4)(V);
		R0 = PACK(R0.L,R1.L) || B[P4] = R2;
		P4 = P4 + P3;
		R2 >>= 16;
		R0 = R7 +|+ R0 || B[P4] = R2;
		P4 = P4 + P3;
		R6 = MAX(R0,R5)(V) || R3 = B[P2];
		P2 = P2 + P3;
		R0 = MIN(R6,R4)(V) || R2 = B[P2];
		P2 = P2 + P3;
		R2 = PACK(R2.L,R3.L) || B[P4] = R0;
		P4 = P4 + P3;
		R0 >>= 16;
		R2 = R7 +|+ R2 || B[P4] = R0;
		P4 = P4 + P3;
		R6 = MAX(R2,R5)(V) || R1 = B[P2];
		P2 = P2 + P3;
		R2 = MIN(R6,R4)(V) || R0 = B[P2];
		R0 = PACK(R0.L,R1.L) || B[P4] = R2;
		P4 = P4 + P3;
		R2 >>= 16;
		R0 = R7 +|+ R0 || B[P4] = R2;
		P4 = P4 + P3;
		R6 = MAX(R0,R5)(V);
		R0 = MIN(R6,R4)(V);
		B[P4] = R0;
		P4 = P4 + P3;
		R0 >>= 16;
		B[P4] = R0;
	_idct_column_add_LE:
		P4 = I1;

	(R7:4,P5:3)=[SP++];
    UNLINK;
    RTS;
DEFUN_END(idct_column_add)

////////////////////////////////////////////////////////////////////////

/*
 * Blackfin asm optimizations for halfpel motion compensation avg functions.
 * Author: Shi Yipeng, HHTech, 2006-11
 * Minor reformatting: Jason Geng, HHTech, 2007-7
 *
 * This file is part of FFmpeg.
 */

#include "config_bfin.h"

#define SAV_R012 [SP+0]=R0; [SP+4]=R1; [SP+8]=R2

DEFUN(avg_pixels8, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=R7;

        I1 = R0;        //block,R0 is free
        I3 = R0;
        I0 = R1;        //pixels
        BITCLR(R1,0);
        BITCLR(R1,1);
        I2 = R1;        //pixels&~3,R1 is free

        R2 += -4;
        M0 = R2;
        R2 += -4;
        M1 = R2;

        P0 = [FP+20] || R0 = [I2++];   //h

        LSETUP(_avg_pixels8_bfin_LB,_avg_pixels8_bfin_LE) LC0=P0;
        _avg_pixels8_bfin_LB:
		R1 = [I2++] || R2 = [I3++];
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++M1] || R3 = [I3++M0];
                R7 = BYTEOP1P(R1:0,R3:2)(R) || [I1++] = R7 || R0 = [I2++];
        _avg_pixels8_bfin_LE:
                [I1++M0] = R7;

        R7=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_pixels8)
//////////////////////////////////////////////////////////

DEFUN(avg_pixels8_x2, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=P5;

        P1 = R0;        //block,R0 is free
        P2 = R0;

        I0 = 0;
        I1 = 0;
        I2 = R1;        //pixels
        R1 += 1;
        I3 = R1;

        R2 += -4;
        M1 = R2;
        P5 = R2;
        P0 = [FP+20];   //h

        DISALGNEXCPT || R0 = [I2++] || R2 = [I3++];
        R0 = BYTEOP1P(R1:0,R3:2) || R2 = [P1++];
        R3 = BYTEOP1P(R1:0,R3:2);
        LSETUP(_avg_pixels8_x2_bfin_LB,_avg_pixels8_x2_bfin_LE) LC0=P0;
        _avg_pixels8_x2_bfin_LB:
                DISALGNEXCPT || R0 = [I2++M1] || R2 = [I3++M1];
                R0 = BYTEOP1P(R1:0,R3:2) || R2 = [P1++P5];
                R3 = BYTEOP1P(R1:0,R3:2) || [P2++] = R3;
                DISALGNEXCPT || R0 = [I2++] || R2 = [I3++];
                R0 = BYTEOP1P(R1:0,R3:2) || R2 = [P1++];
        _avg_pixels8_x2_bfin_LE:
                R3 = BYTEOP1P(R1:0,R3:2) || [P2++P5] = R3;

        P5=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_pixels8_x2)
//////////////////////////////////////////////////////////

DEFUN(avg_no_rnd_pixels8_x2, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:6);

        I3 = R0;        //block,R0 is free

        I0 = R1;        //pixels
        R0 = R1;
        BITCLR(R1,0);
        BITCLR(R1,1);
        R0 = R0 - R1;
        CC = R0==3;
        M0 = R0;
        I2 = R1;        //pixels&~3,R1 is free

        R2 += -4;
        M1 = R2;
        R2 += -4;
        M2 = R2;

        P1 = [FP+20];   //h

        IF CC JUMP _AVG_NO_RND_PIXELS8_X2;
        R0 += 1;
        M3 = R0;
        I1 = R0;

        R0 = [I2] || R2 = [I2++];
        R1 = [I2] || R3 = [I2++];

        LSETUP(_avg_no_rnd_pixels8_x2_bfin_LB,_avg_no_rnd_pixels8_x2_bfin_LE) LC0=P1;
        _avg_no_rnd_pixels8_x2_bfin_LB:
                R0 = BYTEOP1P(R1:0,R3:2)(T) || R2 = [I3++] || I0 -= M0;
                I1 -= M3;
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2] || I0 += M0;
                I1 += M3 || R2 = [I2++M2];
                R0 = BYTEOP1P(R1:0,R3:2)(T,R) || R2 = [I3--] || I0 -= M0;
                I1 -= M3 || [I3++] = R7;
                R6 = BYTEOP1P(R1:0,R3:2) || R0 = [I2] || I0 += M0;
                I1 += M3;
                R2 = [I2++] || [I3++M1] = R6;
        _avg_no_rnd_pixels8_x2_bfin_LE:
                R1 = [I2] || R3 = [I2++];

        (R7:6)=[SP++];
        UNLINK;
        RTS;

_AVG_NO_RND_PIXELS8_X2:
        I1 = 0;
        R0 = [I2++];
        R1 = [I2] || R2 = [I2++];

        LSETUP(_avg_no_rnd_pixels8_x2_bfin1_LB,_avg_no_rnd_pixels8_x2_bfin1_LE) LC0=P1;
        _avg_no_rnd_pixels8_x2_bfin1_LB:
                R0 = BYTEOP1P(R1:0,R3:2)(T) || R2 = [I3] || I0 -= M0;
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2] || I0 += M0;
                R3 = [I2++M2] || [I3++] = R7;
                R0 = BYTEOP1P(R1:0,R3:2)(T,R) || R2 = [I3] || I0 -= M0;
                R6 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++] || I0 += M0;
                R1 = [I2] || R2 = [I2++];
        _avg_no_rnd_pixels8_x2_bfin1_LE:
                [I3++M1] = R6;

        (R7:6)=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_no_rnd_pixels8_x2)
//////////////////////////////////////////////////////////

DEFUN(avg_pixels8_y2, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:6);

        P0 = R0;        //block,R0 is free

        I0 = R1;        //pixels
        I1 = R1;
        M0 = R1;
        BITCLR(R1,0);
        BITCLR(R1,1);
        I2 = R1;        //pixels&~3,R1 is free
        R1 = R1 + R2;
        I3 = R1;

        R2 += -4;
        P2 = R2;
        R2 += -4;
        M2 = R2;

        P1 = [FP+20];   //h

        R0 = [I2++] || R2 = [I3++];
        R1 = [I2++] || R3 = [I3++];

        LSETUP(_avg_pixels8_y2_bfin_LB,_avg_pixels8_y2_bfin_LE) LC0=P1;
        _avg_pixels8_y2_bfin_LB:
                R0 = BYTEOP1P(R1:0,R3:2) || R2 = [P0++];
                I0 -= M0;
                I1 -= M0;
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++M2] || I0 += M0;
                I1 += M0 || R2 = [I3++M2];
                R0 = BYTEOP1P(R1:0,R3:2)(R) || R2 = [P0--];
                I0 -= M0;
                I1 -= M0;
                R6 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++] || I0 += M0;
                I1 += M0 || R2 = [I3++];
                R1 = [I2++] || [P0++] = R7;
        _avg_pixels8_y2_bfin_LE:
                R3 = [I3++] || [P0++P2] = R6;

        (R7:6)=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_pixels8_y2)
//////////////////////////////////////////////////////////

DEFUN(avg_no_rnd_pixels8_y2, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:6);

        P0 = R0;        //block,R0 is free

        I0 = R1;        //pixels
        I1 = R1;
        M0 = R1;
        BITCLR(R1,0);
        BITCLR(R1,1);
        I2 = R1;        //pixels&~3,R1 is free
        R1 = R1 + R2;
        I3 = R1;

        R2 += -4;
        P2 = R2;
        R2 += -4;
        M2 = R2;

        P1 = [FP+20];   //h

        R0 = [I2++] || R2 = [I3++];
        R1 = [I2++] || R3 = [I3++];

        LSETUP(_avg_no_rnd_pixels8_y2_bfin_LB,_avg_no_rnd_pixels8_y2_bfin_LE) LC0=P1;
        _avg_no_rnd_pixels8_y2_bfin_LB:
                R0 = BYTEOP1P(R1:0,R3:2)(T) || R2 = [P0++];
                I0 -= M0;
                I1 -= M0;
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++M2] || I0 += M0;
                I1 += M0 || R2 = [I3++M2];
                R0 = BYTEOP1P(R1:0,R3:2)(T,R) || R2 = [P0--];
                I0 -= M0;
                I1 -= M0;
                R6 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++] || I0 += M0;
                I1 += M0 || R2 = [I3++];
                R1 = [I2++] || [P0++] = R7;
        _avg_no_rnd_pixels8_y2_bfin_LE:
                R3 = [I3++] || [P0++P2] = R6;

        (R7:6)=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_no_rnd_pixels8_y2)
//////////////////////////////////////////////////////////

DEFUN(avg_pixels8_xy2, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:5);

        I3 = R0;        //block,R0 is free
        R0 += 4;
        P2 = R0;

        R0 = R1;
        I0 = R1;
        BITCLR(R1,0);
        BITCLR(R1,1);
        I1 = R1;        //pixels
        R0 = R0 - R1;
        CC = R0==3;
        R1 += 4;
        I2 = R1;
        R5 = R1;

        M0 = R2;
        M1 = 1;
        M2 = R0;
        R0 += 1;
        M3 = R0;
        P0 = [FP+20];      //h
        P1 = 2;

        IF CC JUMP _AVG_PIXLES8_XY2;

        LSETUP(_avg_pixels8_xy2_bfin11_LB,_avg_pixels8_xy2_bfin11_LE) LC0=P1;
        _avg_pixels8_xy2_bfin11_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1] || R3 = [I2++M0];

        LSETUP(_avg_pixels8_xy2_bfin12_LB,_avg_pixels8_xy2_bfin12_LE) LC1=P0>>1;
        _avg_pixels8_xy2_bfin12_LB:
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH) || I0 -= M3;
                R0 = R6 + R7(NS) || R2 = [I3] || R1 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || I0 += M3 || R2 = [I1++M0];
                R0 = [I1] || [I3++M0] = R7;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDH) || I0 -= M1;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 -= M2;
                R0 = R6 + R7(NS) || R2 = [I3] || R3 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || I0 += M2 || R0 = [I1++M0];
        _avg_pixels8_xy2_bfin12_LE:
                R2 = [I1] || [I3++M0] = R7;

                I3 = P2;
                I1 = R5;
                R5 += 4;
        _avg_pixels8_xy2_bfin11_LE:I2 = R5;

        (R7:5)=[SP++];
        UNLINK;
        RTS;

_AVG_PIXLES8_XY2:

        LSETUP(_avg_pixels8_xy2_bfin21_LB,_avg_pixels8_xy2_bfin21_LE) LC0=P1;
        _avg_pixels8_xy2_bfin21_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1] || R3 = [I2++M0];

        LSETUP(_avg_pixels8_xy2_bfin22_LB,_avg_pixels8_xy2_bfin22_LE) LC1=P0>>1;
        _avg_pixels8_xy2_bfin22_LB:
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH,R);
                R0 = R6 + R7(NS) || R2 = [I3] || R1 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || R2 = [I1++M0];
                R0 = [I1] || [I3++M0] = R7;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDH,R) || I0 -= M1;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
                R0 = R6 + R7(NS) || R2 = [I3] || R3 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || I0 -= M1 || R0 = [I1++M0];
        _avg_pixels8_xy2_bfin22_LE:
                R2 = [I1] || [I3++M0] = R7;

                I3 = P2;
                I1 = R5;
                R5 += 4;
        _avg_pixels8_xy2_bfin21_LE:I2 = R5;

        (R7:5)=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_pixels8_xy2)
//////////////////////////////////////////////////////////

DEFUN(avg_no_rnd_pixels8_xy2, mL1,
		(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h)):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:5);

        I3 = R0;        //block,R0 is free
        R0 += 4;
        P2 = R0;

        R0 = R1;
        I0 = R1;
        BITCLR(R1,0);
        BITCLR(R1,1);
        I1 = R1;        //pixels
        R0 = R0 - R1;
        CC = R0==3;
        R1 += 4;
        I2 = R1;
        R5 = R1;

        M0 = R2;
        M1 = 1;
        M2 = R0;
        R0 += 1;
        M3 = R0;
        P0 = [FP+20];      //h
        P1 = 2;

        IF CC JUMP _AVG_NO_RND_PIXLES8_XY2;

        LSETUP(_avg_no_rnd_pixels8_xy2_bfin11_LB,_avg_no_rnd_pixels8_xy2_bfin11_LE) LC0=P1;
        _avg_no_rnd_pixels8_xy2_bfin11_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1] || R3 = [I2++M0];

        LSETUP(_avg_no_rnd_pixels8_xy2_bfin12_LB,_avg_no_rnd_pixels8_xy2_bfin12_LE) LC1=P0>>1;
        _avg_no_rnd_pixels8_xy2_bfin12_LB:
                R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
                R6 = BYTEOP2P(R1:0,R3:2)(TH) || I0 -= M3;
                R0 = R6 + R7(NS) || R2 = [I3] || R1 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || I0 += M3 || R2 = [I1++M0];
                R0 = [I1] || [I3++M0] = R7;
                R7 = BYTEOP2P(R1:0,R3:2)(TH) || I0 -= M1;
                R6 = BYTEOP2P(R1:0,R3:2)(TL) || I0 -= M2;
                R0 = R6 + R7(NS) || R2 = [I3] || R3 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || I0 += M2 || R0 = [I1++M0];
        _avg_no_rnd_pixels8_xy2_bfin12_LE:
                R2 = [I1] || [I3++M0] = R7;

                I3 = P2;
                I1 = R5;
                R5 += 4;
        _avg_no_rnd_pixels8_xy2_bfin11_LE:I2 = P5;

        (R7:5)=[SP++];
        UNLINK;
        RTS;

_AVG_NO_RND_PIXLES8_XY2:

        LSETUP(_avg_no_rnd_pixels8_xy2_bfin21_LB,_avg_no_rnd_pixels8_xy2_bfin21_LE) LC0=P1;
        _avg_no_rnd_pixels8_xy2_bfin21_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1] || R3 = [I2++M0];

        LSETUP(_avg_no_rnd_pixels8_xy2_bfin22_LB,_avg_no_rnd_pixels8_xy2_bfin22_LE) LC1=P0>>1;
        _avg_no_rnd_pixels8_xy2_bfin22_LB:
                R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
                R6 = BYTEOP2P(R1:0,R3:2)(TH,R);
                R0 = R6 + R7(NS) || R2 = [I3] || R1 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || R2 = [I1++M0];
                R0 = [I1] || [I3++M0] = R7;
                R7 = BYTEOP2P(R1:0,R3:2)(TH,R) || I0 -= M1;
                R6 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
                R0 = R6 + R7(NS) || R2 = [I3] || R3 = [I2++M0];
                R7 = BYTEOP1P(R1:0,R3:2) || I0 -= M1 || R0 = [I1++M0];
        _avg_no_rnd_pixels8_xy2_bfin22_LE:
                R2 = [I1] || [I3++M0] = R7;

                I3 = P2;
                I1 = R5;
                R5 += 4;
        _avg_no_rnd_pixels8_xy2_bfin21_LE:I2 = P5;

        (R7:5)=[SP++];
        UNLINK;
        RTS;
DEFUN_END(avg_no_rnd_pixels8_xy2)

/*vim:ts=8:*/ 

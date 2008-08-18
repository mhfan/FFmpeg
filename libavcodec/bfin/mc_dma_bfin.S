#include "config_bfin.h"
#define SAV_R012 [SP+0]=R0; [SP+4]=R1; [SP+8]=R2
DECL_FUNC(interpolate8x8_new_halfpel_h):
        SAV_R012;
        LINK 0;

        I3 = R0;        //block,R0 is free
        I0 = R2;//pixels
	R0=R2;
         BITCLR(R2,0);
	    I2 = R2;        //pixels&~3,R1 is free
        R3=[FP+20];
	R2=[FP+24];
        CC = R2==0;
       R0+=1;
        I1 = R0;
        P0 = 8;   //h
        R3 += -8;
        R1+=4;
        M1 = R3;        //line_size-8
        M2=R1;
        IF !CC JUMP _PUT_PIXELS8_X2;

        R0 = [I2++];
        R1 = [I2++];
        LSETUP(_put_pixels8_x2_bfin_LB,_put_pixels8_x2_bfin_LE) LC0=P0;
        _put_pixels8_x2_bfin_LB:
                R3 = BYTEOP1P(R1:0,R1:0) || R0 = [I2++M1];
                R3 = BYTEOP1P(R1:0,R1:0)(R) || R0 = [I2++] || [I3++] = R3;
        _put_pixels8_x2_bfin_LE:
		R1 = [I2++] || [I3++M2] = R3;
        UNLINK;
        RTS;

_PUT_PIXELS8_X2:

        R0 = [I2++];
        R1 = [I2++] ;
        LSETUP(_put_no_rnd_pixels8_x2_bfin_LB,_put_no_rnd_pixels8_x2_bfin_LE) LC0=P0;
        _put_no_rnd_pixels8_x2_bfin_LB:
                R3 = BYTEOP1P(R1:0,R1:0)(T) || R0 = [I2++M1];
                R3 = BYTEOP1P(R1:0,R1:0)(T,R) || R0 = [I2++] || [I3++] = R3;
        _put_no_rnd_pixels8_x2_bfin_LE:
                R1 = [I2++] || [I3++M2] = R3;

        UNLINK;
        RTS;
END_FUNC(interpolate8x8_new_halfpel_h)
DECL_FUNC(interpolate8x8_new_halfpel_v):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:6);

        P1 = R0;        //block,R0 is free
        I0 = R2;        //pixels
        I1 = R2;
	R3=[FP+20];
        R7=[FP+24];
        CC=R7==0;
        BITCLR(R2,0);
      //  BITCLR(R1,1);
        I2 = R2;        //pixels&~3,R1 is free
        R2 = R2 + R3;
        I3 = R2;
        P0 = 8;   //h

        R3 += -8;
	R1+=4;
         
        M0 = R3;
        P2=R1;
        R0 = [I2++] || R2 = [I3++];
        R1 = [I2++] || R3 = [I3++];
        IF !CC JUMP _PUT_PIXELS8_Y2;

        LSETUP(_put_pixels8_y2_bfin_LB,_put_pixels8_y2_bfin_LE) LC0=P0;
        _put_pixels8_y2_bfin_LB:
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++M0] || R2 = [I3++M0];
                R6 = BYTEOP1P(R1:0,R3:2)(R) || [P1++] = R7 || R0 = [I2++];
                [P1++P2] = R6 || R2 = [I3++];
        _put_pixels8_y2_bfin_LE:
                R1 = [I2++] || R3 = [I3++];

        (R7:6)=[SP++];
        UNLINK;
        RTS;
        _PUT_PIXELS8_Y2:
                LSETUP(_put_no_rnd_pixels8_y2_bfin_LB,_put_no_rnd_pixels8_y2_bfin_LE) LC0=P0;
        _put_no_rnd_pixels8_y2_bfin_LB:
                R7 = BYTEOP1P(R1:0,R3:2)(T) || R0 = [I2++M0] || R2 = [I3++M0];
                R6 = BYTEOP1P(R1:0,R3:2)(T,R) || [P1++] = R7 || R0 = [I2++];
                [P1++P2] = R6 || R2 = [I3++];
        _put_no_rnd_pixels8_y2_bfin_LE:
                R1 = [I2++] || R3 = [I3++];

        (R7:6)=[SP++];
        UNLINK;
        RTS;

END_FUNC(interpolate8x8_new_halfpel_v)
DECL_FUNC(interpolate8x8_new_halfpel_hv):
         SAV_R012;
        LINK 0;
        [--SP]=(R7:5);

        I3 = R0;        //block,R0 is free
        R0 += 4;
        P2 = R0;
        P1 = 2;
        P0 = 4;   //hI
	R3=[FP+20];
        R6=[FP+24];
        CC=R6==0;
        R5 = R2;
        I0 = R2;
	BITCLR(R5,0);
        I1 = R5;        //pixels
        R5 += 4;
        I2 = R5;
        R1+=8;
        M0 = R3;
        M1 = 1;
        M2=R1;
        IF !CC JUMP _PUT_PIXLES8_XY2;

        LSETUP(_put_pixels8_xy2_bfin11_LB,_put_pixels8_xy2_bfin11_LE) LC0=P1;
        _put_pixels8_xy2_bfin11_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1++M0] || R3 = [I2++M0];

        R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
        LSETUP(_put_pixels8_xy2_bfin12_LB,_put_pixels8_xy2_bfin12_LE) LC1=P0;
        _put_pixels8_xy2_bfin12_LB:
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH) || R0 = [I1++M0] || R1 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDH) || I0 -= M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDL) || R2 = [I1++M0] || R3 = [I2++M0];
                R7 = R6 | R7;
        _put_pixels8_xy2_bfin12_LE:
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1 || [I3++M2] = R7;

                I3 = P2;
                I1 = R5;
                R5 += 4;
                I2 = R5;
        _put_pixels8_xy2_bfin11_LE:I0 -= M1;

        (R7:5)=[SP++];
        UNLINK;
        RTS;

_PUT_PIXLES8_XY2:
            LSETUP(_put_no_rnd_pixels8_xy2_bfin11_LB,_put_no_rnd_pixels8_xy2_bfin11_LE) LC0=P1;
        _put_no_rnd_pixels8_xy2_bfin11_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1++M0] || R3 = [I2++M0];

        R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
        LSETUP(_put_no_rnd_pixels8_xy2_bfin12_LB,_put_no_rnd_pixels8_xy2_bfin12_LE) LC1=P0;
        _put_no_rnd_pixels8_xy2_bfin12_LB:
                R6 = BYTEOP2P(R1:0,R3:2)(TH) || R0 = [I1++M0] || R1 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(TH) || I0 -= M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(TL) || R2 = [I1++M0] || R3 = [I2++M0];
                R7 = R6 | R7;
        _put_no_rnd_pixels8_xy2_bfin12_LE:
                R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1 || [I3++M2] = R7;

                I3 = P2;
                I1 = R5;
                R5 += 4;
                I2 = R5;
        _put_no_rnd_pixels8_xy2_bfin11_LE:I0 -= M1;

        (R7:5)=[SP++];
        UNLINK;
        RTS;


END_FUNC(interpolate8x8_new_halfpel_hv)	
DECL_FUNC(transfer8x8_copy_new):
        SAV_R012;
        LINK 0;

        I2 = R0;        //block,R0 is free
        I0 = R2;        //pixels
//        I1 = R2;        //pixels&~3,R1 is free
	BITCLR(R2,0);
	I3=R2;
        P0 = 8;   //h
        R3=[FP+20];
        R3 += -8;
        R1+=4;
        M1 = R3;
        M2=R1;
        R0 = [I3++];
        R1 = [I3++];
        LSETUP(_put_pixels8_bfin_LB,_put_pixels8_bfin_LE) LC0=P0;
        _put_pixels8_bfin_LB:
                (R3,R2)=BYTEUNPACK R1:0 || R0 = [I3++M1];
                R3 = BYTEPACK(R2,R3);
                (R3,R2)=BYTEUNPACK R1:0(R) || R0 = [I3++] || [I2++]=R3;
                R3 = BYTEPACK(R2,R3) || R1 = [I3++];
        _put_pixels8_bfin_LE:
		[I2++M2]=R3;

        UNLINK;
        RTS;
END_FUNC(transfer8x8_copy_new)
DECL_FUNC(transfer16x16_copy_new):
        SAV_R012;
        LINK 0;

        I2 = R0;        //block,R0 is free
        I0 = R1;        //pixels
        I1 = R1;        //pixels&~3,R1 is free
	BITCLR(R1,0);
	I3=R1;
        P0 = 16;   //h

        R2 += -16;
   
        M1 = R2;

        R0 = [I3++];
        R1 = [I3++];
        LSETUP(_put_pixels81_bfin_LB,_put_pixels81_bfin_LE) LC0=P0;
        _put_pixels81_bfin_LB:
                (R3,R2)=BYTEUNPACK R1:0 || R0 = [I3++];
                R3 = BYTEPACK(R2,R3);
                (R3,R2)=BYTEUNPACK R1:0(R) ||  R1 = [I3++]|| [I2++]=R3;
                R3 = BYTEPACK(R2,R3) ;
                (R3,R2)=BYTEUNPACK R1:0 || R0 = [I3++M1]||[I2++]=R3;
                R3 = BYTEPACK(R2,R3);
                (R3,R2)=BYTEUNPACK R1:0(R) || R0 = [I3++] || [I2++]=R3;
             R3 = BYTEPACK(R2,R3);
        _put_pixels81_bfin_LE:
		[I2++]=R3||R1=[I3++];

        UNLINK;
        RTS;
END_FUNC(transfer16x16_copy_new)
DECL_FUNC(interpolate16x16_new_halfpel_h):
        SAV_R012;
        LINK 0;

        I3 = R0;        //block,R0 is free
        I0 = R1;        //pixels
        R3=[FP+20];
        CC = R3==0;
        R3 = R1;
	R3+=1;
        I1 = R3;
	BITCLR(R1,0);
        I2 = R1;        //pixels&~3,R1 is free
        P0 = 16;   //h

        R2 += -16;

        M1 = R2;        //line_size-8

        IF !CC JUMP _PUT_PIXELS16_X2;

        R0 = [I2++];
        R1 = [I2++];
        LSETUP(_put_pixels16_x2_bfin_LB,_put_pixels16_x2_bfin_LE) LC0=P0;
        _put_pixels16_x2_bfin_LB:
                R3 = BYTEOP1P(R1:0,R1:0) || R0 = [I2++];
                R3 = BYTEOP1P(R1:0,R1:0)(R) ||R1 = [I2++] || [I3++] = R3;
                 R3 = BYTEOP1P(R1:0,R1:0) || R0 = [I2++M1]||[I3++] = R3;
                R3 = BYTEOP1P(R1:0,R1:0)(R)||R0=[I2++]||[I3++] = R3;
        _put_pixels16_x2_bfin_LE:
		R1 = [I2++] || [I3++] = R3;

        UNLINK;
        RTS;

_PUT_PIXELS16_X2:

        R0 = [I2++];
        R1 = [I2++] ;
        LSETUP(_put_no_rnd_pixels16_x2_bfin_LB,_put_no_rnd_pixels16_x2_bfin_LE) LC0=P0;
        _put_no_rnd_pixels16_x2_bfin_LB:
                R3 = BYTEOP1P(R1:0,R1:0)(T) || R0 = [I2++];
                R3 = BYTEOP1P(R1:0,R1:0)(T,R) || R1 = [I2++] || [I3++] = R3;
                 R3 = BYTEOP1P(R1:0,R1:0)(T) || R0 = [I2++M1]||[I3++] = R3;
                R3 = BYTEOP1P(R1:0,R1:0)(T,R)||R0=[I2++]||[I3++] = R3;
        _put_no_rnd_pixels16_x2_bfin_LE:
                R1 = [I2++] || [I3++] = R3;

        UNLINK;
        RTS;
END_FUNC(interpolate16x16_new_halfpel_h)
DECL_FUNC(interpolate16x16_new_halfpel_v):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:6);

        P1 = R0;        //block,R0 is free
        I0 = R1;        //pixels
        I1 = R1;
        R3=[FP+20];
        CC=R3==0;
	BITCLR(R1,0);
        I2 = R1;        //pixels&~3,R1 is free
        R1 = R1 + R2;
        I3 = R1;
        P0 = 16;   //h

        R2 += -16;

        M0 = R2;

        R0 = [I2++] || R2 = [I3++];
        R1 = [I2++] || R3 = [I3++];
        IF !CC JUMP _PUT_PIXELS16_Y2;

        LSETUP(_put_pixels16_y2_bfin_LB,_put_pixels16_y2_bfin_LE) LC0=P0;
        _put_pixels16_y2_bfin_LB:
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++] || R2 = [I3++];
                R6 = BYTEOP1P(R1:0,R3:2)(R) || R1 = [I2++] || R3 = [I3++];
                 [P1++] = R7;
                R7 = BYTEOP1P(R1:0,R3:2) || R0 = [I2++M0] || R2 = [I3++M0];
                [P1++] = R6 ;
                R6 = BYTEOP1P(R1:0,R3:2)(R) || R0 = [I2++] || R2 = [I3++];
                [P1++] = R7 ||R1=[I2++];
        _put_pixels16_y2_bfin_LE:   
                [P1++] = R6 ||R3=[I3++];

        (R7:6)=[SP++];
        UNLINK;
        RTS;
        _PUT_PIXELS16_Y2:
                LSETUP(_put_no_rnd_pixels16_y2_bfin_LB,_put_no_rnd_pixels16_y2_bfin_LE) LC0=P0;
        _put_no_rnd_pixels16_y2_bfin_LB:
                R7 = BYTEOP1P(R1:0,R3:2)(T) || R0 = [I2++] || R2 = [I3++];
                R6 = BYTEOP1P(R1:0,R3:2)(T,R) || R1 = [I2++] || R3 = [I3++];
                 [P1++] = R7;
                R7 = BYTEOP1P(R1:0,R3:2)(T) || R0 = [I2++M0] || R2 = [I3++M0];
                [P1++] = R6 ;
                R6 = BYTEOP1P(R1:0,R3:2)(T,R) || R0 = [I2++] || R2 = [I3++];
                [P1++] = R7 ||R1=[I2++];
        _put_no_rnd_pixels16_y2_bfin_LE:
                         [P1++] = R6||R3=[I3++] ;
        (R7:6)=[SP++];
        UNLINK;
        RTS;

END_FUNC(interpolate16x16_new_halfpel_v)
DECL_FUNC(interpolate16x16_new_halfpel_hv):
         SAV_R012;
        LINK 0;
        [--SP]=(R7:5,P5:5);

        I3 = R0;        //block,R0 is free
       // R0 += 4;
       P5=4;
        P2 = R0;
        P1 = 4;
        P0 = 4;   //h
        R6=[FP+20];
        CC=R6==0;
        R5 = R1;
        I0 = R1;
	BITCLR(R5,0);
        I1 = R5;        //pixels
        R5 += 4;
        I2 = R5;

        M0 = R2;
        M1 = 1;
        M2=16;
        IF !CC JUMP _PUT_PIXLES16_XY2;

        LSETUP(_put_pixels16_xy2_bfin11_LB,_put_pixels16_xy2_bfin11_LE) LC0=P1;
        _put_pixels16_xy2_bfin11_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1++M0] || R3 = [I2++M0];

        R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
        LSETUP(_put_pixels16_xy2_bfin12_LB,_put_pixels16_xy2_bfin12_LE) LC1=P0;
        _put_pixels16_xy2_bfin12_LB:
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH) || R0 = [I1++M0] || R1 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDH) || I0 -= M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDL) || R2 = [I1++M0] || R3 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH) || R0 = [I1++M0] || R1 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDH) || I0 -= M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDL) || R2 = [I1++M0] || R3 = [I2++M0];
                R7 = R6 | R7;
        _put_pixels16_xy2_bfin12_LE:
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1 || [I3++M2] = R7;
                P2=P2+P5; 
                I3 = P2;
                I1 = R5;
                R5 += 4;
                I2 = R5;
        _put_pixels16_xy2_bfin11_LE:I0 -= M1;

        (R7:5,P5:5)=[SP++];
        UNLINK;
        RTS;

_PUT_PIXLES16_XY2:
            LSETUP(_put_no_rnd_pixels16_xy2_bfin11_LB,_put_no_rnd_pixels16_xy2_bfin11_LE) LC0=P1;
        _put_no_rnd_pixels16_xy2_bfin11_LB:
        R0 = [I1++M0] || R1 = [I2++M0];
        R2 = [I1++M0] || R3 = [I2++M0];

        R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
        LSETUP(_put_no_rnd_pixels16_xy2_bfin12_LB,_put_no_rnd_pixels16_xy2_bfin12_LE) LC1=P0;
        _put_no_rnd_pixels16_xy2_bfin12_LB:
                R6 = BYTEOP2P(R1:0,R3:2)(TH) || R0 = [I1++M0] || R1 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(TH) || I0 -= M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(TL) || R2 = [I1++M0] || R3 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(TH) || R0 = [I1++M0] || R1 = [I2++M0];
                R7 = R6 | R7;
                R7 = BYTEOP2P(R1:0,R3:2)(TH) || I0 -= M1 || [I3++M2] = R7;
                R6 = BYTEOP2P(R1:0,R3:2)(TL) || R2 = [I1++M0] || R3 = [I2++M0];
                R7 = R6 | R7;
        _put_no_rnd_pixels16_xy2_bfin12_LE:
                R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1 || [I3++M2] = R7;
                 P2=P2+P5;
                I3 = P2;
                I1 = R5;
                R5 += 4;
                I2 = R5;
        _put_no_rnd_pixels16_xy2_bfin11_LE:I0 -= M1;

        (R7:5,P5:5)=[SP++];
        UNLINK;
        RTS;
END_FUNC(interpolate16x16_new_halfpel_hv)	
#include "config_bfin.h"
#define SAV_R012 [SP+0]=R0; [SP+4]=R1; [SP+8]=R2

DECL_FUNC(interpolate8x8_new_halfpel_h_add):
        SAV_R012;
        LINK 0;

        I3 = R0;        //block,R0 is free
        I0 = R2;        //pixels
        R0 = 1;
		I1 = R0;
		R3=[FP+20];
		R0=[FP+24];
        CC = R0==0;
	BITCLR(R2,0);
        I2 = R2;        //pixels&~3,R1 is free
        P0 = 8;   //h
        R3 += -8;
        R1+=4;
        M1 = R3;        //line_size-8
        M2=R1;
        M3=-1;
        M0=1;
        IF !CC JUMP _PUT_PIXELS8_add_X2;

        R0 = [I2++];
        R1 = [I2++];

        LSETUP(_put_pixels8_add_x2_bfin_LB,_put_pixels8_add_x2_bfin_LE) LC0=P0;
        _put_pixels8_add_x2_bfin_LB:
               R0 = BYTEOP1P(R1:0,R1:0) ||R2=[I3++]||I1+=M3;
                R2 = BYTEOP1P(R1:0,R3:2)|| R0 = [I2++M1]||I1+=M0;
                
                R1 = BYTEOP1P(R1:0,R1:0)(R)  || R3=[I3--]|| I1+=M3;
                
                R3 = BYTEOP1P(R1:0,R3:2)(R)||R0 = [I2++] ||[I3++] = R2;
                I1+=M0;
        _put_pixels8_add_x2_bfin_LE:
		 [I3++M2] = R3||R1 = [I2++];
        UNLINK;
        RTS;

_PUT_PIXELS8_add_X2:

        R0 = [I2++];
        R1 = [I2++] ;
        LSETUP(_put_no_rnd_pixels8_add_x2_bfin_LB,_put_no_rnd_pixels8_add_x2_bfin_LE) LC0=P0;
        _put_no_rnd_pixels8_add_x2_bfin_LB:
               R0 = BYTEOP1P(R1:0,R1:0)(T) ||R2=[I3++]||I1+=M3;
                R2 = BYTEOP1P(R1:0,R3:2)|| R0 = [I2++M1]||I1+=M0;
                
                R1 = BYTEOP1P(R1:0,R1:0)(T,R)  || R3=[I3--]|| I1+=M3;
                
                R3 = BYTEOP1P(R1:0,R3:2)(R)||R0 = [I2++] ||[I3++] = R2;
                I1+=M0;
        _put_no_rnd_pixels8_add_x2_bfin_LE:
            	 [I3++M2] = R3||R1 = [I2++];

        UNLINK;
        RTS;
END_FUNC(interpolate8x8_new_halfpel_h_add)
DECL_FUNC(transfer8x8_copy_new_add):
        SAV_R012;
		LINK 0;
		[--SP] = (R7:6);

        I2 = R0;
        I3=R0;        //block,R0 is free
        I0 = R2;        //pixels
	I1=0;
	BITCLR(R2,0);
        P1 = R2;        //pixels&~3,R1 is free
        P0 = 8;   //h
        R3=[FP+20];
        R3 += -8;
        P2 = R3;
        M2=R1;
        R1+=4;
        M1=R1;
        R0 = [P1++]||R2=[I2++];
        R1 = [P1++]||R3=[I2++];
        LSETUP(_put_pixels8_add_bfin_LB,_put_pixels8_add_bfin_LE) LC0=P0;
        _put_pixels8_add_bfin_LB:
              R7=BYTEOP1P( R1:0,R3:2)(T)|| R0 = [P1++P2]||R2=[I2++M2];
              R6=BYTEOP1P( R1:0,R3:2)(T,R)||R0 = [P1++]||R2=[I2++];
              R1 = [P1++]||[I3++]=R7;
        _put_pixels8_add_bfin_LE:
		[I3++M1]=R6||R3=[I2++];
		
		(R7:6)=[SP++];
        UNLINK;
        RTS;
END_FUNC(transfer8x8_copy_new_add)
DECL_FUNC(interpolate8x8_new_halfpel_v_add):
        SAV_R012;
        LINK 0;
        [--SP]=(R7:6);
        [--SP]=P3;

        P1 = R0;
               //block,R0 is free
        I0 = R2;        //pixels
      //  I1 = R2;
		R3=[FP+20];
        R7=[FP+24];
        M1=1;
        CC=BITTST(R2,0);
        IF CC JUMP _PUT_PIXELS8_add_Y3;
//	JUMP _PUT_PIXELS8_add_Y3;
        CC=R7==0;
        BITCLR(R2,0);
	I1=0;
        I2 = R2;        //pixels&~3,R1 is free
        R2 = R2 + R3;
        P3=P1;
        I3 = R2;
        P0 = 8;   //h

        R3 += -4;
		R1+=4;
         
        M0 = R3;
		P2=R1;
        R0 = [I2++] || R2 = [I3++];
        
        R1 = [I2++M0] || R3 = [I3++M0];
         
        IF !CC JUMP _PUT_PIXELS8_add_Y2;

        LSETUP(_put_pixels8_add_y2_bfin_LB,_put_pixels8_add_y2_bfin_LE) LC0=P0;
        _put_pixels8_add_y2_bfin_LB:
                R0 = BYTEOP1P(R1:0,R3:2)||R2=[P3++] ;
                       R7 = BYTEOP1P(R1:0,R3:2)||R0 = [I2++] || R2 = [I3++];
                R1 = BYTEOP1P(R1:0,R3:2)(R)||R3=[P3++P2] ;
                  R6 = BYTEOP1P(R1:0,R3:2)(R)||R1 = [I2++M0] ||[P1++]=R7;
               
        _put_pixels8_add_y2_bfin_LE:
           [P1++P2] = R6||R3 = [I3++M0] ;

        P3=[SP++];
        (R7:6)=[SP++];
        UNLINK;
        RTS;
        _PUT_PIXELS8_add_Y2:
                LSETUP(_put_no_rnd_pixels8_add_y2_bfin_LB,_put_no_rnd_pixels8_add_y2_bfin_LE) LC0=P0;
        _put_no_rnd_pixels8_add_y2_bfin_LB:
                R0 = BYTEOP1P(R1:0,R3:2)(T)||R2=[P3++] ;
                       R7 = BYTEOP1P(R1:0,R3:2)||R0 = [I2++] || R2 = [I3++];
                R1 = BYTEOP1P(R1:0,R3:2)(T,R)||R3=[P3++P2] ;
                  R6 = BYTEOP1P(R1:0,R3:2)(R)||R1 = [I2++M0] ||[P1++]=R7;
        _put_no_rnd_pixels8_add_y2_bfin_LE:
                  [P1++P2] = R6||R3 = [I3++M0] ;

        P3=[SP++];
        (R7:6)=[SP++];
        UNLINK;
        RTS;
#if 1
        _PUT_PIXELS8_add_Y3:
        I1=R2;
                CC=R7==0;
        BITCLR(R2,0);
        I2 = R2;        //pixels&~3,R1 is free
        R2 = R2 + R3;
        P3=P1;
        I3 = R2;
        P0 = 8;   //h

        R3 += -8;
		R1+=4;
         
        M0 = R3;
		P2=R1;
        R0 = [I2++] || R2 = [I3++];
        
        R1 = [I2++] || R3 = [I3++];
         
        IF !CC JUMP _PUT_PIXELS8_add_Y21;
       I1=0;
        LSETUP(_put_pixels8_add_y21_bfin_LB,_put_pixels8_add_y21_bfin_LE) LC0=P0;
        _put_pixels8_add_y21_bfin_LB:

                R0 = BYTEOP1P(R1:0,R3:2)||R2=[P3++] ;
                I0-=M1;
                       R7 = BYTEOP1P(R1:0,R3:2)||R0 = [I2++M0]|| R2 = [I3++M0] ;
                       I0+=M1;
                R1 = BYTEOP1P(R1:0,R3:2)(R)||R3=[P3++P2]||R0=[I2++] ;
                I0-=M1;
                  R6 = BYTEOP1P(R1:0,R3:2)(R)||R1 = [I2++] ||[P1++]=R7;
                  R2 = [I3++]||I0+=M1;              
        _put_pixels8_add_y21_bfin_LE:
           [P1++P2] = R6||R3 = [I3++] ;

        P3=[SP++];
        (R7:6)=[SP++];
        UNLINK;
        RTS;
        _PUT_PIXELS8_add_Y21:
                LSETUP(_put_no_rnd_pixels8_add_y21_bfin_LB,_put_no_rnd_pixels8_add_y21_bfin_LE) LC0=P0;
        _put_no_rnd_pixels8_add_y21_bfin_LB:
                R0 = BYTEOP1P(R1:0,R3:2)(T)||R2=[P3++] ;
                I0-=M1;
                       R7 = BYTEOP1P(R1:0,R3:2)||R0 = [I2++M0] || R2 = [I3++M0];
                    I0+=M1;
                R1 = BYTEOP1P(R1:0,R3:2)(T,R)||R3=[P3++P2]||R0=[I2++] ;
                 I0-=M1;
                  R6 = BYTEOP1P(R1:0,R3:2)(R)||R1 = [I2++] ||[P1++]=R7;
                  R2 = [I3++]||I0+=M1; 
        _put_no_rnd_pixels8_add_y21_bfin_LE:
                  [P1++P2] = R6||R3 = [I3++] ;

        P3=[SP++];
        (R7:6)=[SP++];
        UNLINK;
        RTS;
#endif
END_FUNC(interpolate8x8_new_halfpel_v_add)
DECL_FUNC(interpolate8x8_new_halfpel_hv_add):
         SAV_R012;
        LINK 0;
        [--SP]=(R7:5);

        I3 = R0;        //block,R0 is free
        R0 += 4;
        P2 = R0;
        CC=BITTST(R2,0);
        P1 = 1;
        P0 = 8;   //hI
	R3=[FP+20];
        R6=[FP+24];
        IF CC  JUMP _PUT_PIXLES8_add_XY3;
        CC=R6==0;
        R5 = R2;
        I0 = R2;
        BITCLR(R5,0);
        I1 = R5;        //pixels
        R5 =R5+R3;
        I2 = R5;
        R1+=4;
        R3+=-8;
        M0 = R3;
        M1 = 1;
        M2=R1;
        IF !CC JUMP _PUT_PIXLES8_add_XY2;


        R0 = [I1++] || R2 = [I2++];
        R1 = [I1++] || R3 = [I2++];

         R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
        LSETUP(_put_pixels8_add_xy2_bfin12_LB,_put_pixels8_add_xy2_bfin12_LE) LC1=P0;
        _put_pixels8_add_xy2_bfin12_LB:
       
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH)||I0 -= M1||R0=[I3];
                R2 = R6 | R7;
                R6=BYTEOP1P(R1:0,R3:2)|| R0 = [I1++M0] || R2 = [I2++M0];
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL,R)|| I0 += M1||[I3++]=R6;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH,R)||I0 -= M1||R1=[I3];
                R3 = R6 | R7;
                R7=BYTEOP1P(R1:0,R3:2)(R)||R0=[I1++]||R2=[I2++];
                [I3++M2]=R7||R1 = [I1++];
        _put_pixels8_add_xy2_bfin12_LE:
                  R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1|| R3 = [I2++];


        (R7:5)=[SP++];
        UNLINK;
        RTS;

_PUT_PIXLES8_add_XY2:
        R0 = [I1++] || R2 = [I2++];
        R1 = [I1++] || R3 = [I2++];

        R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
        LSETUP(_put_no_rnd_pixels8_add_xy2_bfin12_LB,_put_no_rnd_pixels8_add_xy2_bfin12_LE) LC1=P0;
        _put_no_rnd_pixels8_add_xy2_bfin12_LB:
                R6 = BYTEOP2P(R1:0,R3:2)(TH)||I0 -= M1||R0=[I3];
                R2 = R6 | R7;
                R6=BYTEOP1P(R1:0,R3:2)|| R0 = [I1++M0] || R2 = [I2++M0];
                R7 = BYTEOP2P(R1:0,R3:2)(TL,R)|| I0 += M1||[I3++]=R6;
                R6 = BYTEOP2P(R1:0,R3:2)(TH,R)||I0 -= M1||R1=[I3];
                R3 = R6 | R7;
                R7=BYTEOP1P(R1:0,R3:2)(R)||R0=[I1++]||R2=[I2++];
                [I3++M2]=R7||R1 = [I1++];
        _put_no_rnd_pixels8_add_xy2_bfin12_LE:
                 R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1|| R3 = [I2++];

        (R7:5)=[SP++];
        UNLINK;
        RTS;
_PUT_PIXLES8_add_XY3:
        CC=R6==0;
        R5 = R2;
        I0 = R2;
        BITCLR(R5,0);
        I1 = R5;        //pixels
        R5 =R5+R3;
        I2 = R5;
        R1+=4;
        R3+=-8;
        M0 = R3;
        M1 = 1;
        M2=R1;
        M3=2;
        IF !CC JUMP _PUT_PIXLES8_add_XY21;


        R0 = [I1++] || R2 = [I2++];
        R1 = [I1++] || R3 = [I2++];

         R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;
        LSETUP(_put_pixels8_add_xy2_bfin12_LB1,_put_pixels8_add_xy2_bfin12_LE1) LC1=P0;
        _put_pixels8_add_xy2_bfin12_LB1:
       
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH)||I0 -= M3||R0=[I3];
                R2 = R6 | R7;
                R6=BYTEOP1P(R1:0,R3:2)|| R0 = [I1++M0] || R2 = [I2++M0];
                I0+=M1;
                R7 = BYTEOP2P(R1:0,R3:2)(RNDL,R)|| I0 += M1||[I3++]=R6;
                R6 = BYTEOP2P(R1:0,R3:2)(RNDH,R)||I0 -= M3||R1=[I3];
                R3 = R6 | R7;
                R7=BYTEOP1P(R1:0,R3:2)(R)||R0=[I1++]||R2=[I2++];
                [I3++M2]=R7||R1 = [I1++];
                 R3 = [I2++]||I0+=M1;
                 
        _put_pixels8_add_xy2_bfin12_LE1:
                  R7 = BYTEOP2P(R1:0,R3:2)(RNDL) || I0 += M1;


        (R7:5)=[SP++];
        UNLINK;
        RTS;

_PUT_PIXLES8_add_XY21:
        R0 = [I1++] || R2 = [I2++];
        R1 = [I1++] || R3 = [I2++];

        R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;
        LSETUP(_put_no_rnd_pixels8_add_xy2_bfin12_LB1,_put_no_rnd_pixels8_add_xy2_bfin12_LE1) LC1=P0;
        _put_no_rnd_pixels8_add_xy2_bfin12_LB1:
                R6 = BYTEOP2P(R1:0,R3:2)(TH)||I0 -= M3||R0=[I3];
                R2 = R6 | R7;
                R6=BYTEOP1P(R1:0,R3:2)|| R0 = [I1++M0] || R2 = [I2++M0];
                I0+=M1;
                R7 = BYTEOP2P(R1:0,R3:2)(TL,R)|| I0 += M1||[I3++]=R6;
                R6 = BYTEOP2P(R1:0,R3:2)(TH,R)||I0 -= M3||R1=[I3];
                R3 = R6 | R7;
                R7=BYTEOP1P(R1:0,R3:2)(R)||R0=[I1++]||R2=[I2++];
                [I3++M2]=R7||R1 = [I1++];
                 R3 = [I2++]||I0+=M1;
        _put_no_rnd_pixels8_add_xy2_bfin12_LE1:
                 R7 = BYTEOP2P(R1:0,R3:2)(TL) || I0 += M1;

        (R7:5)=[SP++];
        UNLINK;
        RTS;

END_FUNC(interpolate8x8_new_halfpel_hv_add)

/*
 * Blackfin asm optimizations for ape decode.
 * Author: Jetta, HHTech, 2007-10
 */			

#include "config_bfin.h"

DEFUN(scalarproduct, mL1,(int16_t* v1, int16_t* v2, int16_t* v3,int32_t order,int32_t sign)):
	LINK 0; 
	[--SP] = (R7:4);
	I0 = R0; //I0:v1
	I1 = R1; //I1:v2
	I2 = R2; //I2:v3
	R0 = 0 ; //R0:res
	P2 = [FP + 20]; //P2:orders
	R7 = [FP + 24]; //R7:sign
	P2 = P2 >> 1;
        /**********************************
	 * while(order){                  *
	 *	res1 = *v1++ * *v2++;     *
	 *      res2 = *v1++ * *v2--;     *
	 *      res += res1 + res2;       *
	 *      *v2++ += sign * *v3++;    *
	 *      *v2++ += sign * *v3++;    * 
	 *      order -= 2;               * 
	 *  }                             *
	 **********************************/		
	MNOP||R1.L = W[I0++]||R2.L = W[I1++]; //load *v1
	MNOP||R1.H = W[I0++]||R2.H = W[I1--];
	LSETUP(_SCAP_LOOP_BEG,_SCAP_LOOP_END)LC0=P2;
_SCAP_LOOP_BEG:
	R3 = R2;
	R4 = (A0=R1.L*R3.L),R5 = (A1=R1.H*R3.H)(IS)||I1 += 4||R6.L = W[I2++] ;
	R5 = R4 + R5(S)||R2.L = W[I1++]||R6.H = W[I2++]; 
	R4.H = (A1=R6.H*R7.H),R4.L = (A0=R6.L*R7.L)(IS)||R1.L = W[I0++]||R2.H = W[I1--]; // sign(*data) * *v3
	R4 = R3+|+R4(S)||I1 -= 4||R1.H = W[I0++]; 
	R0 = R0 + R5(S)||W[I1++] = R4.L;
_SCAP_LOOP_END:
	W[I1++] = R4.H; 
	(R7:4) = [SP++];
	UNLINK;
	RTS;
DEFUN_END(scalarproduct)


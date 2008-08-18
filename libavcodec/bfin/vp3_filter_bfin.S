/*
 *Blackfin asm optimizations for horizontal filter.
 *Author: Jetta, HHTech, 2007-08
 */			
	
#include "config_bfin.h"
	
DEFUN(horizontal_filter, mL1,
		(uint8_t *first_pixel, int32_t stride, int32_t *bounding_values)):
		[--SP] = (R7:4,P5:3)	;
		P0 = R0 				; 
		P1 = R1					;
		P5 = R2					;		
		R1 = 0					;
		R2 = 0x00ff			; 	
		P2 = 8					;
		LSETUP(_horizontal_filter_LB,_horizontal_filter_LE)LC0=P2 ; 
_horizontal_filter_LB:	
		R4 = B[P0-2](Z)			; 
		R5 = B[P0+1](Z) 		;
		R6 = B[P0](Z)			; 
		R7 = B[P0-1](Z)			;
		R4 = R4 - R5			; 
		R5 = R6 - R7			;
		P3 = R5					;
		P3 = P3 + (P3 << 1) 	;
		R5 = P3					;
		R4 = R4 + R5			; 
		R4 += 0x4				;
		R4 >>>= 0x3				;
		P4 = R4					;
		P3 = P5 + (P4 << 2)		; 
		R4 = [P3]				; 
		R7 = R7 + R4			; 
		R6 = R6 - R4			; 
		R7 = MAX(R7,R1)			;
		R7 = MIN(R7,R2)			;
		R6 = MAX(R6,R1)			;
		R6 = MIN(R6,R2) 		;
		B[P0-1] = R7			; 
		B[P0] = R6				; 
_horizontal_filter_LE:	
		P0 = P0 + P1			; 
		(R7:4,P5:3) = [SP++]	;
		RTS						;			
DEFUN_END(horizontal_filter)

///////////////////////////////////////////////////////////
DEFUN(vertical_filter, mL1,
		(uint8_t *first_pixel, int32_t stride, int32_t *bounding_values)):
		[--SP] = (R7:4,P5:3)	;
		P1 = R0					; 
		P5 = R1					;
		P4 = R2					; 
		R3 = -R1				;//-stride
		P0 = R3					;
		P3 = 8					; 
		R6 = 0					;
		R5 = 0xff(Z)			; 
		LSETUP(_vertical_filter_LB,_vertical_filter_LE)LC0=P3 ; 
_vertical_filter_LB:
		P2 = P1 + (P0 << 1)		;//offset = -2*stride
		R1 = B[P2](Z)			; 
		P2 = P1 + P5			;//offset = stride
		R0 = B[P2](Z)			; 
		R1 = R1 - R0			; 
		R0 = B[P1](Z) 			;//offset = 0 
		P2 = P1 + P0			;//offset = -stride
		R2 = B[P2](Z)			;//R0 and R2 are reserved temporarily
		R3 = R0 - R2			; 
		P2 = R3					;
		P2 = P2 + (P2 << 1)		;
		R3 = P2					;
		R1 = R1 + R3			;
		R1 += 0x4				;
		R1 >>>= 0x3				;
		P2 = R1					; 
		P2 = P4 + (P2 << 2)		;
		R1 = [P2]				;
		R2 = R2 + R1			;
		R2 = MAX(R2,R6)			;
		R2 = MIN(R2,R5)			;
		P2 = P1 + P0 			;
		//B[P2] = R2			;
		R0 = R0 - R1 (S)|| B[P2] = R2 ;
		R0 = MAX(R0,R6)			;
		R0 = MIN(R0,R5)			;
_vertical_filter_LE:	
		B[P1++] = R0			;
		(R7:4,P5:3) = [SP++]	;
		RTS 					; 
DEFUN_END(vertical_filter)


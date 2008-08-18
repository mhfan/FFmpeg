/*
 * BlackFin DSPUTILS
 *
 * Copyright (C) 2007 Marc Hoffman <marc.hoffman@analog.com>
 * Copyright (c) 2006 Michael Benjamin <michael.benjamin@analog.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavcodec/avcodec.h"
#include "libavcodec/dsputil.h"
#include "dsputil_bfin.h"
#include "config_bfin.h"	// mhfan

int off;


void ff_bfin_idct (DCTELEM *block) attribute_l1_text;
void ff_bfin_fdct (DCTELEM *block) attribute_l1_text;
void ff_bfin_idct_row(int16_t *input) attribute_l1_text;
void ff_bfin_idct_column_put(uint8_t *dst, int stride, int16_t *input) attribute_l1_text;
void ff_bfin_idct_column_add(uint8_t *dst, int stride, int16_t *input) attribute_l1_text;
void ff_bfin_vp3_idct (DCTELEM *block);
void ff_bfin_vp3_idct_put (uint8_t *dest, int line_size, DCTELEM *block);
void ff_bfin_vp3_idct_add (uint8_t *dest, int line_size, DCTELEM *block);
void ff_bfin_add_pixels_clamped (const DCTELEM *block, uint8_t *dest, int line_size) attribute_l1_text;
void ff_bfin_put_pixels_clamped (const DCTELEM *block, uint8_t *dest, int line_size) attribute_l1_text;
void ff_bfin_diff_pixels (DCTELEM *block, const uint8_t *s1, const uint8_t *s2, int stride)  attribute_l1_text;
void ff_bfin_get_pixels  (DCTELEM *restrict block, const uint8_t *pixels, int line_size) attribute_l1_text;
int  ff_bfin_pix_norm1  (uint8_t * pix, int line_size) attribute_l1_text;
int  ff_bfin_z_sad8x8   (uint8_t *blk1, uint8_t *blk2, int dsz, int line_size, int h) attribute_l1_text;
int  ff_bfin_z_sad16x16 (uint8_t *blk1, uint8_t *blk2, int dsz, int line_size, int h) attribute_l1_text;

void ff_bfin_z_put_pixels16_xy2     (uint8_t *block, const uint8_t *s0, int dest_size, int line_size, int h) attribute_l1_text;
void ff_bfin_z_put_pixels8_xy2      (uint8_t *block, const uint8_t *s0, int dest_size, int line_size, int h) attribute_l1_text;
void ff_bfin_put_pixels16_xy2_nornd (uint8_t *block, const uint8_t *s0, int line_size, int h) attribute_l1_text;
void ff_bfin_put_pixels8_xy2_nornd  (uint8_t *block, const uint8_t *s0, int line_size, int h) attribute_l1_text;


int  ff_bfin_pix_sum (uint8_t *p, int stride) attribute_l1_text;

void ff_bfin_put_pixels8uc        (uint8_t *block, const uint8_t *s0, const uint8_t *s1, int dest_size, int line_size, int h) attribute_l1_text;
void ff_bfin_put_pixels16uc       (uint8_t *block, const uint8_t *s0, const uint8_t *s1, int dest_size, int line_size, int h) attribute_l1_text;
void ff_bfin_put_pixels8uc_nornd  (uint8_t *block, const uint8_t *s0, const uint8_t *s1, int line_size, int h) attribute_l1_text;
void ff_bfin_put_pixels16uc_nornd (uint8_t *block, const uint8_t *s0, const uint8_t *s1, int line_size, int h) attribute_l1_text;

int ff_bfin_sse4  (void *v, uint8_t *pix1, uint8_t *pix2, int line_size, int h) attribute_l1_text;
int ff_bfin_sse8  (void *v, uint8_t *pix1, uint8_t *pix2, int line_size, int h) attribute_l1_text;
int ff_bfin_sse16 (void *v, uint8_t *pix1, uint8_t *pix2, int line_size, int h) attribute_l1_text;

#define ff_bfin_avg_no_rnd_pixels8 ff_bfin_avg_pixels8

void ff_bfin_avg_pixels8(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels8_x2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels8_y2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels8_xy2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels8_x2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels8_y2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels8_xy2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels16(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels16_x2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels16_y2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_pixels16_xy2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels16(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels16_x2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels16_y2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;
void ff_bfin_avg_no_rnd_pixels16_xy2(uint8_t *block, const uint8_t *pixels, int line_size, int h) attribute_l1_text;

#define CALL_2X_PIXELS_L1(a, b, n)\
void a(uint8_t *block, const uint8_t *pixels, int line_size, int h) {\
    b(block  , pixels  , line_size, h);\
    b(block+n, pixels+n, line_size, h);\
}

CALL_2X_PIXELS_L1(ff_bfin_avg_pixels16, ff_bfin_avg_pixels8, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_pixels16_x2, ff_bfin_avg_pixels8_x2, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_pixels16_y2, ff_bfin_avg_pixels8_y2, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_pixels16_xy2, ff_bfin_avg_pixels8_xy2, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_no_rnd_pixels16, ff_bfin_avg_pixels8, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_no_rnd_pixels16_x2, ff_bfin_avg_no_rnd_pixels8_x2, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_no_rnd_pixels16_y2, ff_bfin_avg_no_rnd_pixels8_y2, 8)
CALL_2X_PIXELS_L1(ff_bfin_avg_no_rnd_pixels16_xy2, ff_bfin_avg_no_rnd_pixels8_xy2, 8)

void ff_bfin_put_h264_chroma_mc8( uint8_t *dst/*align 8*/,
		    uint8_t *src0/*align 1*/, uint8_t *src1/*align 1*/,
		    int stride, int h, int a, int b) attribute_l1_text;

void put_h264_chroma_mc8_bfin(uint8_t *dst/*align 8*/,
		    uint8_t *src/*align 1*/, int stride, int h, int x, int y)
{
    int A,B,C;
    if (y==0){
	A=8*(8-x); B=8*x;
        ff_bfin_put_h264_chroma_mc8(dst, src, src+1, stride, h, A, B);
    } else
    if (x==0){
        A=8*(8-y); C=8*y;
        ff_bfin_put_h264_chroma_mc8(dst, src, src+stride, stride, h, A, C);
    }
}

void bfin_idct_add (uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_bfin_idct (block);
    ff_bfin_add_pixels_clamped (block, dest, line_size);
}

static void bfin_idct_put (uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_bfin_idct (block);
    ff_bfin_put_pixels_clamped (block, dest, line_size);
}

void ff_vp3_idct_put_bfin(uint8_t *dest/*align 8*/, int line_size, DCTELEM *block/*align 16*/){
    ff_bfin_idct_row(block);
    ff_bfin_idct_column_put(dest, line_size, block);
}

void ff_vp3_idct_add_bfin(uint8_t *dest/*align 8*/, int line_size, DCTELEM *block/*align 16*/){
    ff_bfin_idct_row(block);
    ff_bfin_idct_column_add(dest, line_size, block);
}

static void bfin_clear_blocks (DCTELEM *blocks)
{
    // This is just a simple memset.
    //
    __asm__("P0=192; "
        "I0=%0;  "
        "R0=0;   "
        "LSETUP(clear_blocks_blkfn_lab,clear_blocks_blkfn_lab)LC0=P0;"
        "clear_blocks_blkfn_lab:"
        "[I0++]=R0;"
        ::"a" (blocks):"P0","I0","R0");
}


void bfin_put_pixels8_new (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    asm(    "I3=R0;     /*block*/"
            "I0=R1;     /*pixels*/"
            "I1=R1;     /*pixels*/"
            "P0=%0;     /*h*/"
            "R2+=-4; M3=R2;"
            "R2+=-4; M0=R2;"
            "DISALGNEXCPT                || R0 = [I0++]  || R2 = [I1++];"
            "LSETUP(pp80$0,pp80$1) LC0=P0;"
            "pp80$0:  DISALGNEXCPT       || R1 = [I0++]  || R3 = [I1++];"
            "R6 = BYTEOP1P(R1:0,R3:2)    || R0 = [I0++M0]|| R2 = [I1++M0];"
            "R7 = BYTEOP1P(R1:0,R3:2)(R) || R0 = [I0++]  || [I3++] = R6 ;"
            "pp80$1:  DISALGNEXCPT       || R2 = [I1++]  || [I3++M3] = R7;"
            :
            :"m"(h)
            :"R0", "R1", "R2", "R3", "P0", "I0", "I1", "I3", "M0", "M3", "R6", "R7");
}

static void bfin_put_pixels8 (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels8uc (block, pixels, pixels, line_size, line_size, h);
}

static void bfin_put_pixels8_x2(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels8uc (block, pixels, pixels+1, line_size, line_size, h);
}

static void bfin_put_pixels8_y2 (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels8uc (block, pixels, pixels+line_size, line_size, line_size, h);
}

static void bfin_put_pixels8_xy2 (uint8_t *block, const uint8_t *s0, int line_size, int h)
{
    ff_bfin_z_put_pixels8_xy2 (block,s0,line_size, line_size, h);
}

static void bfin_put_pixels16 (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels16uc (block, pixels, pixels, line_size, line_size, h);
}

static void bfin_put_pixels16_x2 (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels16uc (block, pixels, pixels+1, line_size, line_size, h);
}

static void bfin_put_pixels16_y2 (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels16uc (block, pixels, pixels+line_size, line_size, line_size, h);
}

static void bfin_put_pixels16_xy2 (uint8_t *block, const uint8_t *s0, int line_size, int h)
{
    ff_bfin_z_put_pixels16_xy2 (block,s0,line_size, line_size, h);
}

void bfin_put_pixels8_nornd (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels8uc_nornd (block, pixels, pixels, line_size, h);
}

static void bfin_put_pixels8_x2_nornd (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels8uc_nornd (block, pixels, pixels+1, line_size, h);
}

static void bfin_put_pixels8_y2_nornd (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels8uc_nornd (block, pixels, pixels+line_size, line_size, h);
}


void bfin_put_pixels16_nornd (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels16uc_nornd (block, pixels, pixels, line_size, h);
}

static void bfin_put_pixels16_x2_nornd (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels16uc_nornd (block, pixels, pixels+1, line_size, h);
}

static void bfin_put_pixels16_y2_nornd (uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    ff_bfin_put_pixels16uc_nornd (block, pixels, pixels+line_size, line_size, h);
}

static int bfin_pix_abs16 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    return ff_bfin_z_sad16x16 (blk1,blk2,line_size,line_size,h);
}

static int bfin_vsad_intra16 (void *c, uint8_t *blk1, uint8_t *dummy, int stride, int h) {
    return ff_bfin_z_sad16x16 (blk1,blk1+stride,stride<<1,stride<<1,h);
}

static int bfin_vsad (void *c, uint8_t *blk1, uint8_t *blk2, int stride, int h) {
    return ff_bfin_z_sad16x16 (blk1,blk1+stride,stride<<1,stride<<1,h)
        + ff_bfin_z_sad16x16 (blk2,blk2+stride,stride<<1,stride<<1,h);
}

#ifdef	USE_L1DATA
static uint8_t vtmp_blk[256] attribute_l1_data_b;
#else// mhfan
static uint8_t vtmp_blk[256];
#endif//USE_L1DATA

static int bfin_pix_abs16_x2 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    ff_bfin_put_pixels16uc (vtmp_blk, blk2, blk2+1, 16, line_size, h);
    return ff_bfin_z_sad16x16 (blk1, vtmp_blk, line_size, 16, h);
}

static int bfin_pix_abs16_y2 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    ff_bfin_put_pixels16uc (vtmp_blk, blk2, blk2+line_size, 16, line_size, h);
    return ff_bfin_z_sad16x16 (blk1, vtmp_blk, line_size, 16, h);
}

static int bfin_pix_abs16_xy2 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    ff_bfin_z_put_pixels16_xy2 (vtmp_blk, blk2, 16, line_size, h);
    return ff_bfin_z_sad16x16 (blk1, vtmp_blk, line_size, 16, h);
}

static int bfin_pix_abs8 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    return ff_bfin_z_sad8x8 (blk1,blk2,line_size,line_size, h);
}

static int bfin_pix_abs8_x2 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    ff_bfin_put_pixels8uc (vtmp_blk, blk2, blk2+1, 8, line_size, h);
    return ff_bfin_z_sad8x8 (blk1, vtmp_blk, line_size, 8, h);
}

static int bfin_pix_abs8_y2 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    ff_bfin_put_pixels8uc (vtmp_blk, blk2, blk2+line_size, 8, line_size, h);
    return ff_bfin_z_sad8x8 (blk1, vtmp_blk, line_size, 8, h);
}

static int bfin_pix_abs8_xy2 (void *c, uint8_t *blk1, uint8_t *blk2, int line_size, int h)
{
    ff_bfin_z_put_pixels8_xy2 (vtmp_blk, blk2, 8, line_size, h);
    return ff_bfin_z_sad8x8 (blk1, vtmp_blk, line_size, 8, h);
}

static void prefetch_bfin(void* mem, int stride, int h)
{
#if 1
    unsigned long p = (unsigned long)mem;
    for (p &= (~31); h--; p += stride)
	  asm volatile("prefetch [%0];" :: "a"(p));
#else// XXX:
    unsigned long e, p = (unsigned long)mem;
    for ( ; h--; p = e)
	for (e = p + stride, p &= (~31); p < e; )
	    asm volatile("prefetch [%0++];" :: "a"(p));
#endif
}

/*
  decoder optimization
  start on 2/11 100 frames of 352x240@25 compiled with no optimization -g debugging
  9.824s ~ 2.44x off
  6.360s ~ 1.58x off with -O2
  5.740s ~ 1.43x off with idcts

  2.64s    2/20 same sman.mp4 decode only

*/

void dsputil_init_bfin( DSPContext* c, AVCodecContext *avctx )
{
    //c->prefetch	  = prefetch_bfin;
    c->get_pixels         = ff_bfin_get_pixels;
    c->diff_pixels        = ff_bfin_diff_pixels;
    c->put_pixels_clamped = ff_bfin_put_pixels_clamped;
    c->add_pixels_clamped = ff_bfin_add_pixels_clamped;

    c->clear_blocks       = bfin_clear_blocks;
    c->pix_sum            = ff_bfin_pix_sum;
    c->pix_norm1          = ff_bfin_pix_norm1;

    c->sad[0]             = bfin_pix_abs16;
    c->sad[1]             = bfin_pix_abs8;

/*     c->vsad[0]            = bfin_vsad; */
/*     c->vsad[4]            = bfin_vsad_intra16; */

    /* TODO [0] 16  [1] 8 */
    c->pix_abs[0][0] = bfin_pix_abs16;
    c->pix_abs[0][1] = bfin_pix_abs16_x2;
    c->pix_abs[0][2] = bfin_pix_abs16_y2;
    c->pix_abs[0][3] = bfin_pix_abs16_xy2;

    c->pix_abs[1][0] = bfin_pix_abs8;
    c->pix_abs[1][1] = bfin_pix_abs8_x2;
    c->pix_abs[1][2] = bfin_pix_abs8_y2;
    c->pix_abs[1][3] = bfin_pix_abs8_xy2;


    c->sse[0] = ff_bfin_sse16;
    c->sse[1] = ff_bfin_sse8;
    c->sse[2] = ff_bfin_sse4;


    /**
     * Halfpel motion compensation with rounding (a+b+1)>>1.
     * This is an array[4][4] of motion compensation functions for 4
     * horizontal blocksizes (8,16) and the 4 halfpel positions
     * *pixels_tab[ 0->16xH 1->8xH ][ xhalfpel + 2*yhalfpel ]
     * @param block destination where the result is stored
     * @param pixels source
     * @param line_size number of bytes in a horizontal line of block
     * @param h height
     */

    c->put_pixels_tab[0][0] = bfin_put_pixels16;
    c->put_pixels_tab[0][1] = bfin_put_pixels16_x2;
    c->put_pixels_tab[0][2] = bfin_put_pixels16_y2;
    c->put_pixels_tab[0][3] = bfin_put_pixels16_xy2;

    c->put_pixels_tab[1][0] = bfin_put_pixels8;
    c->put_pixels_tab[1][1] = bfin_put_pixels8_x2;
    c->put_pixels_tab[1][2] = bfin_put_pixels8_y2;
    c->put_pixels_tab[1][3] = bfin_put_pixels8_xy2;

    c->put_no_rnd_pixels_tab[1][0] = bfin_put_pixels8_nornd;
    c->put_no_rnd_pixels_tab[1][1] = bfin_put_pixels8_x2_nornd;
    c->put_no_rnd_pixels_tab[1][2] = bfin_put_pixels8_y2_nornd;
/*     c->put_no_rnd_pixels_tab[1][3] = ff_bfin_put_pixels8_xy2_nornd; */

    c->put_no_rnd_pixels_tab[0][0] = bfin_put_pixels16_nornd;
    c->put_no_rnd_pixels_tab[0][1] = bfin_put_pixels16_x2_nornd;
    c->put_no_rnd_pixels_tab[0][2] = bfin_put_pixels16_y2_nornd;
/*     c->put_no_rnd_pixels_tab[0][3] = ff_bfin_put_pixels16_xy2_nornd; */

#ifdef	BFIN_MC_AVG_ASM	// Added by gph, 2007-07
#define dspfunc(PFX, IDX, NUM) \
    c->PFX ## _pixels_tab[IDX][0] = ff_bfin_ ## PFX ## _pixels ## NUM;\
    c->PFX ## _pixels_tab[IDX][1] = ff_bfin_ ## PFX ## _pixels ## NUM ## _x2;\
    c->PFX ## _pixels_tab[IDX][2] = ff_bfin_ ## PFX ## _pixels ## NUM ## _y2;\
    c->PFX ## _pixels_tab[IDX][3] = ff_bfin_ ## PFX ## _pixels ## NUM ## _xy2;

#if 0 //bug: shake
    dspfunc(avg, 0, 16);
    dspfunc(avg_no_rnd, 0, 16);
    dspfunc(avg, 1, 8);
#endif
    dspfunc(avg_no_rnd, 1, 8);
#endif//BFIN_MC_AVG_ASM

    if (avctx->dct_algo == FF_DCT_AUTO)
        c->fdct               = ff_bfin_fdct;

    if (avctx->idct_algo==FF_IDCT_VP3) {
        c->idct_permutation_type = FF_NO_IDCT_PERM;
        c->idct               = ff_bfin_vp3_idct;
        c->idct_add           = ff_bfin_vp3_idct_add;
        c->idct_put           = ff_bfin_vp3_idct_put;
	//c->idct_add         = ff_vp3_idct_add_bfin;
	//c->idct_put         = ff_vp3_idct_put_bfin;

	c->put_pixels_tab[1][0] = bfin_put_pixels8_new;
	c->put_h264_chroma_pixels_tab[0]= put_h264_chroma_mc8_bfin;
    } else if (avctx->idct_algo == FF_IDCT_AUTO) {
        c->idct_permutation_type = FF_NO_IDCT_PERM;
        c->idct               = ff_bfin_idct;
        c->idct_add           = bfin_idct_add;
        c->idct_put           = bfin_idct_put;
    }
}


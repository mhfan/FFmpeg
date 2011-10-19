/*
 * File:         mdma.c
 * Based on:
 * Author:       Marc Hoffman
 *
 * Created:      11/15/2007
 * Description:  Blackfin 2D DMA engine interface code user level API.
 *
 * Modified:
 *               Copyright 2004-2007 Analog Devices Inc.
 *
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdio.h>
#include <malloc.h>
#include <bfin_sram.h>
#include "mdma.h"
#include "bfin_cache.h"

/*

  0xffc0 0x0e00 nxt desc
  0xffc0 0x0e40
  0: SAL
  2: SAH
  4: CFG
  6: XCNT
  8: XM
  A: YCNT
  C: YM

  config

   FLOW	   |	      |NDSZ
   0 stop  |	      |	 0000 autobuf
   1 auto  |   	      |	 xxxx sz
   4 ary   |	      |
   6 sm	 __|__ 	  ____|___
   7 lg	|     |	 |     	  |
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   |15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
       	       	       	     | 	|  |  |	\   /  |  |
    			     |	|  |  |	 \ /   |  +--1:en 0:di0
    			     |	|  |  |	  +    |
    			     |	|  |  |	  |    +-----1:wr 0:rd
    			     |	|  |  |	  |
    			     |	|  |  |	  +----------00: 8bit
    			     |	|  |  |		     01: 16bit
    			     |	|  |  |		     10: 32bit
    			     |	|  |  |
    			     |	|  |  +--------------0:linear 1:2d
    			     |	|  |
    			     |	|  +-----------------0:retain 1:discard (RESTART)
    			     |	|
    			     |	+--------------------0:int after whole, 1:int after row
    			     |
    			     +-----------------------0:dis int, 1:en int


  src2d 0100 0111 0001 0101  4615
  dst1d 0100 0111 0000 0111  4607

*/

#define clock()      ({ int _t; asm volatile ("%0=cycles;" : "=d" (_t)); _t; })

#define L1CODE __attribute__ ((l1_text))
#define L1DATA __attribute__ ((l1_data))

/*
  Cache Line Alignment to simplify the FLUSH/INVALIDATE codes.
*/
#define ALIGN __attribute__ ((aligned(32)))


#define LO(x) (((unsigned)(x))&0xffff)
#define HI(x) (((unsigned)(x)>>16)&0xffff)
#define DADDR(x) LO(x),HI(x)

/*
 *  corresponding kernel patch in entry.S
 *
 *  ENTRY(_ex_dmach1)
 *	p5.h = hi(MDMA_D1_CURR_DESC_PTR);
 *	p5.l = lo(MDMA_D1_CURR_DESC_PTR);
 *	p4.h = hi(MDMA_S1_CURR_DESC_PTR);
 *	p4.l = lo(MDMA_S1_CURR_DESC_PTR);
 *
 *	[p5]=r1;
 *	[p4]=r0;
 *	p5.l = lo(MDMA_D1_CONFIG);
 *	p4.l = lo(MDMA_S1_CONFIG);
 *
 *	r2 = r2 >> 16 || w[p4]=r2;
 *	w[p5]=r2;
 *	csync;
 *	jump _return_from_exception;
 *
 */

static int hardwired_ones = -1;


bfdmactrl_t *alloc_dmactrl (int nmax)
{
  bfdmactrl_t *dmas = memalign (32, sizeof(struct dmactrl_block) + sizeof(dmadsc_t)*2*nmax);
  memset(dmas, sizeof(struct dmactrl_block)+sizeof(dmadsc_t)*2*nmax, 0);
  dmas->src      = &dmas->desc[0];
  dmas->dst      = &dmas->desc[nmax];
  dmas->maxmoves = nmax;
  dmas->n        = 0;
  dmas->psrc     = dmas->src;
  dmas->pdst     = dmas->dst;
  //cache_flush(dmas, dmas+sizeof(struct dmactrl_block));
  return dmas;
}

void bfmdma_reset_chain (bfdmactrl_t *dmas)
{
  dmas->n        = 0;
  dmas->psrc     = dmas->src;
  dmas->pdst     = dmas->dst;
  //cache_flush(dmas, dmas+sizeof(struct dmactrl_block));

}

void dma_add_block_move (bfdmactrl_t *dmac, int ws,
			 unsigned *dsta, int dw, int dh, int ds,
			 unsigned *srca, int sw, int sh, int ss)
{
  unsigned scfg,dcfg;
  unsigned src    = (unsigned)srca;
  unsigned dst    = (unsigned)dsta;
  dmadsc_t *ddma  = dmac->pdst++;
  dmadsc_t *sdma  = dmac->psrc++;
  int i;

  sdma->cfg = FLOW | NDSIZE | DMAEN;
  sdma->cfg |= WDSIZE_8;

  sdma->sal = src;
  sdma->sah = src>>16;
  if (sh > 1) sdma->cfg |= DMA2D;
  sdma->xc = sw;
  sdma->xm = ws;
  sdma->yc = sh;
  sdma->ym = ss;
  scfg = sdma->cfg;

  ddma->cfg = FLOW | NDSIZE | DMAEN | WNR;
  ddma->cfg |= WDSIZE_8;

  ddma->sal = dst;
  ddma->sah = dst>>16;
  if (sh > 1) ddma->cfg |= DMA2D;
  ddma->xc = dw;
  ddma->xm = ws;
  ddma->yc = dh;
  ddma->ym = ds;
  dcfg = ddma->cfg;

  //FLUSH(sdma);
  //FLUSH(ddma);
  //cache_flush(sdma, sdma+sizeof(dmadsc_t));   
  //cache_flush(ddma, ddma+sizeof(dmadsc_t));   

}

void dma_add_move (bfdmactrl_t *dmac, unsigned *dsta, unsigned *srca, int n)
{
  dma_add_block_move (dmac, 4, dsta, n, 1, 4, srca, n, 1, 4);
}

void dma_add_stop_flag (bfdmactrl_t *dmac)
{
  unsigned flag   = (unsigned)&dmac->sem;
  unsigned hwones = (unsigned)&hardwired_ones;
  dmadsc_t *ddma  = dmac->pdst++;
  dmadsc_t *sdma  = dmac->psrc++;

#if 1
  sdma->cfg = 0;
  ddma->cfg = 0;
#else
  sdma->cfg = DMAEN;
  sdma->cfg |= WDSIZE_8;

  sdma->sal = hwones;
  sdma->sah = hwones>>16;
  sdma->xc = 1;
  sdma->xm = 1;

  ddma->cfg = DMAEN | WNR;
  ddma->cfg |= WDSIZE_8;

  ddma->sal = flag;
  ddma->sah = flag>>16;
  ddma->xc = 1;
  ddma->xm = 1;
#endif
  //FLUSH(sdma);
  //FLUSH(ddma);
  //cache_flush(sdma, sdma+sizeof(dmadsc_t));   
  //cache_flush(ddma, ddma+sizeof(dmadsc_t));   

}

void pblk (void *a, int w, int h, int s)
{
  unsigned char *p = a;
  int x,y;
  printf ("%08x\n", p);
  for (y=0;y<h;y++) {
    for (x=0;x<w;x++)
      printf ("%4d ", *p++);
    printf ("\n");
    if (s)
      p += (s-w);
  }
  printf ("\n");
}

void pblkl (void *a, int w, int h, int s)
{
  unsigned *p = a;
  int x,y;
  printf ("%08x\n", p);
  for (y=0;y<h;y++) {
    for (x=0;x<w;x++)
      printf ("%4d ", *p++);
    printf ("\n");
    if (s)
      p += (s-w);
  }
  printf ("\n");
}

void dma_print (bfdmactrl_t *dmac)
{
  unsigned short *s = (unsigned short *)dmac->src;
  unsigned short *d = (unsigned short *)dmac->dst;
  int sn,dn;
  int i;
  int stop,end;

  for (i=0;i<dmac->maxmoves;i++) {
    stop = (s[2]&0xff00) == 0;
    printf ("src %2d: %08x: ARY SAH/L: %04x%04x CFG: %04x X(%d,+%d) Y(%d,+%d)\n",i,
	    s, s[1],s[0],s[2],s[3],s[4],s[5],s[6]);
    printf ("dst %2d: %08x: ARY SAH/L: %04x%04x CFG: %04x X(%d,+%d) Y(%d,+%d)\n",i,
	    d, d[1],d[0],d[2],d[3],d[4],d[5],d[6]);
    sn = (s[2]>>8)&0xf;
    dn = (d[2]>>8)&0xf;
    s += sn;
    d += dn;
    end = s[0] == 0xFFFF;
    if (stop) break;
  }
}

unsigned  bfmdma (bfdmactrl_t *dmac)
{
  sync_write32 (&dmac->sem, 0);
  cache_flush(dmac, dmac+sizeof(struct dmactrl_block)+sizeof(dmadsc_t)*2*dmac->maxmoves);
  bfin_dodma (dmac->src, dmac->dst, dmac->dst[0].cfg<<16 | dmac->src[0].cfg);
  return (unsigned)dmac;
}

/* deprecated */
unsigned dma_wait (unsigned flag)
{
  unsigned t0 = clock();
  while (sync_read32 ((unsigned *)flag) == 0);
  return clock()-t0;
}

unsigned bfdma_wait (bfdmactrl_t *dmac)
{
  unsigned t0 = clock();
  while (sync_read32 ((unsigned *)dmac) == 0);
  return clock()-t0;
}


unsigned bfdma_align32move (bfdmactrl_t *dmac,
                            unsigned char *dst, unsigned char *src,
                            int w,int h,
                            int dst_stride,
                            int src_stride)
{
#if 0
    dma_add_block_move (dmac,   4,
                        dst, w>>2, h, dst_stride - (w - 4),
                        src, w>>2, h, src_stride - (w - 4));
#else
	dma_add_block_move (dmac,   1,
                        dst, w, h, dst_stride - (w - 1),
                        src, w, h, src_stride - (w - 1));
#endif

}

unsigned bfdma_synchronous_align32move (unsigned char *dst, unsigned char *src,
                                        int w,int h,
                                        int dst_stride,
                                        int src_stride)
{
    static bfdmactrl_t *b;

    if (b == 0)     b = alloc_dmactrl (2);
    else            bfmdma_reset_chain (b);
    bfdma_align32move (b, dst, src, w, h, dst_stride, src_stride);
	cache_flush(src, src+src_stride*h);
	cache_clean(dst, dst+dst_stride*h);
    dma_add_stop_flag (b);
#if 0
    printf (">>> %p = %p , (%d,%d) dst_stride=%d, src_stride=%d\n",
            dst, src, w, h, dst_stride, src_stride);

    dma_print (b);
#endif

    bfmdma (b);
    bfdma_wait (b);

#if 1
    while (h--){
		 if(memcmp(dst, src, w)){
			  fprintf(stderr,"transfer error\n");
			  fflush(stderr);
			  while(1);
		 }			  
		 dst += dst_stride;
		 src += src_stride;
    }
#endif
}

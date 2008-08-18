/*
 * File:         mdma.h
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
#ifndef _MDMA_H
#define _MDMA_H

/* Configuration word masks */
#define DMAEN           0x00000001  // Channel Enable
#define WNR             0x00000002  // Channel Direction (W/R*)
#define WDSIZE_8        0x00000000  // Word Size 8 bits
#define WDSIZE_16       0x00000004  // Word Size 16 bits
#define WDSIZE_32       0x00000008  // Word Size 32 bits
#define DMA2D           0x00000010  // 2D/1D* Mode
#define RESTART         0x00000020  // Restart
#define DI_SEL          0x00000040  // Data Interrupt Select
#define DI_EN           0x00000080  // Data Interrupt Enable
#define NDSIZE          0x00000700  // Next Descriptor Size
#define NDSIZE_3HW      0x00000500  // NDSIZE : 3 16-bit words
#define FLOW            0x00004000  // Flow (descriptor array mode)

#define NO_XMOD 0
#define XMOD_B  1
#define XMOD_H  2
#define XMOD_W  4

#define SRC_CFG_1D_B (FLOW | NDSIZE | DMAEN)                           /* 0x4701 */
#define DST_CFG_1D_B (FLOW | NDSIZE | WR | DMAEN)                      /* 0x4703 */

#define SRC_CFG_2D_B (FLOW | NDSIZE | DMA2D       | DMAEN)             /* 0x4711 */
#define DST_CFG_2D_B (FLOW | NDSIZE | DMA2D | WNR | DMAEN)             /* 0x4713 */

#define SRC_CFG_1D_H (FLOW | NDSIZE | WDSIZE_16 | DMAEN)               /* 0x4705 */
#define DST_CFG_1D_H (FLOW | NDSIZE | WDSIZE_16 | WNR | DMAEN)         /* 0x4707 */

#define SRC_CFG_2D_H (FLOW | NDSIZE | DMA2D | WDSIZE_16 | DMAEN)       /* 0x4715 */
#define DST_CFG_2D_H (FLOW | NDSIZE | DMA2D | WDSIZE_16 | WNR | DMAEN) /* 0x4717 */

#define SRC_CFG_3HW_2D_H (FLOW | NDSIZE_3HW | DMA2D | WDSIZE_16 | DMAEN)
#define DST_CFG_3HW_2D_H (FLOW | NDSIZE_3HW | DMA2D | WDSIZE_16 | WNR | DMAEN)

#define SRC_CFG_1D_W (FLOW | NDSIZE | WDSIZE_32 | DMAEN)               /* 0x4709 */
#define DST_CFG_1D_W (FLOW | NDSIZE | WDSIZE_32 | WNR | DMAEN)         /* 0x470b */

#define SRC_CFG_2D_W (FLOW | NDSIZE | DMA2D | WDSIZE_32 | DMAEN)       /* 0x4719 */
#define DST_CFG_2D_W (FLOW | NDSIZE | DMA2D | WDSIZE_32 | WNR | DMAEN) /* 0x471b */

#define SRC_CFG_3HW_2D_W (FLOW | NDSIZE_3HW | DMA2D | WDSIZE_32 | DMAEN)
#define DST_CFG_3HW_2D_W (FLOW | NDSIZE_3HW | DMA2D | WDSIZE_32 | WNR | DMAEN)

#define STP_SRC_CFG_B (WDSIZE_8 | DMAEN)                               /* 0x0001 */
#define STP_DST_CFG_B (WDSIZE_8 | WNR | DMAEN)                         /* 0x0003 */

#define STP_SRC_CFG_2D_B (WDSIZE_8 | DMA2D | DMAEN)                    /* 0x0011 */ 
#define STP_DST_CFG_2D_B (WDSIZE_8 | DMA2D | WNR | DMAEN)              /* 0x0013 */   

#define STP_SRC_CFG_H (WDSIZE_16 | DMAEN)                              /* 0x0005 */
#define STP_DST_CFG_H (WDSIZE_16 | WNR | DMAEN)                        /* 0x0007 */

#define STP_SRC_CFG_2D_H (WDSIZE_16 | DMA2D | DMAEN)                   /* 0x0015 */
#define STP_DST_CFG_2D_H (WDSIZE_16 | DMA2D | WNR | DMAEN)             /* 0x0017 */

#define STP_SRC_CFG_W (WDSIZE_32 | DMAEN)                              /* 0x0009 */
#define STP_DST_CFG_W (WDSIZE_32 | WNR | DMAEN)                        /* 0x000b */

#define STP_SRC_CFG_2D_W (WDSIZE_32 | DMA2D | DMAEN)                   /* 0x0019 */
#define STP_DST_CFG_2D_W (WDSIZE_32 | DMA2D | WNR | DMAEN)             /* 0x001b */

typedef unsigned short uword;

typedef struct {
  uword sal;
  uword sah;
  uword cfg;
  uword xc;
  uword xm;
  uword yc;
  uword ym;
} dmadsc_t;


typedef struct dmactrl_block {
  unsigned sem;
  int      maxmoves;
  dmadsc_t *src;
  dmadsc_t *dst;
  unsigned control;
  dmadsc_t *psrc;
  dmadsc_t *pdst;
  int       n;     /* require 32 byte alignment for cache line flushing */
  dmadsc_t desc[1];
} bfdmactrl_t;


/* Low Level DMA Kick Off */
#define bfin_dodma_ch1(sin,din,sdcfg) \
   asm volatile  ("excpt 0xd;\n\t" : : "q0" (sin), "q1" (din), "q2" (sdcfg))
#define bfin_dodma_ch3(sin,din,sdcfg) \
   asm volatile  ("excpt 0xf;\n\t" : : "q0" (sin), "q1" (din), "q2" (sdcfg))
#define bfin_dodma(num, sin, din, sdcfg) \
	 bfin_dodma_ch##num(sin, din, sdcfg) 

/* Low Level Coherence Utilities. */
#define FLUSH(x)      asm ("flush [%0];\n\t" : : "a" (x))
#define CLEAN(x)      asm ("flushinv [%0];\n\t" : : "a" (x))

#define INVALIDATE(x) ({ unsigned _v; \
  asm volatile ("flushinv [%1]; %0=[%1];\n\t" : "=d" (_v) : "a" (x)); _v; })

static inline void sync_write32 (unsigned *addr, unsigned val)
{
  *addr=val;
  FLUSH(addr);
}

static inline void update_target (dmadsc_t *p, unsigned val)
{
  p->sal=val;
  p->sah=val>>16;
  FLUSH(p);
}

static inline void *get_pointer (dmadsc_t *p)
{
  return (void *)((p->sah<<16)|p->sal);
}

static inline unsigned sync_read32 (unsigned *addr)
{
  return INVALIDATE(addr);
}


extern bfdmactrl_t *alloc_dmactrl (int nmax);
extern void bfmdma_reset_chain (bfdmactrl_t *dmas);

extern void dma_add_block_move (bfdmactrl_t *dmac, int ws,
				unsigned *dsta, int dw, int dh, int ds,
				unsigned *srca, int sw, int sh, int ss);

extern void dma_add_move (bfdmactrl_t *dmac, unsigned *dsta, unsigned *srca, int n);

extern void dma_add_stop_flag (bfdmactrl_t *dmac);
extern void dma_print (bfdmactrl_t *dmac);

extern unsigned  bfmdma (bfdmactrl_t *dmac);

extern unsigned bfdma_wait (bfdmactrl_t *dmac);

#endif

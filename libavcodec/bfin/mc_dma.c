#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "bfin_cache.h"
#include "mdma.h"
#include "l1dsram.h"
#include "mc_dma.h"
#undef printf
#undef fprintf
#undef time

/* actually reading data size */
#define BW1 18
#define BH1 17
#define CW1 10
#define CH1 9

typedef struct _CSP
{
	 uint8_t *y;
	 uint8_t *u;
	 uint8_t *v;
}CSP;

typedef struct _DecCtx
{
	 int32_t flags;
	 int32_t linesize;
	 int32_t uvlinesize;
	 int32_t mb_height;
	 int32_t mb_width;
	 int32_t mb_x;
	 int32_t mb_y;
	 int32_t mv_type;
	 int32_t dir;
	 int32_t dxy[2][2][4];	 	 
     uint8_t offset[2][2][4];
	 int32_t no_rounding;
	 int32_t qscale;
	 int32_t chroma_qscale;
	 int32_t block_last_index[12];  ///< last non zero coefficient in block
	 ScanTable inter_scantable; ///< if inter == intra then intra should be used to reduce tha cache usage
	 int32_t mpeg_quant; ///< use MPEG quantizers instead of H.263
	 uint8_t *dest_y;
	 uint8_t *dest_u;
	 uint8_t *dest_v;
}DecCtx;

/* A MC buffer structur for MDMA use */
typedef struct _McPara{
	 int32_t mb_done;
	 int32_t do_nothing;
	 int32_t hurry_up;
	 int32_t skip_idct;
	 CSP   refmb[2];
	 CSP   outbuf;
	 int16_t *residual;
	 DecCtx decctx;
	 struct MCPara *prev;
}McPara;

McPara mc_para[2];
McPara *mcp_ptr;

static uint32_t first_inter_mb = 0;

static uint32_t dma_1mv_sign __attribute__ ((aligned(32)));
static uint32_t dma_1mv_done __attribute__ ((aligned(32)));

typedef void (* op_pixels2_func)(uint8_t * const dst, const uint32_t dstride,
								 const uint8_t * const src, const uint32_t sstride, 
								 const uint32_t rounding);

extern void put_dct(MpegEncContext *s,
					DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale);

#ifndef __BFIN__ 
uint8_t ref_by_tmp0[18*24];
uint8_t ref_bu_tmp0[9*12];
uint8_t ref_bv_tmp0[9*12];
uint8_t ref_fy_tmp0[18*24];
uint8_t ref_fu_tmp0[9*12];
uint8_t ref_fv_tmp0[9*12];
int16_t residual0[8*64];
uint8_t outbuf_y_tmp0[256];
uint8_t outbuf_u_tmp0[64];
uint8_t outbuf_v_tmp0[64];

uint8_t ref_by_tmp1[18*24];
uint8_t ref_bu_tmp1[9*12];
uint8_t ref_bv_tmp1[9*12];
uint8_t ref_fy_tmp1[18*24];
uint8_t ref_fu_tmp1[9*12];
uint8_t ref_fv_tmp1[9*12];
int16_t residual1[8*64];
uint8_t outbuf_y_tmp1[256];
uint8_t outbuf_u_tmp1[64];
uint8_t outbuf_v_tmp1[64];

#else

#define TRUNC(x) ((uint32_t)(x)&~1)
 
#define INIT_DSC(num, dsc, src, cfg, xc, xm, yc, ym)	\
	 (*(uint16_t *)(dsc##num+SAL))  = TRUNC(src);			 \
	 (*(uint16_t *)(dsc##num+SAH))  = (uint32_t)(src)>>16;	 \
	 (*(uint16_t *)(dsc##num+CFG))  = cfg;  \
	 (*(uint16_t *)(dsc##num+XCNT)) = xc;	\
	 (*(uint16_t *)(dsc##num+XMOD)) = xm;	\
	 (*(uint16_t *)(dsc##num+YCNT)) = yc;	\
	 (*(uint16_t *)(dsc##num+YMOD)) = ym

#define UPDATE_DSC(num, dsc, src)					\
	 (*(uint16_t *)(dsc##num+SAL)) = TRUNC(src);	\
	 (*(uint16_t *)(dsc##num+SAH)) = (uint32_t)(src)>>16

#ifdef DMA_DEBUG
#define DSC_PRINT(begin, end)					\
	 {											\
		  uint8_t *ptr;						\
		  static int32_t flag = 0;              \
		  if(!flag++){												\
			   printf("Descriptor:%s\n",#begin);					\
			   for(ptr=(uint16_t *)begin; ptr<=(uint16_t *)end; ptr+=DTOR_SIZE){ \
					printf("SAL=0x%x\tSAH=0x%x\n"						\
						   "CFG=0x%x\n"									\
						   "XCNT=%d\tXMOD=%d\n"							\
						   "YCNT=%d\tYMOD=%d\n",						\
						   (*(uint16_t*)(ptr+SAL)),(*(uint16_t*)(ptr+SAH)), \
						   (*(uint16_t*)(ptr+CFG)),						\
						   (*(uint16_t*)(ptr+XCNT)),(*(int16_t*)(ptr+XMOD)), \
						   (*(uint16_t*)(ptr+YCNT)),(*(int16_t*)(ptr+YMOD))); \
			   }														\
		  }																\
	 }
#else
#define DSC_PRINT(begin, end)
#endif

static inline void
dma_write_cur_mb(uint8_t *dst_y, uint8_t *dst_u, uint8_t *dst_v, 
				 uint32_t dst_y_stride, uint32_t dst_c_stride, 
				 uint8_t *src_y, uint8_t *src_u, uint8_t *src_v)
{
	 int32_t i;
	 uint8_t *dst_ptr;

	 start_cache_timer();
	 asm volatile ("csync;");	
	 for(i=0,dst_ptr=dst_y ; i<16; i++,dst_ptr+=dst_c_stride){
		  cache_clean((uint32_t)dst_ptr, (uint32_t)(dst_ptr+16));
	 }
	 for(i=0,dst_ptr=dst_u; i<8; i++){
		  cache_clean((uint32_t)dst_ptr, (uint32_t)(dst_ptr+8));
		  dst_ptr += dst_c_stride;
	 }
	 for(i=0,dst_ptr=dst_v; i<8; i++){
		  cache_clean((uint32_t)dst_ptr, (uint32_t)(dst_ptr+8));
		  dst_ptr += dst_c_stride;
	 }
	 asm volatile ("ssync;");

	 while((*(uint32_t *)DMA_WRT_DONE) == 0);
	 UPDATE_DSC(3, DSC_SRC_Y0_WRT_CH, src_y);
	 UPDATE_DSC(3, DSC_DST_Y0_WRT_CH, dst_y);
	 UPDATE_DSC(3, DSC_SRC_U0_WRT_CH, src_u);
	 UPDATE_DSC(3, DSC_DST_U0_WRT_CH, dst_u);
	 UPDATE_DSC(3, DSC_SRC_V0_WRT_CH, src_v);
	 UPDATE_DSC(3, DSC_DST_V0_WRT_CH, dst_v);
	 
	 (*(uint32_t *)DMA_WRT_DONE) = 0;
	 bfin_dodma_ch3((uint32_t *)DSC_SRC_Y0_WRT_CH3, (uint32_t *)DSC_DST_Y0_WRT_CH3, 
					(uint32_t)SRC_CFG_2D_W|((uint32_t)DST_CFG_2D_W<<16));
	 stop_cache_timer();
}

#endif//__BFIN__

static inline void dma_wait()
{
#if __BFIN__
	 start_wait_timer();
	 while(sync_read32(&dma_1mv_done) == 0);
	 stop_wait_timer();
#else
	 return;
#endif
}

static inline void
dma_read_ref_1mv(uint8_t *src_y, uint8_t *src_u, uint8_t *src_v, 
				 uint32_t src_y_stride, uint32_t src_c_stride,  
				 uint8_t *dst_y, uint8_t *dst_u, uint8_t *dst_v)
{
	 int32_t i;
	 uint8_t *src_ptr;

#ifdef __BFIN__
	 // Cache flush 
	 start_cache_timer();

	 asm volatile ("csync;");	 
	 for(i=0,src_ptr = src_y; i<BH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+BW1));
	 }
	 for(i=0,src_ptr = src_u; i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_v; i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 asm volatile ("ssync;");	

	 dma_wait();

	 UPDATE_DSC(1, DSC_SRC_Y0_CH, src_y);
	 UPDATE_DSC(1, DSC_SRC_U0_CH, src_u);
	 UPDATE_DSC(1, DSC_SRC_V0_CH, src_v);
	 UPDATE_DSC(1, DSC_DST_Y0_CH, dst_y);	 
	 UPDATE_DSC(1, DSC_DST_U0_CH, dst_u);
	 UPDATE_DSC(1, DSC_DST_V0_CH, dst_v);
	 sync_write32(&dma_1mv_done, 0);
	 bfin_dodma(1, 
				(uint32_t *)DSC_SRC_Y0_CH1, (uint32_t *)DSC_DST_Y0_CH1, 
				(uint32_t)SRC_CFG_2D_H|((uint32_t)DST_CFG_2D_H<<16));
		     
	 stop_cache_timer();
#else
	 uint8_t *dst_ptr; 
	 for(i=0, src_ptr=src_y, dst_ptr=dst_y; i<BH1; i++){					
		  memcpy(dst_ptr, src_ptr, BW1);
		  src_ptr += src_y_stride;   
		  dst_ptr += BW;
	 }
	 for(i=0, src_ptr=src_u, dst_ptr=dst_u; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride;
		  dst_ptr += CW;
	 }
	 for(i=0, src_ptr=src_v, dst_ptr=dst_v; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride;
		  dst_ptr += CW;
	 }	 
#endif
}

static inline void
dma_read_ref_bidir(int32_t channel,
				   uint8_t *src_y[2], uint8_t *src_u[2], uint8_t *src_v[2], 
				   uint32_t src_y_stride, uint32_t src_c_stride,  
				   uint8_t *dst_fy, uint8_t *dst_fu, uint8_t *dst_fv,
				   uint8_t *dst_by, uint8_t *dst_bu, uint8_t *dst_bv)
{
	 int32_t i;
	 uint8_t *src_ptr;

#ifdef __BFIN__
	 start_cache_timer();
	 
	 // Cache flush 
	 asm volatile ("csync;");	
	 for(i=0,src_ptr = src_y[0]; i<BH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+BW1));
	 }
	 for(i=0,src_ptr = src_y[1]; i<BH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+BW1));
	 }
	 for(i=0,src_ptr = src_u[0]; i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_u[1]; i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_v[0]; i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_v[1]; i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 asm volatile ("ssync;");
	
	 dma_wait();

	 UPDATE_DSC(1, DSC_SRC_FY_CH, src_y[0]);
	 UPDATE_DSC(1, DSC_SRC_FU_CH, src_u[0]);
	 UPDATE_DSC(1, DSC_SRC_FV_CH, src_v[0]);
	 UPDATE_DSC(1, DSC_SRC_BY_CH, src_y[1]);
	 UPDATE_DSC(1, DSC_SRC_BU_CH, src_u[1]);
	 UPDATE_DSC(1, DSC_SRC_BV_CH, src_v[1]);
	 
	 UPDATE_DSC(1, DSC_DST_FY_CH, dst_fy);	 
	 UPDATE_DSC(1, DSC_DST_FU_CH, dst_fu);
	 UPDATE_DSC(1, DSC_DST_FV_CH, dst_fv);
	 UPDATE_DSC(1, DSC_DST_BY_CH, dst_by);	 
	 UPDATE_DSC(1, DSC_DST_BU_CH, dst_bu);
	 UPDATE_DSC(1, DSC_DST_BV_CH, dst_bv);

	 sync_write32(&dma_1mv_done, 0);
	 
	 bfin_dodma(1, 
				(uint32_t *)DSC_SRC_BY_CH1, (uint32_t *)DSC_DST_BY_CH1, 
				(uint32_t)SRC_CFG_2D_H|((uint32_t)DST_CFG_2D_H<<16));

	 stop_cache_timer();
#else
	 uint8_t *dst_ptr; 	  
	 for(i=0, src_ptr=src_y[0], dst_ptr=dst_fy; i<BH1; i++){					
		  memcpy(dst_ptr, src_ptr, BW1);
		  src_ptr += src_y_stride;   
		  dst_ptr += BW;
	 }
	 for(i=0, src_ptr=src_u[0], dst_ptr=dst_fu; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride;
		  dst_ptr += CW;
	 }
	 for(i=0, src_ptr=src_v[0], dst_ptr=dst_fv; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW);
		  src_ptr += src_c_stride;
		  dst_ptr += CW;
	 }
	 for(i=0, src_ptr=src_y[1], dst_ptr=dst_by; i<BH1; i++){					
		  memcpy(dst_ptr, src_ptr, BW1);
		  src_ptr += src_y_stride;   
		  dst_ptr += BW;
	 }
	 for(i=0, src_ptr=src_u[1], dst_ptr=dst_bu; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride;
		  dst_ptr += CW;
	 }
	 for(i=0, src_ptr=src_v[1], dst_ptr=dst_bv; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride;
		  dst_ptr += CW;
	 }
#endif
}

static inline void
dma_read_ref_4mv(int32_t channel,
				 uint8_t *src_y0, uint8_t *src_y1, uint8_t *src_y2, uint8_t *src_y3, 
				 uint8_t *src_u,  uint8_t *src_v,  uint32_t src_y_stride, uint32_t src_c_stride,		
				 uint8_t *dst_y,  uint8_t *dst_u,  uint8_t *dst_v)
{
	 int32_t i;
	 uint8_t *src_ptr;
	 uint8_t *dst_y0, *dst_y1, *dst_y2, *dst_y3;

#ifdef __BFIN__
	 start_cache_timer();
	 dst_y0 = dst_y;
	 dst_y1 = dst_y0  + (BW>>1);
	 dst_y2 = dst_y0  + (BW*BH>>1);
	 dst_y3 = dst_y2  + (BW>>1);
	 
	 // Cache flush
	 asm volatile ("csync;");	
	 for(i=0,src_ptr = src_y0; i<CH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_y1; i<CH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_y2; i<CH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_y3; i<CH1; i++,src_ptr+=src_y_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_u;  i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 for(i=0,src_ptr = src_v;  i<CH1; i++,src_ptr+=src_c_stride){
		  cache_flush((uint32_t)src_ptr, (uint32_t)(src_ptr+CW1));
	 }
	 asm volatile ("ssync;");	

	 dma_wait();

	 UPDATE_DSC(1, DSC_SRC_Y0_4MV_CH, src_y0);
	 UPDATE_DSC(1, DSC_SRC_Y1_4MV_CH, src_y1);
	 UPDATE_DSC(1, DSC_SRC_Y2_4MV_CH, src_y2);
	 UPDATE_DSC(1, DSC_SRC_Y3_4MV_CH, src_y3);
	 UPDATE_DSC(1, DSC_SRC_U0_4MV_CH, src_u);
	 UPDATE_DSC(1, DSC_SRC_V0_4MV_CH, src_v);
	 UPDATE_DSC(1, DSC_DST_Y0_4MV_CH, dst_y0);		  
	 UPDATE_DSC(1, DSC_DST_Y1_4MV_CH, dst_y1);
	 UPDATE_DSC(1, DSC_DST_Y2_4MV_CH, dst_y2);
	 UPDATE_DSC(1, DSC_DST_Y3_4MV_CH, dst_y3);		  
	 UPDATE_DSC(1, DSC_DST_U0_4MV_CH, dst_u);
	 UPDATE_DSC(1, DSC_DST_V0_4MV_CH, dst_v);

	 sync_write32(&dma_1mv_done, 0);
	 
	 bfin_dodma(1, 
				(uint32_t *)DSC_SRC_Y0_4MV_CH1, (uint32_t *)DSC_DST_Y0_4MV_CH1, 
				(uint32_t)SRC_CFG_2D_H|((uint32_t)DST_CFG_2D_H<<16)); 

	 stop_cache_timer();
#else
	 uint8_t *dst_ptr;

	 dst_y0 = dst_y;
	 dst_y1 = dst_y0  + (BW>>1);
	 dst_y2 = dst_y0  + (BW*BH>>1);
	 dst_y3 = dst_y2  + (BW>>1);

	 for(i=0; i<CH1; i++){
		  memcpy(dst_y0, src_y0, CW1);
		  memcpy(dst_y1, src_y1, CW1);
		  memcpy(dst_y2, src_y2, CW1);
		  memcpy(dst_y3, src_y3, CW1);
		  dst_y0 += BW;
		  dst_y1 += BW;
		  dst_y2 += BW;
		  dst_y3 += BW;
		  src_y0 += src_y_stride;
		  src_y1 += src_y_stride;
		  src_y2 += src_y_stride;
		  src_y3 += src_y_stride;
	 }
	 for(i=0, src_ptr=src_u, dst_ptr=dst_u; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride; 
		  dst_ptr += CW;
	 }
	 for(i=0, src_ptr=src_v, dst_ptr=dst_v; i<CH1; i++){
		  memcpy(dst_ptr, src_ptr, CW1);
		  src_ptr += src_c_stride; 
		  dst_ptr += CW;
	 }

#endif//__BFIN__		
}

void dma_para_init(MpegEncContext *s)
{
#ifndef __BFIN__
	 mc_para[0].refmb[0].y = (void*)ref_by_tmp0;
	 mc_para[0].refmb[0].u = (void*)ref_bu_tmp0;
	 mc_para[0].refmb[0].v = (void*)ref_bv_tmp0;
	 mc_para[0].refmb[1].y = (void*)ref_fy_tmp0;
	 mc_para[0].refmb[1].u = (void*)ref_fu_tmp0;
	 mc_para[0].refmb[1].v = (void*)ref_fv_tmp0;
	 mc_para[0].residual   = residual0;
	 mc_para[0].outbuf.y   = (void*)outbuf_y_tmp0; 	 
	 mc_para[0].outbuf.u   = (void*)outbuf_u_tmp0;
	 mc_para[0].outbuf.v   = (void*)outbuf_v_tmp0; 	 
	 
	 mc_para[1].refmb[0].y = (void*)ref_by_tmp1;
	 mc_para[1].refmb[0].u = (void*)ref_bu_tmp1;
	 mc_para[1].refmb[0].v = (void*)ref_bv_tmp1;
	 mc_para[1].refmb[1].y = (void*)ref_fy_tmp1;
	 mc_para[1].refmb[1].u = (void*)ref_fu_tmp1;
	 mc_para[1].refmb[1].v = (void*)ref_fv_tmp1;
	 mc_para[1].residual   = residual1;
	 mc_para[1].outbuf.y   = (void*)outbuf_y_tmp1; 	 
	 mc_para[1].outbuf.u   = (void*)outbuf_u_tmp1;
	 mc_para[1].outbuf.v   = (void*)outbuf_v_tmp1; 	 	 	 
	 
#else
     mc_para[0].refmb[0].y = (void*)DMA_MCBUF_BY0;
	 mc_para[0].refmb[0].u = (void*)DMA_MCBUF_BU0;
	 mc_para[0].refmb[0].v = (void*)DMA_MCBUF_BV0;
	 mc_para[0].refmb[1].y = (void*)DMA_MCBUF_FY0;
	 mc_para[0].refmb[1].u = (void*)DMA_MCBUF_FU0;
	 mc_para[0].refmb[1].v = (void*)DMA_MCBUF_FV0;
	 mc_para[0].residual   = (void*)DMA_DCTBUF0;
	 mc_para[0].outbuf.y   = (void*)DMA_MBBUF_Y0; 	 
	 mc_para[0].outbuf.u   = (void*)DMA_MBBUF_U0;
	 mc_para[0].outbuf.v   = (void*)DMA_MBBUF_V0; 	 
	 
	 mc_para[1].refmb[0].y = (void*)DMA_MCBUF_BY1;
	 mc_para[1].refmb[0].u = (void*)DMA_MCBUF_BU1;
	 mc_para[1].refmb[0].v = (void*)DMA_MCBUF_BV1;
	 mc_para[1].refmb[1].y = (void*)DMA_MCBUF_FY1;
	 mc_para[1].refmb[1].u = (void*)DMA_MCBUF_FU1;
	 mc_para[1].refmb[1].v = (void*)DMA_MCBUF_FV1;
	 mc_para[1].residual   = (void*)DMA_DCTBUF1;
	 mc_para[1].outbuf.y   = (void*)DMA_MBBUF_Y1; 
	 mc_para[1].outbuf.u   = (void*)DMA_MBBUF_U1; 
	 mc_para[1].outbuf.v   = (void*)DMA_MBBUF_V1; 

	 /* NOTICE : the argument "s->width" may be WRONG! */
	 /* initialize 1mv reading descriptors */
	 INIT_DSC(1, DSC_SRC_Y0_CH, 0, SRC_CFG_2D_H, BW1>>1, XMOD_H, BH1, s->h_edge_pos-(BW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_Y0_CH, 0, DST_CFG_2D_H, BW1>>1, XMOD_H, BH1, BW-(BW1-XMOD_H));	
	 INIT_DSC(1, DSC_SRC_U0_CH, 0, SRC_CFG_2D_H, CW1>>1, XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_U0_CH, 0, DST_CFG_2D_H, CW1>>1, XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_V0_CH, 0, SRC_CFG_3HW_2D_H, CW1>>1, XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_V0_CH, 0, DST_CFG_3HW_2D_H, CW1>>1, XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DMA_1MV_BEG_CH, &dma_1mv_sign,  STP_SRC_CFG_H, 1, XMOD_H, 0, 0);
	 INIT_DSC(1, DMA_1MV_END_CH, &dma_1mv_done,  STP_DST_CFG_H, 1, XMOD_H, 0, 0);
	 
	 /* initialize 4mvs reading descriptors */
	 INIT_DSC(1, DSC_SRC_Y0_4MV_CH, 0, SRC_CFG_2D_H, CW1>>1, XMOD_H, CH1, s->h_edge_pos-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_Y0_4MV_CH, 0, DST_CFG_2D_H, CW1>>1, XMOD_H, CH1, BW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_Y1_4MV_CH, 0, SRC_CFG_2D_H, CW1>>1, XMOD_H, CH1, s->h_edge_pos-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_Y1_4MV_CH, 0, DST_CFG_2D_H, CW1>>1, XMOD_H, CH1, BW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_Y2_4MV_CH, 0, SRC_CFG_2D_H, CW1>>1, XMOD_H, CH1, s->h_edge_pos-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_Y2_4MV_CH, 0, DST_CFG_2D_H, CW1>>1, XMOD_H, CH1, BW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_Y3_4MV_CH, 0, SRC_CFG_2D_H, CW1>>1, XMOD_H, CH1, s->h_edge_pos-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_Y3_4MV_CH, 0, DST_CFG_2D_H, CW1>>1, XMOD_H, CH1, BW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_U0_4MV_CH, 0, SRC_CFG_2D_H, CW1>>1, XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_U0_4MV_CH, 0, DST_CFG_2D_H, CW1>>1, XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_V0_4MV_CH, 0, SRC_CFG_3HW_2D_H, CW1>>1, XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_V0_4MV_CH, 0, DST_CFG_3HW_2D_H, CW1>>1, XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DMA_4MV_BEG_CH, &dma_1mv_sign,  STP_SRC_CFG_H, 1, XMOD_H, 0, 0);
	 INIT_DSC(1, DMA_4MV_END_CH, &dma_1mv_done,  STP_DST_CFG_H, 1, XMOD_H, 0, 0);
	 
	 /* B Frame */
	 INIT_DSC(1, DSC_SRC_FY_CH, 0, SRC_CFG_2D_H, BW1>>1,  XMOD_H, BH1, s->h_edge_pos-(BW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_FY_CH, 0, DST_CFG_2D_H, BW1>>1,  XMOD_H, BH1, BW-(BW1-XMOD_H));	
	 INIT_DSC(1, DSC_SRC_FU_CH, 0, SRC_CFG_2D_H, CW1>>1,  XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_FU_CH, 0, DST_CFG_2D_H, CW1>>1,  XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_FV_CH, 0, SRC_CFG_2D_H, CW1>>1,  XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_FV_CH, 0, DST_CFG_2D_H, CW1>>1,  XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_BY_CH, 0, SRC_CFG_2D_H, BW1>>1,  XMOD_H, BH1, s->h_edge_pos-(BW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_BY_CH, 0, DST_CFG_2D_H, BW1>>1,  XMOD_H, BH1, BW-(BW1-XMOD_H));	
	 INIT_DSC(1, DSC_SRC_BU_CH, 0, SRC_CFG_2D_H, CW1>>1,  XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_BU_CH, 0, DST_CFG_2D_H, CW1>>1,  XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_SRC_BV_CH, 0, SRC_CFG_2D_H, CW1>>1,  XMOD_H, CH1, (s->h_edge_pos>>1)-(CW1-XMOD_H));
	 INIT_DSC(1, DSC_DST_BV_CH, 0, DST_CFG_2D_H, CW1>>1,  XMOD_H, CH1, CW-(CW1-XMOD_H));
	 INIT_DSC(1, DMA_BI_BEG_CH, &dma_1mv_sign,  STP_SRC_CFG_H, 1, XMOD_H, 0, 0);
	 INIT_DSC(1, DMA_BI_END_CH, &dma_1mv_done,  STP_DST_CFG_H, 1, XMOD_H, 0, 0);
	 	 
	 /* initiallize current mb writing descriptors */
	 INIT_DSC(3, DSC_SRC_Y0_WRT_CH, 0, SRC_CFG_2D_W, 4, XMOD_W, 16, XMOD_W);
	 INIT_DSC(3, DSC_DST_Y0_WRT_CH, 0, DST_CFG_2D_W, 4, XMOD_W, 16, s->h_edge_pos-(16-XMOD_W));
	 INIT_DSC(3, DSC_SRC_U0_WRT_CH, 0, SRC_CFG_2D_W, 2, XMOD_W,  8, XMOD_W);
	 INIT_DSC(3, DSC_DST_U0_WRT_CH, 0, DST_CFG_2D_W, 2, XMOD_W,  8, (s->h_edge_pos>>1)-(8-XMOD_W));
	 INIT_DSC(3, DSC_SRC_V0_WRT_CH, 0, SRC_CFG_3HW_2D_W, 2, XMOD_W,  8, XMOD_W);
	 INIT_DSC(3, DSC_DST_V0_WRT_CH, 0, DST_CFG_3HW_2D_W, 2, XMOD_W,  8, (s->h_edge_pos>>1)-(8-XMOD_W));
	 INIT_DSC(3, DMA_WRT_BEG_CH, DMA_WRT_SIGN, STP_SRC_CFG_W, 1, XMOD_W, 0, 0);
	 INIT_DSC(3, DMA_WRT_END_CH, DMA_WRT_DONE, STP_DST_CFG_W, 1, XMOD_W, 0, 0);

	 sync_write32(&dma_1mv_sign, -1);
	 sync_write32(&dma_1mv_done, -1);
	 (*(uint32_t *)DMA_WRT_SIGN) = -1; 
	 (*(uint32_t *)DMA_WRT_DONE) = -1; 

#endif	 
	 mc_para[0].mb_done = 1;
	 mc_para[1].mb_done = 1;
	 mcp_ptr = &mc_para[0];
	 mcp_ptr->prev = &mc_para[1]; 
	 mc_para[1].prev = &mc_para[0];	 	 
}

inline void MPV_dma_update_buffer(int16_t (**block)[64])
{
	 //memset(mcp_ptr->residual, 0, 64*6*sizeof(int16_t));
	 *block = mcp_ptr->residual; //cleared later
	 mcp_ptr->mb_done = 1;
	 mcp_ptr->do_nothing = 0;
	 mcp_ptr->hurry_up = 0;
	 mcp_ptr->skip_idct = 0;	 
}

static inline void dma_hpel_motion_read(MpegEncContext *s, DecCtx *p, int32_t dir, 
											uint8_t **ptr_y, uint8_t *src,
											int field_based, int field_select,
											int i,
											int src_x, int src_y,
											int width, int height, int stride,
											int h_edge_pos, int v_edge_pos,
											int w, int h, op_pixels_func *pix_op,
											int motion_x, int motion_y)
{
    int emu=0;
	uint8_t *ybuf;

    p->dxy[dir][0][i] = ((motion_y & 1) << 1) | (motion_x & 1);

    src_x += motion_x >> 1;
    src_y += motion_y >> 1;

    /* WARNING: do no forget half pels */
    src_x = av_clip(src_x, -16, width); //FIXME unneeded for emu?
    if (src_x == width)
        p->dxy[dir][0][i] &= ~1;
    src_y = av_clip(src_y, -16, height);
    if (src_y == height)
        p->dxy[dir][0][i] &= ~2;
    src += src_y * stride + src_x;

	p->offset[dir][0][i] = src_x & 1;

    if(s->unrestricted_mv && (s->flags&CODEC_FLAG_EMU_EDGE)){
		 if(   (unsigned)src_x > h_edge_pos - (motion_x&1) - (CW1-1)
			   || (unsigned)src_y > v_edge_pos - (motion_y&1) - (CH1-1)){
			 ybuf = s->edge_emu_buffer + (i & 1) * CW + (i >> 1) * CH * stride;
			 p->offset[dir][0][i] = 0;
			 ff_emulated_edge_mc(ybuf,
								src, s->linesize, CW1, CH1,
								src_x, src_y, h_edge_pos, v_edge_pos);
			 src = ybuf;
			 emu=1;
        }
    }
	*ptr_y = src;
}

static inline void dma_chroma_4mv_motion_read(MpegEncContext *s, DecCtx *p, int32_t dir,
												  uint8_t **ptr_cb, uint8_t **ptr_cr,
												  uint8_t **ref_picture,
												  op_pixels_func *pix_op,
												  int mx, int my)
{
    int src_x, src_y, offset;
	uint8_t *uvbuf;

    /* In case of 8X8, we construct a single chroma motion vector
       with a special rounding */
    mx= ff_h263_round_chroma(mx);
    my= ff_h263_round_chroma(my);

    p->dxy[dir][1][0] = ((my & 1) << 1) | (mx & 1);
    mx >>= 1;
    my >>= 1;

    src_x = s->mb_x * 8 + mx;
    src_y = s->mb_y * 8 + my;
    src_x = av_clip(src_x, -8, s->width/2);
    if (src_x == s->width/2)
        p->dxy[dir][1][0] &= ~1;
    src_y = av_clip(src_y, -8, s->height/2);
    if (src_y == s->height/2)
        p->dxy[dir][1][0] &= ~2;
	
	p->offset[dir][1][0] = src_x & 1;

    offset = (src_y * (s->uvlinesize)) + src_x;
    *ptr_cb = ref_picture[1] + offset;
	*ptr_cr = ref_picture[2] + offset;

    if(s->flags&CODEC_FLAG_EMU_EDGE){
		 if(   (unsigned)src_x > (s->h_edge_pos>>1) - (p->dxy[dir][1][0] & 1) - (CW1-1)
			   || (unsigned)src_y > (s->v_edge_pos>>1) - (p->dxy[dir][1][0] >> 1) - (CH1-1)){
			  uvbuf = s->edge_emu_buffer+18*s->linesize;
			  p->offset[dir][1][0] = 0;
			  ff_emulated_edge_mc(uvbuf, 
								  *ptr_cb, s->uvlinesize, CW1, CH1, 
								  src_x, src_y, s->h_edge_pos>>1, s->v_edge_pos>>1);
			  *ptr_cb = uvbuf;
			  ff_emulated_edge_mc(uvbuf + 16, 
								  *ptr_cr, s->uvlinesize, CW1, CH1, 
								  src_x, src_y, s->h_edge_pos>>1, s->v_edge_pos>>1);
			  *ptr_cr = uvbuf + 16;
		 }
    }
}

/* apply one mpeg motion vector to the three components */
static inline void dma_mpeg_motion_read(MpegEncContext *s, DecCtx *p, int32_t dir,
											uint8_t **ptr_y, uint8_t **ptr_cb, uint8_t **ptr_cr,
											uint8_t **ref_picture)
{
	 int32_t src_x, src_y, uvsrc_x, uvsrc_y, uvlinesize, linesize;
	 int32_t motion_x, motion_y;

	 linesize   = s->current_picture.linesize[0];
	 uvlinesize = s->current_picture.linesize[1];
	 motion_x = s->mv[dir][0][0];
	 motion_y = s->mv[dir][0][1];

	 p->dxy[dir][0][0] = ((motion_y & 1) << 1) | (motion_x & 1);
	 p->offset[dir][0][0] = (motion_x >> 1) & 1;

	 src_x = (s->mb_x << 4) + (motion_x >> 1);
	 src_y = (s->mb_y << 4) + (motion_y >> 1);

	 /* assume out format is H263 */
	 p->dxy[dir][1][0] = p->dxy[dir][0][0] | (motion_y & 2) | ((motion_x & 2) >> 1);
	 p->offset[dir][1][0] = (motion_x >> 2) & 1;

	 uvsrc_x = src_x>>1;
	 uvsrc_y = src_y>>1;	 

	 *ptr_y  = ref_picture[0] + src_y * linesize + src_x;
	 *ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x;
	 *ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

	 if((unsigned)src_x > s->h_edge_pos - (motion_x&1) - (BW1-1)
		|| (unsigned)src_y > s->v_edge_pos - (motion_y&1) - (BH1-1)){
		  uint8_t *ybuf = s->edge_emu_buffer + 32*dir;
		  ff_emulated_edge_mc(ybuf, *ptr_y, s->linesize, BW1, BH1,
							  src_x, src_y, s->h_edge_pos, s->v_edge_pos);
		  *ptr_y = ybuf;
		  p->offset[dir][0][0] = 0;
		  p->offset[dir][1][0] = 0;
		  if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
			   uint8_t *uvbuf= s->edge_emu_buffer+18*s->linesize +32*dir;
			   ff_emulated_edge_mc(uvbuf  , *ptr_cb, s->uvlinesize, CW1, CH1,
								   uvsrc_x, uvsrc_y, s->h_edge_pos>>1, s->v_edge_pos>>1);
			   ff_emulated_edge_mc(uvbuf+16, *ptr_cr, s->uvlinesize, CW1, CH1,
								   uvsrc_x, uvsrc_y, s->h_edge_pos>>1, s->v_edge_pos>>1);
			   *ptr_cb= uvbuf;
			   *ptr_cr= uvbuf+16;
		  }
	 }
}

/********************************************************************************
 * dma read and write functions
 ********************************************************************************/
void inline 
dma_read_ref(MpegEncContext *s, McPara *para)
{
	 int32_t i, mx, my;
	 uint8_t *tmp[4];
	 int32_t linesize, uvlinesize;
	 uint8_t *ptr_y[2] = {0}, *ptr_cb[2] = {0}, *ptr_cr[2] ={0};
     DecCtx *decctx = &para->decctx;

	 // Assume field_based = 0!
	 linesize   = s->current_picture.linesize[0];
	 uvlinesize = s->current_picture.linesize[1];

	 decctx->linesize = linesize;
	 decctx->uvlinesize = uvlinesize;
	 decctx->mb_width = s->mb_width;
	 decctx->mb_height = s->mb_height;
	 decctx->mb_x = s->mb_x;
	 decctx->mb_y = s->mb_y;
	 decctx->mv_type = s->mv_type;
	 decctx->dir = s->mv_dir;
	 decctx->no_rounding = s->no_rounding;
	 if(s->pict_type == FF_B_TYPE)
		  decctx->no_rounding = 0;
	 decctx->dest_y = s->dest[0];
	 decctx->dest_u = s->dest[1];
	 decctx->dest_v = s->dest[2];

	 switch(s->mv_type){
	 case MV_TYPE_16X16:
		  if(s->mv_dir & MV_DIR_FORWARD){
			   dma_mpeg_motion_read(s, decctx, 0, &ptr_y[0], &ptr_cb[0], &ptr_cr[0], s->last_picture.data);
		  }
		  if(s->mv_dir & MV_DIR_BACKWARD){
			   dma_mpeg_motion_read(s, decctx, 1, &ptr_y[1], &ptr_cb[1], &ptr_cr[1], s->next_picture.data);
		  }
		  if((s->mv_dir & MV_DIR_FORWARD) && (s->mv_dir & MV_DIR_BACKWARD)){
			   /* forward & backward */
			   dma_read_ref_bidir(2,
								  ptr_y, ptr_cb, ptr_cr, 
								  linesize, uvlinesize, 
								  para->refmb[0].y, para->refmb[0].u, para->refmb[0].v,
								  para->refmb[1].y, para->refmb[1].u, para->refmb[1].v);
			   
		  }else if( s->mv_dir & MV_DIR_FORWARD){
			   /* only forward */
			   dma_read_ref_1mv(ptr_y[0], ptr_cb[0], ptr_cr[0], 
								linesize, uvlinesize, 
								para->refmb[0].y, para->refmb[0].u, para->refmb[0].v);

		  }else{
			   /* only backward */
			   dma_read_ref_1mv(ptr_y[1], ptr_cb[1], ptr_cr[1], 
								linesize, uvlinesize, 
								para->refmb[1].y, para->refmb[1].u, para->refmb[1].v);
		  }

		  break;
	 case MV_TYPE_8X8:
		  mx = 0;
		  my = 0;
		  for(i = 0; i < 4; i++){
			   dma_hpel_motion_read(s, decctx, 0, 
									&tmp[i], s->last_picture.data[0], 
									0, 0, i,
									s->mb_x * 16 + (i & 1) * 8, s->mb_y * 16 + (i >>1) * 8,
									s->width, s->height, s->linesize,
									s->h_edge_pos, s->v_edge_pos,
									8, 8, NULL,
									s->mv[0][i][0], s->mv[0][i][1]);
			   mx += s->mv[0][i][0];
			   my += s->mv[0][i][1];
		  }

		  if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
			   dma_chroma_4mv_motion_read(s, decctx, 0, 
										  &ptr_cb[0], &ptr_cr[0], 
										  s->last_picture.data, NULL, mx, my);

		  dma_read_ref_4mv(2,
						   tmp[0], tmp[1], tmp[2], tmp[3], ptr_cb[0], ptr_cr[0], 
						   s->linesize, s->uvlinesize,
						   para->refmb[0].y, para->refmb[0].u, para->refmb[0].v);

		  break;
	 case MV_TYPE_16X8:
	 case MV_TYPE_FIELD:
	 case MV_TYPE_DMV:
	 default: assert(0);   

	 }

} 

void inline 
dma_write_cur(McPara *para)
{
#ifdef __BFIN__
	 dma_write_cur_mb((para->decctx).dest_y, (para->decctx).dest_u, (para->decctx).dest_v, 
					  (para->decctx).linesize, (para->decctx).uvlinesize,
					  (para->outbuf).y, (para->outbuf).u, (para->outbuf).v);
#else
	 {
		  int32_t i;
		  uint8_t *src_ptr, *dst_ptr; 
		  
		  for(i=0,src_ptr=(para->outbuf).y, dst_ptr=(para->decctx).dest_y; i<16; i++){
			   memcpy(dst_ptr, src_ptr, 16);
			   src_ptr += 16;
			   dst_ptr += (para->decctx).linesize;
		  }
		  for(i=0,src_ptr=(para->outbuf).u, dst_ptr=(para->decctx).dest_u; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += (para->decctx).uvlinesize;
		  }
		  for(i=0,src_ptr=(para->outbuf).v, dst_ptr=(para->decctx).dest_v; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += (para->decctx).uvlinesize;
		  }
	 }
#endif
}

/***********************************************************************************************
 * interpolate functions!
 ***********************************************************************************************/
#ifndef __BFIN__ 
void transfer8x8_copy_c(uint8_t * const dst, const uint32_t dstride,
						const uint8_t * const src, const uint32_t sstride, 
						const uint32_t rounding/*useless*/)
{
	 int j, i;
	 
	 for (j = 0; j < 8; ++j) {
	    uint8_t *d = dst + j*dstride;
		const uint8_t *s = src + j*sstride;
		
		for (i = 0; i < 8; ++i)	{
			 *d++ = *s++;
		}
	 }
}

/* dst = interpolate(src) */
void
interpolate8x8_halfpel_h_c(uint8_t * const dst, const uint32_t dstride,
						   const uint8_t * const src, const uint32_t sstride, 
						   const uint32_t rounding)
{
	 uint32_t i, j;

	 if (rounding) {
		  for(i = 0, j = 0; j < 8*sstride; i+=dstride, j+=sstride) {
			   dst[i + 0] = (uint8_t)((src[j + 0] + src[j + 1] )>>1);
			   dst[i + 1] = (uint8_t)((src[j + 1] + src[j + 2] )>>1);
			   dst[i + 2] = (uint8_t)((src[j + 2] + src[j + 3] )>>1);
			   dst[i + 3] = (uint8_t)((src[j + 3] + src[j + 4] )>>1);
			   dst[i + 4] = (uint8_t)((src[j + 4] + src[j + 5] )>>1);
			   dst[i + 5] = (uint8_t)((src[j + 5] + src[j + 6] )>>1);
			   dst[i + 6] = (uint8_t)((src[j + 6] + src[j + 7] )>>1);
			   dst[i + 7] = (uint8_t)((src[j + 7] + src[j + 8] )>>1);
		  }
	 } else {
		  for(i = 0, j = 0; j < 8*sstride; i+=dstride, j+=sstride) {
			   dst[i + 0] = (uint8_t)((src[j + 0] + src[j + 1] + 1)>>1);
			   dst[i + 1] = (uint8_t)((src[j + 1] + src[j + 2] + 1)>>1);
			   dst[i + 2] = (uint8_t)((src[j + 2] + src[j + 3] + 1)>>1);
			   dst[i + 3] = (uint8_t)((src[j + 3] + src[j + 4] + 1)>>1);
			   dst[i + 4] = (uint8_t)((src[j + 4] + src[j + 5] + 1)>>1);
			   dst[i + 5] = (uint8_t)((src[j + 5] + src[j + 6] + 1)>>1);
			   dst[i + 6] = (uint8_t)((src[j + 6] + src[j + 7] + 1)>>1);
			   dst[i + 7] = (uint8_t)((src[j + 7] + src[j + 8] + 1)>>1);
		  }
	 }
}

void
interpolate8x8_halfpel_v_c(uint8_t * const dst, const uint32_t dstride,
						   const uint8_t * const src, const uint32_t sstride, 
						   const uint32_t rounding)
{
	 uint32_t i, j;

	 if(rounding){
		  for(i = 0, j = 0; j < 8*sstride; i+=dstride,j+=sstride) {
			   dst[i + 0] = (uint8_t)((src[j + 0] + src[j + sstride + 0] )>>1);
			   dst[i + 1] = (uint8_t)((src[j + 1] + src[j + sstride + 1] )>>1);
			   dst[i + 2] = (uint8_t)((src[j + 2] + src[j + sstride + 2] )>>1);
			   dst[i + 3] = (uint8_t)((src[j + 3] + src[j + sstride + 3] )>>1);
			   dst[i + 4] = (uint8_t)((src[j + 4] + src[j + sstride + 4] )>>1);
			   dst[i + 5] = (uint8_t)((src[j + 5] + src[j + sstride + 5] )>>1);
			   dst[i + 6] = (uint8_t)((src[j + 6] + src[j + sstride + 6] )>>1);
			   dst[i + 7] = (uint8_t)((src[j + 7] + src[j + sstride + 7] )>>1);
		  }
	 }else{
		  for(i = 0, j = 0; j < 8*sstride; i+=dstride,j+=sstride) {
			   dst[i + 0] = (uint8_t)((src[j + 0] + src[j + sstride + 0] + 1)>>1);
			   dst[i + 1] = (uint8_t)((src[j + 1] + src[j + sstride + 1] + 1)>>1);
			   dst[i + 2] = (uint8_t)((src[j + 2] + src[j + sstride + 2] + 1)>>1);
			   dst[i + 3] = (uint8_t)((src[j + 3] + src[j + sstride + 3] + 1)>>1);
			   dst[i + 4] = (uint8_t)((src[j + 4] + src[j + sstride + 4] + 1)>>1);
			   dst[i + 5] = (uint8_t)((src[j + 5] + src[j + sstride + 5] + 1)>>1);
			   dst[i + 6] = (uint8_t)((src[j + 6] + src[j + sstride + 6] + 1)>>1);
			   dst[i + 7] = (uint8_t)((src[j + 7] + src[j + sstride + 7] + 1)>>1);
		  }
	 }
}

void
interpolate8x8_halfpel_hv_c(uint8_t * const dst, const uint32_t dstride,
							const uint8_t * const src, const uint32_t sstride, 
							const uint32_t rounding)
{
	 uint32_t i, j;

	 if (rounding) {
		  for (i = 0, j = 0; j < 8*sstride; i+=dstride,j+=sstride) {
			   dst[i + 0] = (uint8_t)((src[j+0] + src[j+1] + src[j+sstride+0] + src[j+sstride+1] +1)>>2);
			   dst[i + 1] = (uint8_t)((src[j+1] + src[j+2] + src[j+sstride+1] + src[j+sstride+2] +1)>>2);
			   dst[i + 2] = (uint8_t)((src[j+2] + src[j+3] + src[j+sstride+2] + src[j+sstride+3] +1)>>2);
			   dst[i + 3] = (uint8_t)((src[j+3] + src[j+4] + src[j+sstride+3] + src[j+sstride+4] +1)>>2);
			   dst[i + 4] = (uint8_t)((src[j+4] + src[j+5] + src[j+sstride+4] + src[j+sstride+5] +1)>>2);
			   dst[i + 5] = (uint8_t)((src[j+5] + src[j+6] + src[j+sstride+5] + src[j+sstride+6] +1)>>2);
			   dst[i + 6] = (uint8_t)((src[j+6] + src[j+7] + src[j+sstride+6] + src[j+sstride+7] +1)>>2);
			   dst[i + 7] = (uint8_t)((src[j+7] + src[j+8] + src[j+sstride+7] + src[j+sstride+8] +1)>>2);
		  }
	 } else {
		  for (i = 0,j = 0; j < 8*sstride; i+=dstride,j+=sstride) {
			   dst[i + 0] = (uint8_t)((src[j+0] + src[j+1] + src[j+sstride+0] + src[j+sstride+1] +2)>>2);
			   dst[i + 1] = (uint8_t)((src[j+1] + src[j+2] + src[j+sstride+1] + src[j+sstride+2] +2)>>2);
			   dst[i + 2] = (uint8_t)((src[j+2] + src[j+3] + src[j+sstride+2] + src[j+sstride+3] +2)>>2);
			   dst[i + 3] = (uint8_t)((src[j+3] + src[j+4] + src[j+sstride+3] + src[j+sstride+4] +2)>>2);
			   dst[i + 4] = (uint8_t)((src[j+4] + src[j+5] + src[j+sstride+4] + src[j+sstride+5] +2)>>2);
			   dst[i + 5] = (uint8_t)((src[j+5] + src[j+6] + src[j+sstride+5] + src[j+sstride+6] +2)>>2);
			   dst[i + 6] = (uint8_t)((src[j+6] + src[j+7] + src[j+sstride+6] + src[j+sstride+7] +2)>>2);
			   dst[i + 7] = (uint8_t)((src[j+7] + src[j+8] + src[j+sstride+7] + src[j+sstride+8] +2)>>2);
		  }
	 }
}

void
transfer16x16_copy_c(uint8_t * const dst, const uint32_t dstride,
					 const uint8_t * const src, const uint32_t sstride, 
					 const uint32_t rounding/*useless */)
{
	int j, i;

	for (j = 0; j < 16; ++j) {
	    uint8_t *d = dst + j*dstride;
		const uint8_t *s = src + j*sstride;

		for (i = 0; i < 16; ++i)
		{
			*d++ = *s++;
		}
	}
}

void
interpolate16x16_halfpel_h_c(uint8_t * const dst, const uint32_t dstride,
							 const uint8_t * const src, const uint32_t sstride, 
							 const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++)
		  interpolate8x8_halfpel_h_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
									 src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
}

void
interpolate16x16_halfpel_v_c(uint8_t * const dst, const uint32_t dstride,
							 const uint8_t * const src, const uint32_t sstride, 
							 const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++)
		  interpolate8x8_halfpel_v_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
									 src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
}

void
interpolate16x16_halfpel_hv_c(uint8_t * const dst, const uint32_t dstride,
							  const uint8_t * const src, const uint32_t sstride, 
							  const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++)
		  interpolate8x8_halfpel_hv_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
									  src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
}

/* dst = (dst + interpolate(src))/2 */
void
interpolate8x8_halfpel_add_c(uint8_t * const dst, const uint32_t dstride,
							   const uint8_t * const src, const uint32_t sstride,
							   const uint32_t rounding)
{
	 uint32_t i,j;

	 for(i = 0, j = 0; j < 8*sstride; i += dstride,j += sstride){
		  dst[i + 0] = (uint8_t)((src[j + 0] + dst[i+0] + 1)>>1);
		  dst[i + 1] = (uint8_t)((src[j + 1] + dst[i+1] + 1)>>1);
		  dst[i + 2] = (uint8_t)((src[j + 2] + dst[i+2] + 1)>>1);
		  dst[i + 3] = (uint8_t)((src[j + 3] + dst[i+3] + 1)>>1);
		  dst[i + 4] = (uint8_t)((src[j + 4] + dst[i+4] + 1)>>1);
		  dst[i + 5] = (uint8_t)((src[j + 5] + dst[i+5] + 1)>>1);
		  dst[i + 6] = (uint8_t)((src[j + 6] + dst[i+6] + 1)>>1);
		  dst[i + 7] = (uint8_t)((src[j + 7] + dst[i+7] + 1)>>1);
	 }
}

void
interpolate8x8_halfpel_h_add_c(uint8_t * const dst, const uint32_t dstride,
							   const uint8_t * const src, const uint32_t sstride,
							   const uint32_t rounding)
{
	 uint32_t i,j;

	 if(rounding){
		  for(i = 0, j = 0; j < 8*sstride; i += dstride,j += sstride){
			   dst[i + 0] = (uint8_t)((((src[j + 0] + src[j + 1] )>>1) + dst[i+0] + 1)>>1);
			   dst[i + 1] = (uint8_t)((((src[j + 1] + src[j + 2] )>>1) + dst[i+1] + 1)>>1);
			   dst[i + 2] = (uint8_t)((((src[j + 2] + src[j + 3] )>>1) + dst[i+2] + 1)>>1);
			   dst[i + 3] = (uint8_t)((((src[j + 3] + src[j + 4] )>>1) + dst[i+3] + 1)>>1);
			   dst[i + 4] = (uint8_t)((((src[j + 4] + src[j + 5] )>>1) + dst[i+4] + 1)>>1);
			   dst[i + 5] = (uint8_t)((((src[j + 5] + src[j + 6] )>>1) + dst[i+5] + 1)>>1);
			   dst[i + 6] = (uint8_t)((((src[j + 6] + src[j + 7] )>>1) + dst[i+6] + 1)>>1);
			   dst[i + 7] = (uint8_t)((((src[j + 7] + src[j + 8] )>>1) + dst[i+7] + 1)>>1);
		  }
	 } else {
		  for (i = 0, j = 0; j < 8*sstride; i+=dstride,j+=sstride) {
			   dst[i + 0] = (uint8_t)((((src[j + 0] + src[j + 1] + 1)>>1) + dst[i+0] + 1)>>1);
			   dst[i + 1] = (uint8_t)((((src[j + 1] + src[j + 2] + 1)>>1) + dst[i+1] + 1)>>1);
			   dst[i + 2] = (uint8_t)((((src[j + 2] + src[j + 3] + 1)>>1) + dst[i+2] + 1)>>1);
			   dst[i + 3] = (uint8_t)((((src[j + 3] + src[j + 4] + 1)>>1) + dst[i+3] + 1)>>1);
			   dst[i + 4] = (uint8_t)((((src[j + 4] + src[j + 5] + 1)>>1) + dst[i+4] + 1)>>1);
			   dst[i + 5] = (uint8_t)((((src[j + 5] + src[j + 6] + 1)>>1) + dst[i+5] + 1)>>1);
			   dst[i + 6] = (uint8_t)((((src[j + 6] + src[j + 7] + 1)>>1) + dst[i+6] + 1)>>1);
			   dst[i + 7] = (uint8_t)((((src[j + 7] + src[j + 8] + 1)>>1) + dst[i+7] + 1)>>1);
		  }
	 }
}

void
interpolate8x8_halfpel_v_add_c(uint8_t * const dst, const uint32_t dstride,
						   const uint8_t * const src, const uint32_t sstride,
						   const uint32_t rounding)
{
	 uint32_t i, j;

	 if (rounding) {
		  for(i = 0, j = 0; j < 8*sstride; i += dstride,j += sstride){
			   dst[i + 0] = (uint8_t)((((src[j + 0] + src[j + sstride + 0] )>>1) + dst[i+0] + 1)>>1);
			   dst[i + 1] = (uint8_t)((((src[j + 1] + src[j + sstride + 1] )>>1) + dst[i+1] + 1)>>1);
			   dst[i + 2] = (uint8_t)((((src[j + 2] + src[j + sstride + 2] )>>1) + dst[i+2] + 1)>>1);
			   dst[i + 3] = (uint8_t)((((src[j + 3] + src[j + sstride + 3] )>>1) + dst[i+3] + 1)>>1);
			   dst[i + 4] = (uint8_t)((((src[j + 4] + src[j + sstride + 4] )>>1) + dst[i+4] + 1)>>1);
			   dst[i + 5] = (uint8_t)((((src[j + 5] + src[j + sstride + 5] )>>1) + dst[i+5] + 1)>>1);
			   dst[i + 6] = (uint8_t)((((src[j + 6] + src[j + sstride + 6] )>>1) + dst[i+6] + 1)>>1);
			   dst[i + 7] = (uint8_t)((((src[j + 7] + src[j + sstride + 7] )>>1) + dst[i+7] + 1)>>1);
		  }
	 } else {
		  for(i = 0, j = 0; j < 8*sstride; i += dstride,j += sstride){
			   dst[i + 0] = (uint8_t)((((src[j + 0] + src[j + sstride + 0] + 1)>>1) + dst[i+0] + 1)>>1);
			   dst[i + 1] = (uint8_t)((((src[j + 1] + src[j + sstride + 1] + 1)>>1) + dst[i+1] + 1)>>1);
			   dst[i + 2] = (uint8_t)((((src[j + 2] + src[j + sstride + 2] + 1)>>1) + dst[i+2] + 1)>>1);
			   dst[i + 3] = (uint8_t)((((src[j + 3] + src[j + sstride + 3] + 1)>>1) + dst[i+3] + 1)>>1);
			   dst[i + 4] = (uint8_t)((((src[j + 4] + src[j + sstride + 4] + 1)>>1) + dst[i+4] + 1)>>1);
			   dst[i + 5] = (uint8_t)((((src[j + 5] + src[j + sstride + 5] + 1)>>1) + dst[i+5] + 1)>>1);
			   dst[i + 6] = (uint8_t)((((src[j + 6] + src[j + sstride + 6] + 1)>>1) + dst[i+6] + 1)>>1);
			   dst[i + 7] = (uint8_t)((((src[j + 7] + src[j + sstride + 7] + 1)>>1) + dst[i+7] + 1)>>1);
		  }
	 }
}

void
interpolate8x8_halfpel_hv_add_c(uint8_t * const dst, const uint32_t dstride, 
								const uint8_t * const src, const uint32_t sstride,
								const uint32_t rounding)
{
	 uint32_t i, j;

	 if (rounding) {
		  for(i = 0, j = 0; j < 8*sstride; i += dstride,j += sstride){
			   dst[i + 0] = (uint8_t)
					((((src[j+0] + src[j+1] + src[j+sstride+0] + src[j+sstride+1] +1)>>2) + dst[i+0])>>1);
			   dst[i + 1] = (uint8_t)
					((((src[j+1] + src[j+2] + src[j+sstride+1] + src[j+sstride+2] +1)>>2) + dst[i+1])>>1);
			   dst[i + 2] = (uint8_t)
					((((src[j+2] + src[j+3] + src[j+sstride+2] + src[j+sstride+3] +1)>>2) + dst[i+2])>>1);
			   dst[i + 3] = (uint8_t)
					((((src[j+3] + src[j+4] + src[j+sstride+3] + src[j+sstride+4] +1)>>2) + dst[i+3])>>1);
			   dst[i + 4] = (uint8_t)
					((((src[j+4] + src[j+5] + src[j+sstride+4] + src[j+sstride+5] +1)>>2) + dst[i+4])>>1);
			   dst[i + 5] = (uint8_t)
					((((src[j+5] + src[j+6] + src[j+sstride+5] + src[j+sstride+6] +1)>>2) + dst[i+5])>>1);
			   dst[i + 6] = (uint8_t)
					((((src[j+6] + src[j+7] + src[j+sstride+6] + src[j+sstride+7] +1)>>2) + dst[i+6])>>1);
			   dst[i + 7] = (uint8_t)
					((((src[j+7] + src[j+8] + src[j+sstride+7] + src[j+sstride+8] +1)>>2) + dst[i+7])>>1);
		  }
	 } else {
		  for(i = 0, j = 0; j < 8*sstride; i += dstride,j += sstride){
			   dst[i + 0] = (uint8_t)((((src[j+0] + src[j+1] + src[j+sstride+0] + src[j+sstride+1] +2)>>2) + dst[i+0] + 1)>>1);
			   dst[i + 1] = (uint8_t)((((src[j+1] + src[j+2] + src[j+sstride+1] + src[j+sstride+2] +2)>>2) + dst[i+1] + 1)>>1);
			   dst[i + 2] = (uint8_t)((((src[j+2] + src[j+3] + src[j+sstride+2] + src[j+sstride+3] +2)>>2) + dst[i+2] + 1)>>1);
			   dst[i + 3] = (uint8_t)((((src[j+3] + src[j+4] + src[j+sstride+3] + src[j+sstride+4] +2)>>2) + dst[i+3] + 1)>>1);
			   dst[i + 4] = (uint8_t)((((src[j+4] + src[j+5] + src[j+sstride+4] + src[j+sstride+5] +2)>>2) + dst[i+4] + 1)>>1);
			   dst[i + 5] = (uint8_t)((((src[j+5] + src[j+6] + src[j+sstride+5] + src[j+sstride+6] +2)>>2) + dst[i+5] + 1)>>1);
			   dst[i + 6] = (uint8_t)((((src[j+6] + src[j+7] + src[j+sstride+6] + src[j+sstride+7] +2)>>2) + dst[i+6] + 1)>>1);
			   dst[i + 7] = (uint8_t)((((src[j+7] + src[j+8] + src[j+sstride+7] + src[j+sstride+8] +2)>>2) + dst[i+7] + 1)>>1);
		  }
	 }
}

#else
extern void ff_bfin_transfer8x8_copy_new(uint8_t * const dst, const uint32_t dstride,
										 const uint8_t * const src, const uint32_t sstride)__attribute__ ((l1_text));
extern void ff_bfin_interpolate8x8_new_halfpel_h(uint8_t * const dst, const uint32_t dstride,
												 const uint8_t * const src, const uint32_t sstride, 
												 const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate8x8_new_halfpel_v(uint8_t * const dst, const uint32_t dstride,
												 const uint8_t * const src, const uint32_t sstride, 
												 const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate8x8_new_halfpel_hv(uint8_t * const dst, const uint32_t dstride,
												  const uint8_t * const src, const uint32_t sstride, 
												  const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_transfer16x16_copy_new(uint8_t * const dst,
										   const uint8_t * const src, const uint32_t sstride)__attribute__ ((l1_text));
extern void ff_bfin_interpolate16x16_new_halfpel_h(uint8_t * const dst, 
												   const uint8_t * const src, const uint32_t sstride, 
												   const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate16x16_new_halfpel_v(uint8_t * const dst, 
												   const uint8_t * const src, const uint32_t sstride, 
												   const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate16x16_new_halfpel_hv(uint8_t * const dst, 
													const uint8_t * const src, const uint32_t sstride, 
													const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_transfer8x8_copy_new_add(uint8_t * const dst, const uint32_t dstride,
											 const uint8_t * const src, const uint32_t sstride, 
											 const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate8x8_new_halfpel_h_add(uint8_t * const dst, const uint32_t dstride,
													 const uint8_t * const src, const uint32_t sstride, 
													 const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate8x8_new_halfpel_v_add(uint8_t * const dst, const uint32_t dstride,
													 const uint8_t * const src, const uint32_t sstride, 
													 const uint32_t rounding)__attribute__ ((l1_text));
extern void ff_bfin_interpolate8x8_new_halfpel_hv_add(uint8_t * const dst, const uint32_t dstride,
													  const uint8_t * const src, const uint32_t sstride, 
													  const uint32_t rounding)__attribute__ ((l1_text));
#endif//!__BFIN__

static inline void
interpolate16x16_halfpel_add(uint8_t * const dst, const uint32_t dstride,
							 const uint8_t * const src, const uint32_t sstride,
							 const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++){
#ifndef __BFIN__
		  interpolate8x8_halfpel_add_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
									   src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
#else
		  ff_bfin_transfer8x8_copy_new_add(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride - 8, 
										   src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, 
										   rounding);
#endif
	 }
}
static inline void
interpolate16x16_halfpel_h_add(uint8_t * const dst, const uint32_t dstride,
							   const uint8_t * const src, const uint32_t sstride, 
							   const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++){
#ifndef __BFIN__
	      interpolate8x8_halfpel_h_add_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
										 src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
#else
		  ff_bfin_interpolate8x8_new_halfpel_h_add(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride - 8, 
												   src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, 
												   rounding);
#endif
	 }
}

static inline void
interpolate16x16_halfpel_v_add(uint8_t * const dst, const uint32_t dstride,
							   const uint8_t * const src, const uint32_t sstride, 
							   const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++){
#ifndef __BFIN__
		  interpolate8x8_halfpel_v_add_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
										 src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
#else
		  ff_bfin_interpolate8x8_new_halfpel_v_add(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride - 8, 
												   src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, 
												   rounding);
#endif
	 }
}

static inline void
interpolate16x16_halfpel_hv_add(uint8_t * const dst, const uint32_t dstride,
								const uint8_t * const src, const uint32_t sstride, 
								const uint32_t rounding)
{
	 int i;
	 for(i = 0; i < 4; i++){
#ifndef __BFIN__
		  interpolate8x8_halfpel_hv_add_c(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride, 
										  src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, rounding);
#else
		  ff_bfin_interpolate8x8_new_halfpel_hv_add(dst + (i & 1) * 8 + (i >> 1) * 8 * dstride, dstride - 8, 
													src + (i & 1) * 8 + (i >> 1) * 8 * sstride, sstride, 
													rounding);
#endif
	 }
}

/*******************************************************************************************
 * motion compensation of a single macroblock
 *******************************************************************************************/
static inline void dma_hpel_motion(DecCtx *s, int32_t dir,
								   uint8_t *dest, uint8_t *src, op_pixels2_func (*pix_op)[4],
								   int32_t index, int32_t no_rounding)
{
#ifndef __BFIN__
	 pix_op[1][s->dxy[dir][0][index]](dest, 16, src, BW, no_rounding);
#else
	 /* reuse the new interploate functions from Xvid! 
		chaos,confused interface :( */
	 switch (s->dxy[dir][0][index]){ 
	 case 0:
		  ff_bfin_transfer8x8_copy_new(dest, 8, src+s->offset[dir][0][index], BW);
		  break;
	 case 1:
		  ff_bfin_interpolate8x8_new_halfpel_h(dest, 8, src+s->offset[dir][0][index], BW, no_rounding);
		  break;
	 case 2:
		  ff_bfin_interpolate8x8_new_halfpel_v(dest, 8, src+s->offset[dir][0][index], BW, no_rounding);
		  break;
	 default:
		  ff_bfin_interpolate8x8_new_halfpel_hv(dest, 8, src+s->offset[dir][0][index], BW, no_rounding);
		  break;
	}
#endif
}

static inline void dma_mpeg_motion(DecCtx *s, int32_t dir,
									   uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
									   CSP *ref_picture, op_pixels2_func (*pix_op)[4], int h)
{
	 /* assume out format is FMT_H263 */

#ifndef __BFIN__
	 pix_op[0][s->dxy[dir][0][0]](dest_y, 16, ref_picture->y, BW, s->no_rounding);
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
		  pix_op[1][s->dxy[dir][1][0]](dest_cb, 8, ref_picture->u, CW, s->no_rounding);
		  pix_op[1][s->dxy[dir][1][0]](dest_cr, 8, ref_picture->v, CW, s->no_rounding);
	 }
#else
	 /* reuse the new interploate functions from Xvid! 
		chaos,confused interface :( */
	 switch(s->dxy[dir][0][0]){
	 case 0:
		  ff_bfin_transfer16x16_copy_new(dest_y, ref_picture->y+s->offset[dir][0][0], BW);			
		  break;
	 case 1:
		  ff_bfin_interpolate16x16_new_halfpel_h(dest_y, ref_picture->y+s->offset[dir][0][0], BW, 
												 s->no_rounding);
		  break;
	 case 2:
		  ff_bfin_interpolate16x16_new_halfpel_v(dest_y, ref_picture->y+s->offset[dir][0][0], BW, 
												 s->no_rounding);
		  break;
	 default:
		  ff_bfin_interpolate16x16_new_halfpel_hv(dest_y, ref_picture->y+s->offset[dir][0][0], BW, 
												  s->no_rounding);
		  break;
	 }
	 
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
		  switch (s->dxy[dir][1][0]){ 
		  case 0:
			   ff_bfin_transfer8x8_copy_new(dest_cb, 0, ref_picture->u+s->offset[dir][1][0], CW);
			   ff_bfin_transfer8x8_copy_new(dest_cr, 0, ref_picture->v+s->offset[dir][1][0], CW);
			   break;
		  case 1:
			   ff_bfin_interpolate8x8_new_halfpel_h(
					dest_cb, 0, ref_picture->u+s->offset[dir][1][0], CW, s->no_rounding);
			   ff_bfin_interpolate8x8_new_halfpel_h(
					dest_cr, 0, ref_picture->v+s->offset[dir][1][0], CW, s->no_rounding);
			   break;
		  case 2:
			   ff_bfin_interpolate8x8_new_halfpel_v(
					dest_cb, 0, ref_picture->u+s->offset[dir][1][0], CW, s->no_rounding);
			   ff_bfin_interpolate8x8_new_halfpel_v(
					dest_cr, 0, ref_picture->v+s->offset[dir][1][0], CW, s->no_rounding);
			   break;
		  default:
			   ff_bfin_interpolate8x8_new_halfpel_hv(
					dest_cb, 0, ref_picture->u+s->offset[dir][1][0], CW, s->no_rounding);
			   ff_bfin_interpolate8x8_new_halfpel_hv(
					dest_cr, 0, ref_picture->v+s->offset[dir][1][0], CW, s->no_rounding);
			   break;
		  }
	 }
#endif
}

static inline void 
dma_mpeg_motion_bidir(DecCtx *s, 
					  uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
					  CSP *forward_ref, CSP *backward_ref,
					  op_pixels2_func (*pix_op)[4], int h)
{
#define BI_BACKFORWARD
#ifdef BI_BACKFORWARD
	 op_pixels2_func avg_op[2][4] =
		  {{interpolate16x16_halfpel_add,
			interpolate16x16_halfpel_h_add,
			interpolate16x16_halfpel_v_add,
			interpolate16x16_halfpel_hv_add},
#ifndef __BFIN__
		   {interpolate8x8_halfpel_add_c,
			interpolate8x8_halfpel_h_add_c,
			interpolate8x8_halfpel_v_add_c,
			interpolate8x8_halfpel_hv_add_c}
#else
		   {ff_bfin_transfer8x8_copy_new_add,
			ff_bfin_interpolate8x8_new_halfpel_h_add,
			ff_bfin_interpolate8x8_new_halfpel_v_add,
			ff_bfin_interpolate8x8_new_halfpel_hv_add}
#endif
		  };
#endif//BI_BACKFORWARD

#ifndef __BFIN__
	 pix_op[0][s->dxy[0][0][0]](dest_y, 16, forward_ref->y, BW, s->no_rounding);
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
		  pix_op[1][s->dxy[0][1][0]](dest_cb, 8, forward_ref->u, CW, s->no_rounding);
		  pix_op[1][s->dxy[0][1][0]](dest_cr, 8, forward_ref->v, CW, s->no_rounding);
	 }
#else
	 switch(s->dxy[0][0][0]){
	 case 0:
		  ff_bfin_transfer16x16_copy_new(dest_y, forward_ref->y+s->offset[0][0][0], BW);			
		  break;
	 case 1:
		  ff_bfin_interpolate16x16_new_halfpel_h(dest_y, forward_ref->y+s->offset[0][0][0], BW, 
												 s->no_rounding);
		  break;
	 case 2:
		  ff_bfin_interpolate16x16_new_halfpel_v(dest_y, forward_ref->y+s->offset[0][0][0], BW, 
												 s->no_rounding);
		  break;
	 default:
		  ff_bfin_interpolate16x16_new_halfpel_hv(dest_y, forward_ref->y+s->offset[0][0][0], BW, 
												  s->no_rounding);
		  break;
	 }
	 
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
		  switch (s->dxy[0][1][0]){ 
		  case 0:
			   ff_bfin_transfer8x8_copy_new(dest_cb, 0, forward_ref->u+s->offset[0][1][0], CW);
			   ff_bfin_transfer8x8_copy_new(dest_cr, 0, forward_ref->v+s->offset[0][1][0], CW);
			   break;
		  case 1:
			   ff_bfin_interpolate8x8_new_halfpel_h(
					dest_cb, 0, forward_ref->u+s->offset[0][1][0], CW, s->no_rounding);
			   ff_bfin_interpolate8x8_new_halfpel_h(
					dest_cr, 0, forward_ref->v+s->offset[0][1][0], CW, s->no_rounding);
			   break;
		  case 2:
			   ff_bfin_interpolate8x8_new_halfpel_v(
					dest_cb, 0, forward_ref->u+s->offset[0][1][0], CW, s->no_rounding);
			   ff_bfin_interpolate8x8_new_halfpel_v(
					dest_cr, 0, forward_ref->v+s->offset[0][1][0], CW, s->no_rounding);
			   break;
		  default:
			   ff_bfin_interpolate8x8_new_halfpel_hv(
					dest_cb, 0, forward_ref->u+s->offset[0][1][0], CW, s->no_rounding);
			   ff_bfin_interpolate8x8_new_halfpel_hv(
					dest_cr, 0, forward_ref->v+s->offset[0][1][0], CW, s->no_rounding);
			   break;
		  }
	 }
#endif

#ifdef BI_BACKFORWARD
	 avg_op[0][s->dxy[1][0][0]](dest_y, 16, backward_ref->y+s->offset[1][0][0], BW, s->no_rounding);
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
#ifdef __BFIN__
		  avg_op[1][s->dxy[1][1][0]](dest_cb, 0, backward_ref->u+s->offset[1][1][0], CW, s->no_rounding);
		  avg_op[1][s->dxy[1][1][0]](dest_cr, 0, backward_ref->v+s->offset[1][1][0], CW, s->no_rounding);
#else
		  avg_op[1][s->dxy[1][1][0]](dest_cb, 8, backward_ref->u+s->offset[1][1][0], CW, s->no_rounding);
		  avg_op[1][s->dxy[1][1][0]](dest_cr, 8, backward_ref->v+s->offset[1][1][0], CW, s->no_rounding);
#endif
	 }
#endif
}

static inline void 
MPV_dma_motion(McPara *para,
			   uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
			   op_pixels2_func (*pix_op)[4], qpel_mc_func (*qpix_op)[16])
{
    int mb_x, mb_y, i;
 	
	DecCtx *s = &para->decctx;

    mb_x = s->mb_x;
    mb_y = s->mb_y;

	switch(s->mv_type) {
    case MV_TYPE_16X16:
		 if((s->dir & MV_DIR_FORWARD) && (s->dir & MV_DIR_BACKWARD)){
			  dma_mpeg_motion_bidir(s, 
									dest_y, dest_cb, dest_cr, 
									&para->refmb[0], &para->refmb[1], pix_op, 16);
		 }else if(s->dir & MV_DIR_FORWARD){
			  dma_mpeg_motion(s, 0, dest_y, dest_cb, dest_cr, &para->refmb[0], pix_op, 16);
		 }else{
			  dma_mpeg_motion(s, 1, dest_y, dest_cb, dest_cr, &para->refmb[1], pix_op, 16);
		 }
		 break;
    case MV_TYPE_8X8:
		 for(i=0;i<4;i++) {
			  dma_hpel_motion(s, 0,
							  dest_y + ((i & 1) * 8) + (i >> 1) * 8 * 16,
							  para->refmb[0].y + ((i & 1) * CW) + (i >> 1) * CH * BW, pix_op, 
							  i, s->no_rounding);
			  
		 }
 

		 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
			  
#ifndef __BFIN__
			  pix_op[1][(para->decctx).dxy[0][1][0]](dest_cb, 8, para->refmb[0].u, CW, 
													 s->no_rounding);
			  pix_op[1][(para->decctx).dxy[0][1][0]](dest_cr, 8, para->refmb[0].v, CW, 
													 s->no_rounding);

#else
			  switch ((para->decctx).dxy[0][1][0]){
			  case 0:
				   ff_bfin_transfer8x8_copy_new(dest_cb, 0, para->refmb[0].u+s->offset[0][1][0], CW);
				   ff_bfin_transfer8x8_copy_new(dest_cr, 0, para->refmb[0].v+s->offset[0][1][0], CW);
				   break;
			  case 1:
				   ff_bfin_interpolate8x8_new_halfpel_h (dest_cb, 0, para->refmb[0].u+s->offset[0][1][0], CW,
														 s->no_rounding);
				   ff_bfin_interpolate8x8_new_halfpel_h (dest_cr, 0, para->refmb[0].v+s->offset[0][1][0], CW,
														 s->no_rounding);
				   break;
			  case 2:
				   ff_bfin_interpolate8x8_new_halfpel_v (dest_cb, 0, para->refmb[0].u+s->offset[0][1][0], CW,
														 s->no_rounding);
				   ff_bfin_interpolate8x8_new_halfpel_v (dest_cr, 0, para->refmb[0].v+s->offset[0][1][0], CW,
														 s->no_rounding);
				   break;
			  default:
				   ff_bfin_interpolate8x8_new_halfpel_hv(dest_cb, 0, para->refmb[0].u+s->offset[0][1][0], CW,
														 s->no_rounding);
				   ff_bfin_interpolate8x8_new_halfpel_hv(dest_cr, 0, para->refmb[0].v+s->offset[0][1][0], CW,
														 s->no_rounding);
				   break;
			  }
#endif
		 }
		 
    default: assert(0);
    }
}

/**************************************************************************************
 * dequant and idct functions!
 **************************************************************************************/ 
static inline void dma_dct_unquantize_h263_inter_c(DecCtx *s,
													   DCTELEM *block, int n, int qscale)
{
	 int32_t i, level, qmul, qadd;
	 int32_t nCoeffs;

	 assert(s->block_last_index[n]>=0);
	
	 qadd = (qscale - 1) | 1;
	 qmul = qscale << 1;
	 
	 nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];

	 for(i=0; i<=nCoeffs; i++) {
		  level = block[i];
		  if (level) {
			   if (level < 0) {
					level = level * qmul - qadd;
			   } else {
					level = level * qmul + qadd;
			   }
			   block[i] = level;
		  }
	 }
}

static inline void dma_add_dct(DecCtx *s,
									DCTELEM *block, int i, uint8_t *dest, 
									int line_size, int qscale)
{
    if (s->block_last_index[i] >= 0) {
		 if(s->mpeg_quant)
			  dma_dct_unquantize_h263_inter_c(s, block, i, qscale);
#ifdef __BFIN__
		 bfin_idct_add(dest, line_size, block);
#else
		 ff_simple_idct_add(dest, line_size, block);
#endif
    }
}

void MPV_dma_mc_addidct(void)
{
	 int16_t (*block)[64];
	 McPara *para = mcp_ptr->prev;
	 DecCtx *prev_decctx = &para->decctx;


	 op_pixels2_func op_pix[2][4] =
#ifndef __BFIN__
		  {{transfer16x16_copy_c,
			interpolate16x16_halfpel_h_c,
			interpolate16x16_halfpel_v_c,
			interpolate16x16_halfpel_hv_c},
		   {transfer8x8_copy_c,
			interpolate8x8_halfpel_h_c,
			interpolate8x8_halfpel_v_c,
			interpolate8x8_halfpel_hv_c}};
#else
	 {{NULL, NULL, NULL, NULL},{NULL, NULL, NULL, NULL}};
#endif

#ifdef __BFIN__
	 if(prev_decctx->mb_x == (prev_decctx->mb_width - 1) && prev_decctx->mb_y == (prev_decctx->mb_height - 1))
#else
	 if(prev_decctx->mb_x == (prev_decctx->mb_width - 1))
#endif
		  first_inter_mb = 0;

	 if(para->mb_done) return;
	 para->mb_done = 1;
     if(para->do_nothing) return;

	 block = para->residual;
	 dma_wait();
	 MPV_dma_motion(para, (para->outbuf).y, (para->outbuf).u, (para->outbuf).v, op_pix, NULL);
	 
#if 0
	 /* skip dequant / idct if we are really late ;) */
	 if(para->hurry_up>1) return;
 
	 if(para->skip_idct)
		  if((para->skip_idct >= AVDISCARD_NONKEY) || para->skip_idct >= AVDISCARD_ALL)
			   return;
		  
#endif

	 dma_add_dct(prev_decctx, block[0],    0, 
				 (para->outbuf).y, 16, prev_decctx->qscale);
	 dma_add_dct(prev_decctx, block[1],   1, 
				 (para->outbuf).y + 8, 16, prev_decctx->qscale);
	 dma_add_dct(prev_decctx, block[2], 2, 
				 (para->outbuf).y + 128, 16, prev_decctx->qscale);
	 dma_add_dct(prev_decctx, block[3], 3, 
				 (para->outbuf).y + 128 + 8, 16, prev_decctx->qscale);
	 if(!CONFIG_GRAY || !(prev_decctx->flags&CODEC_FLAG_GRAY)){
		  dma_add_dct(prev_decctx, block[4], 4, 
					  (para->outbuf).u , 8, prev_decctx->chroma_qscale);
		  dma_add_dct(prev_decctx, block[5], 5, 
					  (para->outbuf).v , 8, prev_decctx->chroma_qscale);
	 }
#if 1
	 dma_write_cur(para);
#else
	 {
		  int32_t i;
		  uint8_t *src_ptr, *dst_ptr; 
		  
		  for(i=0,src_ptr=(para->outbuf).y, dst_ptr=(para->decctx).dest_y; i<16; i++){
			   memcpy(dst_ptr, src_ptr, 16);
			   src_ptr += 16;
			   dst_ptr += (para->decctx).linesize;
		  }
		  for(i=0,src_ptr=(para->outbuf).u, dst_ptr=(para->decctx).dest_u; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += (para->decctx).uvlinesize;
		  }
		  for(i=0,src_ptr=(para->outbuf).v, dst_ptr=(para->decctx).dest_v; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += (para->decctx).uvlinesize;
		  }
	 }
#endif
}

/**************************************************************************************
 * generic function called after a macroblock has been parsed by the decoder 
 **************************************************************************************/
void MPV_dma_decode_mbinter(MpegEncContext *s, int32_t lowres_flag)
{
	 
	 int32_t mb_x, mb_y, i;
	 int8_t *src_ptr, *dst_ptr;
	 int16_t (*block)[64];

	 const int32_t mb_xy = s->mb_y * s->mb_stride + s->mb_x;
	 const int32_t linesize= s->current_picture.linesize[0]; 
	 const int32_t uvlinesize= s->current_picture.linesize[1];

	 McPara *prev_mc = (McPara *)mcp_ptr->prev;
	 DecCtx *prev_decctx = &prev_mc->decctx;
	 
	 /* only support half pixel tmporarily! */ 
	 op_pixels2_func op_pix[2][4] =
#ifndef __BFIN__
		  {{transfer16x16_copy_c,
			interpolate16x16_halfpel_h_c,
			interpolate16x16_halfpel_v_c,
			interpolate16x16_halfpel_hv_c},
		   {transfer8x8_copy_c,
			interpolate8x8_halfpel_h_c,
			interpolate8x8_halfpel_v_c,
			interpolate8x8_halfpel_hv_c}};
#else
	 {{NULL, NULL, NULL, NULL},{NULL, NULL, NULL, NULL}};
#endif

	 mb_x = s->mb_x;
   	 mb_y = s->mb_y;

	 s->current_picture.qscale_table[mb_xy]= s->qscale;

	 /* update DC predictors for P macroblocks */
	 if(!s->mb_intra){
		  if(s->h263_pred || s->h263_aic){
			   if(s->mbintra_table[mb_xy])
					ff_clean_intra_table_entries(s);
		  }else{
			   s->last_dc[0] =
			   s->last_dc[1] =
			   s->last_dc[2] = 128 << s->intra_dc_precision;
		  }
	 }

	 else if(s->h263_pred || s->h263_aic)
		  s->mbintra_table[mb_xy]=1;
	 
	 /* avoid copy if macroblock skipped in last frame too */
	 /* skip only during decoding as we might trash the buffers during encoding a bit */
	 {
		  uint8_t *mbskip_ptr = &s->mbskip_table[mb_xy];
		  const int age= s->current_picture.age;
		  
		  assert(age);
		  
		  if (s->mb_skipped) {
			   s->mb_skipped= 0;
			   assert(s->pict_type!=FF_I_TYPE);
			   
			   (*mbskip_ptr) ++; /* indicate that this time we skipped it */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;
				
                /* if previous was skipped too, then nothing to do !  */
                if (*mbskip_ptr >= age && s->current_picture.reference)
					 mcp_ptr->do_nothing = 1;

		  } else if(!s->current_picture.reference){
			   (*mbskip_ptr) ++; /* increase counter so the age can be compared cleanly */
			   if(*mbskip_ptr >99) *mbskip_ptr= 99;
		  } else{
			   *mbskip_ptr = 0; /* not skipped */
		  }
	 }
	 
	 if(s->mb_intra){
		  block = mcp_ptr->residual;
		  goto intra;
	 }else
		  block = prev_mc->residual;

	 /* read ref mb(n) */
	 start_read_timer();		  
	 if(mcp_ptr->do_nothing == 0){
		  dma_read_ref(s, mcp_ptr);
	 }
	 stop_read_timer();
	 	 
	 /* for 1st mb, only read ref */
	 if(__builtin_expect((first_inter_mb == 0), 0)){
		  first_inter_mb = 1;
		  goto skip_idct;
	 }	 

	 if(!prev_mc->mb_done){

		  prev_mc->mb_done = 1;

		  if(prev_mc->do_nothing) goto skip_idct;

		  /* mc mb(n-1) */
		  /* motion handling */
		  if(mcp_ptr->do_nothing)
			   dma_wait();

		  start_mc_timer();
		  MPV_dma_motion(prev_mc,
						 (prev_mc->outbuf).y, (prev_mc->outbuf).u, (prev_mc->outbuf).v, 
						 op_pix, NULL);
		  stop_mc_timer();
	 
#if 0
		  /* skip dequant / idct if we are really late ;) */
		  if(prev_mc->hurry_up>1) goto skip_idct;
		  if(prev_mc->skip_idct){
			   if((prev_mc->skip_idct >= AVDISCARD_NONKEY) || prev_mc->skip_idct >= AVDISCARD_ALL)
					goto skip_idct;
		  }
#endif
		  /* add dct residual */
		  start_idct_timer();
		  dma_add_dct(prev_decctx, block[0],    0, 
					  (prev_mc->outbuf).y, 16, prev_decctx->qscale);
		  dma_add_dct(prev_decctx, block[1],   1, 
					  (prev_mc->outbuf).y + 8, 16, prev_decctx->qscale);
		  dma_add_dct(prev_decctx, block[2], 2, 
					  (prev_mc->outbuf).y + 128, 16, prev_decctx->qscale);
		  dma_add_dct(prev_decctx, block[3], 3, 
					  (prev_mc->outbuf).y + 128 + 8, 16, prev_decctx->qscale);
		  if(!CONFIG_GRAY || !(prev_decctx->flags&CODEC_FLAG_GRAY)){
			   dma_add_dct(prev_decctx, block[4], 4, 
						   (prev_mc->outbuf).u , 8, prev_decctx->chroma_qscale);
			   dma_add_dct(prev_decctx, block[5], 5, 
						   (prev_mc->outbuf).v , 8, prev_decctx->chroma_qscale);
		  }
		  stop_idct_timer();

		  /* write cur mb(n-1) */
		  start_write_timer();
		  dma_write_cur(prev_mc);
		  stop_write_timer();
	 }
	 	
skip_idct:
	 /* save the current mb decode context! */
	 /* this mb not mc and idct yet */	 
	 mcp_ptr->mb_done = 0;
	 mcp_ptr->hurry_up = s->hurry_up;
	 mcp_ptr->skip_idct = s->avctx->skip_idct;
	 mcp_ptr->decctx.flags = s->flags;
	 mcp_ptr->decctx.qscale = s->qscale;
	 mcp_ptr->decctx.chroma_qscale = s->chroma_qscale;
	 for(i=0; i<12; i++)
		  mcp_ptr->decctx.block_last_index[i] = s->block_last_index[i];
	 mcp_ptr->decctx.inter_scantable = s->inter_scantable;
	 mcp_ptr->decctx.mpeg_quant = s->mpeg_quant;

	 mcp_ptr = (McPara *)mcp_ptr->prev;
	 	 
	 /* last 1 mb write cur only */
#ifdef __BFIN__
	 if(__builtin_expect((s->mb_x == (s->mb_width-1) && s->mb_y == (s->mb_height-1)),0)){
#else
	 if(__builtin_expect((s->mb_x == (s->mb_width-1)),0)){	  
#endif
		  first_inter_mb = 0;
		  mcp_ptr = (McPara *)mcp_ptr->prev;
		  prev_decctx = &mcp_ptr->decctx;
		  block = mcp_ptr->residual;

		  if(mcp_ptr->do_nothing) return;
		  
#if 0
		  if(mcp_ptr->hurry_up>1) return;
		  if(mcp_ptr->skip_idct){
			   if((mcp_ptr->skip_idct >= AVDISCARD_NONKEY) || mcp_ptr->skip_idct >= AVDISCARD_ALL)
					return;
		  }
#endif
		  dma_wait(); 	

		  MPV_dma_motion(mcp_ptr,
						 (mcp_ptr->outbuf).y, (mcp_ptr->outbuf).u, (mcp_ptr->outbuf).v,
						 op_pix, NULL);
		  dma_add_dct(prev_decctx, block[0],    0, 
					  (mcp_ptr->outbuf).y, 16, prev_decctx->qscale);
		  dma_add_dct(prev_decctx, block[1],   1, 
					  (mcp_ptr->outbuf).y + 8, 16, prev_decctx->qscale);
		  dma_add_dct(prev_decctx, block[2], 2, 
					  (mcp_ptr->outbuf).y + 128, 16, prev_decctx->qscale);
		  dma_add_dct(prev_decctx, block[3], 3, 
					  (mcp_ptr->outbuf).y + 128 + 8, 16, prev_decctx->qscale);

		  if(!CONFIG_GRAY || !(prev_decctx->flags&CODEC_FLAG_GRAY)){
			   dma_add_dct(prev_decctx, block[4], 4, 
						   (mcp_ptr->outbuf).u , 8, prev_decctx->chroma_qscale);
			   dma_add_dct(prev_decctx, block[5], 5, 
						   (mcp_ptr->outbuf).v , 8, prev_decctx->chroma_qscale);
		  }
		  
#if 1
		  dma_write_cur(mcp_ptr);
#else
		  for(i=0,src_ptr=(mcp_ptr->outbuf).y, dst_ptr=prev_decctx->dest_y; i<16; i++){
			   memcpy(dst_ptr, src_ptr, 16);
			   src_ptr += 16;
			   dst_ptr += (mcp_ptr->decctx).linesize;
		  }
		  for(i=0,src_ptr=(mcp_ptr->outbuf).u, dst_ptr=prev_decctx->dest_u; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += (mcp_ptr->decctx).uvlinesize;
		  }
		  for(i=0,src_ptr=(mcp_ptr->outbuf).v, dst_ptr=prev_decctx->dest_v; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += (mcp_ptr->decctx).uvlinesize;
		  }
#endif
		  
	 }
	 return;

intra:
	 /* dct only in intra block */
#if 0
	 put_dct(s, block[0], 0, s->dest[0]                 , linesize, s->qscale);
	 put_dct(s, block[1], 1, s->dest[0]              + 8, linesize, s->qscale);
	 put_dct(s, block[2], 2, s->dest[0] + 8*linesize    , linesize, s->qscale);
	 put_dct(s, block[3], 3, s->dest[0] + 8*linesize + 8, linesize, s->qscale);
	 
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
		  put_dct(s, block[4], 4, s->dest[1], uvlinesize, s->chroma_qscale);
		  put_dct(s, block[5], 5, s->dest[2], uvlinesize, s->chroma_qscale);
     }
#else
	 put_dct(s, block[0], 0, mcp_ptr->outbuf.y          , 16, s->qscale);
	 put_dct(s, block[1], 1, mcp_ptr->outbuf.y       + 8, 16, s->qscale);
	 put_dct(s, block[2], 2, mcp_ptr->outbuf.y + 128    , 16, s->qscale);
	 put_dct(s, block[3], 3, mcp_ptr->outbuf.y + 128 + 8, 16, s->qscale);
	 
	 if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
		  put_dct(s, block[4], 4, mcp_ptr->outbuf.u, 8, s->chroma_qscale);
		  put_dct(s, block[5], 5, mcp_ptr->outbuf.v, 8, s->chroma_qscale);
     }
#ifdef __BFIN__
	 dma_write_cur_mb(s->dest[0], s->dest[1], s->dest[2], 
					  linesize, uvlinesize,
					  (mcp_ptr->outbuf).y, (mcp_ptr->outbuf).u, (mcp_ptr->outbuf).v);
#else
	 {
		  int32_t i;
		  uint8_t *src_ptr, *dst_ptr; 
		  
		  for(i=0,src_ptr=(mcp_ptr->outbuf).y, dst_ptr=s->dest[0]; i<16; i++){
			   memcpy(dst_ptr, src_ptr, 16);
			   src_ptr += 16;
			   dst_ptr += linesize;
		  }
		  for(i=0,src_ptr=(mcp_ptr->outbuf).u, dst_ptr=s->dest[1]; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += uvlinesize;
		  }
		  for(i=0,src_ptr=(mcp_ptr->outbuf).v, dst_ptr=s->dest[2]; i<8; i++){
			   memcpy(dst_ptr, src_ptr, 8);
			   src_ptr += 8;
			   dst_ptr += uvlinesize;
		  }
	 }
#endif//__BFIN__
#endif
}

#if defined(_PROFILING_)

#ifdef __BFIN__
static inline uint64_t read_counter(void)			
{
    union{
		  struct{
			   unsigned lo;
			   unsigned hi;
		  }p;
		  unsigned long long c;
	 } t;
	 asm volatile("%0=cycles; %1=cycles2;" : "=d" (t.p.lo), "=d" (t.p.hi));
	 return t.c;
}
#else
static inline uint64_t read_counter(void)			
{
}
#endif

struct ts
{
	 int64_t current;
	 int64_t global;
	 int64_t overall;
	 int64_t init;
	 int64_t comp;	 
	 int64_t vld;
	 int64_t read_ref_start;
	 int64_t read_ref_end;
	 int64_t mc_start;
	 int64_t mc_end;
	 int64_t idct_start;
	 int64_t idct_end;
	 int64_t write_cur_start;
	 int64_t write_cur_end;
	 int64_t cache_start;
	 int64_t cache_end;
	 int64_t wait_start;
	 int64_t wait_end;
	 int64_t debug_start;
	 int64_t debug_end;
};

struct ts tim;

double frequency = 0.0;

/*
    determine cpu frequency
	not very precise but sufficient
*/
double
get_freq()
{
	int64_t x, y;
	int32_t i;

	i = time(NULL);

	while (i == time(NULL));
	x = read_counter();

	i++;

	while (i == time(NULL));

	y = read_counter();

	return (double) (y - x) / 1000.;
}

/* set everything to zero */
void 
init_timer()
{
	frequency = get_freq();
	
	tim.global = tim.overall = tim.init = tim.comp = tim.vld = 0;
	tim.read_ref_start = tim.read_ref_end = 
	tim.mc_start = tim.mc_end = 
	tim.idct_start = tim.idct_end = 
	tim.write_cur_start = tim.write_cur_end =  
    tim.cache_start = tim.cache_end =
	tim.wait_start = tim.wait_end = 
	tim.debug_start = tim.debug_end = 0;
}

void
start_timer()
{
	tim.current = read_counter();
}

void
start_global_timer()
{
	tim.global = read_counter();
}

void
stop_init_timer()
{
	tim.init += (read_counter() - tim.current);
}

void
stop_vld_timer()
{
	tim.vld += (read_counter() - tim.current);
}
 
void
stop_comp_timer()
{
	tim.comp += (read_counter() - tim.current);
}

void
stop_global_timer()
{
	tim.overall += (read_counter() - tim.global);
}

void
start_read_timer()
{
	tim.read_ref_start = read_counter();
}

void
start_mc_timer()
{
	tim.mc_start = read_counter();
}

void
start_idct_timer()
{
	tim.idct_start = read_counter();
}

void
start_write_timer()
{
	tim.write_cur_start = read_counter();
}

void
start_cache_timer()
{
	tim.cache_start = read_counter();
}

void
start_wait_timer()
{
	tim.wait_start = read_counter();
}

void 
start_debug_timer()
{
	tim.debug_start = read_counter();
}

void
stop_read_timer()
{
	tim.read_ref_end += (read_counter() - tim.read_ref_start);
}

void
stop_mc_timer()
{
	tim.mc_end += (read_counter() - tim.mc_start);
}

void
stop_idct_timer()
{
	tim.idct_end += (read_counter() - tim.idct_start);
}

void
stop_write_timer()
{
	tim.write_cur_end += (read_counter() - tim.write_cur_start);
}

void
stop_cache_timer()
{
	tim.cache_end += (read_counter() - tim.cache_start);
}

void
stop_wait_timer()
{
	tim.wait_end += (read_counter() - tim.wait_start);
}

void
stop_debug_timer()
{
	tim.debug_end += (read_counter() - tim.debug_start);
}

void
write_timer()
{
	 float vld_per, comp_per;
	 float read_per, mc_per, idct_per, write_per, cache_per, wait_per;
	 FILE *fp;

	 fp = fopen("decoder.log", "w+");
	 if(fp == NULL){
		  fprintf(stderr, "open log file error...\n");
		  abort();
	 }

	 vld_per =
		  (float) (((float) ((float) tim.vld / (float) tim.overall)) *
				   100.0);

	 comp_per =
		  (float) (((float) ((float) tim.comp / (float) tim.overall)) *
				   100.0);

	 read_per = 
		  (float) (((float) ((float) tim.read_ref_end / (float) tim.comp)) *
				   100.0);

	 mc_per = 
		  (float) (((float) ((float) tim.mc_end / (float) tim.comp)) *
				   100.0);

	 idct_per = 
		  (float) (((float) ((float) tim.idct_end / (float) tim.comp)) *
				   100.0);
	 
	 write_per = 
		  (float) (((float) ((float) tim.write_cur_end / (float) tim.comp)) *
				   100.0);

	 cache_per = 
		  (float) (((float) ((float) (tim.cache_end -tim.wait_end)/ (float) tim.comp)) *
				   100.0);

	 wait_per = 
		  (float) (((float) ((float) tim.wait_end / (float) tim.comp)) *
				   100.0);
	 
	 fprintf(fp,
			 "Decoding:\nTotal time: %f ms\n\n"
			 "DMA init:\nTotal time: %f ms\n\n"
			 "VLD:\nTotal time: %f ms (%3f%% of total decoding time)\n\n"
			 "Mot compensation & IDCT:\nTotal time: %f ms (%3f%% of total decoding time)\n\n"
			 "DMA reading operation:\nTotal time: %f ms (%3f%% of total compensation time)\n\n"
			 "MC:\nTotal time: %f ms (%3f%% of total compensation time)\n\n"
			 "IDCT:\nTotal time: %f ms (%3f%% of total compensation time)\n\n"
			 "DMA writing operation:\nTotal time: %f ms (%3f%% of total compensation time)\n\n"
			 "DMA cache operation:\nTotal time: %f ms (%3f%% of total compensation time)\n\n"
			 "DMA wait:\nTotal time: %f ms (%3f%% of total compensation time)\n\n"
			 "DEBUG:\nTotal time: %f ms\n\n",
			 (float) (tim.overall / frequency),
			 (float) (tim.init / frequency),
			 (float) (tim.vld / frequency), vld_per,
			 (float) (tim.comp / frequency), comp_per,
			 (float) (tim.read_ref_end / frequency),  read_per,
			 (float) (tim.mc_end / frequency), mc_per,
			 (float) (tim.idct_end / frequency), idct_per,
			 (float) (tim.write_cur_end / frequency), write_per,
			 (float) ((tim.cache_end-tim.wait_end)/ frequency), cache_per,
			 (float) (tim.wait_end / frequency), wait_per,
			 (float) (tim.debug_end / frequency)
		  );
	 
			 
	 fclose(fp);
}

#endif//_PROFILING_

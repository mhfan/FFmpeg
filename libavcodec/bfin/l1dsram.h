#ifndef _L1DSRAM_H_
#define _L1DSRAM_H_

/*buffer size  */
#define BW  24
#define BH  18
#define CW  (BW>>1)
#define CH  (BH>>1)

//cpu specific
#define BF533_L1DA_START (0xFF800000)
#define BF533_L1DA_LENGTH (0x4000)
#define BF533_L1DB_START (0xFF900000)//the lower 0x1000 is reserved for Kernel
#define BF533_L1DB_LENGTH (0x3000)

#define L1DA_START BF533_L1DA_START
#define L1DA_LENGTH BF533_L1DA_LENGTH
#define L1DA_END (L1DA_START+L1DA_LENGTH)
#define L1DB_START BF533_L1DB_START
#define L1DB_LENGTH BF533_L1DB_LENGTH
#define L1DB_END (L1DB_START+L1DB_LENGTH)
#define BFIN_L1DA_SUBBANK0 (L1DA_START+0x0000)
#define BFIN_L1DA_SUBBANK1 (L1DA_START+0x1000)
#define BFIN_L1DA_SUBBANK2 (L1DA_START+0x2000)
#define BFIN_L1DA_SUBBANK3 (L1DA_START+0x3000)
#define BFIN_L1DB_SUBBANK0 (L1DB_START+0x0000)
#define BFIN_L1DB_SUBBANK1 (L1DB_START+0x1000)
#define BFIN_L1DB_SUBBANK2 (L1DB_START+0x2000)
#define BFIN_L1DB_SUBBANK3 (L1DB_START+0x3000) 


/*=============SRAM Block Offset=========*/
#define MB_BLOCK0	        (BFIN_L1DA_SUBBANK0)
#define MB_BLOCK1	        (BFIN_L1DA_SUBBANK1)
#define DMA_DSC_CH1_START	(BFIN_L1DA_SUBBANK2)
#define DMA_FINISH_FLAG     (BFIN_L1DA_SUBBANK3)

/***************************************************
 * IDCT buffer size: (8*8*2*8)*2 bytes
 * Total: xxx bytes
 ***************************************************/
#define MB_DCT_BUFSIZE		(8*8*2*8)
#define DMA_DCTBUF0			(MB_BLOCK0)
#define DMA_DCTBUF1			(MB_BLOCK1)

/***************************************************
 * Macro block buffer(2):	(16*16+8*8+8*8)*2=384*2
 * Total: xxx bytes
 ***************************************************/
#define MB_OUT_BUFSIZE		384
#define DMA_MBBUF0			(DMA_DCTBUF0 + MB_DCT_BUFSIZE)
#define DMA_MBBUF1			(DMA_DCTBUF1 + MB_DCT_BUFSIZE)

#define DMA_MBBUF_Y0		(DMA_MBBUF0)
#define DMA_MBBUF_U0		(DMA_MBBUF_Y0+16*16) 
#define DMA_MBBUF_V0		(DMA_MBBUF_U0+8*8) 
#define DMA_MBBUF_Y1		(DMA_MBBUF1)
#define DMA_MBBUF_U1		(DMA_MBBUF_Y1+16*16) 
#define DMA_MBBUF_V1		(DMA_MBBUF_U1+8*8) 
 
/**************************************************************
 * MC backward & forward buffer(2):	(BW*BH+2*CW*CH)*2*2=1296*2
 * Total: xxx bytes
 **************************************************************/
#define DMA_MCBUF_YSIZE		(BW*BH)
#define DMA_MCBUF_CSIZE		(CW*CH)

#define MB_MC_BUFSIZE       1296
#define DMA_MCBUF0		    (DMA_MBBUF0 + MB_OUT_BUFSIZE)
#define DMA_MCBUF1		    (DMA_MBBUF1 + MB_OUT_BUFSIZE)

#define DMA_MCBUF_BY0		(DMA_MCBUF0)
#define DMA_MCBUF_BU0		(DMA_MCBUF_BY0+DMA_MCBUF_YSIZE) 
#define DMA_MCBUF_BV0		(DMA_MCBUF_BU0+DMA_MCBUF_CSIZE) 
#define DMA_MCBUF_FY0		(DMA_MCBUF_BV0+DMA_MCBUF_CSIZE)
#define DMA_MCBUF_FU0		(DMA_MCBUF_FY0+DMA_MCBUF_YSIZE) 
#define DMA_MCBUF_FV0		(DMA_MCBUF_FU0+DMA_MCBUF_CSIZE) 
#define DMA_MCBUF_BY1		(DMA_MCBUF1)
#define DMA_MCBUF_BU1		(DMA_MCBUF_BY1+DMA_MCBUF_YSIZE) 
#define DMA_MCBUF_BV1		(DMA_MCBUF_BU1+DMA_MCBUF_CSIZE) 
#define DMA_MCBUF_FY1		(DMA_MCBUF_BV1+DMA_MCBUF_CSIZE)
#define DMA_MCBUF_FU1		(DMA_MCBUF_FY1+DMA_MCBUF_YSIZE) 
#define DMA_MCBUF_FV1		(DMA_MCBUF_FU1+DMA_MCBUF_CSIZE) 

/*************************************************************
 * We put DMA discriptors here to avoid unnecessary 
 * cache operations.
 * Only support ARRAY MODE now!
 *************************************************************/
#define DTOR_SIZE 0xE

/* Descriptor offset */
#define SAL    0x0
#define SAH    0x2
#define CFG    0x4
#define XCNT   0x6
#define XMOD   0x8
#define YCNT   0xA
#define YMOD   0xC

//r 1mv des
#define DSC_SRC_Y0_CH1      (DMA_DSC_CH1_START)
#define DSC_SRC_U0_CH1	    (DSC_SRC_Y0_CH1  + DTOR_SIZE)
#define DSC_SRC_V0_CH1	    (DSC_SRC_U0_CH1  + DTOR_SIZE)
#define DMA_1MV_BEG_CH1     (DSC_SRC_V0_CH1  + DTOR_SIZE)
#define DSC_DST_Y0_CH1      (DMA_1MV_BEG_CH1 + DTOR_SIZE)
#define DSC_DST_U0_CH1	    (DSC_DST_Y0_CH1  + DTOR_SIZE)
#define DSC_DST_V0_CH1 	    (DSC_DST_U0_CH1  + DTOR_SIZE)
#define DMA_1MV_END_CH1     (DSC_DST_V0_CH1  + DTOR_SIZE)

//r 4mvs des
#define DSC_SRC_Y0_4MV_CH1  (DMA_1MV_END_CH1    + DTOR_SIZE)
#define DSC_SRC_Y1_4MV_CH1  (DSC_SRC_Y0_4MV_CH1 + DTOR_SIZE)
#define DSC_SRC_Y2_4MV_CH1  (DSC_SRC_Y1_4MV_CH1 + DTOR_SIZE)
#define DSC_SRC_Y3_4MV_CH1  (DSC_SRC_Y2_4MV_CH1 + DTOR_SIZE)
#define DSC_SRC_U0_4MV_CH1	(DSC_SRC_Y3_4MV_CH1 + DTOR_SIZE)
#define DSC_SRC_V0_4MV_CH1	(DSC_SRC_U0_4MV_CH1 + DTOR_SIZE)
#define DMA_4MV_BEG_CH1     (DSC_SRC_V0_4MV_CH1 + DTOR_SIZE)
#define DSC_DST_Y0_4MV_CH1	(DMA_4MV_BEG_CH1    + DTOR_SIZE)
#define DSC_DST_Y1_4MV_CH1	(DSC_DST_Y0_4MV_CH1 + DTOR_SIZE)
#define DSC_DST_Y2_4MV_CH1	(DSC_DST_Y1_4MV_CH1 + DTOR_SIZE)
#define DSC_DST_Y3_4MV_CH1	(DSC_DST_Y2_4MV_CH1 + DTOR_SIZE)
#define DSC_DST_U0_4MV_CH1	(DSC_DST_Y3_4MV_CH1 + DTOR_SIZE)
#define DSC_DST_V0_4MV_CH1 	(DSC_DST_U0_4MV_CH1 + DTOR_SIZE)
#define DMA_4MV_END_CH1     (DSC_DST_V0_4MV_CH1 + DTOR_SIZE)

//b frame r des
#define DSC_SRC_BY_CH1      (DMA_4MV_END_CH1 + DTOR_SIZE)
#define DSC_SRC_BU_CH1	    (DSC_SRC_BY_CH1  + DTOR_SIZE)
#define DSC_SRC_BV_CH1	    (DSC_SRC_BU_CH1  + DTOR_SIZE)
#define DSC_SRC_FY_CH1      (DSC_SRC_BV_CH1  + DTOR_SIZE)
#define DSC_SRC_FU_CH1	    (DSC_SRC_FY_CH1  + DTOR_SIZE)
#define DSC_SRC_FV_CH1	    (DSC_SRC_FU_CH1  + DTOR_SIZE)
#define DMA_BI_BEG_CH1      (DSC_SRC_FV_CH1  + DTOR_SIZE)

#define DSC_DST_BY_CH1	    (DMA_BI_BEG_CH1  + DTOR_SIZE)
#define DSC_DST_BU_CH1	    (DSC_DST_BY_CH1  + DTOR_SIZE)
#define DSC_DST_BV_CH1 	    (DSC_DST_BU_CH1  + DTOR_SIZE)
#define DSC_DST_FY_CH1	    (DSC_DST_BV_CH1  + DTOR_SIZE)
#define DSC_DST_FU_CH1	    (DSC_DST_FY_CH1  + DTOR_SIZE)
#define DSC_DST_FV_CH1 	    (DSC_DST_FU_CH1  + DTOR_SIZE)
#define DMA_BI_END_CH1      (DSC_DST_FV_CH1  + DTOR_SIZE)

//w des
#define DSC_SRC_Y0_WRT_CH3  (DMA_BI_END_CH1     + DTOR_SIZE)
#define DSC_SRC_U0_WRT_CH3	(DSC_SRC_Y0_WRT_CH3 + DTOR_SIZE)
#define DSC_SRC_V0_WRT_CH3	(DSC_SRC_U0_WRT_CH3 + DTOR_SIZE)
#define DMA_WRT_BEG_CH3     (DSC_SRC_V0_WRT_CH3 + DTOR_SIZE)
#define DSC_DST_Y0_WRT_CH3	(DMA_WRT_BEG_CH3    + DTOR_SIZE)
#define DSC_DST_U0_WRT_CH3	(DSC_DST_Y0_WRT_CH3 + DTOR_SIZE)
#define DSC_DST_V0_WRT_CH3 	(DSC_DST_U0_WRT_CH3 + DTOR_SIZE)
#define DMA_WRT_END_CH3     (DSC_DST_V0_WRT_CH3 + DTOR_SIZE)

#define DMA_WRT_SIGN        (DMA_FINISH_FLAG)
#define DMA_WRT_DONE        (DMA_WRT_SIGN + 32)
#endif


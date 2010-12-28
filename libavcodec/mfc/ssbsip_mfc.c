//#!/usr/bin/tcc -run
/****************************************************************
 * $ID: ssbsip_mfc.c   Ò», 27 12ÔÂ 2010 21:15:57 +0800  mhfan $ *
 *                                                              *
 * Description:                                                 *
 *                                                              *
 * Maintainer:  ·¶ÃÀ»Ô(MeiHui FAN)  <mhfan@ustc.edu>            *
 *                                                              *
 * CopyLeft (c)  2010  M.H.Fan                                  *
 *   All rights reserved.                                       *
 *                                                              *
 * This file is free software;                                  *
 *   you are free to modify and/or redistribute it   	        *
 *   under the terms of the GNU General Public Licence (GPL).   *
 ****************************************************************/

#include "libavcodec/avcodec.h"
#include "libavcore/imgutils.h"

#ifdef CONFIG_SSBMFC

//#include "JPGApi.h"
#include "SsbSipMfcApi.h"

/*
 * SsbSipMfc API refer to Android repository:
 *  device/samsung/crespo/sec_mm/sec_omx/sec_codecs/
 *
 * TODO: (M)JPEG decoder/encoder support
 */

#if CONFIG_DECODERS
#define MAX_TIMESTAMP	16	// XXX:
#define DECODER_INPUT_BUFFER_SIZE	(1024 * 1024)

typedef struct ssbsip_mfcd_priv {
    void* mfch;
    void* vaddr;

    SSBSIP_MFC_CODEC_TYPE type;

    int64_t bufpts[MAX_TIMESTAMP];
    unsigned inited:1, tsidx:5;

    AVFrame pict;	// XXX:
}   ssbsip_mfcd_priv;

static av_cold int ssbsip_mfcd_exit(AVCodecContext* avctx)
{
    ssbsip_mfcd_priv* priv = avctx->priv_data;

    if (0 < priv->mfch) SsbSipMfcDecClose(priv->mfch);
    if (priv->pict.data[0]) avctx->release_buffer(avctx, &priv->pict);

    return 0;
}

static av_cold int ssbsip_mfcd_init(AVCodecContext* avctx)
{
    ssbsip_mfcd_priv* priv = avctx->priv_data;

    SSBSIP_MFC_ERROR_CODE mfc_err;
    //SSBSIP_MFC_CODEC_TYPE type;

    void* mfch, *paddr;
    int cval;

    switch (avctx->codec_id) {	// XXX: avctx->codec->id
    case CODEC_ID_H264:	 cval = H264_DEC;	break;
    case CODEC_ID_H263:  cval = H263_DEC;	break;
    case CODEC_ID_MPEG4: cval = MPEG4_DEC;	break;	// XXX: XVID_DEC
    case CODEC_ID_MPEG2VIDEO: cval = MPEG2_DEC;	break;
    case CODEC_ID_MPEG1VIDEO: cval = MPEG1_DEC;	break;
    case CODEC_ID_VC1:	 cval = VC1_DEC;	break;	// XXX: VC1RCV_DEC

    //case FIMV1_DEC: case FIMV2_DEC: case FIMV3_DEC: case FIMV4_DEC: // FIXME:
    default:		 //cval = UNKNOWN_TYPE;
	av_log(avctx, AV_LOG_ERROR, "Unsupported codec: 0x%x\n",
		avctx->codec_id);	return -1;
    }

    if (!(mfch = SsbSipMfcDecOpen())) {
	av_log(avctx, AV_LOG_ERROR, "Fail to open SsbSip MFC decoder!\n");
	return -1;
    }

    if (!(priv->vaddr = SsbSipMfcDecGetInBuf(mfch, &paddr,
	    DECODER_INPUT_BUFFER_SIZE))) {
	av_log(avctx, AV_LOG_ERROR,
		"Fail to allocate memory for SsbSip MFC decoder!\n");
	SsbSipMfcDecClose(mfch);	return -1;
    }

    priv->mfch = mfch;
    priv->type = cval;

    priv->pict.data[0] = NULL;
    priv->inited = 0;
    priv->tsidx = 0;

return 0;	// XXX: 
    cval = 3;	mfc_err = SsbSipMfcDecSetConfig(mfch,
	    MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &cval);	// default 5
    cval = 8;	mfc_err = SsbSipMfcDecSetConfig(mfch,
	    MFC_DEC_SETCONF_DISPLAY_DELAY, &cval);	// XXX:
    cval = 1;	mfc_err = SsbSipMfcDecSetConfig(mfch,
	    MFC_DEC_SETCONF_POST_ENABLE, &cval);
    // mpeg4 deblocking filter

    return 0;
}

static int ssbsip_mfcd_frame(AVCodecContext* avctx,
	void* data, int* data_size, AVPacket* avpkt)
{
    ssbsip_mfcd_priv* priv = avctx->priv_data;
    SSBSIP_MFC_DEC_OUTBUF_STATUS mfc_status;
    SSBSIP_MFC_DEC_OUTPUT_INFO mfc_output;
    SSBSIP_MFC_ERROR_CODE mfc_err;
    void* mfch = priv->mfch;
    AVFrame *pict = data;
    int tsidx;

    *data_size = 0;
    if (avpkt->size < 1) return 0;

    if (DECODER_INPUT_BUFFER_SIZE < avpkt->size)
	avpkt->size = DECODER_INPUT_BUFFER_SIZE;
    memcpy(priv->vaddr, avpkt->data, avpkt->size);

    if (!priv->inited) {
	SSBSIP_MFC_IMG_RESOLUTION mfc_resol;
	SSBSIP_MFC_CROP_INFORMATION mfc_crop;

	if ((mfc_err = SsbSipMfcDecInit(mfch, priv->type,
		avpkt->size)) != MFC_RET_OK) {
	    av_log(avctx, AV_LOG_ERROR,
		    "Fail to initialize SsbSip MFC decoder: %d\n", mfc_err);
	    return -1;
	}

	mfc_err = SsbSipMfcDecGetConfig(mfch,
		MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &mfc_resol);
	if (mfc_err == MFC_RET_OK) 
	mfc_err = SsbSipMfcDecGetConfig(mfch,
		MFC_DEC_GETCONF_CROP_INFO, &mfc_crop);
	if (mfc_err != MFC_RET_OK) av_log(avctx, AV_LOG_ERROR,
		"Fail to to get resolution and crop info.: %d\n", mfc_err);

	// XXX: avctx->coded_width, avctx->coded_height
	avctx->width  = mfc_resol.width  - mfc_crop.crop_left_offset
					 - mfc_crop.crop_right_offset;
	avctx->height = mfc_resol.height - mfc_crop.crop_top_offset
					 - mfc_crop.crop_bottom_offset;

	priv->inited = 1;	return avpkt->size;
    }

    priv->bufpts[tsidx =  priv->tsidx] = pict->pts;
    mfc_err = SsbSipMfcDecSetConfig(mfch, MFC_DEC_SETCONF_FRAME_TAG, &tsidx);
    if (MAX_TIMESTAMP < ++priv->tsidx) priv->tsidx = 0;

    if ((mfc_err = SsbSipMfcDecExe(mfch, avpkt->size)) != MFC_RET_OK) {
	av_log(avctx, AV_LOG_ERROR, "SsbSip MFC fail to decode frame: %d\n",
		mfc_err);	return -1;
    }

    mfc_status = SsbSipMfcDecGetOutBuf(mfch, &mfc_output);
    switch (mfc_status) {
    //case MFC_GETOUTBUF_STATUS_NULL: break;
    case MFC_GETOUTBUF_DECODING_ONLY: return avpkt->size;	// XXX:
    case MFC_GETOUTBUF_DISPLAY_DECODING:
    case MFC_GETOUTBUF_DISPLAY_ONLY:
	if (pict->data[0]) avctx->release_buffer(avctx, pict);
	if (avctx->get_buffer(avctx, pict) < 0) {	// XXX:
	    av_log(avctx, AV_LOG_ERROR, "Fail to get_buffer()\n");
	    return -1;
	}   priv->pict = *pict;		break;
    //case MFC_GETOUTBUF_DISPLAY_END:	break;
    default: ;
    }

    pict->linesize[0] = mfc_output.buf_width;
    pict->linesize[1] = pict->linesize[2] = pict->linesize[0] >> 1;

    pict->data[0] = mfc_output.YVirAddr;
    pict->data[1] = mfc_output.CVirAddr;
    pict->data[2] = pict->data[1] +
	(pict->linesize[1] * mfc_output.buf_height);

    mfc_err = SsbSipMfcDecGetConfig(mfch, MFC_DEC_GETCONF_FRAME_TAG, &tsidx);
    if (mfc_err != MFC_RET_OK || tsidx < 0 || MAX_TIMESTAMP < tsidx)
	; else pict->pts = priv->bufpts[tsidx];

    *data_size = sizeof(AVFrame);
    return avpkt->size;
}

#define SSBSIP_MFC_DECODER(id_, name_, long_name_)	\
    AVCodec ff_ ## name_ ## _ssbmfc_decoder = { 	\
	.name = "ssbsip_mfc_" #name_,			\
	.type = AVMEDIA_TYPE_VIDEO,			\
	.id   = id_,					\
	.init	= ssbsip_mfcd_init,			\
	.decode = ssbsip_mfcd_frame,			\
	.close  = ssbsip_mfcd_exit,			\
	.priv_data_size = sizeof(ssbsip_mfcd_priv),	\
	.capabilities   = CODEC_CAP_DR1 | 0x0100,	\
	.long_name = NULL_IF_CONFIG_SMALL(long_name_),  \
    }
#else
#define SSBSIP_MFC_DECODER(id_, name_, long_name_)
#endif

#if CONFIG_ENCODERS
typedef struct ssbsip_mfce_priv {
    void* mfch;

    SSBSIP_MFC_ENC_OUTPUT_INFO output;
    SSBSIP_MFC_ENC_INPUT_INFO input;

    int64_t bufpts[MAX_TIMESTAMP];
    unsigned inited:1, tsidx:5;

    AVFrame pict;	// XXX:
}   ssbsip_mfce_priv;

static av_cold int ssbsip_mfce_exit(AVCodecContext* avctx)
{
    ssbsip_mfce_priv* priv = avctx->priv_data;

    if (0 < priv->mfch) SsbSipMfcEncClose(priv->mfch);

    return 0;
}

static av_cold int ssbsip_mfce_init(AVCodecContext* avctx)
{
    ssbsip_mfce_priv* priv = avctx->priv_data;

    SSBSIP_MFC_ENC_OUTPUT_INFO* output;
    SSBSIP_MFC_ENC_H264_PARAM* param;
    SSBSIP_MFC_ERROR_CODE mfc_err;
    //SSBSIP_MFC_CODEC_TYPE type;

    void* mfch;
    int cval;

    union {
	SSBSIP_MFC_ENC_H264_PARAM h264;
	SSBSIP_MFC_ENC_MPEG4_PARAM mpeg4;
	SSBSIP_MFC_ENC_H263_PARAM h263;
    }	param_u;

    param = (void*)&param_u;

    switch (avctx->codec_id) {	// XXX: avctx->codec->id
    case CODEC_ID_H264: {
	param->LevelIDC   = avctx->level;
	param->ProfileIDC = avctx->profile;
	param->FrameRate  = (int)av_q2d(avctx->time_base);
	param->FrameQp_B  = param->FrameQp;

	param->NumberRefForPframes =
	param->NumberReferenceFrames = avctx->refs;
	param->Transform8x8Mode = avctx->flags & CODEC_FLAG2_8X8DCT;
	param->SliceArgument    = avctx->slices;

	avctx->has_b_frames = param->NumberBFrames = 0; //avctx->max_b_frames;
	param->LoopFilterDisable = !(avctx->flags & CODEC_FLAG_LOOP_FILTER);
	param->LoopFilterAlphaC0Offset = param->LoopFilterBetaOffset = 0;

	param->SymbolMode = 0;	// XXX: 0-CAVLC, 1-CABAC
	param->PictureInterlace = 0;
	param->EnableMBRateControl = 0;

	param->DarkDisable      = param->SmoothDisable =
	param->StaticDisable    = param->ActivityDisable = 1;

	cval = H264_ENC;
    }   break;

    case CODEC_ID_MPEG4: {
	SSBSIP_MFC_ENC_MPEG4_PARAM* mpeg4p = (void*)param;

	mpeg4p->LevelIDC   = avctx->level;
	mpeg4p->ProfileIDC = avctx->profile;
	mpeg4p->FrameQp_B  = mpeg4p->FrameQp;
	mpeg4p->SliceArgument = avctx->slices;
	mpeg4p->TimeIncreamentRes = (int)av_q2d(avctx->time_base);
	avctx->has_b_frames = mpeg4p->NumberBFrames = 0; //avctx->max_b_frames;

	mpeg4p->VopTimeIncreament = 1;
	mpeg4p->DisableQpelME = 1;

	cval = MPEG4_ENC;
    }  break;

    case CODEC_ID_H263: {
	SSBSIP_MFC_ENC_H263_PARAM* h263p = (void*)param;
	h263p->FrameRate = (int)av_q2d(avctx->time_base);
	cval = H263_ENC;
    }   break;

    default:		 //cval = UNKNOWN_TYPE;
	av_log(avctx, AV_LOG_ERROR, "Unsupported codec: 0x%x\n",
		avctx->codec_id);	return -1;
    }

    if (!(mfch = SsbSipMfcEncOpen())) {
	av_log(avctx, AV_LOG_ERROR, "Fail to open SsbSip MFC encoder!\n");
	return -1;
    }

    cval = 1;	mfc_err = SsbSipMfcEncSetConfig(mfch,
	    MFC_ENC_SETCONF_ALLOW_FRAME_SKIP, &cval);	// XXX:

    param->codecType = cval;
    param->SourceWidth  = avctx->width;
    param->SourceHeight = avctx->height;
    param->IDRPeriod = avctx->gop_size;	// avctx->keyint_min
    param->Bitrate   = avctx->bit_rate;
    param->QSCodeMin = avctx->qmin;
    param->QSCodeMax = avctx->qmax;

    param->FrameQp_P =
    param->FrameQp = avctx->cqp;
    param->EnableFRMRateControl = 1;
    param->RandomIntraMBRefresh = 0;
    param->CBRPeriodRf = 100;
    param->SliceMode = 0;

    param->PadControlOn = 0;
    param->LumaPadVal = 0x10;
    param->CbPadVal = param->CrPadVal = 0x80;

    if ((mfc_err = SsbSipMfcEncInit(mfch, (void*)param)) != MFC_RET_OK) {
	av_log(avctx, AV_LOG_ERROR,
		"Fail to initialize SsbSip MFC encoder: %d\n", mfc_err);
	SsbSipMfcEncClose(mfch);	return -1;
    }

    if ((mfc_err = SsbSipMfcEncGetOutBuf(mfch, output = &priv->output))
	    == MFC_RET_OK) {
	avctx->extradata_size =  output->headerSize;
	avctx->extradata = av_malloc(avctx->extradata_size);
	memcpy(avctx->extradata, output->StrmVirAddr, avctx->extradata_size);
    } else {	// XXX:
	avctx->extradata_size = 0;
	avctx->extradata = NULL;
    }

#ifndef USE_FIMC_FRAME_BUFFER
    if ((mfc_err = SsbSipMfcEncGetInBuf(mfch, &priv->input)) != MFC_RET_OK) {
	av_log(avctx, AV_LOG_ERROR,
		"Fail to allocate memory for SsbSip MFC encoder!\n");
	SsbSipMfcEncClose(mfch);	return -1;
    }
#endif

    avctx->coded_frame = &priv->pict;
    priv->mfch = mfch;
    priv->inited = 0;
    priv->tsidx = 0;

    return 0;
}

static int ssbsip_mfce_frame(AVCodecContext* avctx,
	unsigned char* buf, int buf_size, void* data)
{
    ssbsip_mfce_priv* priv = avctx->priv_data;
    SSBSIP_MFC_ENC_OUTPUT_INFO* output;
    SSBSIP_MFC_ENC_INPUT_INFO*   input;
    SSBSIP_MFC_ERROR_CODE mfc_err;
    void* mfch = priv->mfch;
    AVFrame *pict = data;
    int tsidx;

    if (buf_size < 1) return 0;

     input = &priv->input;
    output = &priv->output;

#ifdef USE_FIMC_FRAME_BUFFER
    input->YPhyAddr = pict->data[2];
    input->CPhyAddr = pict->data[3];	// XXX:

    if ((mfc_err = SsbSipMfcEncSetInBuf(mfch, input)) != MFC_RET_OK) {
	av_log(avctx, AV_LOG_ERROR, "SsbSip MFC fail to input buffer: %d\n",
		mfc_err);	return -1;
    }
#else
    if (pict->linesize[0] ==  avctx->width)
	memcpy(input->YVirAddr, pict->data[0],
		 avctx->width * avctx->height); else
	av_image_copy_plane(input->YVirAddr, avctx->width,
		pict->data[0], pict->linesize[0],
		avctx->width, avctx->height); 

    if (pict->linesize[1] == (tsidx = avctx->width >> 1))
	memcpy(input->YVirAddr, pict->data[1],
		(avctx->width * avctx->height) >> 2); else
	av_image_copy_plane(input->CVirAddr, tsidx,
		pict->data[1], pict->linesize[1],
		tsidx, avctx->height >> 1);
#endif

    priv->bufpts[tsidx =  priv->tsidx] = pict->pts;
    mfc_err = SsbSipMfcEncSetConfig(mfch, MFC_ENC_SETCONF_FRAME_TAG, &tsidx);
    if (MAX_TIMESTAMP < ++priv->tsidx) priv->tsidx = 0;

    if ((mfc_err = SsbSipMfcEncExe(mfch)) != MFC_RET_OK) {
	av_log(avctx, AV_LOG_ERROR, "SsbSip MFC fail to encode frame: %d\n",
		mfc_err);	return -1;
    }

    *avctx->coded_frame = *pict; // XXX:

    mfc_err = SsbSipMfcEncGetConfig(mfch, MFC_ENC_GETCONF_FRAME_TAG, &tsidx);
    if (mfc_err != MFC_RET_OK || tsidx < 0 || MAX_TIMESTAMP < tsidx)
	; else pict->pts = priv->bufpts[tsidx];

    if ((mfc_err = SsbSipMfcEncGetOutBuf(mfch, output)) != MFC_RET_OK)
	return 0;	// XXX:

    if (output->dataSize < buf_size) buf_size = output->dataSize; else
	av_log(avctx, AV_LOG_ERROR, "Less bitstream buffer: %d < %d\n",
		buf_size, output->dataSize);
    memcpy(buf, output->StrmVirAddr, buf_size);

    return buf_size;
}

#define SSBSIP_MFC_ENCODER(id_, name_, long_name_)	\
    AVCodec ff_ ## name_ ## _ssbmfc_encoder = {		\
	.name = "ssbsip_mfc_" #name_,			\
	.type = AVMEDIA_TYPE_VIDEO,			\
	.id   = id_,					\
	.init	= ssbsip_mfce_init,			\
	.encode = ssbsip_mfce_frame,			\
	.close  = ssbsip_mfce_exit,			\
	.priv_data_size = sizeof(ssbsip_mfce_priv),	\
	.pix_fmts = (const enum PixelFormat[]){		\
		    PIX_FMT_NV12, PIX_FMT_NONE },	\
	.capabilities   = CODEC_CAP_DR1 | 0x0100,	\
	.long_name = NULL_IF_CONFIG_SMALL(long_name_),  \
    }
#else
#define SSBSIP_MFC_ENCODER(id_, name_, long_name_)
#endif

#define SSBSIP_MFC_CODEC(id_, name_, long_name_) \
	SSBSIP_MFC_ENCODER(id_, name_, long_name_); \
	SSBSIP_MFC_DECODER(id_, name_, long_name_)

SSBSIP_MFC_CODEC  (CODEC_ID_H264, h264,   "SSBSIP MFC H.264 / MPEG-4 part 10");
SSBSIP_MFC_CODEC  (CODEC_ID_MPEG4, mpeg4, "SSBSIP MFC MPEG-4 part 2");
SSBSIP_MFC_DECODER(CODEC_ID_MPEG2VIDEO, mpeg2, "SSBSIP MFC MPEG-2");
SSBSIP_MFC_DECODER(CODEC_ID_MPEG1VIDEO, mpeg1, "SSBSIP MFC MPEG-1");
SSBSIP_MFC_CODEC  (CODEC_ID_H263, h263,   "SSBSIP MFC H.263");
SSBSIP_MFC_DECODER(CODEC_ID_VC1, vc1,     "SSBSIP MFC VC1");

#endif

// vim:sts=4:ts=8:

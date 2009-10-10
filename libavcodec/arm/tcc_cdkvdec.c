/*
 * The simplest mpeg encoder (well, it was the simplest!)
 * Copyright (C) 2009 HHTech Co., Ltd.
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

/**
 * @file libavcodec/arm/tcc_cdkvdec.c
 * $Date: 2009-09-24 $
 * $Author: Jetta $
 */

#include <Virtual.h>
#include <cdk_core.h>
#include <TCC_VPU_CODEC.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "libavcodec/avcodec.h"

typedef struct CdkVdecContext {
    cdk_callback_func_t callback_func;
    cdk_handle_t cdk_handle;

    cdk_func_t* dec_func;
    vdec_init_t   sVDecInit;
    vdec_info_t* psVDecInfo;

    vdec_input_t  sVDecInput;
    vdec_output_t sVDecOutput;

    dec_disp_info_ctrl_t dec_disp_info_ctrl;
    dec_disp_info_input_t dec_disp_info_input;
    dec_disp_info_t dec_disp_info[32];	    // XXX:

    AVFrame pic;

#define MAX_DECBUF	6	// XXX:
    int bufidx[MAX_DECBUF];
    short bufr, bufw;
    int skipped;

    int video_codec_type;
    int sequence_init_done;
    int number_of_vpu_error;

    AVBitStreamFilterContext *bsfc;	// H.264
}   CdkVdecContext;

static void tcc_video_omx_tccdec_cdk_init(CdkVdecContext *s)
{
    int ret;
    cdk_command_t cdk_cmd;
    cdk_callback_func_t* p_callback = &s->callback_func;

    ret = TCC_CDK_PROC(CDK_COMMAND_CREATE, &s->cdk_handle, 0, 0);
    if (ret < 0) av_log(NULL, AV_LOG_ERROR, "CDK_COMMAND_CREATE failed\n");

    p_callback->m_pfNonCacheMalloc =
    p_callback->m_pfMalloc = (void* (*)(unsigned int))av_malloc;
    p_callback->m_pfCalloc = (void* (*)(unsigned int, unsigned int))calloc;
    p_callback->m_pfNonCacheFree =
    p_callback->m_pfFree   = (void (*)(void*))av_free;

    p_callback->m_pfMemcpy =
	    (void* (*)(void*, const void*, unsigned int))memcpy;
    p_callback->m_pfMemset  = (void  (*)(void*, int, unsigned int))memset;
    p_callback->m_pfRealloc = (void* (*)(void*, unsigned int))av_realloc;
    p_callback->m_pfMemmove =
	    (void* (*)(void*, const void*, unsigned int))memmove;

    p_callback->m_pfPhysicalAlloc =
	    (void* (*)(unsigned int))sys_malloc_physical_addr;
    p_callback->m_pfPhysicalFree =
	    (void  (*)(void*, unsigned int))sys_free_physical_addr;
    p_callback->m_pfVirtualAlloc = (void* (*)(int*,
		    unsigned int, unsigned int))sys_malloc_virtual_addr;
    p_callback->m_pfVirtualFree	=
	    (void  (*)(int*, unsigned int, unsigned int))sys_free_virtual_addr;

#if 0
    p_callback->m_pfFopen = (void* (*)(const char*, const char*))fopen;
    p_callback->m_pfFread =
	    (unsigned int (*)(void*, unsigned int, unsigned int, void*))fread;
    p_callback->m_pfFseek = (int   (*)(void*, long, int ))fseek;
    p_callback->m_pfFtell = (long  (*)(void*))ftell;
    p_callback->m_pfFwrite = (unsigned int (*)(const void*,
		    unsigned int, unsigned int, void*))fwrite;
    //p_callback->m_pfUnlink = (int (*)(const char*))_unlink;
    p_callback->m_pfFflush = (unsigned int (*)(void*))fflush;
    p_callback->m_pfFeof = (unsigned int (*)(void*))feof;
    p_callback->m_pfFclose = (int (*)(void *))fclose;

    p_callback->m_pfFtell64 = (int64_t (*)(void*))ftello;
    p_callback->m_pfFseek64 = (int (*)(void*, int64_t, int))fseeko;
#endif

    cdk_cmd.m_iCmdNumber = 3;
    cdk_cmd.m_tMessage[0] = CP(p_callback);
    cdk_cmd.m_tMessage[1] = (cdk_param_t)1;
    cdk_cmd.m_tMessage[2] = (cdk_param_t)1;
    cdk_cmd.m_tMessage[3] = (cdk_param_t)1;

    ret = TCC_CDK_PROC(CDK_COMMAND_SET_IO_FILES, &s->cdk_handle, &cdk_cmd, 0);
    if (ret < 0) av_log(NULL, AV_LOG_ERROR,
	    "CDK_COMMAND_SET_IO_FILES failed\n");

    sys_init_physical_size();
    TCC_CDK_SetRegBaseSize();
    TCC_CDK_SetRegBaseAddr();
    TCC_CDK_SetBitstreamBufSize();
    TCC_CDK_SetBitstreamBufAddr();
}

static void tcc_video_omx_tccdec_cdk_deinit(CdkVdecContext *s)
{
    int ret;

    if ((ret = TCC_CDK_PROC(CDK_COMMAND_CLOSE, &s->cdk_handle, 0, 0)) < 0)
	av_log(NULL, AV_LOG_ERROR, "CDK_COMMAND_CLOSE failed: %d!\n", ret);
    TCC_CDK_PROC(CDK_COMMAND_DESTROY, &s->cdk_handle, 0, 0);

    TCC_CDK_FreeBitstreamBufAddr();
    TCC_CDK_FreeRegBaseAddr();
}

static av_cold int cdk_decode_init(AVCodecContext* avctx)
{
    CdkVdecContext *s = avctx->priv_data;
    vdec_init_t* psVDecInit;
    cdk_core_t* pCdk;
    int ret;

    s->bsfc = NULL;
    s->pic.data[0] = NULL;
    s->sequence_init_done = s->number_of_vpu_error = 0;
    s->bufr = s->bufw = -1;
    s->skipped = 0;

    //avctx->has_b_frames = 8;	// XXX:
    avctx->pix_fmt = PIX_FMT_YUV420P;

    switch (avctx->codec->id) {
    case CODEC_ID_MPEG4:    s->video_codec_type = STD_MPEG4;	break;
    case CODEC_ID_H264:	    s->video_codec_type = STD_AVC;
	if (avctx->extradata || avctx->extradata_size < 6) s->bsfc =
		av_bitstream_filter_init("h264_mp4toannexb");	break;
    case CODEC_ID_RV20:
    case CODEC_ID_RV30:
    case CODEC_ID_RV40:	    s->video_codec_type = STD_RV;	break;
    case CODEC_ID_VC1:
    case CODEC_ID_WMV3:	    s->video_codec_type = STD_VC1;	break;
    case CODEC_ID_H263:
    case CODEC_ID_FLV1:	    s->video_codec_type = STD_H263;	break;
    case CODEC_ID_MPEG1VIDEO:	// XXX:
    case CODEC_ID_MPEG2VIDEO:	TCC_CDK_InitMpeg2PTSCtrl(0);
			    s->video_codec_type = STD_MPEG2;	break;
    default:
	av_log(avctx, AV_LOG_ERROR, "Unsupported codec: 0x%x\n",
		avctx->codec->id);	return -1;
    }

    if (-1 < (ret = open("/sys/devices/platform/hhtech_gpio/play_video",
	    O_WRONLY))) { write(ret, "1", 1); close(ret); }

    tcc_video_omx_tccdec_cdk_init(s);

    psVDecInit = &s->sVDecInit;
    psVDecInit->m_RegBaseAddr = TCC_CDK_GetRegBaseAddr();
    psVDecInit->m_iBitstreamFormat = s->video_codec_type;
    psVDecInit->m_BitstreamBufAddr[PA] = TCC_CDK_GetBitstreamBufAddr(PA);
    psVDecInit->m_BitstreamBufAddr[VA] = TCC_CDK_GetBitstreamBufAddr(VA);
    psVDecInit->m_iBitstreamBufSize = TCC_CDK_GetBitstreamBufSize();
    psVDecInit->m_bHasSeqHeader = avctx->extradata_size ? 1 : 0;
    psVDecInit->m_iExtraDataLen = avctx->extradata_size;
    psVDecInit->m_pExtraData = avctx->extradata;
    psVDecInit->m_iFourCC = avctx->codec_tag;
    psVDecInit->m_iPicHeight = avctx->height;
    psVDecInit->m_iPicWidth  = avctx->width;
    psVDecInit->m_bCbCrInterleaveMode = 0;
    psVDecInit->m_bEnableVideoCache = 1;
    psVDecInit->m_pfInterrupt = 0;

    psVDecInit->m_pfMemcpy =
	    (void* (*)(void*, const void*, unsigned int))memcpy;
    psVDecInit->m_pfMemset = (void (*)(void*, int, unsigned int))memset;

    s->psVDecInfo = TCC_CDK_GetVDecInfo();
    memset(s->psVDecInfo, 0, sizeof(vdec_info_t));
    //s->psVDecInfo->m_iMinFrameBufferCount = 6;	// XXX:

    s->dec_func = TCC_CDK_GetDECFunc(s->video_codec_type);
    pCdk = TCC_CDK_GetCoreAddr();
    while ((ret = s->dec_func(VDEC_INIT, NULL,
	    psVDecInit, pCdk->m_psCallback)) < 0) {
	if (++s->number_of_vpu_error < 4) { sleep(1); continue; }
	av_log(avctx, AV_LOG_ERROR, "CDK VDEC_INIT failed: %d\n", ret);
	s->dec_func(VDEC_CLOSE, NULL, NULL, NULL);  return ret;
    }	s->number_of_vpu_error = 0;

    memset(&s->dec_disp_info_ctrl, 0, sizeof(dec_disp_info_ctrl_t));
    memset( s->dec_disp_info, 0, sizeof(s->dec_disp_info));

#if 0
    disp_pic_info(0, (void*)&s->dec_disp_info_ctrl, (void*)s->dec_disp_info,
	    s->video_codec_type);
#else// v1.200
    memset(&s->dec_disp_info_input, 0, sizeof(dec_disp_info_input_t));
    s->dec_disp_info_input.m_iStdType = s->video_codec_type;

if (0) disp_pic_info(0, (void*)&s->dec_disp_info_ctrl,
	    (void*)s->dec_disp_info, (void*)&s->dec_disp_info_input);
#endif

    s->sVDecInput.m_pInp[PA] = (void*)TCC_CDK_GetBitstreamBufAddr(PA);
    s->sVDecInput.m_pInp[VA] = (void*)TCC_CDK_GetBitstreamBufAddr(VA);

    if (1 || !avctx->extradata) return 0;	// XXX:

    memcpy(psVDecInit->m_BitstreamBufAddr[VA],
	    avctx->extradata, avctx->extradata_size);

    if ((ret = s->dec_func(VDEC_DEC_SEQ_HEADER, NULL,
	    avctx->extradata_size, s->psVDecInfo)) < 0)
	av_log(avctx, AV_LOG_ERROR,
		"VDEC_DEC_SEQ_HEADER failed: %d\n", ret);

    return 0;
}

int cdk_decode_frame(AVCodecContext* avctx,
	void* data, int* data_size, AVPacket* avpkt)
{
    CdkVdecContext *s  = avctx->priv_data;
    const uint8_t* buf = avpkt->data;
    uint32_t  buf_size = avpkt->size;
    AVFrame *pict = data;
    int ret;

    vdec_input_t*  psVDecInput  = &s->sVDecInput;
    vdec_output_t* psVDecOutput = &s->sVDecOutput;
    dec_disp_info_t *pdec_disp_info = NULL, dec_info;

    *data_size = 0;
    if (buf_size < 1) return 0;		// XXX:

    if (s->bsfc)	// XXX:
	ret = av_bitstream_filter_filter(s->bsfc, avctx,
	    NULL, &buf, &buf_size, buf, buf_size, 0); else
    if (!s->sequence_init_done && avctx->extradata && //0 &&	// XXX:
	    (s->video_codec_type == STD_MPEG4 ||
	     s->video_codec_type == STD_RV)) {
	buf = avctx->extradata = av_realloc(avctx->extradata,
		buf_size += avctx->extradata_size);
	memcpy(buf + avctx->extradata_size, avpkt->data, avpkt->size);
    }	//if (!buf) ;	// XXX:

    memcpy(psVDecInput->m_pInp[VA], buf,
	   psVDecInput->m_iInpLen = buf_size);
    if (s->bsfc && -1 < ret) av_free(buf);

    if (s->sequence_init_done == 0) {
        s->sequence_init_done = 1;

	if (avpkt->pts < 1 && 0 < avpkt->dts) {	// XXX:
	    s->dec_disp_info_input.m_iTimeStampType = CDMX_DTS_MODE;
	    av_log(avctx, AV_LOG_INFO, "Demuxer is DTS mode.\n");
	}
	disp_pic_info(0, (void*)&s->dec_disp_info_ctrl,
		(void*)s->dec_disp_info, (void*)&s->dec_disp_info_input);

        if ((ret = s->dec_func(VDEC_DEC_SEQ_HEADER,
		NULL, buf_size, s->psVDecInfo)) < 0) {
            av_log(avctx, AV_LOG_ERROR,
		    "VDEC_DEC_SEQ_HEADER failed: %d\n", ret);
	    ++s->number_of_vpu_error;	return ret;
        }

	if (avctx->width  != s->psVDecInfo->m_iPicWidth ||
	    avctx->height != s->psVDecInfo->m_iPicHeight) {
	    av_log(avctx, AV_LOG_INFO, "Resolution changed: %dx%d -> %dx%d\n",
		    avctx->width, avctx->height, s->psVDecInfo->m_iPicWidth,
						 s->psVDecInfo->m_iPicHeight);
	    avctx->height  = s->psVDecInfo->m_iPicHeight;
	    avctx->width   = s->psVDecInfo->m_iPicWidth;
	}
    }

    if (s->skipped) *pict = s->pic; else {
	if (pict->data[0]) {	// XXX:
	    if (s->bufr < 0) {
		if (s->bufw + 1 == MAX_DECBUF) s->bufr = 0;
		else goto SKIP;	// XXX:
	    }	else s->bufr = ++s->bufr % MAX_DECBUF;

	    //s->bufidx[s->bufr] = (unsigned)pict->data[3] & 0x0f;
	    if (s->dec_func(VDEC_BUF_FLAG_CLEAR, NULL,
		    &s->bufidx[s->bufr], NULL) < 0) av_log(avctx,
		    AV_LOG_ERROR, "VDEC_BUF_FLAG_CLEAR failed: %d\n",
		     s->bufidx[s->bufr]);

SKIP:	    avctx->release_buffer(avctx, pict);
	}
	if ((ret = avctx->get_buffer(avctx, pict)) < 0) {
	    av_log(avctx, AV_LOG_ERROR,
		    "get_buffer() failed\n");	return ret;
	}   s->pic = *pict;
    }

    switch (avctx->skip_frame) {
    case AVDISCARD_BIDIR:
	psVDecInput->m_iSkipFrameNum = 1;
	psVDecInput->m_iFrameSearchEnable = 0;
	psVDecInput->m_iSkipFrameMode = VDEC_SKIP_FRAME_ONLY_B;
	break;

    case AVDISCARD_NONKEY:
	psVDecInput->m_iSkipFrameNum = 1;
	psVDecInput->m_iFrameSearchEnable = 0;
	psVDecInput->m_iSkipFrameMode = VDEC_SKIP_FRAME_EXCEPT_I;
	break;

    case AVDISCARD_ALL:
	psVDecInput->m_iSkipFrameNum = 1;
	psVDecInput->m_iFrameSearchEnable = 1;	// XXX:
	psVDecInput->m_iSkipFrameMode = VDEC_SKIP_FRAME_UNCOND;
	break;

    default:
	psVDecInput->m_iSkipFrameNum = 0;
	psVDecInput->m_iFrameSearchEnable = 0;
	psVDecInput->m_iSkipFrameMode = VDEC_SKIP_FRAME_DISABLE;
    }

    if ((ret = s->dec_func(VDEC_DECODE, NULL,
	    psVDecInput, psVDecOutput)) < 0) {
	av_log(avctx, AV_LOG_ERROR, "VDEC_DECODE failed: %d\n", ret);
	if (3 < ++s->number_of_vpu_error) return ret;
    }

    if (psVDecOutput->m_DecOutInfo.m_iDecodingStatus
	    == VPU_DEC_BUF_FULL) {
	av_log(avctx, AV_LOG_ERROR, "VPU decoding buffer full: %d\n",
		psVDecOutput->m_DecOutInfo.m_iDecodedIdx);
	//s->skipped = 1;	return 0;	// XXX:
    }

    dec_info.m_iFrameType    = psVDecOutput->m_DecOutInfo.m_iPicType;
    dec_info.m_iPicStructure = psVDecOutput->m_DecOutInfo.m_iPictureStructure;
    dec_info.m_iRvTimeStamp  = dec_info.m_iM2vFieldSequence = 0;
    dec_info.m_iTimeStamp    = pict->pts / 1000;   // XXX:

    switch (s->video_codec_type) {
    case STD_RV:
	//dec_info.m_iData1 =
	dec_info.m_iRvTimeStamp =
		psVDecOutput->m_DecOutInfo.m_iRvTimestamp;	break;

    case STD_MPEG2:
	if (dec_info.m_iPicStructure != 3)
	    dec_info.m_iFrameDuration = 1; else
	if (s->psVDecInfo->m_iInterlace == 0) {
	    if (psVDecOutput->m_DecOutInfo.m_iRepeatFirstField == 0)
		dec_info.m_iFrameDuration = 2; else
		dec_info.m_iFrameDuration =
		    psVDecOutput->m_DecOutInfo.m_iTopFieldFirst == 0 ? 4 : 6;
	} else {
	//    pict->interlaced_frame = !
	//	psVDecOutput->m_DecOutInfo.m_iM2vProgressiveFrame;

	    if (psVDecOutput->m_DecOutInfo.m_iM2vProgressiveFrame == 0)
		dec_info.m_iFrameDuration = 2; else
		dec_info.m_iFrameDuration =
		    psVDecOutput->m_DecOutInfo.m_iRepeatFirstField == 0 ? 2 : 3;
	}
	//pict->top_field_first = psVDecOutput->m_DecOutInfo.m_iTopFieldFirst;
	//pict->repeat_pict = psVDecOutput->m_DecOutInfo.m_iRepeatFirstField;

	//dec_info.m_iData1 =
	dec_info.m_iM2vFieldSequence =	// v1.200
		psVDecOutput->m_DecOutInfo.m_iM2vFieldSequence;	break;

    default: ;//dec_info.m_iData1 = 0;
    }

#if 0
    disp_pic_info(1, (void*)&s->dec_disp_info_ctrl, (void*)&dec_info,
	    psVDecOutput->m_DecOutInfo.m_iDecodedIdx);
#else// v1.200
    s->dec_disp_info_input.m_iFrameIdx =
	    psVDecOutput->m_DecOutInfo.m_iDecodedIdx;

    disp_pic_info(1, (void*)&s->dec_disp_info_ctrl,
	    (void*)&dec_info, (void*)&s->dec_disp_info_input);
#endif

    if (psVDecOutput->m_DecOutInfo.m_iOutputStatus
	    != VPU_DEC_OUTPUT_SUCCESS) {
	av_log(avctx, AV_LOG_ERROR, "VPU skip output frame: %d\n",
		psVDecOutput->m_DecOutInfo.m_iOutputStatus);
	s->skipped = 1;		return avpkt->size;	// XXX:
    }	s->skipped = 0;

#if 0
    disp_pic_info(2, (void*)&s->dec_disp_info_ctrl, &pdec_disp_info,
	    psVDecOutput->m_DecOutInfo.m_iDispOutIdx);
#else// v1.200
    s->dec_disp_info_input.m_iFrameIdx =
	    psVDecOutput->m_DecOutInfo.m_iDispOutIdx;

    disp_pic_info(2, (void*)&s->dec_disp_info_ctrl,
	    &pdec_disp_info, (void*)&s->dec_disp_info_input);
#endif

    if (0) {	uint32_t* ptr = (void*)pict->data[0];
	ptr[0] = (unsigned)psVDecOutput->m_pDispOut[PA][0];
	ptr[1] = (unsigned)psVDecOutput->m_pDispOut[PA][1];
	ptr[2] = (unsigned)psVDecOutput->m_pDispOut[PA][2];
	ptr[3] = (unsigned)psVDecOutput->m_pDispOut[VA][0];
	ptr[4] = (unsigned)psVDecOutput->m_pDispOut[VA][1];
	ptr[5] = (unsigned)psVDecOutput->m_pDispOut[VA][2];
	//ptr[6] = psVDecOutput->m_DecOutInfo.m_iDispOutIdx;
    } else {
        pict->data[0] = psVDecOutput->m_pDispOut[VA][0];
        pict->data[1] = psVDecOutput->m_pDispOut[VA][1];
        pict->data[2] = psVDecOutput->m_pDispOut[VA][2];
        pict->data[3] = psVDecOutput->m_pDispOut[PA][0];	// XXX:
	//pict->data[3]+= psVDecOutput->m_DecOutInfo.m_iDispOutIdx;
    }

    *data_size = sizeof(AVFrame);
    pict->linesize[0] = avctx->width;
    pict->linesize[2] = pict->linesize[1] = pict->linesize[0] >> 1;

    pict->pict_type = psVDecOutput->m_DecOutInfo.m_iPicType + 1;
    if (-1 < psVDecOutput->m_DecOutInfo.m_iDecodedIdx) {
	if (psVDecInput->m_iFrameSearchEnable && pict->pict_type == FF_I_TYPE)
	    avctx->skip_frame = AVDISCARD_BIDIR; else	// XXX:
	if (psVDecInput->m_iSkipFrameMode == VDEC_SKIP_FRAME_ONLY_B)
	    avctx->skip_frame = AVDISCARD_DEFAULT;
    }

    if (!pdec_disp_info) av_log(avctx, AV_LOG_ERROR,
	    "invalid display information: %d!\n", avpkt->size); else {
if (0) fprintf(stderr, "PTS: %d vs %d, RV-%d, PKT: %d vs %d\n",
	(unsigned)pict->pts, pdec_disp_info->m_iTimeStamp,
	pdec_disp_info->m_iRvTimeStamp, (int)avpkt->pts, (int)avpkt->dts);

    if (s->video_codec_type == STD_RV)	// XXX:
	pict->pts = (int64_t)pdec_disp_info->m_iRvTimeStamp * 1000 + 1; else
    if (1 || 0 < avpkt->pts || s->video_codec_type == STD_AVC
			    || s->video_codec_type == STD_MPEG2)
	pict->pts = (int64_t)pdec_disp_info->m_iTimeStamp   * 1000 + 1;
    }

    s->bufidx[s->bufw = ++s->bufw % MAX_DECBUF] =
	    psVDecOutput->m_DecOutInfo.m_iDispOutIdx;
    s->number_of_vpu_error = 0;

    return avpkt->size;
}

static av_cold int cdk_decode_close(AVCodecContext *avctx)
{
    CdkVdecContext *s = avctx->priv_data;
    int ret;

    if (s->pic.data[0]) {
	avctx->release_buffer(avctx, &s->pic);

	for (s->bufw = ++s->bufw % MAX_DECBUF; s->bufw !=
	    (s->bufr = ++s->bufr % MAX_DECBUF); )
	if (s->dec_func(VDEC_BUF_FLAG_CLEAR, NULL,	// XXX:
		&s->bufidx[s->bufr], NULL) < 0) av_log(avctx, AV_LOG_ERROR,
		"VDEC_BUF_FLAG_CLEAR failed: %d\n", s->bufidx[s->bufr]);
    }

    if (s->bsfc) av_bitstream_filter_close(s->bsfc);

    s->dec_func(VDEC_CLOSE, NULL, NULL, &s->sVDecOutput);
    tcc_video_omx_tccdec_cdk_deinit(s);

    if (-1 < (ret = open("/sys/devices/platform/hhtech_gpio/play_video",
	    O_WRONLY))) { write(ret, "0", 1); close(ret); }

    av_log(avctx, AV_LOG_ERROR, "TCC CDK video decoder closed\n");
    return 0;
}

#define TCC_CODEC(id_, name_, long_name_) \
    AVCodec name_ ## _cdk_decoder = { \
        .id = id_, \
        .name = "cdk_" #name_, \
        .type = CODEC_TYPE_VIDEO, \
        .init   = cdk_decode_init, \
        .close  = cdk_decode_close, \
        .decode = cdk_decode_frame, \
	.capabilities  = CODEC_CAP_DR1 | 0x0100, \
        .priv_data_size = sizeof(CdkVdecContext), \
        .long_name = NULL_IF_CONFIG_SMALL(long_name_), \
    }	// XXX: CODEC_CAP_DR1

//TCC_CODEC(CODEC_ID_FLV1, flv, "TCC89XX CDK for FLV1");
//TCC_CODEC(CODEC_ID_H263, h263, "TCC89XX CDK for H263+/S263");
TCC_CODEC(CODEC_ID_H264, h264, "TCC89XX CDK for MPEG-4 part 10");
TCC_CODEC(CODEC_ID_MPEG4, mpeg4, "TCC89XX CDK for MPEG-4 part 2");
TCC_CODEC(CODEC_ID_MPEG2VIDEO, mpeg2, "TCC89XX CDK for MPEG2 video");
TCC_CODEC(CODEC_ID_MPEG1VIDEO, mpeg1, "TCC89XX CDK for MPEG1 video");
TCC_CODEC(CODEC_ID_RV40, rv40, "TCC89XX CDK for RV40");
TCC_CODEC(CODEC_ID_RV30, rv30, "TCC89XX CDK for RV30");
TCC_CODEC(CODEC_ID_RV20, rv20, "TCC89XX CDK for RV20");
//TCC_CODEC(CODEC_ID_RV10, rv10, "TCC89XX CDK for RV10");	// XXX:
TCC_CODEC(CODEC_ID_WMV3, wmv3, "TCC89XX CDK for WMV3");
//TCC_CODEC(CODEC_ID_WMV2, wmv2, "TCC89XX CDK for WMV2");
//TCC_CODEC(CODEC_ID_WMV1, wmv1, "TCC89XX CDK for WMV1");
TCC_CODEC(CODEC_ID_VC1, vc1, "TCC89XX CDK for VC1");

#undef TCC_CODEC

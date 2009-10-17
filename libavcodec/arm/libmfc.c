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
 * @file libavcodec/arm/libmfc.c
 * $Date: 2009-01-24 $
 * $Author: Jetta $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libavcodec/avcodec.h"

#include "SsbSipH264Decode.h"
#include "SsbSipMpeg4Decode.h"
#include "SsbSipVC1Decode.h"
#include "FrameExtractor.h"
#include "SsbSipLogMsg.h"
#include "MPEG4Frames.h"
#include "H263Frames.h"
#include "H264Frames.h"
#include "VC1Frames.h"
#include "MfcDriver.h"
#include "FileRead.h"
#include "s3c_pp.h"

//#define OUTPUT_TYPE_FILE

#define INPUT_BUFFER_SIZE (204800)
#define OUT_PATH_DMA

typedef void * (mfc_decode_init)();
typedef int (mfc_decode_deinit)(void *openHandle);
typedef int (mfc_decode_exe)(void *openHandle, long lengthBufFill);
typedef int (mfc_decode_set_config)(void *openHandle,
	H264_DEC_CONF conf_type, void *value);
typedef int (mfc_decode_get_config)(void *openHandle,
	H264_DEC_CONF conf_type, void *value);
typedef void *(mfc_decode_get_inbuf) (void *openHandle, long  size);
typedef void *(mfc_decode_get_outbuf)(void *openHandle, long *size);

typedef int (mfc_extract_config_stream)(FRAMEX_CTX *pFrameExCtx,
	void *fp, unsigned char buf[], int buf_size,
	H264_CONFIG_DATA *conf_data);
typedef int (mfc_next_frame)(FRAMEX_CTX  *pFrameExCtx, void *fp,
	unsigned char buf[], int buf_size, unsigned int *coding_type);

static unsigned char delimiter_mpeg4[3] = {0x00, 0x00, 0x01};
static unsigned char delimiter_h264[4]  = {0x00, 0x00, 0x00, 0x01};

typedef struct MFCContext {
    void *handle;
    void *stream_buf;

    FRAMEX_CTX *frameex_ctx;  // frame extractor context

    MMAP_STRM_PTR file_strm;
    //FRAMEX_STRM_PTR file_strm;
    SSBSIP_MPEG4_STREAM_INFO stream_info;
    //SSBSIP_H264_STREAM_INFO stream_info;

    // functions:
    mfc_decode_exe *decode_exe;
    mfc_decode_init *decode_init;
    mfc_decode_deinit *decode_deinit;
    mfc_decode_get_inbuf *decode_get_inbuf;
    mfc_decode_get_outbuf *decode_get_outbuf;
    mfc_decode_set_config *decode_set_config;
    mfc_decode_get_config *decode_get_config;

    // ioctl codes
    unsigned int MFC_DEC_GETCONF_PHYADDR_FRAM_BUF;
    unsigned int MFC_DEC_GETCONF_STREAMINFO;

    // error codes
    unsigned int SSBSIP_DEC_RET_OK;

    int h264_strip;
    int frame_len;

#ifdef	OUTPUT_TYPE_FILE
    int out_fd;
#endif

    int init;

    AVFrame pic;

    AVBitStreamFilterContext *bsfc;
}   MFCContext;

uint8_t *phy_addr;

static av_cold int MFC_decode_init(AVCodecContext *avctx)
{
    MFCContext *mfc = avctx->priv_data;
    avctx->pix_fmt = PIX_FMT_YUV420P;
    mfc->init = 0;

    switch (avctx->codec->id) {
    case CODEC_ID_H264:
	// retrieve SPS's profile_idc from extradata if exist
	if (avctx->extradata && 9 < avctx->extradata_size) {
	    const uint8_t *ptr = avctx->extradata;

	    if ((ptr[5] & 0x1f) && (ptr[8] == 7) && (ptr[9] != 66)) {
		av_log(avctx, AV_LOG_ERROR,
			"Support H.264 baseline profile ONLY: %d\n",
			ptr[9]);
		return -1;
	    }
	}

	mfc->frame_len = 0;
	mfc->decode_exe			= SsbSipH264DecodeExe;
	mfc->decode_init		= SsbSipH264DecodeInit;
	mfc->decode_deinit		= SsbSipH264DecodeDeInit;
	mfc->decode_get_inbuf		= SsbSipH264DecodeGetInBuf;
	mfc->decode_get_outbuf		= SsbSipH264DecodeGetOutBuf;
	mfc->decode_get_config		= SsbSipH264DecodeGetConfig;
	mfc->MFC_DEC_GETCONF_STREAMINFO = H264_DEC_GETCONF_STREAMINFO;
	mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF =
		H264_DEC_GETCONF_PHYADDR_FRAM_BUF;
	mfc->SSBSIP_DEC_RET_OK = SSBSIP_H264_DEC_RET_OK;

        mfc->bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	break;

    case CODEC_ID_MPEG4:
	mfc->frame_len = 0;
	mfc->decode_exe			= SsbSipMPEG4DecodeExe;
	mfc->decode_init		= SsbSipMPEG4DecodeInit;
	mfc->decode_deinit		= SsbSipMPEG4DecodeDeInit;
	mfc->decode_get_inbuf		= SsbSipMPEG4DecodeGetInBuf;
	mfc->decode_get_outbuf		= SsbSipMPEG4DecodeGetOutBuf;
	mfc->decode_get_config		= SsbSipMPEG4DecodeGetConfig;
	mfc->MFC_DEC_GETCONF_STREAMINFO = MPEG4_DEC_GETCONF_STREAMINFO;
	mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF =
		MPEG4_DEC_GETCONF_PHYADDR_FRAM_BUF;
	mfc->SSBSIP_DEC_RET_OK = SSBSIP_MPEG4_DEC_RET_OK;
	break;

    case CODEC_ID_H263:
	mfc->frame_len = 200000;
	mfc->decode_exe			= SsbSipMPEG4DecodeExe;
	mfc->decode_init		= SsbSipMPEG4DecodeInit;
	mfc->decode_deinit		= SsbSipMPEG4DecodeDeInit;
	mfc->decode_get_inbuf		= SsbSipMPEG4DecodeGetInBuf;
	mfc->decode_get_outbuf		= SsbSipMPEG4DecodeGetOutBuf;
	mfc->decode_get_config		= SsbSipMPEG4DecodeGetConfig;
	mfc->MFC_DEC_GETCONF_STREAMINFO = MPEG4_DEC_GETCONF_STREAMINFO;
	mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF =
		MPEG4_DEC_GETCONF_PHYADDR_FRAM_BUF;
	mfc->SSBSIP_DEC_RET_OK = SSBSIP_MPEG4_DEC_RET_OK;
	break;

    case CODEC_ID_VC1:
    case CODEC_ID_WMV3:
	mfc->frame_len = 200000;
	mfc->decode_exe			= SsbSipVC1DecodeExe;
	mfc->decode_init		= SsbSipVC1DecodeInit;
	mfc->decode_deinit		= SsbSipVC1DecodeDeInit;
	mfc->decode_get_inbuf		= SsbSipVC1DecodeGetInBuf;
	mfc->decode_get_config		= SsbSipVC1DecodeGetConfig;
	mfc->MFC_DEC_GETCONF_STREAMINFO = VC1_DEC_GETCONF_STREAMINFO;
	mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF =
		VC1_DEC_GETCONF_PHYADDR_FRAM_BUF;
	mfc->SSBSIP_DEC_RET_OK = SSBSIP_VC1_DEC_RET_OK;
	break;

    default:
	av_log(avctx, AV_LOG_ERROR, "Unsupported codec: %x\n",
		avctx->codec->id);
	return -1;
    }

    // create new instance
    if (!(mfc->handle = mfc->decode_init())) {
	av_log(avctx, AV_LOG_ERROR, "Fail to create new instance\n");
	return -1;
    }

    // obtaining the input buffer
    mfc->stream_buf = mfc->decode_get_inbuf(mfc->handle, mfc->frame_len);
    if (!mfc->stream_buf) {
        av_log(avctx, AV_LOG_ERROR, "Fail to obtain the input buffer\n");
        mfc->decode_deinit(mfc->handle);
        return -1;
    }

#ifdef	OUTPUT_TYPE_FILE
    mfc->out_fd = open("/fs/Desktop/Media/test_media/out.yuv",
	    O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (mfc->out_fd < 0) {
	av_log(avctx, AV_LOG_ERROR, "output file open failed\n");
	return -1;
    }
#endif
    return 0;
}

int MFC_decode_frame(AVCodecContext *avctx,
	void *data, int *data_size, AVPacket* avpkt)
{
    MFCContext *mfc = avctx->priv_data;
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    unsigned yuv_buf, yuv_len;
    AVFrame *pict = data;

    *data_size = 0;
    if (!mfc->init) {
	switch (avctx->codec->id) {
	case CODEC_ID_MPEG4:
            if (avctx->extradata) {
		buf = avctx->extradata = av_realloc(avctx->extradata,
			buf_size += avctx->extradata_size);
		memcpy(buf + avctx->extradata_size, avpkt->data, avpkt->size);
            }

	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf;
	    mfc->file_strm.p_end   = buf + buf_size;

            // FrameExtractor Initializaion
            mfc->frameex_ctx = FrameExtractorInit(FRAMEX_IN_TYPE_MEM,
		    delimiter_mpeg4, sizeof(delimiter_mpeg4), 1);
            FrameExtractorFirst(mfc->frameex_ctx, &mfc->file_strm);

            // CONFIG stream extraction
            mfc->frame_len = ExtractConfigStreamMpeg4(mfc->frameex_ctx,
		    &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
	    break;

	case CODEC_ID_H264:
	    if (mfc->bsfc) av_bitstream_filter_filter(mfc->bsfc, avctx, NULL,
		    &buf, &buf_size, buf, buf_size, 0);

            if (avctx->extradata)
                mfc->h264_strip = (avctx->extradata[4] & 0x3) + 1;

	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frameex_ctx = FrameExtractorInit(FRAMEX_IN_TYPE_MEM,
		    delimiter_h264, sizeof(delimiter_h264), 1);
            FrameExtractorFirst(mfc->frameex_ctx, &mfc->file_strm);

            mfc->frame_len = ExtractConfigStreamH264(mfc->frameex_ctx,
		    &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
	    break;

	case CODEC_ID_VC1:
	case CODEC_ID_WMV3:
	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frame_len = ExtractConfigStreamVC1(&mfc->file_strm,
		    mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
	    break;

	case CODEC_ID_H263:
	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frame_len = ExtractConfigStreamH263(&mfc->file_strm,
		    mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
	    break;
        }

        // configuring the instance with the config stream
        if (mfc->decode_exe(mfc->handle, mfc->frame_len) !=
		mfc->SSBSIP_DEC_RET_OK) {
            av_log(avctx, AV_LOG_ERROR,
		    "Fail to configure the instance with stream\n");
            return -1;
        }

        // get stream information
        mfc->decode_get_config(mfc->handle, mfc->MFC_DEC_GETCONF_STREAMINFO,
		&mfc->stream_info);
	avctx->height = mfc->stream_info.height;
	avctx->width  = mfc->stream_info.width;

        mfc->init = 1;
    } else { // next video stream
	switch (avctx->codec->id) {
	case CODEC_ID_MPEG4:
	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf + 3;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frame_len = NextFrameMpeg4(mfc->frameex_ctx,
		    &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
	    break;

	case CODEC_ID_H264:
	    mfc->file_strm.p_start = mfc->file_strm.p_cur =
		    buf + mfc->h264_strip;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frame_len = NextFrameH264(mfc->frameex_ctx,
		    &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);

            // Ugly hack to get the right slice length!
            mfc->frame_len = buf_size;	// XXX:
	    break;

	case CODEC_ID_VC1:
	case CODEC_ID_WMV3:
	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frame_len = NextFrameVC1(&mfc->file_strm, mfc->stream_buf,
		    INPUT_BUFFER_SIZE, NULL);
	    break;

	case CODEC_ID_H263:
	    mfc->file_strm.p_start = mfc->file_strm.p_cur = buf + 3;
	    mfc->file_strm.p_end   = buf + buf_size;

            mfc->frame_len = NextFrameH263(&mfc->file_strm, mfc->stream_buf,
		    INPUT_BUFFER_SIZE, NULL);
	    break;
        }

        if (mfc->frame_len < 4){
            av_log(avctx, AV_LOG_ERROR, "Fail to extract stream\n");
            return -1;
        }
    }

    if (pict->data[0]) avctx->release_buffer(avctx, pict);
    if (avctx->get_buffer(avctx, pict) < 0) {
        av_log(avctx, AV_LOG_ERROR, "Fail to get_buffer()\n");	return -1;
    }	mfc->pic = *pict;

    // decoding
    if (mfc->decode_exe(mfc->handle, mfc->frame_len) !=
	    mfc->SSBSIP_DEC_RET_OK) {
        av_log(avctx, AV_LOG_ERROR, "Oh, NO! Decoding failed!\n");
        return -1;
    }

    // obtaining the output buffer virtual address, XXX:
    yuv_buf = mfc->decode_get_outbuf(mfc->handle, &yuv_len);

    pict->data[0] = (void*)yuv_buf;
    pict->data[1] = pict->data[0] +  (avctx->width * avctx->height);
    pict->data[2] = pict->data[1] + ((avctx->width * avctx->height) >> 2);

    pict->linesize[0] = avctx->width;
    pict->linesize[2] = pict->linesize[1] = pict->linesize[0] >> 1;
    *data_size = sizeof(AVFrame);

#ifdef	OUTPUT_TYPE_FILE
    yuv_len = write(mfc->out_fd, yuv_buf, yuv_len);
    if (yuv_len < (mfc->stream_info.width * mfc->stream_info.height * 3) >> 1)
       av_log(avctx, AV_LOG_ERROR, "write fail\n");
#endif

    // physical address, reserved for pp!
    mfc->decode_get_config(mfc->handle, mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF,
	    &yuv_buf);
    phy_addr =	// XXX:
    pict->data[3] = (void*)yuv_buf;

    return avpkt->size;
}

static av_cold int MFC_decode_close(AVCodecContext *avctx)
{
    MFCContext *mfc = avctx->priv_data;
    if (mfc->pic.data[0]) avctx->release_buffer(avctx, &mfc->pic);
    if (mfc->bsfc) av_bitstream_filter_close(mfc->bsfc);
    mfc->decode_deinit(mfc->handle);

#ifdef	OUTPUT_TYPE_FILE
    close(mfc->out_fd);
#endif
    return 0;
}

#define MFC_CODEC(id_, name_, long_name_)		\
    AVCodec name_ ## _mfc_decoder = {			\
	.name = "libmfc_" #name_,			\
	.type = CODEC_TYPE_VIDEO,			\
	.id = id_,					\
	.init	= MFC_decode_init,			\
	.decode = MFC_decode_frame,			\
	.close  = MFC_decode_close,			\
	.priv_data_size = sizeof(MFCContext),		\
	.capabilities  = /*CODEC_CAP_DR1 | */0x0100,	\
	.long_name = NULL_IF_CONFIG_SMALL(long_name_),  \
    }

MFC_CODEC(CODEC_ID_H264, h264, "LIBMFC H.264 / MPEG-4 part 10");
MFC_CODEC(CODEC_ID_MPEG4, mpeg4, "LIBMFC MPEG-4 part 2");
MFC_CODEC(CODEC_ID_H263, h263, "LIBMFC H.263");
MFC_CODEC(CODEC_ID_WMV3, wmv3, "LIBMFC WMV3");
MFC_CODEC(CODEC_ID_VC1, vc1, "LIBMFC VC1");

#undef MFC_CODEC

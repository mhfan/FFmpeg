/*
 * Interface to the Android Stagefright library for
 * H/W accelerated H.264 decoding
 *
 * Copyright (C) 2011 Mohamed Naufal
 * Copyright (C) 2011 Martin Storsjö
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

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <utils/List.h>
#include <new>
#include <map>

extern "C" {
#include "avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/intreadwrite.h"
}

#if LIBAVCODEC_VERSION_MAJOR < 54
#define AV_PIX_FMT_NV12	    PIX_FMT_NV12
#define AV_PIX_FMT_NV21	    PIX_FMT_NV21
#define AV_PIX_FMT_YUYV422  PIX_FMT_YUYV422
#define AV_PIX_FMT_UYVY422  PIX_FMT_UYVY422
#define AV_PIX_FMT_YUV420P  PIX_FMT_YUV420P
#define AV_PIX_FMT_RGBA	    PIX_FMT_RGBA

#define AV_CODEC_ID_H264    CODEC_ID_H264
#define av_pix_fmt_desc_get(a) &av_pix_fmt_descriptors[a]
#define ff_get_buffer(a, b) a->get_buffer(a, b)
#endif

#define OMX_TI_COLOR_FormatYUV420PackedSemiPlanar 0x7F000100
#define OMX_QCOM_COLOR_FormatYVU420SemiPlanar 0x7FA30C00

using namespace android;

struct Frame {
    status_t status;
    size_t size;
    int64_t time;
    int key;
    uint8_t *buffer;
    AVFrame *vframe;
};

struct TimeStamp {
    int64_t pts;
    int64_t reordered_opaque;
};

class CustomSource;

struct StagefrightContext {
    AVBitStreamFilterContext *bsfc;
    uint8_t* orig_extradata;
    int orig_extradata_size;
    sp<MediaSource> *source;
    List<Frame*> *in_queue, *out_queue;
    pthread_mutex_t in_mutex, out_mutex;
    pthread_cond_t condition;
    pthread_t decode_thread_id;

    Frame *end_frame;
    bool source_done;
    volatile sig_atomic_t thread_started, thread_exited, stop_decode;

    AVFrame *prev_frame;
    std::map<int64_t, TimeStamp> *ts_map;
    int64_t frame_index;
    int width, height;

    uint8_t *dummy_buf;
    int dummy_bufsize;

    OMXClient *client;
    sp<MediaSource> *decoder;
    const char *decoder_component;
};

class CustomSource : public MediaSource {
public:
    CustomSource(AVCodecContext *avctx, sp<MetaData> meta) {
        s = (StagefrightContext*)avctx->priv_data;
        source_meta = meta;
        frame_size  = (avctx->width * avctx->height * 3) / 2;
        buf_group.add_buffer(new MediaBuffer(frame_size));
    }

    virtual sp<MetaData> getFormat() {
        return source_meta;
    }

    virtual status_t start(MetaData *params) {
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

    virtual status_t read(MediaBuffer **buffer,
                          const MediaSource::ReadOptions *options) {
        Frame *frame;
        status_t ret;

        if (s->thread_exited)
            return ERROR_END_OF_STREAM;
        pthread_mutex_lock(&s->in_mutex);

        while (s->in_queue->empty())
            pthread_cond_wait(&s->condition, &s->in_mutex);

        frame = *s->in_queue->begin();
        ret = frame->status;

        if (ret == OK) {
            ret = buf_group.acquire_buffer(buffer);
            if (ret == OK) {
                memcpy((*buffer)->data(), frame->buffer, frame->size);
                (*buffer)->set_range(0, frame->size);
                (*buffer)->meta_data()->clear();
                (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame,frame->key);
                (*buffer)->meta_data()->setInt64(kKeyTime, frame->time);
            } else {
                av_log(NULL, AV_LOG_ERROR, "Failed to acquire MediaBuffer\n");
            }
            av_freep(&frame->buffer);
        }

        s->in_queue->erase(s->in_queue->begin());
        pthread_mutex_unlock(&s->in_mutex);

        av_freep(&frame);
        return ret;
    }

private:
    MediaBufferGroup buf_group;
    sp<MetaData> source_meta;
    StagefrightContext *s;
    int frame_size;
};

#if HAVE_NEON
typedef uint8_t uint8;

// XXX: from libyuv projects

// Reads 16 pairs of UV and write even values to dst_u and odd to dst_v.
static void SplitUVRow_NEON(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                     int width) {
  asm volatile (
    ".p2align  2                               \n"
  "1:                                          \n"
    "vld2.u8    {q0, q1}, [%0]!                \n"  // load 16 pairs of UV
    "subs       %3, %3, #16                    \n"  // 16 processed per loop
    "vst1.u8    {q0}, [%1]!                    \n"  // store U
    "vst1.u8    {q1}, [%2]!                    \n"  // store V
    "bgt        1b                             \n"
    : "+r"(src_uv),  // %0
      "+r"(dst_u),   // %1
      "+r"(dst_v),   // %2
      "+r"(width)    // %3  // Output registers
    :                       // Input registers
    : "cc", "memory", "q0", "q1"  // Clobber List
  );
}

static inline void SplitScaleUVRowDown2_NEON(const uint8* src_ptr,
	uint8* dst_u, uint8* dst_ptr, int dst_width) {
  asm volatile (
  "1:                                          \n"
    "vld4.u8      {d0, d1, d2, d3}, [%0]!      \n"  // src line 0
    "subs         %3, %3, #8                   \n"  // 8 processed per loop
    "vst1.u8      {d0}, [%1]!                  \n"  // store U
    "vst1.u8      {d1}, [%2]!                  \n"  // store V
    "bgt          1b                           \n"
  : "+r"(src_ptr),          // %0
    "+r"(dst_u),            // %1
    "+r"(dst_ptr),          // %2
    "+r"(dst_width)         // %3
  :
  : "d0", "d1", "d2", "d3", "memory", "cc"
  );
}

static inline void ScaleRowDown2_NEON(const uint8* src_ptr,
	ptrdiff_t /* src_stride */, uint8* dst, int dst_width) {
  asm volatile (
  "1:                                          \n"
    // load even pixels into q0, odd into q1
    "vld2.u8    {q0,q1}, [%0]!                 \n"
    "subs       %2, %2, #16                    \n"  // 16 processed per loop
    "vst1.u8    {q0}, [%1]!                    \n"  // store even pixels
    "bgt        1b                             \n"
  : "+r"(src_ptr),          // %0
    "+r"(dst),              // %1
    "+r"(dst_width)         // %2
  :
  : "q0", "q1"              // Clobber List
  );
}

static inline void ScaleRowDown2Int_NEON(const uint8* src_ptr,
	ptrdiff_t src_stride, uint8* dst, int dst_width) {
  asm volatile (
    // change the stride to row 2 pointer
    "add        %1, %0                         \n"
  "1:                                          \n"
    "vld1.u8    {q0,q1}, [%0]!                 \n"  // load row 1 and post inc
    "vld1.u8    {q2,q3}, [%1]!                 \n"  // load row 2 and post inc
    "subs       %3, %3, #16                    \n"  // 16 processed per loop
    "vpaddl.u8  q0, q0                         \n"  // row 1 add adjacent
    "vpaddl.u8  q1, q1                         \n"
    "vpadal.u8  q0, q2                         \n"  // row 2 add adjacent + row1
    "vpadal.u8  q1, q3                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  // downshift, round and pack
    "vrshrn.u16 d1, q1, #2                     \n"
    "vst1.u8    {q0}, [%2]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          // %0
    "+r"(src_stride),       // %1
    "+r"(dst),              // %2
    "+r"(dst_width)         // %3
  :
  : "q0", "q1", "q2", "q3"     // Clobber List
  );
}
#endif

void* decode_thread(void *arg)
{
    AVCodecContext *avctx = (AVCodecContext*)arg;
    StagefrightContext *s = (StagefrightContext*)avctx->priv_data;
    const AVPixFmtDescriptor *pix_desc = av_pix_fmt_desc_get(avctx->pix_fmt);
    Frame* frame;
    MediaBuffer *buffer;
    int32_t w, h;
    int decode_done = 0;
    int ret;
    int src_linesize[3];
    const uint8_t *src_data[3];
    int64_t out_frame_index = 0;

    do {
        buffer = NULL;
        frame = (Frame*)av_mallocz(sizeof(Frame));
        if (!frame) {
            frame         = s->end_frame;
            frame->status = AVERROR(ENOMEM);
            decode_done   = 1;
            s->end_frame  = NULL;
            goto push_frame;
        }
        frame->status = (*s->decoder)->read(&buffer);
        if (frame->status == OK) {
	    if (0 && AVDISCARD_DEFAULT < avctx->skip_frame) {	// XXX:
		buffer->meta_data()->findInt32(kKeyIsSyncFrame, &frame->key);
		if (!frame->key) {
		    buffer->release();
		    decode_done = 1;
		    goto push_frame;
		}
	    }
            frame->vframe = (AVFrame*)av_mallocz(sizeof(AVFrame));
            if (!frame->vframe) {
                frame->status = AVERROR(ENOMEM);
                decode_done   = 1;
                buffer->release();
                goto push_frame;
            }
            ret = ff_get_buffer(avctx, frame->vframe);
            if (ret < 0) {
                av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
                frame->status = ret;
                decode_done   = 1;
                buffer->release();
                goto push_frame;
            }

            sp<MetaData> outFormat = (*s->decoder)->getFormat();
            outFormat->findInt32(kKeyWidth , &w);
            outFormat->findInt32(kKeyHeight, &h);

            // The OMX.SEC decoder doesn't signal the modified width/height
            if (s->decoder_component && !strncmp(s->decoder_component, "OMX.SEC", 7) &&
                (w & 15 || h & 15)) {
                if (((w + 15)&~15) * ((h + 15)&~15) * 3/2 == buffer->range_length()) {
                    w = (w + 15)&~15;
                    h = (h + 15)&~15;
                }
            }

            if (!avctx->width || !avctx->height || avctx->width > w || avctx->height > h) {
                avctx->width  = w;
                avctx->height = h;
            }

            src_linesize[0] = av_image_get_linesize(avctx->pix_fmt, w, 0);
            src_linesize[1] = av_image_get_linesize(avctx->pix_fmt, w, 1);
            src_linesize[2] = av_image_get_linesize(avctx->pix_fmt, w, 2);

            src_data[0] = (uint8_t*)buffer->data();
            src_data[1] = src_data[0] + src_linesize[0] * h;
            src_data[2] = src_data[1] + src_linesize[1] * -(-h>>pix_desc->log2_chroma_h);

	    int32_t colorFormat = 0;
	    outFormat->findInt32(kKeyColorFormat, &colorFormat);

	    if (colorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) {
		int32_t l, t, r, b;
		outFormat->findRect(kKeyCropRect, &l, &t, &r, &b);
		if (l || t) {
		    src_data[0] += l + t * w;
		    src_data[1] += l + t * w / 2;	// XXX:
		}
	    }

	    if (avctx->pix_fmt == AV_PIX_FMT_YUV420P &&
		(colorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar ||
		 colorFormat == OMX_QCOM_COLOR_FormatYVU420SemiPlanar ||
		 colorFormat == OMX_COLOR_FormatYUV420SemiPlanar)) {
		src_linesize[1] = av_image_get_linesize(AV_PIX_FMT_NV12, w, 1);

	      if (1 && (1280 < s->width || 1280 < s->height)) {
		avctx->width  = s->width  >> 1,	// XXX:
		avctx->height = s->height >> 1;

		const uint8_t *sY = src_data[0];
		uint8_t *pY = frame->vframe->data[0];
		w = avctx->width, h = avctx->height;

		if (HAVE_NEON) {
		    ret = w & 0x0f, w &= ~0x0f;

		    while (0 < h--) {
			ScaleRowDown2_NEON(sY, src_linesize[0], pY, w);

			pY += frame->vframe->linesize[0];
			sY += src_linesize[0] << 1;
		    }

		    if (ret) {
			sY = src_data[0] + (w << 1);
			pY = frame->vframe->data[0] + w;
			h  = avctx->height, w = ret;
		    }
		}

		while (0 < h--) {
		    for (ret = 0; ret < w; ++ret) pY[ret] = sY[ret << 1];

		    pY += frame->vframe->linesize[0];
		    sY += src_linesize[0] << 1;
		}

		uint8_t *pU, *pV;
		if (colorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) {
		    pU = frame->vframe->data[1];
		    pV = frame->vframe->data[2];
		} else {
		    pU = frame->vframe->data[2];
		    pV = frame->vframe->data[1];
		}

		const uint16_t *pUV = (const uint16_t*)src_data[1];
		w = avctx->width  >> 1, h = avctx->height >> 1;

		if (HAVE_NEON) {
		    ret = w & 0x0f, w &= ~0x0f;

		    while (0 < h--) {
			SplitScaleUVRowDown2_NEON((const uint8*)pUV,
				pU, pV, w);

			pU  += frame->vframe->linesize[1];
			pV  += frame->vframe->linesize[2];
			pUV += src_linesize[1];
		    }

		    if (ret) {
			if (colorFormat ==
				OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) {
			    pU  = frame->vframe->data[1] + w;
			    pV  = frame->vframe->data[2] + w;
			} else {
			    pU  = frame->vframe->data[2] + w;
			    pV  = frame->vframe->data[1] + w;
			}   pUV = (const uint16_t*)src_data[1] + w;

			h = avctx->height >> 1, w = ret;
		    }
		}

		while (0 < h--) {
		    for (ret = 0; ret < w; ++ret) {
			pU[ret] = pUV[ret << 1] & 0xff;
			pV[ret] = pUV[ret << 1] >> 8;
		    }

		    pUV += src_linesize[1];	// XXX:
		    pU  += frame->vframe->linesize[1];
		    pV  += frame->vframe->linesize[2];
		}
	      } else	// XXX:
	      if (0 && (1024 < s->width || 1024 < s->height)) {
	      } else {
		av_image_copy_plane(frame->vframe->data[0],
			frame->vframe->linesize[0],
			src_data[0], src_linesize[0],
			avctx->width, avctx->height);

		uint8_t  *pU, *pV;
		if (colorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) {
		    pU = frame->vframe->data[1];
		    pV = frame->vframe->data[2];
		} else {
		    pU = frame->vframe->data[2];
		    pV = frame->vframe->data[1];
		}

		const uint16_t *pUV = (const uint16_t*)src_data[1];
		w = avctx->width  >> 1, h = avctx->height >> 1,
		src_linesize[1] /= sizeof(*pUV);	// XXX:

		if (HAVE_NEON) {
		    ret = w & 0x0f, w &= ~0x0f;

		    while (0 < h--) {
			SplitUVRow_NEON((const uint8*)pUV, pU, pV, w);

			pU  += frame->vframe->linesize[1];
			pV  += frame->vframe->linesize[2];
			pUV += src_linesize[1];
		    }

		    if (ret) {
			if (colorFormat ==
				OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) {
			    pU  = frame->vframe->data[1] + w;
			    pV  = frame->vframe->data[2] + w;
			} else {
			    pU  = frame->vframe->data[2] + w;
			    pV  = frame->vframe->data[1] + w;
			}   pUV = (const uint16_t*)src_data[1] + w;

			h = avctx->height >> 1, w = ret;
		    }
		}

		while (0 < h--) {
		    for (ret = 0; ret < w; ++ret) {
			pU[ret] = pUV[ret] & 0xff;
			pV[ret] = pUV[ret] >> 8;
		    }

		    pUV += src_linesize[1];
		    pU  += frame->vframe->linesize[1];
		    pV  += frame->vframe->linesize[2];
		}
	      }
	    } else

            av_image_copy(frame->vframe->data, frame->vframe->linesize,
                          src_data, src_linesize,
                          avctx->pix_fmt, avctx->width, avctx->height);

            buffer->meta_data()->findInt64(kKeyTime, &out_frame_index);
            if (out_frame_index && s->ts_map->count(out_frame_index) > 0) {
                frame->vframe->pts = (*s->ts_map)[out_frame_index].pts;
                frame->vframe->reordered_opaque = (*s->ts_map)[out_frame_index].reordered_opaque;
                s->ts_map->erase(out_frame_index);
            }
            buffer->release();
	} else if (frame->status == INFO_FORMAT_CHANGED) {
	    if (buffer)
		buffer->release();
	    av_free(frame);
	    continue;
	} else {
	    decode_done = 1;
	}
push_frame:
        while (true) {
            pthread_mutex_lock(&s->out_mutex);
            if (s->out_queue->size() >= 10) {
                pthread_mutex_unlock(&s->out_mutex);
                usleep(10000);
                continue;
            }
            break;
        }
        s->out_queue->push_back(frame);
        pthread_mutex_unlock(&s->out_mutex);
    } while (!decode_done && !s->stop_decode);

    s->thread_exited = true;

    return 0;
}

static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p,
	const uint8_t *end)
{
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

static const uint8_t *ff_avc_find_startcode(const uint8_t *p,
	const uint8_t *end){
    const uint8_t *out= ff_avc_find_startcode_internal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

static int ff_extradata_to_avcc(uint8_t *pb, const uint8_t *buf_in, int size)
{
    const uint8_t *end = buf_in + size;
    const uint8_t *nal_start, *nal_end;

    uint32_t sps_size = 0, pps_size = 0;
    const uint8_t *sps = NULL, *pps = NULL;

    for (nal_start = ff_avc_find_startcode(buf_in, end); ;
	 nal_start = nal_end) {
        while (nal_start < end && !*(nal_start++)) ;
        if (nal_start == end) break;
        nal_end = ff_avc_find_startcode(nal_start, end);

	size = nal_end - nal_start;
        int nal_type = nal_start[0] & 0x1f;
	if (nal_type == 7) { /* SPS */
	    sps = nal_start;
	    sps_size = size;
	} else
	if (nal_type == 8) { /* PPS */
	    pps = nal_start;
	    pps_size = size;
	}

	if (sps && pps) {
	    end = pb;

            AV_WB8(pb, 1); /* version */
	    pb += 1;

            AV_WB8(pb, sps[1]); /* profile */
	    pb += 1;
            AV_WB8(pb, sps[2]); /* profile compat */
	    pb += 1;
            AV_WB8(pb, sps[3]); /* level */
	    pb += 1;

            AV_WB8(pb, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
	    pb += 1;
            AV_WB8(pb, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */
	    pb += 1;

            AV_WB16(pb, sps_size);
	    pb += 2;
            memcpy(pb, sps, sps_size);
	    pb += sps_size;

            AV_WB8(pb, 1); /* number of pps */
	    pb += 1;

            AV_WB16(pb, pps_size);
	    pb += 2;
            memcpy(pb, pps, pps_size);
	    pb += pps_size;

	    return pb - end;
	}
    }

    return 0;
}

static av_cold int Stagefright_init(AVCodecContext *avctx)
{
    StagefrightContext *s = (StagefrightContext*)avctx->priv_data;
    sp<MetaData> meta, outFormat;
    int32_t colorFormat = 0;
    int ret;

    if (!avctx->extradata || avctx->extradata_size < 7)
        return -1;

  if (avctx->extradata[0] == 1) {
    s->bsfc  = av_bitstream_filter_init("h264_mp4toannexb");
    if (!s->bsfc) {
        av_log(avctx, AV_LOG_ERROR, "Cannot open the h264_mp4toannexb BSF!\n");
        return -1;
    }
  }

    s->orig_extradata_size = avctx->extradata_size;
    s->orig_extradata = (uint8_t*) av_mallocz(avctx->extradata_size +
                                              FF_INPUT_BUFFER_PADDING_SIZE);
    if (!s->orig_extradata) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    memcpy(s->orig_extradata, avctx->extradata, avctx->extradata_size);

    if (avctx->extradata[0] != 1) {
	avctx->extradata_size = ff_extradata_to_avcc(avctx->extradata,
		s->orig_extradata,  s->orig_extradata_size);
	if (avctx->extradata_size < 1) {
	    memcpy(avctx->extradata, s->orig_extradata,
		   avctx->extradata_size = s->orig_extradata_size);
	    av_log(avctx, AV_LOG_ERROR, "Can't convert to avcC extradata!\n");
	    ret = -1;	goto fail;
	}
    }

    meta = new MetaData;
    if (meta == NULL) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
    meta->setInt32(kKeyWidth, avctx->width);
    meta->setInt32(kKeyHeight, avctx->height);
    meta->setData(kKeyAVCC, kTypeAVCC, avctx->extradata, avctx->extradata_size);

    android::ProcessState::self()->startThreadPool();

    s->source    = new sp<MediaSource>();
    *s->source   = new CustomSource(avctx, meta);
    s->in_queue  = new List<Frame*>;
    s->out_queue = new List<Frame*>;
    s->ts_map    = new std::map<int64_t, TimeStamp>;
    s->client    = new OMXClient;
    s->end_frame = (Frame*)av_mallocz(sizeof(Frame));
    if (s->source == NULL || !s->in_queue || !s->out_queue || !s->client ||
        !s->ts_map || !s->end_frame) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    if (s->client->connect() !=  OK) {
        av_log(avctx, AV_LOG_ERROR, "Cannot connect OMX client\n");
        ret = -1;
        goto fail;
    }

    s->decoder  = new sp<MediaSource>();
    *s->decoder = OMXCodec::Create(s->client->interface(), meta,
                                  false, *s->source, NULL,
                                  OMXCodec::kClientNeedsFramebuffer);
    if ((*s->decoder)->start() !=  OK) {
        av_log(avctx, AV_LOG_ERROR, "Cannot start decoder\n");
        ret = -1;
        s->client->disconnect();
        goto fail;
    }

    outFormat = (*s->decoder)->getFormat();
    outFormat->findInt32(kKeyColorFormat, &colorFormat);
    if (0 && colorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar)
	avctx->pix_fmt = AV_PIX_FMT_RGBA; else
    if (0 && colorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar)
	avctx->pix_fmt = AV_PIX_FMT_NV12; else
    if (0 && (colorFormat == OMX_QCOM_COLOR_FormatYVU420SemiPlanar ||
        colorFormat == OMX_COLOR_FormatYUV420SemiPlanar))
        avctx->pix_fmt = AV_PIX_FMT_NV21;
    else if (colorFormat == OMX_COLOR_FormatYCbYCr)
        avctx->pix_fmt = AV_PIX_FMT_YUYV422;
    else if (colorFormat == OMX_COLOR_FormatCbYCrY)
        avctx->pix_fmt = AV_PIX_FMT_UYVY422;
    else
        avctx->pix_fmt = AV_PIX_FMT_YUV420P;

    s->width = avctx->width, s->height = avctx->height;

    outFormat->findCString(kKeyDecoderComponent, &s->decoder_component);
    if (s->decoder_component)
        s->decoder_component = av_strdup(s->decoder_component);

    pthread_mutex_init(&s->in_mutex, NULL);
    pthread_mutex_init(&s->out_mutex, NULL);
    pthread_cond_init(&s->condition, NULL);
    return 0;

fail:
  if (s->bsfc)
    av_bitstream_filter_close(s->bsfc);
    av_freep(&s->orig_extradata);
    av_freep(&s->end_frame);
    delete s->in_queue;
    delete s->out_queue;
    delete s->ts_map;
    delete s->client;
    return ret;
}

static int Stagefright_decode_frame(AVCodecContext *avctx, void *data,
                                    int *got_frame, AVPacket *avpkt)
{
    StagefrightContext *s = (StagefrightContext*)avctx->priv_data;
    Frame *frame;
    status_t status;
    int orig_size = avpkt->size;
    AVFrame *ret_frame;
    AVPacket pkt;

    if (!s->thread_started) {
        pthread_create(&s->decode_thread_id, NULL, &decode_thread, avctx);
        s->thread_started = true;
    }

    if (avpkt && avpkt->data && s->bsfc) {
	pkt = *avpkt;
        av_bitstream_filter_filter(s->bsfc, avctx, NULL, &pkt.data, &pkt.size,
                                   avpkt->data, avpkt->size, avpkt->flags & AV_PKT_FLAG_KEY);
        avpkt = &pkt;
    }

    if (!s->source_done) {
        if(!s->dummy_buf) {
            s->dummy_buf = (uint8_t*)av_malloc(avpkt->size +
		    FF_INPUT_BUFFER_PADDING_SIZE);
            if (!s->dummy_buf)
                return AVERROR(ENOMEM);
            s->dummy_bufsize = avpkt->size;
            memcpy(s->dummy_buf, avpkt->data, avpkt->size);
        }

        frame = (Frame*)av_mallocz(sizeof(Frame));
        if (avpkt->data) {
            frame->status  = OK;
            frame->size    = avpkt->size;
            frame->key     = avpkt->flags & AV_PKT_FLAG_KEY ? 1 : 0;
            frame->buffer  = (uint8_t*)av_malloc(avpkt->size +
		    FF_INPUT_BUFFER_PADDING_SIZE);
            if (!frame->buffer) {
                av_freep(&frame);
                return AVERROR(ENOMEM);
            }
            uint8_t *ptr = avpkt->data;
            // The OMX.SEC decoder fails without this.
            if (avpkt->size == orig_size + avctx->extradata_size) {
                ptr += avctx->extradata_size;
                frame->size = orig_size;
            }
            memcpy(frame->buffer, ptr, frame->size);
            if (avpkt == &pkt)
                av_free(avpkt->data);

            frame->time = ++s->frame_index;
            (*s->ts_map)[s->frame_index].pts = avpkt->pts;
            (*s->ts_map)[s->frame_index].reordered_opaque = avctx->reordered_opaque;
        } else {
            frame->status  = ERROR_END_OF_STREAM;
            s->source_done = true;
        }

        while (true) {
            if (s->thread_exited) {
                s->source_done = true;
                break;
            }
            pthread_mutex_lock(&s->in_mutex);
            if (s->in_queue->size() >= 10) {
                pthread_mutex_unlock(&s->in_mutex);
                usleep(10000);
                continue;
            }
            s->in_queue->push_back(frame);
            pthread_cond_signal(&s->condition);
            pthread_mutex_unlock(&s->in_mutex);
            break;
        }
    }
    while (true) {
        pthread_mutex_lock(&s->out_mutex);
        if (!s->out_queue->empty()) break;
        pthread_mutex_unlock(&s->out_mutex);
        if (s->source_done) {
            usleep(10000);
            continue;
        } else {
            return orig_size;
        }
    }

    frame = *s->out_queue->begin();
    s->out_queue->erase(s->out_queue->begin());
    pthread_mutex_unlock(&s->out_mutex);

    ret_frame = frame->vframe;
    status  = frame->status;
    av_freep(&frame);

    if (status == ERROR_END_OF_STREAM)
        return 0;
    if (status != OK) {
        if (status == AVERROR(ENOMEM))
            return status;
        av_log(avctx, AV_LOG_ERROR, "Decode failed: %x\n", status);
        return -1;
    }

    if (s->prev_frame) {
        avctx->release_buffer(avctx, s->prev_frame);
        av_freep(&s->prev_frame);
    }
    s->prev_frame = ret_frame;

    if (!ret_frame) return orig_size;
    *got_frame = 1;
    *(AVFrame*)data = *ret_frame;
    return orig_size;
}

static av_cold int Stagefright_close(AVCodecContext *avctx)
{
    StagefrightContext *s = (StagefrightContext*)avctx->priv_data;
    Frame *frame;

    if (s->thread_started) {
        if (!s->thread_exited) {
            s->stop_decode = 1;

            // Make sure decode_thread() doesn't get stuck
            pthread_mutex_lock(&s->out_mutex);
            while (!s->out_queue->empty()) {
                frame = *s->out_queue->begin();
                s->out_queue->erase(s->out_queue->begin());
                if (frame->vframe) {
                    avctx->release_buffer(avctx, frame->vframe);
                    av_freep(&frame->vframe);
                }
                av_freep(&frame);
            }
            pthread_mutex_unlock(&s->out_mutex);

            // Feed a dummy frame prior to signalling EOF.
            // This is required to terminate the decoder(OMX.SEC)
            // when only one frame is read during stream info detection.
            if (s->dummy_buf && (frame = (Frame*)av_mallocz(sizeof(Frame)))) {
                frame->status = OK;
                frame->size   = s->dummy_bufsize;
                frame->key    = 1;
                frame->buffer = s->dummy_buf;
                pthread_mutex_lock(&s->in_mutex);
                s->in_queue->push_back(frame);
                pthread_cond_signal(&s->condition);
                pthread_mutex_unlock(&s->in_mutex);
                s->dummy_buf = NULL;
            }

            pthread_mutex_lock(&s->in_mutex);
            s->end_frame->status = ERROR_END_OF_STREAM;
            s->in_queue->push_back(s->end_frame);
            pthread_cond_signal(&s->condition);
            pthread_mutex_unlock(&s->in_mutex);
            s->end_frame = NULL;
        }

        pthread_join(s->decode_thread_id, NULL);

        if (s->prev_frame) {
            avctx->release_buffer(avctx, s->prev_frame);
            av_freep(&s->prev_frame);
        }

        s->thread_started = false;
    }

    while (!s->in_queue->empty()) {
        frame = *s->in_queue->begin();
        s->in_queue->erase(s->in_queue->begin());
        if (frame->size)
            av_freep(&frame->buffer);
        av_freep(&frame);
    }

    while (!s->out_queue->empty()) {
        frame = *s->out_queue->begin();
        s->out_queue->erase(s->out_queue->begin());
        if (frame->vframe) {
            avctx->release_buffer(avctx, frame->vframe);
            av_freep(&frame->vframe);
        }
        av_freep(&frame);
    }

    (*s->decoder)->stop();
    s->client->disconnect();

    if (s->decoder_component)
        av_freep(&s->decoder_component);
    av_freep(&s->dummy_buf);
    av_freep(&s->end_frame);

    // Reset the extradata back to the original mp4 format, so that
    // the next invocation (both when decoding and when called from
    // av_find_stream_info) get the original mp4 format extradata.
    av_freep(&avctx->extradata);
    avctx->extradata = s->orig_extradata;
    avctx->extradata_size = s->orig_extradata_size;

    delete s->in_queue;
    delete s->out_queue;
    delete s->ts_map;
    delete s->client;
    delete s->decoder;
    delete s->source;

    pthread_mutex_destroy(&s->in_mutex);
    pthread_mutex_destroy(&s->out_mutex);
    pthread_cond_destroy(&s->condition);
  if (s->bsfc)
    av_bitstream_filter_close(s->bsfc);
    return 0;
}

#if LIBAVCODEC_VERSION_MAJOR < 54
AVCodec ff_libstagefright_h264_decoder = {
    "libstagefright_h264",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_H264,
    sizeof(StagefrightContext),
    Stagefright_init,
    NULL, //encode
    Stagefright_close,
    Stagefright_decode_frame,
    CODEC_CAP_DELAY,
    NULL, //next
    NULL, //flush
    NULL, //supported_framerates
    NULL, //pixel_formats
    NULL_IF_CONFIG_SMALL("libstagefright H.264"),
};
#else
AVCodec ff_libstagefright_h264_decoder = {
    "libstagefright_h264",
    NULL_IF_CONFIG_SMALL("libstagefright H.264"),
    AVMEDIA_TYPE_VIDEO,
    AV_CODEC_ID_H264,
    CODEC_CAP_DELAY,
    NULL, //supported_framerates
    NULL, //pix_fmts
    NULL, //supported_samplerates
    NULL, //sample_fmts
    NULL, //channel_layouts
    0,    //max_lowres
    NULL, //priv_class
    NULL, //profiles
    sizeof(StagefrightContext),
    NULL, //next
    NULL, //init_thread_copy
    NULL, //update_thread_context
    NULL, //defaults
    NULL, //init_static_data
    Stagefright_init,
    NULL, //encode
    NULL, //encode2
    Stagefright_decode_frame,
    Stagefright_close,
};
#endif

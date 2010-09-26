/****************************************************************
 * $ID: ffdroidplayer.h  , 08 10ÔÂ 2010 15:35:27 +0800  mhfan $ *
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
#ifndef FFDROIDPLAYER_H
#define FFDROIDPLAYER_H

#include <sys/time.h>
#include <sys/resource.h>

#ifdef  ANDROID
#include <binder/MemoryHeapBase.h>
//#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>

#include <media/MediaPlayerInterface.h>
#include <media/AudioTrack.h>
#include <media/Metadata.h>

//#include <utils/RefBase.h>
#include <utils/threads.h>
#include <utils/Log.h>
#else// XXX:
#define ANDROID_PRIORITY_DISPLAY 1
#define ANDROID_PRIORITY_AUDIO	 2
#define FFDROID_PLAYER 7
#define UNKNOWN_ERROR -1
#define status_t int
#define OK 0
#endif

#define TAG "FFDroidPlayer"

extern "C" {

#define GNU_SOURCE 1
#define _ISOC99_SOURCE 1
#define _POSIX_C_SOURCE 200112

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1

#define _XOPEN_SOURCE 600
#define NDEBUG 1    // XXX:

#define _UNDEF__STDC_CONSTANT_MACROS 1
#define _UNDEF__STDC_FORMAT_MACROS 1
#define _UNDEF__STDC_LIMIT_MACROS 1

#define __STDC_CONSTANT_MACROS 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1

#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <inttypes.h>

//#include <libavutil/avutil.h>
//#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

//#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>

#include <libavutil/avassert.h>
#include "libavcodec/audioconvert.h"

#undef  AV_NOPTS_VALUE	// XXX:
#define AV_NOPTS_VALUE	(int64_t)INT64_C(0x8000000000000000)
#define ARRAY_SIZE(x)	((unsigned)(sizeof(x) / sizeof((x)[0])))

extern int av_image_fill_linesizes(int linesizes[4],
	enum PixelFormat pix_fmt, int width);
};

namespace android {

struct AudioBlock {
    uint8_t* data;
    uint32_t size;

    int64_t pts;
};

struct AVPictureP {
    uint8_t *data[4];
    int  linesize[4];

    int64_t pts;
};

template <typename ElemType>
struct MediaElemList {
    MediaElemList<ElemType>* next;
    ElemType elem;
};

template <typename ElemType>
unsigned getSize(ElemType* elem) { return 0; }

template <>
unsigned getSize(AVPacket* pkt)  { return pkt->size; }

template <typename ElemType>
void freeElem(ElemType* elem) { }

template <>
void freeElem(AVPacket* pkt) { av_free_packet(pkt); }

template <>
void freeElem(AVSubtitle* sup) { avsubtitle_free(sup); }

template <>
void freeElem(AudioBlock* blk) { av_free(blk->data); }

template <>
void freeElem(AVPictureP* pic) {
    int n = ARRAY_SIZE(pic->linesize);
    while (n--) if (pic->linesize[n]) av_free(pic->data[n]);
}


template <>
void freeElem(SwsContext* cvt) { sws_freeContext(cvt); }

template <>
void freeElem(AVAudioConvert* cvt) { av_audio_convert_free(cvt); }


template <typename ElemType>
class MediaFIFO {
    typedef MediaElemList<ElemType> NodeType;

    const unsigned MAX_NODES, MAX_BYTES;
    unsigned len, num:15, req:1, idx:8;

    NodeType *head, *tail;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;

  public:
    void clear() {
	pthread_mutex_lock  (&mutex);
	for (NodeType* node; (node = head); ) {
	    head =     node->next;
	    freeElem (&node->elem);
	    av_free   (node);
	}   num = 0;	len = 0;
	pthread_mutex_unlock(&mutex);
    }

    void abort() {
	pthread_mutex_lock  (&mutex);
	req = 0;
	pthread_cond_signal (&cond);
	pthread_mutex_unlock(&mutex);
    }

    bool isempty() { return !(num && head); }
    bool isfull()  { return !(num < MAX_NODES && len < MAX_BYTES); }

    int pop (ElemType* elem, bool block = true) {
	int ret;

	pthread_mutex_lock  (&mutex);
	for ( ; ; ) {
	    if (!req) { ret = -1; break; }

	    if (head) {
		NodeType* node = head;
		if (!(head = head->next)) tail = NULL;
		len -= getSize(&node->elem);	--num;
		*elem = node->elem;	av_free(node);
		ret = 0; break;
	    } else
	    if (!block) { ret = 1; break; }

	    pthread_cond_wait(&cond, &mutex);
	}   pthread_mutex_unlock(&mutex);

	return ret;
    }

    int push(ElemType* elem, bool block = false) {
	int ret;

	pthread_mutex_lock  (&mutex);
	for ( ; ; ) {
	    NodeType* node;

	    if (0 && isfull()) {	// XXX:
		if (!block) { ret =  1; break; }
		pthread_cond_wait(&cond, &mutex);
	    }	if (!req)   { ret = -1; break; }

	    node = (NodeType*)av_malloc(sizeof(*node));
	    av_assert1(node);

	    node->elem = *elem;
	    node->next = NULL;

	    if (tail) tail->next = node; else head = node;
	    len += getSize(&node->elem);
	    tail = node;	++num;

	    pthread_cond_signal (&cond);
	    ret = 0;	break;
	}   pthread_mutex_unlock(&mutex);

	return ret;
    }

     MediaFIFO(unsigned n = 25, unsigned s = 2 << 20, int i = 0):
	    MAX_NODES(n), MAX_BYTES(s), idx(i) {
	pthread_cond_init (&cond,  NULL);
	pthread_mutex_init(&mutex, NULL);
	head = tail = NULL;
	len = num = 0;
	req = 1;
    }

    ~MediaFIFO() {	clear();
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy (&cond);
    }
};

struct MediaDecoderBase {
    MediaFIFO<AVPacket>  pktq;

    MediaDecoderBase(unsigned pn = 25, unsigned pb = 2 << 20):
	pktq(pn, pb) { }

    virtual void clear() = 0;
    virtual ~MediaDecoderBase() { };
};

template <typename FrameType, typename ConvType>
int decode(AVFormatContext *avfc, AVPacket* pkt,
	FrameType* frm, ConvType* cvt) { return -1; }	// XXX:

template <>
int decode(AVFormatContext *avfc, AVPacket* pkt,
	AudioBlock* blk, AVAudioConvert* acvt) {
    AVCodecContext* avctx;	AVStream *avstm;

    DECLARE_ALIGNED(16, uint8_t, abuf)[
	    (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

    avctx = (avstm = avfc->streams[pkt->stream_index])->codec;
    if (!pkt->priv) pkt->priv = (void*)pkt->data;

    while (0 < pkt->size) {
	int ret, blen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	ret = avcodec_decode_audio3(avctx, (int16_t*)abuf, &blen, pkt);

	if (ret < 0) {
	    pkt->data = (uint8_t*)pkt->priv;
	    pkt->size = 0;	return ret;
	}	// XXX:

	pkt->data += ret;	pkt->size -= ret;
	if (blen < 1) continue;

	if (acvt) {
	    const void *ibufs[6] = { abuf };
	    void *obufs[6] = { NULL };
	    int ostride[6] = { 2 };		// XXX:
	    int istride[6] = {
		av_get_bits_per_sample_fmt(avctx->sample_fmt) / 8 };

	    blk->size = (blen /= istride[0]) * ostride[0];
	    blk->data = (uint8_t*)av_malloc(blk->size);
	    av_assert1(blk->data);

	    obufs[0] = blk->data;
	    if (av_audio_convert(acvt, obufs, ostride,
		    ibufs, istride, blen) < 0)	// XXX:
		av_log(NULL, AV_LOG_WARNING, "Fail to convert audio!\n");
	} else {
	    blk->data = (uint8_t*)av_malloc(blk->size = blen);
	    av_assert1(blk->data);
	    memcpy(blk->data, abuf, blk->size);
	}

	blk->pts = 0;	break;	// XXX:
    }

    if (pkt->size < 1) {
	pkt->data = (uint8_t*)pkt->priv;	// XXX:

	if (pkt->pts != AV_NOPTS_VALUE)
	    blk->pts = av_rescale(pkt->pts, avstm->time_base.den,
		    AV_TIME_BASE * (int64_t)avstm->time_base.num);
    }

    return 0;
}

template <>
int decode(AVFormatContext *avfc, AVPacket* pkt,
	AVPictureP* pic, SwsContext* icvt) {
    AVCodecContext* avctx;	AVStream *avstm;

    AVFrame frm_, *frm = &frm_;
    int i, ret, fdelay;	// XXX:

    avctx = (avstm = avfc->streams[pkt->stream_index])->codec;

    avctx->reordered_opaque = pkt->pts;
    ret = avcodec_decode_video2(avctx, frm, &i, pkt);
    pkt->size = 0;	// XXX:

    if (!i) return ret;	// XXX:

    if (1) frm->pts = frm->reordered_opaque; else frm->pts = pkt->dts;
    if (frm->pts == AV_NOPTS_VALUE) pic->pts = 0; else
	pic->pts = av_rescale(frm->pts, AV_TIME_BASE *
		(int64_t)avstm->time_base.num, avstm->time_base.den);

    fdelay  = av_rescale(1, avstm->time_base.den,
	    AV_TIME_BASE * (int64_t)avstm->time_base.num);
    fdelay += frm->repeat_pict * (fdelay * 0.5);

    if (pic->linesize[0] < 1) {	// XXX:
	if (icvt) {	int dstw;
	    enum ::PixelFormat dstp;
	    sws_getDstInfo(icvt, &dstp, &dstw, NULL);
	    av_image_fill_linesizes(pic->linesize, dstp, dstw);
	} else
	    for (i = 0; i < (int)ARRAY_SIZE(pic->linesize); ++i)
		pic->linesize[i] = frm->linesize[i];
    }

    if (icvt) {		int dsth;
	sws_getDstInfo(icvt, NULL, NULL, &dsth);
	    ret = dsth;
    } else  ret = avctx->height;

    for (i = 0; i < (int)ARRAY_SIZE(pic->linesize); ++i) {
	if (pic->linesize[i] < 1) continue;
	pic->data[i] = (uint8_t*)av_malloc(pic->linesize[i] * ret + 16);
	av_assert1(pic->data[i]);
    }

    if (icvt) sws_scale(icvt, frm->data, frm->linesize,
	    0, avctx->height, pic->data, pic->linesize); else
	av_picture_copy((AVPicture*)pic, (AVPicture*)frm,
	    avctx->pix_fmt, avctx->width, avctx->height);

    return 0;
}

template <>
int decode(AVFormatContext *avfc, AVPacket* pkt,
	AVSubtitle* sup, void* cvt) {
    AVCodecContext* avctx;	AVStream *avstm;
    int i, ret;

    avctx = (avstm = avfc->streams[pkt->stream_index])->codec;
    ret = avcodec_decode_subtitle2(avctx, sup, &i, pkt);
    pkt->size = 0;	// XXX:

    if (!i || sup->format != 0) {	// XXX:
	av_log(NULL, AV_LOG_WARNING, "Fail to render subtitle!\n");
	return ret;
    }

    sup->pts = (sup->pts == AV_NOPTS_VALUE) ? 0 :
	    av_rescale(sup->pts, avstm->time_base.den,
	    AV_TIME_BASE * (int64_t)avstm->time_base.num);
    sup->pts += sup->start_display_time * 1000;

    return 0;
}

template<typename FrameType, typename ConvType>
void* decode_thread(void* data);

template<typename FrameType, typename ConvType>
struct MediaDecoder: public MediaDecoderBase {
    MediaFIFO<FrameType> frmq;
    AVFormatContext *avfc;
    ConvType* cvtr;

    virtual void clear() {
	pktq.clear();
	frmq.clear();
    }

    void* decoding(void* data) {
	AVPacket pkt_, *pkt = &pkt_;
	FrameType frm;

	pkt->size = 0;
	pkt->data = NULL;
	memset((void*)&frm, 0, sizeof(frm));

	for ( ; ; ) {
	    if (frmq.isfull()) {
		usleep(40*1000);
		continue;
	    }

	    if (pkt->size < 1) {
		if (pkt->data) freeElem(pkt);  // XXX:
		if (pktq.pop(pkt, true) < 0) return NULL;
	    }

	    if (!decode<FrameType, ConvType>(avfc, pkt, &frm, cvtr))
		frmq.push(&frm);
	}   freeElem(pkt);

	return NULL;
    }

     MediaDecoder(AVFormatContext* ic, ConvType* cvt,
	    unsigned fn = 5, unsigned pn = 25, unsigned pb = 2 << 20):
	    MediaDecoderBase(pn, pb), frmq(fn), avfc(ic), cvtr(cvt) {
	pthread_t thid;	// XXX:

	if (pthread_create(&thid, NULL,
		decode_thread<FrameType, ConvType>, this) < 0) ;
	pthread_detach(thid);
    }

    virtual ~MediaDecoder() {
	pktq.abort();
	frmq.abort();

	freeElem(cvtr);
    }
};

template<typename FrameType, typename ConvType>
void* decode_thread(void* data) {
    MediaDecoder<FrameType, ConvType>* md =
	    (MediaDecoder<FrameType, ConvType>*)data;
    return md->decoding(data);
}

enum MediaPlayerStates {
    MP_IDLE,

    MP_INITED,
    MP_PREPARED,
    MP_PLAYING,
    MP_PAUSED,

    MP_END, MP_EOF = MP_END,

    MP_STOP,  MP_QUIT = MP_STOP,

    MP_PAUSE,
    MP_START, MP_PLAY = MP_START,

    // http://developer.android.com/reference/android/media/MediaPlayer.html#StateDiagram
};

static void* demux_thread(void* data);
static void* outputA_thread(void* data);
static void* outputV_thread(void* data);

class FFDroidPlayer
#ifdef  ANDROID
	: public MediaPlayerInterface
#endif
{
    MediaPlayerStates state;
    uint16_t loop, qn;

    int64_t clkt, clkr;
    int64_t spos;

    AVFormatContext *avfc;
    short psidx[AVMEDIA_TYPE_NB];

    MediaDecoderBase* mdec[AVMEDIA_TYPE_NB];

#ifdef  ANDROID
    sp<MemoryHeapBase> mheap;
    sp<ISurface> isurf;
#endif

  public:
    /*static */int url_interrupt_cb(void) {
	return (state == MP_STOP/* || state == MP_END*/);
    }

    void* outputA(void* data) {
	int bps, us;
	int64_t lpts = 0;
	AVCodecContext* avctx;
	MediaDecoder<AudioBlock, AVAudioConvert>* adec;

	avctx = avfc->streams[psidx[AVMEDIA_TYPE_AUDIO]]->codec;
	bps = avctx->sample_rate * avctx->channels * sizeof(int16_t);
	adec = (MediaDecoder<AudioBlock, AVAudioConvert>*)
		mdec[AVMEDIA_TYPE_AUDIO];
	if (bps < 1) bps = 1;

	for ( ; ; ) {
	    AudioBlock blk_, *blk = &blk_;

	    if (state == MP_STOP/* || state == MP_END*/) break; else
	    if (state == MP_PAUSED) {
		usleep(40*1000 / 2);
		continue;
	    }

	    if (adec->frmq.pop(blk, true)) break;

	    us = blk->pts ? blk->pts - lpts : blk->size * AV_TIME_BASE / bps;
	    clkr = lpts;	lpts += us;	// XXX:

#ifdef  ANDROID
	    if (mAudioSink->write(blk->data, blk->size) < 0) ;
#else// XXX:
	    usleep(us);
#endif

	    freeElem(blk);

	    us = av_gettime() - clkt - lpts;
	    if (40*1000 / 2 < abs(us))	// XXX:
		av_log(NULL, AV_LOG_DEBUG, "Audio delayed too much: %gs\n",
			us / 1000000.f);
	}

	return NULL;
    }

    void* outputV(void* data) {
	int dsth;
	//int64_t lpts = 0;	// XXX:
	AVSubtitle sup_, *sup = &sup_;
	MediaDecoder<AVSubtitle, void>* sdec;
	MediaDecoder<AVPictureP, SwsContext>* vdec;

	sdec = (MediaDecoder<AVSubtitle, void>*)mdec[AVMEDIA_TYPE_SUBTITLE];
	vdec = (MediaDecoder<AVPictureP, SwsContext>*)mdec[AVMEDIA_TYPE_VIDEO];
	if (!vdec->cvtr) getVideoHeight(&dsth); else
	sws_getDstInfo(vdec->cvtr, NULL, NULL, &dsth);
	sup->rects = NULL;

dtrace;
	for ( ; ; ) {
	    AVPictureP pic_, *pic = &pic_;
	    int diff;

	    if (state == MP_STOP/* || state == MP_END*/) break; else
	    if (state == MP_PAUSED) {
		usleep(40*1000 / 2);
		continue;
	    }

	    if (vdec->frmq.pop(pic, true)) break;
	    if (sdec && !sup->rects) sdec->frmq.pop(sup, false);

#ifdef  ANDROID
	    if (mAudioSink.get()) ; else
#endif

	    if (psidx[AVMEDIA_TYPE_AUDIO] < 0)	// XXX:
		clkr = av_gettime() - clkt;

	    if (40*1000 * 10 < (diff = clkr - pic->pts)) {
		if (0) av_log(NULL, AV_LOG_WARNING,	// XXX:
			"Out of sync. A-V: %gs\n", diff / 1000000.f);
		freeElem(pic);	continue;
	    } else  if (diff < -40*1000 / 2) usleep(-diff);

	    if (sup->rects) {
		if (!(sup->pts < pic->pts)) {
		    // TODO: subtitle overlay
		} else {
		    int64_t endpts = sup->pts + 1000 *
			    (sup->end_display_time - sup->start_display_time);
		    if (endpts < pic->pts) avsubtitle_free(sup);
		}
	    }

#ifdef  ANDROID
	    if (isurf.get()) {
		memcpy(mheap->getBase(), pic->data[0],
			pic->linesize[0] * dsth);
		isurf->postBuffer(0);	// XXX:
	    }
#endif

	    freeElem(pic);
	}

	return NULL;
    }

    void close() {
	unsigned i;

	state = MP_STOP;    // XXX:
	if (!avfc) return;
	av_log(NULL, AV_LOG_INFO,
		"Play duration: %gs\n", clkr / 1000000.f);
	usleep(40*1000);    // XXX:

	for (i = 0; i < AVMEDIA_TYPE_NB; ++i)
	    if (mdec[i]) { delete mdec[i]; mdec[i] = NULL; }

	for (i = 0; i < avfc->nb_streams; ++i) {
	    AVCodecContext *avctx = avfc->streams[i]->codec;
	    avcodec_close(avctx);
	}   av_close_input_file(avfc);
	avfc = NULL;

#ifdef  ANDROID
	if (isurf.get()) {
	    isurf->unregisterBuffers();
	    //isurf.clear();
	}

	if (mheap.get()) ;//mheap.clear();	// XXX:

	if (mAudioSink.get()) {
	    mAudioSink->close();
	    //mAudioSink.clear();
	}
#endif
    }

    int open(const char* url) {
	int ret;

	//if (avfc) close();
	//url_set_interrupt_cb(FFDroidPlayer::url_interrupt_cb); // FIXME:

	if ((ret = av_open_input_file(&avfc, url, NULL, 0, NULL)) < 0 ||
	    (ret = av_find_stream_info(avfc)) < 0)
	    av_log(NULL, AV_LOG_ERROR, "Fail to load: %s\n", url); else {
	    av_log(NULL, AV_LOG_INFO,  "Loading: %s, %d\n", url, ret);
	    ret = OK;	// XXX:
	}

	if (avfc->pb) avfc->pb->eof_reached = 0;
	    avfc->flags |= AVFMT_FLAG_GENPTS;	// XXX:

	state = MP_INITED;
	return ret;
    }

    int open(int fd, int64_t offset, int64_t length) {
	char fn[16] = "file:";
	av_log(NULL, AV_LOG_WARNING, "Play fd-%d: +%"PRId64" =%"PRId64"\n",
		fd, offset, length);	// XXX:
	if (sprintf(&fn[5], "%d", dup(fd)) < 1) ;
	return open(fn);
    }

    int preparing() {
	int ret, i;
	short strm_cnt[AVMEDIA_TYPE_NB];
	short strm_play[AVMEDIA_TYPE_NB];
	//short strm_best[AVMEDIA_TYPE_NB];

	for (i = 0; i < AVMEDIA_TYPE_NB; ++i) {
	    //strm_best[i] = -1;
	    strm_play[i] = -1;
	    strm_cnt[i] = 0;
	}   ret = OK;

	for (i = 0; i < (int)avfc->nb_streams; ++i) {
	    AVStream *st = avfc->streams[i];
	    AVCodecContext  *avctx = st->codec;
	    avfc->streams[i]->discard = AVDISCARD_ALL;

	    if (!((ret = avctx->codec_type) < AVMEDIA_TYPE_NB) ||
		(strm_cnt[ret]++ != psidx[ret] && -1 < psidx[ret])) continue;

	    //if (!(st->codec_info_nb_frames < strm_best[ret])) continue;
	    //strm_best[ret] = st->codec_info_nb_frames;

	    strm_play[ret] = i;
	}

	state = MP_PREPARED;
	for (i = 0; i < AVMEDIA_TYPE_NB; ++i) {
	    AVCodecContext *avctx;
	    AVCodec *codec;
	    pthread_t out;
	    int idx;

	    psidx[i] = strm_play[i];	// XXX:
	    if ((idx = psidx[i]) < 0) continue;
	    avctx = avfc->streams[idx]->codec;

	    switch (i) {
	    case AVMEDIA_TYPE_AUDIO: {
		AVAudioConvert *acvt = NULL;	// XXX:
		//avctx->request_channel_layout = CH_LAYOUT_STEREO;
		avctx->request_channels = (0 < avctx->channels) ?
			FFMIN(2, avctx->channels) : 2;

		if (avctx->sample_fmt != SAMPLE_FMT_S16 &&
			!(acvt = av_audio_convert_alloc(SAMPLE_FMT_S16, 1,
			avctx->sample_fmt, 1, NULL, 0)))
		    av_log(NULL, AV_LOG_WARNING,
			    "Can't convert audio: %s -> %s\n",
			    av_get_sample_fmt_name(avctx->sample_fmt),
			    av_get_sample_fmt_name(SAMPLE_FMT_S16));

		mdec[i] = new MediaDecoder<AudioBlock,
			AVAudioConvert>(avfc, acvt);

#ifdef  ANDROID
		if (mAudioSink.get()) {
		    status_t err;

		    err = mAudioSink->open(avctx->sample_rate,
			    avctx->request_channels,	// XXX:
			    AudioSystem::PCM_16_BIT,
			    DEFAULT_AUDIOSINK_BUFFERCOUNT,
			    asink_callback, this);

		    if (err != OK) {
			av_log(NULL, AV_LOG_WARNING,
				"Fail to initialize audio sink!\n");
			return UNKNOWN_ERROR;	// XXX:
		    }

		    mAudioSink->start();
		    break;	// XXX:
		}
#endif
		if (pthread_create(&out, NULL, outputA_thread, this) < 0) ;
		pthread_detach(out);
	    }   break;

	    case AVMEDIA_TYPE_VIDEO: {
		SwsContext *icvt = NULL;

		int swsf, dstw, dsth;
		enum ::PixelFormat dstp;

		// XXX: all must be set by user
		dstw = avctx->width, dsth = avctx->height;
		swsf = SWS_FAST_BILINEAR;
		dstp = PIX_FMT_RGB565;

		if (avctx->width != dstw || avctx->height != dsth ||
		    avctx->pix_fmt != dstp) {

		    if (!(icvt = sws_getCachedContext(icvt,
			avctx->width, avctx->height, avctx->pix_fmt,
			dstw, dsth, dstp, swsf, NULL, NULL, NULL)))
			av_log(NULL, AV_LOG_WARNING,
				"Can't convert video: %ux%u/%s -> %ux%u/%s\n",
				avctx->width, avctx->height,
				avcodec_get_pix_fmt_name(avctx->pix_fmt),
				dstw, dsth, avcodec_get_pix_fmt_name(dstp));
		}

		mdec[i] = new MediaDecoder<AVPictureP, SwsContext>(avfc, icvt);

#ifdef  ANDROID
		sendEvent(MEDIA_SET_VIDEO_SIZE, dstw, dsth);	// XXX:

		if (isurf.get()) {
		    mheap = new MemoryHeapBase(dstw * dsth * 2);// XXX:

		    if (mheap->heapID() < 0) {
			av_log(NULL, AV_LOG_ERROR,
				"Fail to create frame heap!\n");
			return UNKNOWN_ERROR;	// XXX:
		    }
dprintn(mheap->getSize());

		    ISurface::BufferHeap bheap(dstw, dsth, dstw, dsth,
			    PIXEL_FORMAT_RGB_565, mheap);	// XXX:

		    if (isurf->registerBuffers(bheap) < 0) {
			av_log(NULL, AV_LOG_ERROR,
				"Fail to register buffers!\n");
			return UNKNOWN_ERROR;	// XXX:
		    }
		} else dtrace;
#endif

		if (pthread_create(&out, NULL, outputV_thread, this) < 0) ;
		pthread_detach(out);
	    }   break;

	    case AVMEDIA_TYPE_SUBTITLE: {
		mdec[i] = new MediaDecoder<AVSubtitle, int>(avfc, NULL);
	    }   break;

	    default: continue;	// XXX:
	    }

	    if (!(codec = avcodec_find_decoder(avctx->codec_id))) {
		av_log(NULL, AV_LOG_ERROR, "Can't find codec: %d-%u\n",
			idx, avctx->codec_id);
		delete mdec[i];
		continue;
	    }

	  if (0) {
	    avctx->debug = 0;
	    avctx->debug_mv = 0;
	    avctx->workaround_bugs = 1;

	    avctx->idct_algo = FF_IDCT_AUTO;
	    if ((avctx->lowres = 0)) avctx->flags |= CODEC_FLAG_EMU_EDGE;
	    if (0) avctx->flags2 |= CODEC_FLAG2_FAST;

	    avctx->skip_frame = AVDISCARD_DEFAULT;
	    avctx->skip_idct  = AVDISCARD_DEFAULT;
	    avctx->skip_loop_filter  = AVDISCARD_DEFAULT;
	    avctx->error_recognition = FF_ER_CAREFUL;
	    avctx->error_concealment = 3;

	    //avcodec_thread_init(avctx, 1);	// XXX:
	  }

	    if ((ret = avcodec_open(avctx, codec)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Fail to open codec: %d-%s\n",
			idx, codec->name);
		avcodec_close(avctx);
		delete mdec[i];
		continue;
	    }

	    avfc->streams[idx]->discard = AVDISCARD_DEFAULT;
	}   clkt = av_gettime();	clkr = avfc->start_time;
	spos = 0;   loop = 1;  qn = 0;	// XXX:

dtrace;
	return ret;
    }

    void* demuxing(void* data) {
	bool paused = true, eof = false;

dtrace;
	state = MP_PLAYING;
	for ( ; ; ) {
	    AVPacket pkt_, *pkt = &pkt_;
	    int i, ret;

	    if (state == MP_STOP/* || state == MP_END*/) break; else
	    if (state == MP_PAUSED) {
		av_read_pause(avfc);
		//mAudioSink->pause();
		paused = true;
	    } else
	    if (paused) {
		//mAudioSink->start();
		av_read_play(avfc);
		paused = false;
	    }

	    if (spos) {
		int64_t smin, smax;

		i = 0;	// XXX: relative, set by user
		smin = 0 < i ? spos - i + 2 : INT64_MIN;
		smax = i < 0 ? spos - i - 2 : INT64_MAX;

		av_log(NULL, AV_LOG_WARNING,
			"Seeking to %gs!\n", spos / 1000000.f);
		if ((ret = avformat_seek_file(avfc, -1,
			smin, spos, smax, 0)) < 0)	// XXX:
		    av_log(NULL, AV_LOG_WARNING, "Seeking failed!\n");
		spos = 0;   eof = false;

#ifdef  ANDROID
		mAudioSink->flush();

		sendEvent(MEDIA_SEEK_COMPLETE, 0, 0);
#endif

		for (i = 0; i < AVMEDIA_TYPE_NB; ++i)
		    if (mdec[i]) mdec[i]->clear();

		//clkr = ;	// XXX:
	    }

	    for (i = 0; i < AVMEDIA_TYPE_NB; ++i)
		if (mdec[i] && mdec[i]->pktq.isfull()) break;
	    if (i < AVMEDIA_TYPE_NB) {
		if (65534 < ++qn) break; // XXX:
		usleep(40*1000);
		continue;
	    }	qn = 0;

	    if (eof) {
		for (i = 0; i < AVMEDIA_TYPE_NB; ++i)
		    if (mdec[i] && !mdec[i]->pktq.isempty()) break;
		if (i < AVMEDIA_TYPE_NB) { usleep(40*1000);  continue; }
		if (loop != 1 && (!loop || --loop)) { spos = 1; continue; }
		break;	// XXX:
	    }

	    if ((ret = av_read_frame(avfc, pkt)) < 0) {
		if  (ret == AVERROR_EOF || avfc->pb->eof_reached) eof = true;
		if ((ret = avfc->pb->error)) {
		    av_log(NULL, AV_LOG_WARNING,
			    "Fail to get packet: %d!\n", ret);
		    break;	// XXX:
		}   continue;	// XXX:
	    }

	    ret = pkt->stream_index;
	    i = avfc->streams[ret]->codec->codec_type;
	    if (!mdec[i] || ret != psidx[i]) { freeElem(pkt);  continue; }
	    if (av_dup_packet(pkt) < 0) break;

	    pkt->priv = NULL;	// XXX:
	    mdec[i]->pktq.push(pkt);
	}   qn = ~0x00; //close();

#ifdef  ANDROID
	sendEvent(MEDIA_PLAYBACK_COMPLETE, 0, 0);
	//sendEvent(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, ERROR_NO_LICENSE);
dtrace;
#endif

	return NULL;
    }

    bool isplay() {
	return /*(state == MP_PLAYING || state == MP_PAUSED) && */qn < 25 * 8;
    }

    int playing(char* url) {
	pthread_t dmxr;	// XXX:
	int ret;

	if ((ret = open(url)) || (ret = preparing())) return ret;
	if (pthread_create(&dmxr, NULL, demux_thread, this) < 0) ;
	pthread_detach(dmxr);

	while (isplay()) usleep(40*1000);
	if (avfc) close();	// XXX:

	return 0;
    }

  public:
    FFDroidPlayer(): state(MP_IDLE), avfc(NULL) {
	//av_log_set_level(AV_LOG_INFO);
	av_log_set_flags(AV_LOG_SKIP_REPEATED);

	//avcodec_register_all();
	av_register_all(); 	// register all codecs, demux and protocols

#if CONFIG_AVDEVICE
	avdevice_register_all();
#endif

#if CONFIG_AVFILTER
	avfilter_register_all();
#endif

	for (int i = 0; i < AVMEDIA_TYPE_NB; ++i) {
	    mdec[i] = NULL;	psidx[i] = -1;
	}

	// XXX: psidx[] must be set by user
	psidx[AVMEDIA_TYPE_SUBTITLE] = //-1;
	psidx[AVMEDIA_TYPE_AUDIO] = //-1;
	psidx[AVMEDIA_TYPE_VIDEO] = 0;
    }

    virtual
    ~FFDroidPlayer() {
#if CONFIG_AVFILTER
	avfilter_uninit();
#endif

dtrace;
	if (avfc) close();
    }

    virtual void onFirstRef() { }	// AudioTrack.h

    virtual status_t initCheck() { return OK; }	// XXX:

    //virtual status_t setDataSource(const char *url) { return open(url); }

    virtual status_t setDataSource(int fd, int64_t offset, int64_t length) {
	return open(fd, offset, length);
    }

    virtual status_t prepare() { return preparing(); }

    virtual status_t prepareAsync() {
	status_t ret = prepare();
#ifdef  ANDROID
dtrace;
	if (ret == OK) sendEvent(MEDIA_PREPARED, 0, 0);
#endif
	return ret;	// XXX:
    }

    virtual status_t start() {
	pthread_t dmxr;	// XXX:
	if (pthread_create(&dmxr, NULL, demux_thread, this) < 0) ;
	pthread_detach(dmxr);
	return OK;
    }

    virtual status_t stop() {
dtrace;
#ifdef  ANDROID
	mAudioSink->stop();
#endif
	state = MP_STOP;
	//close();
	return OK;
    }

    virtual status_t pause() {
#ifdef  ANDROID
	mAudioSink->pause();
#endif
	state = MP_PAUSED;
	return OK;
    }

    virtual status_t reset() {
dtrace;
	//close();
	state = MP_IDLE;
	return OK;
    }

    virtual bool isPlaying() { return isplay(); }

    virtual status_t getVideoWidth(int *w) {
	AVCodecContext* avctx;
	if (!avfc || !avfc->streams) return UNKNOWN_ERROR;
	avctx = avfc->streams[psidx[AVMEDIA_TYPE_VIDEO]]->codec;
	if (!avctx || !w) return UNKNOWN_ERROR;
	*w = avctx->width;
	return OK;
    }

    virtual status_t getVideoHeight(int *h) {
	AVCodecContext* avctx;
	if (!avfc || !avfc->streams) return UNKNOWN_ERROR;
	avctx = avfc->streams[psidx[AVMEDIA_TYPE_VIDEO]]->codec;
	if (!avctx || !h) return UNKNOWN_ERROR;
	*h = avctx->height;
	return OK;
    }

    virtual status_t getCurrentPosition(int *msec) {
	if (!avfc || !msec) return UNKNOWN_ERROR;
	*msec = (clkr - avfc->start_time) / 1000;	// XXX:
	return OK;
    }

    virtual status_t getDuration(int *msec) {
	if (!avfc || !msec) return UNKNOWN_ERROR;
	*msec = avfc->duration / (AV_TIME_BASE / 1000);
	return OK;
    }

    virtual status_t seekTo(int msec) {
dtrace;
	if (!msec) spos = 1; else
	spos = msec * 1000;
	return OK;
    }

    virtual status_t setLooping(int l) {
dtrace;
	loop = l;	return OK;
    }

#ifdef  ANDROID
    virtual status_t setDataSource(const char *url,
	    const KeyedVector<String8, String8> *headers = NULL) {
dprints(url);
	// XXX: headers
	return open(url);
    }

    //virtual status_t setAudioStreamType(int type);
    virtual status_t setVideoSurface(const sp<ISurface>& surface) {
dtrace;
	isurf = surface;	return OK;
    }

    static size_t asink_callback(MediaPlayerBase::AudioSink *asink,
	    void* buf, size_t len, void* cookie) {
	FFDroidPlayer* ffdp = (FFDroidPlayer*)cookie;
	MediaDecoder<AudioBlock, AVAudioConvert>* adec;
	static AudioBlock blk_ = { NULL, 0, 0 };	// XXX:
	size_t outs = 0;

	adec = (MediaDecoder<AudioBlock, AVAudioConvert>*)
		ffdp->mdec[AVMEDIA_TYPE_AUDIO];

	while (outs < len) {
	    AudioBlock* blk = &blk_;
	    int num;

	    if (ffdp->state == MP_STOP/* || ffdp->state == MP_END*/) break;
	    if (!blk->size) {
		if (adec->frmq.pop(blk, true)) break; else {
		    if (blk->pts) ffdp->clkr = blk->pts; else	// XXX:
		    ffdp->clkr += outs * 1000 * asink->msecsPerFrame() /
			    asink->frameSize();
		}
	    }

	    outs += (num = FFMIN(blk->size, len - outs));
	    memcpy(buf, blk->data, num);
	    if (!(blk->size -= num)) freeElem(blk);
	}

	return outs;
    }

    virtual status_t invoke(const Parcel &request, Parcel *reply) {
dtrace;
	return INVALID_OPERATION;
    }

    virtual status_t getMetadata(const media::Metadata::Filter& ids,
	    Parcel *records) {
dtrace;
	return OK;
    }

    virtual status_t suspend() {
dtrace;
	return OK;
    }

    virtual status_t resume() {
dtrace;
	return OK;
    }

    virtual player_type playerType() { return FFDROID_PLAYER; }
#endif
};

static void* demux_thread(void* data) {
    FFDroidPlayer* fp = (FFDroidPlayer*)data;
    return fp->demuxing(data);
}

static void* outputV_thread(void* data) {
    FFDroidPlayer* fp = (FFDroidPlayer*)data;
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
    return fp->outputV(data);
}

static void* outputA_thread(void* data) {
    FFDroidPlayer* fp = (FFDroidPlayer*)data;
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    return fp->outputA(data);
}

};

#endif//FFDROIDPLAYER_H
// vim:sts=4:ts=8:

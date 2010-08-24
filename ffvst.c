//#!/usr/bin/tcc -run
/****************************************************************
 * $ID: ffvst.c        Îå, 20  8ÔÂ 2010 15:35:14 +0800  mhfan $ *
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

#define _XOPEN_SOURCE 600   // XXX: for usleep

#include <libavformat/avformat.h>

#include "ffvst.h"

#undef  fprintf	// XXX:

#ifdef  BUILD_LIBFFVST
#include <setjmp.h>

static jmp_buf long_jmpbuf;
static char ffmpeg_errbuf[256]; // XXX:
int ffmpeg_main(int argc, char* argv[]);

#define fprintf(a, ...) \
	snprintf(ffmpeg_errbuf, sizeof(ffmpeg_errbuf), __VA_ARGS__)
#define exit(a) if (a) longjmp(long_jmpbuf, a); else return(a)
#define abort(...) ffmpeg_exit(1)
#define main ffmpeg_main

#include "cmdutils.c"
#include "ffmpeg.c"

#undef  fprintf

static AVCodecContext avctx_dummy;

static void vst_log_cb(void* ptr, int level, const char* fmt, va_list vl)
{
    vsnprintf(ffmpeg_errbuf, sizeof(ffmpeg_errbuf), fmt, vl);
return;	// XXX:
    av_log_default_callback(ptr, level, fmt, vl);
}

int ConverterInit(int dpf)
{
    AVCodecContext *avctx = &avctx_dummy;

    ffmpeg_errbuf[0] = '\0';
    av_log_set_callback(vst_log_cb);

    memset(avctx, 0, sizeof(*avctx));
    avctx->request_channel_layout = -1;
    avctx->flags = INT_MIN;
    avctx->delay = dpf;

    return avcodec_thread_init(avctx, 1); // XXX:
}

int ConverterExit(void)
{
    AVCodecContext *avctx = &avctx_dummy;

    avcodec_thread_free(avctx);

    return 0;
}

static int ffmpeg_thread(AVCodecContext *avctx, void* arg)
{
    //extern int ffmpeg_main(int argc, char *argv[]);
    //extern const char program_name[];

    MediaFile* mf = arg;
    char* *argv;
    int i, argc;

    argc = 1 + mf->srcn * 5 + 32 + 1;
    argv = malloc(sizeof(char*) * argc);
    argv[argc = 0] = (char*)program_name;

    if (avctx->delay) {		char buf[8];
	if (sprintf(buf, "%d", avctx->delay) < 1) ;
	argv[++argc] = "-dpf";	argv[++argc] = buf;
    }

    for (i = 0; i < mf->srcn; ++i) {
	argv[++argc] = "-i";	argv[++argc] = (char*)mf->srcf[i];
    }

    argv[++argc] = "-vcodec";	argv[++argc] = "mpeg2video";
    argv[++argc] = "-acodec";	argv[++argc] = "mp2";
    argv[++argc] = "-sameq";	//argv[++argc] = "-shortest";
    argv[++argc] = "-aq";	argv[++argc] = "99";
    argv[++argc] = "-ab";	argv[++argc] = "128000";

  if (0) {
    argv[++argc] = "-b";	argv[++argc] = "6000000";
    argv[++argc] = "-maxrate";	argv[++argc] = "9000000";
    argv[++argc] = "-bufsize";	argv[++argc] = "1835008";
  }

  if (1) {
    char buf[] = "comment=\"Transformed by M.H.Fan\"";
    argv[++argc] = "-timestamp";argv[++argc] = "now";
    argv[++argc] = "-metadata"; argv[++argc] = buf;
  }

    argv[++argc] = mf->dstf;

    for (i = 2; i < mf->srcn; ++i) {	// XXX:
	argv[++argc] = "-acodec";	argv[++argc] = "mp2";
	argv[++argc] = "-newaudio";
    }

    argc = ffmpeg_main(++argc, argv);

    free(argv);	return argc;
}

int ConvertMedia2MPG(MediaFile* mf)
{
    AVCodecContext *avctx = &avctx_dummy;
    avctx->execute( avctx, ffmpeg_thread, mf,
	    &avctx->flags, 1, sizeof(void*));
    return 0;
}

int GetMediaProperty(MediaProperty mp, void* arg)
{
    AVCodecContext *avctx = &avctx_dummy;

    switch (mp) {
    case MP_PROGRESS:
RPT:	if (0 <  avctx->flags) return ECHILD; else
	if (0 == avctx->flags) *(unsigned*)arg = 100; else {
	    //extern AVFormatContext* input_files[ ];
	    AVFormatContext *is =   input_files[0].ctx;
	    if (!is) goto RPT;	// XXX:

		avctx->channel_layout = avio_tell(is->pb);
	    if (avctx->channel_layout < 1) return ECHILD;

	    if (avctx->request_channel_layout < 1)
		avctx->request_channel_layout = avio_size(is->pb);

	    *(unsigned*)arg = 100 * avctx->channel_layout /
		    avctx->request_channel_layout;
	}   break;

    case MP_ERRMSG:	*(char**)arg = ffmpeg_errbuf;	break;
    default: fprintf(stderr, "Unhandled MediaProperty: %x\n", mp);
    }

    return 0;
}
#endif

#if defined(BUILD_STANDALONE) || !defined(BUILD_LIBFFVST)
//#include "config.h"

#if 0//HAVE_W32THREADS
#define usleep(v) Sleep(v/1000)
//extern void Sleep(unsigned v);
#else// XXX:
#include <unistd.h>	// for usleep
#endif

int main(int argc, char* argv[])
{
    MediaFile mf;

    if (argc < 3) {
	fprintf(stderr,
		"Usage: %s <dst-file> <src-file> [...]\n", argv[0]);
	return EINVAL;
    }

    if (ConverterInit(0)) return 1;	// XXX:

    mf.dstf = argv[1],
    mf.srcn = argc - 2;
    mf.srcf = (const char**)&argv[2];

    ConvertMedia2MPG(&mf);

    while (!GetMediaProperty(MP_PROGRESS, &argc) && argc < 100) {
	fprintf(stdout, "\r%d%%", argc);  usleep(1 * 1000 * 1000);
    }

    if (argc == 100) fprintf(stdout, "100%% completed\n"); else
    if (argc <  100 && !GetMediaProperty(MP_ERRMSG, &argv[0]))
	fprintf(stderr, "%s", argv[0]);

    ConverterExit();

    return 0;
}

#if 0  // build instruction:
gcc -DBUILD_STANDALONE=1 -I../include -L../lib -Wall -pipe -O2 -o ffvst{,.c} \
	-l{ffvst,av{util,codec,format,filter,device,core},postproc,swscale}
#endif
#endif

// vim:sts=4:ts=8:

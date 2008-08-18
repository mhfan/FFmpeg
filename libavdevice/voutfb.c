/*
 * Video Out /dev/fb Video Hook
 * Copyright (c) 2007 Marc Hoffman
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
#include <stdio.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
//#include <bfin_sram.h>	// XXX: mhfan

#include "framehook.h"
#include "swscale.h"

static int sws_flags = SWS_BICUBIC;

typedef struct {
  int dummy;

  // This vhook first converts frame to RGB ...
  struct SwsContext *toRGB_convert_ctx;

  int devfb;
  int sz;
  unsigned char *screen_ptr;
  AVPicture *dpy;

} ContextInfo;

void Release(void *ctx)
{
    ContextInfo *ci;
    ci = (ContextInfo *) ctx;

    if (ctx) {
      munmap (ci->screen_ptr, ci->sz);
      close (ci->devfb);
      av_free (ci->dpy);
      if (ci->toRGB_convert_ctx)
	sws_freeContext(ci->toRGB_convert_ctx);
      av_free(ctx);
    }
}

AVPicture *oscreen (ContextInfo *ci)
{
  AVPicture *dpy;
  char * device = "/dev/fb0";
  struct fb_var_screeninfo screeninfo;
  int screen_fd;
  unsigned char * screen_ptr;
  int screen_width;
  int screen_height;
  int bits_per_pixel;
  int sz;
  screen_fd = open(device, O_RDWR);
  
  if (screen_fd == -1) {
    perror("Unable to open frame buffer device /dev/fb0");
    exit(0);
  }
  
  if (ioctl(screen_fd, FBIOGET_VSCREENINFO, &screeninfo)==-1) {
    perror("Unable to retrieve framebuffer information");
    exit(0);
  }
  screen_width = screeninfo.xres_virtual;
  screen_height = screeninfo.yres_virtual;
  bits_per_pixel = screeninfo.bits_per_pixel;
  
  //  printf ("w:%d h:%d bits:%d\n", screen_width, screen_height, bits_per_pixel);
  sz = screen_height * screen_width * (bits_per_pixel/ 8);
  screen_ptr = mmap(0, sz, 
		    PROT_READ|PROT_WRITE, MAP_FILE|MAP_PRIVATE, screen_fd, 0);
  
  if (screen_ptr==MAP_FAILED) {
    perror("Unable to mmap frame buffer\n");
  }

  //  memset (screen_ptr, 0, screen_width*screen_height*bits_per_pixel/2);
  dpy = av_mallocz (sizeof(AVPicture));
  dpy->data[0]     = screen_ptr;
  dpy->linesize[0] = screen_width*2;
  ci->sz = sz;
  ci->screen_ptr = screen_ptr;
  ci->devfb = screen_fd;
  ci->screen_ptr = screen_ptr;

  return dpy;
}

int Configure(void **ctxp, int argc, char *argv[])
{
    ContextInfo *ci;
    
    *ctxp = av_mallocz(sizeof(ContextInfo));
    ci = (ContextInfo *) *ctxp;
    ci->dpy = oscreen(ci);
    fprintf (stderr, "1: %08x => %08x %d\n", ci->dpy, ci->dpy->data[0], ci->dpy->linesize[0]);
    fprintf(stderr, "Called with argc=%d\n", argc);

    return 0;
}

void Process(void *ctx, AVPicture *picture, enum PixelFormat pix_fmt, int width, int height, int64_t pts)
{
    ContextInfo *ci = (ContextInfo *) ctx;
    AVPicture *dpy = ci->dpy;

    if (ci->toRGB_convert_ctx == 0)
      ci->toRGB_convert_ctx = sws_getCachedContext(ci->toRGB_convert_ctx,
						   width, height, pix_fmt,
						   width, height, PIX_FMT_RGB565,
						   sws_flags, NULL, NULL, NULL);
    

    if (ci->toRGB_convert_ctx == NULL) {
      av_log(NULL, AV_LOG_ERROR,
	     "Cannot initialize the toRGB conversion context\n");
      exit(1);
    }

    sws_scale(ci->toRGB_convert_ctx,
	      picture->data, picture->linesize, 0, height,
	      dpy->data, dpy->linesize);

}


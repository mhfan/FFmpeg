//#!/usr/bin/tcc -run
/****************************************************************
 * $ID: ffdroidplayer.cpp  08 10ÔÂ 2010 15:35:18 +0800  mhfan $ *
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

#include "ffdroidplayer.h"

//using namespace android;

#if 1//ndef ANDROID
int main(int argc, char* argv[])
{
    android::FFDroidPlayer ffdp;

    if (argc < 2) {
	av_log(NULL, AV_LOG_ERROR, "Usage: %s /path/to/media.file\n",
		argv[0]);	return 1;
    }

    ffdp.playing(argv[1]);

    return 0;
}
#endif

#if 0
g++ -o ffdroidplayer{,.cpp} -pthread -I.. -Wall \
	-l{av{util,codec,format,core,filter,device},postproc,swscale}
#endif

// vim:sts=4:ts=8:

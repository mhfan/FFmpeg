/****************************************************************
 * $ID: ffvst.h        Îå, 20  8ÔÂ 2010 14:39:17 +0800  mhfan $ *
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
#ifndef FFVST_H
#define FFVST_H

typedef enum MediaType {
    MT_UNKNOWN,
    MT_MXF, MT_GXF,
    MT_AVI,	// XXX:
    MT_LIMIT
}   MediaType;

typedef struct MediaFile {
    const char* dstf;	// .mpg suffix

    MediaType type;	// not needed currently

    unsigned srcn;	// total number of source media files
    const char* *srcf;
}   MediaFile;

typedef enum MediaProperty {
    MP_UNKNOWN,
    MP_PROGRESS,	// percent
    MP_ERRMSG,
    MP_LIMIT
}   MediaProperty;

int ConverterInit(int delay_per_frame);	// release CPU in ms

int ConvertMedia2MPG(MediaFile* mf);
// execution in a sub-thread

int GetMediaProperty(MediaProperty mp, void* arg);
// must be called after ConvertMedia2MPG returned
/*
 * int pos;	GetMediaProperty(MP_PROGRESS, &pos);
 *
 * char* str;	GetMediaProperty(MP_ERRMSG, &str);
 * (NOTE: str will point to a static memory block, never free it.)
 */

int ConverterExit(void);

#endif//FFVST_H
// vim:sts=4:ts=8:

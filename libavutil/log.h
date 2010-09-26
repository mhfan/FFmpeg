/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
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

#ifndef AVUTIL_LOG_H
#define AVUTIL_LOG_H

#include <stdarg.h>
#include "avutil.h"
#include "attributes.h"

/**
 * Describe the class of an AVClass context structure. That is an
 * arbitrary struct of which the first field is a pointer to an
 * AVClass struct (e.g. AVCodecContext, AVFormatContext etc.).
 */
typedef struct AVClass {
    /**
     * The name of the class; usually it is the same name as the
     * context structure type to which the AVClass is associated.
     */
    const char* class_name;

    /**
     * A pointer to a function which returns the name of a context
     * instance ctx associated with the class.
     */
    const char* (*item_name)(void* ctx);

    /**
     * a pointer to the first option specified in the class if any or NULL
     *
     * @see av_set_default_options()
     */
    const struct AVOption *option;

    /**
     * LIBAVUTIL_VERSION with which this structure was created.
     * This is used to allow fields to be added without requiring major
     * version bumps everywhere.
     */

    int version;

    /**
     * Offset in the structure where log_level_offset is stored.
     * 0 means there is no such variable
     */
    int log_level_offset_offset;

    /**
     * Offset in the structure where a pointer to the parent context for loging is stored.
     * for example a decoder that uses eval.c could pass its AVCodecContext to eval as such
     * parent context. And a av_log() implementation could then display the parent context
     * can be NULL of course
     */
    int parent_log_context_offset;

    /**
     * Return next AVOptions-enabled child or NULL
     */
    void* (*child_next)(void *obj, void *prev);

    /**
     * Return an AVClass corresponding to next potential
     * AVOptions-enabled child.
     *
     * The difference between child_next and this is that
     * child_next iterates over _already existing_ objects, while
     * child_class_next iterates over _all possible_ children.
     */
    const struct AVClass* (*child_class_next)(const struct AVClass *prev);
} AVClass;

/* av_log API */

#define AV_LOG_QUIET    -8

/**
 * Something went really wrong and we will crash now.
 */
#define AV_LOG_PANIC     0

/**
 * Something went wrong and recovery is not possible.
 * For example, no header was found for a format which depends
 * on headers or an illegal combination of parameters is used.
 */
#define AV_LOG_FATAL     8

/**
 * Something went wrong and cannot losslessly be recovered.
 * However, not all future data is affected.
 */
#define AV_LOG_ERROR    16

/**
 * Something somehow does not look correct. This may or may not
 * lead to problems. An example would be the use of '-vstrict -2'.
 */
#define AV_LOG_WARNING  24

#define AV_LOG_INFO     32
#define AV_LOG_VERBOSE  40

/**
 * Stuff which is only useful for libav* developers.
 */
#define AV_LOG_DEBUG    48

/**
 * Send the specified message to the log if the level is less than or equal
 * to the current av_log_level. By default, all logging messages are sent to
 * stderr. This behavior can be altered by setting a different av_vlog callback
 * function.
 *
 * @param avcl A pointer to an arbitrary struct of which the first field is a
 * pointer to an AVClass struct.
 * @param level The importance level of the message, lower values signifying
 * higher importance.
 * @param fmt The format string (printf-compatible) that specifies how
 * subsequent arguments are converted to output.
 * @see av_vlog
 */
void av_log(void *avcl, int level, const char *fmt, ...) av_printf_format(3, 4);

void av_vlog(void *avcl, int level, const char *fmt, va_list);
int av_log_get_level(void);
void av_log_set_level(int);
void av_log_set_callback(void (*)(void*, int, const char*, va_list));
void av_log_default_callback(void* ptr, int level, const char* fmt, va_list vl);
const char* av_default_item_name(void* ctx);

/**
 * av_dlog macros
 * Useful to print debug messages that shouldn't get compiled in normally.
 */

#ifdef DEBUG
#    define av_dlog(pctx, ...) av_log(pctx, AV_LOG_DEBUG, __VA_ARGS__)
#else
#    define av_dlog(pctx, ...) do { if (0) av_log(pctx, AV_LOG_DEBUG, __VA_ARGS__); } while (0)
#endif

/**
 * Skip repeated messages, this requires the user app to use av_log() instead of
 * (f)printf as the 2 would otherwise interfere and lead to
 * "Last message repeated x times" messages below (f)printf messages with some
 * bad luck.
 * Also to receive the last, "last repeated" line if any, the user app must
 * call av_log(NULL, AV_LOG_QUIET, "%s", ""); at the end
 */
#define AV_LOG_SKIP_REPEATED 1
void av_log_set_flags(int arg);

#ifndef dtrace
#undef	printf
#undef	fprintf

#if	defined(__KERNEL__)
#define errno 0 // XXX:
#define fprintf(_, ...) printk(__VA_ARGS__)
#elif	defined(ANDROID)
#include <android/log.h>
#define fprintf(_, ...) __android_log_print(ANDROID_LOG_DEBUG, \
	"FFmpeg", __VA_ARGS__)
#elif	defined(LIBBB_H)
#define fprintf(_, ...) fdprintf(STDERR_FILENO, __VA_ARGS__)
#elif	defined(AVUTIL_LOG_H)
#define fprintf(_, ...) av_log(NULL, AV_LOG_ERROR, __VA_ARGS__)
#elif	0 && defined(MPLAYER_MP_MSG_H)
#define fprintf(_, ...) mp_msg(MSGT_GLOBAL, MSGL_ERR, __VA_ARGS__)
#elif	defined(_XF86_H)
#define fprintf(_, ...) xf86DrvMsg(0, X_ERROR, __VA_ARGS__)
#elif	0 && defined(VLC_MSG_DBG)
#define fprintf(_, ...) msg_Dbg(NULL, __VA_ARGS__)
#endif// XXX:

#define dtrace	do { fprintf(stderr, \
	    "\033[36mTRACE\033[1;34m==>\033[33m%16s" \
	    "\033[36m: \033[32m%4d\033[36m: \033[35m%-24s \033[34m" \
	    "[\033[0;37m%s\033[1;34m, \033[0;36m%s\033[1;34m]\033[0m\n", \
	    __FILE__, __LINE__, __func__, __TIME__, __DATE__); \
	    if (errno < 0) fprintf(stderr, "Errmsg: %s (%d)\n", \
		    strerror(errno), errno); \
	} while (0)

#define dprintp(a, n) do { unsigned short i_, m_ = sizeof((a)[0]); \
	fprintf(stderr, "\033[33m" #a ": \033[36m" \
		"%p\033[0m ==> %x\n", a, (n)); \
	m_ = (m_ < 2 ? 24 : (m_ < 4 ? 16 : 8)); \
	for (i_ = 0; i_ < (n); ) { \
	    unsigned short j_ = ((n) < i_ + m_ ? (n) - i_ : m_); \
	    for ( ; j_--; ++i_) \
		if (16 < m_) fprintf(stderr, "%02x ", (a)[i_]); else \
		if ( 8 < m_) fprintf(stderr, "%04x ", (a)[i_]); else \
			     fprintf(stderr, "%08x ", (a)[i_]); \
	    fprintf(stderr, "\n"); } \
	} while (0)

#define dprintn(a) do { fprintf(stderr, "\033[33m" #a \
		": \033[36m%#x, %d, %g\033[0m\n", a, a, (double)a); \
	} while (0)	// XXX:

#define dprints(a) do { fprintf(stderr, "\033[33m" #a \
		": \033[36m%s\033[0m\n", a); } while (0)

//#undef	fprintf
#endif  /* defined by mhfan */

#endif /* AVUTIL_LOG_H */

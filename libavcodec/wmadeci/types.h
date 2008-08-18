
#ifdef ROCKBOX
#include <codecs/lib/codeclib.h>
#else

#include "config.h"

#if __STDC_VERSION__ >= 199901L
#define asm __asm__
#endif

#if ARCH_ARM || defined(__arm__)
#define CPU_ARM 1
#elif ARCH_M68K
#define CPU_COLDFIRE 1
#endif

#if HAVE_BIGENDIAN
#define ROCKBOX_BIG_ENDIAN 1
#else
#define ROCKBOX_LITTLE_ENDIAN 1
#endif

#ifndef ICODE_ATTR
#define ICODE_ATTR
#endif

#ifndef ICONST_ATTR
#define ICONST_ATTR
#endif

#ifndef IBSS_ATTR
#define IBSS_ATTR
#endif

#ifndef IBSS_ATTR_WMA_LARGE_IRAM
#define IBSS_ATTR_WMA_LARGE_IRAM
#endif

#endif	/* comment by mhfan */

#define fixed32         int32_t
#define fixed64         int64_t


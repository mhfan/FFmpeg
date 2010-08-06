
#ifdef ROCKBOX
#include <codecs/lib/codeclib.h>
#else

#include "config.h"

#if __STDC_VERSION__ >= 199901L
#define asm __asm__
#endif

#if ARCH_ARM //|| defined(__arm__)
#define CPU_ARM 1

#if HAVE_ARMV6
#define ARM_ARCH 6
#else// XXX:
#define ARM_ARCH 5
#endif

#define MEM_ALIGN_ATTR __attribute__((aligned(32)))

#elif ARCH_M68K
#define CPU_COLDFIRE 1

#define MEM_ALIGN_ATTR __attribute__((aligned(16)))

#else
#define MEM_ALIGN_ATTR __attribute__((aligned(32)))
#endif

#if HAVE_BIGENDIAN
#define ROCKBOX_BIG_ENDIAN 1
#else
#define ROCKBOX_LITTLE_ENDIAN 1
#endif

#define LIKELY(x) (x)

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


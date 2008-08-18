/****************************************************************
 * $Id: bfin_cache.h 1611 2007-10-17 02:24:15Z svn $
 *                                                              *
 * Description:                                                 *
 *                                                              *
 * Maintainer:  ∑∂√¿ª‘(Meihui Fan)  <mhfan@hhcn.com>            *
 *                                                              *
 * Copyright (C)  2007~2008  HHTech                             *
 *   www.hhcn.com, www.hhcn.org                                 *
 *   All rights reserved.                                       *
 *                                                              *
 * This file is free software;                                  *
 *   you are free to modify and/or redistribute it   	        *
 *   under the terms of the GNU General Public Licence (GPL).   *
 ****************************************************************/
#ifndef BFIN_CACHE_H
#define BFIN_CACHE_H

#ifdef	__BFIN__
#if 0
#include <sys/syscall.h>
//#include <unistd.h>

#define posix_fadvise(fd, off, len, adv) \
        syscall(SYS_fadvise64_64, fd, off, 0, len, 0, adv)

#define	fstatfs(fd, sfs) syscall(SYS_fstatfs64, fd, sizeof(*sfs), sfs)
#endif

static inline void cache_invalid(void)
{
#define	pDMEM_CONTROL	((volatile unsigned long*)0xFFE00004)
    unsigned save = *pDMEM_CONTROL;	asm volatile ("csync; ssync;");
    *pDMEM_CONTROL = 0;			asm volatile ("csync; ssync;");
    *pDMEM_CONTROL = save;		asm volatile ("csync; ssync;");
}

#define	CACHE_LINE_SIZE		(0x01 << 5)
static inline void cache_clean(unsigned long pb, unsigned long pe)
{
#if 1
	//asm volatile ("csync;");
    for (pb &= -CACHE_LINE_SIZE; pb < pe; )
	asm volatile ("flushinv [%0++];" : "+p" (pb));
#else// XXX:
	asm volatile ("csync;");	pb &= -CACHE_LINE_SIZE;
	asm volatile ("flushinv [%0  ];" : "+p" (pb));
    do  asm volatile ("flushinv [%0++];" : "+p" (pb)); while (pb < pe);
	asm volatile ("flushinv [%0  ];" : "+p" (pb));
#endif
	asm volatile ("ssync;");
}

static inline void cache_flush(unsigned long pb, unsigned long pe)
{
#if 1
	//asm volatile ("csync;");
    for (pb &= -CACHE_LINE_SIZE; pb < pe; )
	asm volatile ("flush    [%0++];" : "+p" (pb));
#else// XXX:
	asm volatile ("csync;");	pb &= -CACHE_LINE_SIZE;
	asm volatile ("flush    [%0  ];" : "+p" (pb));
    do  asm volatile ("flush    [%0++];" : "+p" (pb)); while (pb < pe);
	asm volatile ("flush    [%0  ];" : "+p" (pb));
#endif
	//asm volatile ("ssync;");
}
#else
#define	cache_flush(...)
#define	cache_clean(...)
#define	cache_invalid(...)
#endif//__BFIN__

#define	cache_clean_var(var) \
	cache_clean((unsigned long)&var, (unsigned long)&var + sizeof(var))
#define	cache_clean_ptr(ptr, len) \
	cache_clean((unsigned long) ptr, (unsigned long) ptr + len)
#define	cache_flush_var(var) \
	cache_flush((unsigned long)&var, (unsigned long)&var + sizeof(var))
#define	cache_flush_ptr(ptr, len) \
	cache_flush((unsigned long) ptr, (unsigned long) ptr + len)

#endif//BFIN_CACHE_H
// vim:sts=4:ts=8: 

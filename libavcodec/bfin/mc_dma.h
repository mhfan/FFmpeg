#ifndef MC_DMA_H
#define MC_DMA_H

#include "../dsputil.h"
#include "../mpegvideo.h"

#define BFIN_MDMA
//#define DMA_DEBUG
//#define _PROFILING_

/* external API */
extern void dma_para_init(MpegEncContext *s);
extern void MPV_dma_update_buffer(int16_t (**block)[64]);
extern void MPV_dma_mc_addidct(void);
extern void MPV_dma_decode_mbinter(MpegEncContext *s, int32_t lowres_flag);

#ifdef _PROFILING_
extern void init_timer(void);
extern void start_timer(void);
extern void start_global_timer(void);
extern void start_read_timer(void);
extern void start_mc_timer(void);
extern void start_idct_timer(void);
extern void start_write_timer(void);
extern void start_cache_timer(void);
extern void start_wait_timer(void);
extern void start_debug_timer(void);
extern void stop_init_timer(void);
extern void stop_comp_timer(void);
extern void stop_vld_timer(void);
extern void stop_read_timer(void);
extern void stop_mc_timer(void);
extern void stop_idct_timer(void);
extern void stop_write_timer(void);
extern void stop_cache_timer(void);
extern void stop_wait_timer(void);
extern void stop_debug_timer(void);
extern void stop_global_timer(void);
extern void write_timer(void);
#else
static inline void
init_timer(void)
{
}
static inline void
start_timer(void)
{
}
static inline void
start_global_timer(void)
{
}
static inline void
start_read_timer(void)
{
}
static inline void
start_mc_timer(void)
{
}
static inline void
start_idct_timer(void)
{
}
static inline void
start_write_timer(void)
{
}
static inline void
start_cache_timer(void)
{
}
static inline void
start_wait_timer(void)
{
}
static inline void
start_debug_timer(void)
{
}
static inline void
stop_init_timer(void)
{
}
static inline void
stop_comp_timer(void)
{
}
static inline void
stop_vld_timer(void)
{
}
static inline void
stop_read_timer(void)
{
}
static inline void
stop_mc_timer(void)
{
}
static inline void
stop_idct_timer(void)
{
}
static inline void
stop_write_timer(void)
{
}
static inline void
stop_cache_timer(void)
{
}
static inline void
stop_wait_timer(void)
{
}
static inline void
stop_debug_timer(void)
{
}
static inline void
stop_global_timer(void)
{
}
static inline void
write_timer(void)
{
}
#endif//_PROFILING_
 
#endif//MC_DMA_H

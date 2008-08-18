#!/bin/sh

if false; then

apps/fixedpoint.c
apps/fixedpoint.h

apps/codecs/lib/asm_arm.h
apps/codecs/lib/asm_mcf5249.h

apps/codecs/lib/codeclib_misc.h

apps/codecs/lib/fft-ffmpeg_arm.h
apps/codecs/lib/fft-ffmpeg_cf.h

apps/codecs/lib/fft-ffmpeg.c
apps/codecs/lib/fft.h

apps/codecs/lib/mdct_arm.S
apps/codecs/lib/mdct2.c
apps/codecs/lib/mdct2.h

apps/codecs/lib/mdct.c
apps/codecs/lib/mdct.h
apps/codecs/lib/mdct_lookup.c
apps/codecs/lib/mdct_lookup.h

apps/codecs/libwma/types.h

apps/codecs/libwma/wmadata.h
apps/codecs/libwma/wmadec.h
apps/codecs/libwma/wmadeci.c
apps/codecs/libwma/wmafixed.c
apps/codecs/libwma/wmafixed.h

fi

for f in `\grep '^apps' $0`; do \cp ~/devel/rockbox/$f .; done

# vim:sts=4:ts=8:

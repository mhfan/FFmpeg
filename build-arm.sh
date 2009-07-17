#!/bin/bash
 ################################################################
 # $ID: build.sh       Thu, 30 Oct 2008 17:14:22 +0800  mhfan $ #
 #                                                              #
 # Description:                                                 #
 #                                                              #
 # Maintainer:  ∑∂√¿ª‘(MeiHui FAN)  <mhfan@ustc.edu>            #
 #                                                              #
 # CopyLeft (c)  2008~2009  M.H.Fan                             #
 #   All rights reserved.                                       #
 #                                                              #
 # This file is free software;                                  #
 #   you are free to modify and/or redistribute it  	        #
 #   under the terms of the GNU General Public Licence (GPL).   #
 ################################################################

ARCH=arm; PREFIX=/usr; CROSS_COMPILE=;
source ../scripts/hhtech_config.sh || . ../../scripts/hhtech_config.sh

#CPPFLAGS="$CPPFLAGS -DCONFIG_NO_LFS=1"
#CPPFLAGS="$CPPFLAGS -DFREETYPE_TTONLY=1"

$(dirname $0)/configure \
  --prefix=${PREFIX} \
  \
  --enable-shared \
  --enable-gpl \
  --enable-version3 \
  --enable-nonfree \
  \
  --disable-doc \
  --disable-ffmpeg \
  --enable-ffplay \
  --disable-ffserver \
    --enable-postproc \
    --enable-avfilter \
  --enable-avfilter-lavf \
  \
  --disable-pthreads \
  \
  --disable-x11grab \
  --enable-network \
  --disable-ipv6 \
  --enable-mpegaudio-hp \
  --disable-gray \
  --disable-swscale-alpha \
  \
  --enable-small \
  \
  --enable-hardcoded-tables \
  --disable-memalign-hack \
  \
  --disable-decoders \
  --enable-decoder="flv,flashsv,vp?,vp6?,vc1,rv?0,png,mjpeg" \
  --enable-decoder="h26?,h263?,mpeg4,mpeg*video,msmpeg4v?,wmv?" \
  --enable-decoder="ape,flac,wmav?,cook,libopencore_amr?b,aac,ac3,dca" \
  --enable-decoder="adpcm_swf,adpcm_ms,adpcm_ima_*v,adpcm_g726,adpcm_ea" \
  --enable-decoder="pcm_*aw,pcm_?16?e,pcm_?8,wmapro,mp2,mp3*,vorbis" \
  --disable-encoders \
  --enable-encoder="png" \
  --disable-parsers \
  --enable-parser="h26?,mpeg*video,vc1,vp3,mjpeg,mpegaudio,aac,ac3,dca" \
  --disable-demuxers \
  --enable-demuxer="mpeg?s,mpeg*video,vc1?,rawvideo,mjpeg,image2" \
  --enable-demuxer="avi,asf,ogg,mov,matroska,flv,swf,h26?,rm,nsv" \
  --enable-demuxer="ape,mp3,wav,flac,aac,ac3,dts,pcm_*aw,pcm_?16?e,pcm_?8" \
  --disable-muxers \
  \
  --disable-protocols \
  --enable-protocol="file" \
  --disable-bsfs \
  --enable-bsf="h264_mp4toannexb,dump_extradata" \
  --enable-bsf="mp3_header_decompress,aac_adtstoasc" \
  --disable-hwaccels \
  \
  --disable-filters \
  --enable-filter="crop" \
  --disable-devices \
  --enable-indev="alsa,oss,v4l2" \
  --enable-outdev="alsa,oss,v4l2" \
  \
  --disable-bzlib \
  --enable-libopencore-amrnb \
  --enable-libopencore-amrwb \
  \
    --enable-zlib \
  \
  --cross-prefix=$CROSS_COMPILE \
  --enable-cross-compile \
  \
  --arch=$ARCH \
  --cpu=$CPU \
  \
  --disable-debug \
  \
  --enable-extra-warnings \
  \
  $* \
  \
&& sed -i -e 's/\(.*CONFIG_SMALL\).*/\1 1/' config.h \
&& touch configure-stamp \

#&& sed -i -e 's/-D\(_LARGEFILE_SOURCE|_FILE_OFFSET_BITS=64\)//g' config.mak \

#    --enable-libfaad \
#    --enable-libfaadbin \
#    --disable-decoder="aac" \
#    --enable-decoder="libfaad" \
#  \
#  --enable-decoder="*_mfc" \
#  --extra-ldflags="-Wl,-rpath-link,/opt/mm-s3c/lib" \
#  \
#  --enable-decoder="*_cdk" \
#  --extra-ldflags="-Wl,-rpath-link,/opt/vpu-tcc/lib" \

################################################################################
#Usage: configure [options]
#Options: [defaults in brackets after descriptions]
#
#Standard options:
#  --help                   print this message
#  --logfile=FILE           log tests and output to FILE [config.err]
#  --disable-logging        do not log configure debug information
#  --prefix=PREFIX          install in PREFIX []
#  --bindir=DIR             install binaries in DIR [PREFIX/bin]
#  --datadir=DIR            install data files in DIR [PREFIX/share/ffmpeg]
#  --libdir=DIR             install libs in DIR [PREFIX/lib]
#  --shlibdir=DIR           install shared libs in DIR [PREFIX/lib]
#  --incdir=DIR             install includes in DIR [PREFIX/include]
#  --mandir=DIR             install man page in DIR [PREFIX/share/man]
#
#Configuration options:
#  --disable-static         do not build static libraries [no]
#  --enable-shared          build shared libraries [no]
#  --enable-gpl             allow use of GPL code, the resulting libs
#                           and binaries will be under GPL [no]
#  --enable-version3        upgrade (L)GPL to version 3 [no]
#  --enable-nonfree         allow use of nonfree code, the resulting libs
#                           and binaries will be unredistributable [no]
#  --disable-doc            do not build documentation
#  --disable-ffmpeg         disable ffmpeg build
#  --disable-ffplay         disable ffplay build
#  --disable-ffserver       disable ffserver build
#  --disable-avdevice       disable libavdevice build
#  --disable-avcodec        disable libavcodec build
#  --disable-avformat       disable libavformat build
#  --disable-swscale        disable libswscale build
#  --enable-postproc        enable GPLed postprocessing support [no]
#  --enable-avfilter        video filter support [no]
#  --enable-avfilter-lavf   video filters dependent on avformat [no]
#  --enable-beosthreads     use BeOS threads [no]
#  --enable-os2threads      use OS/2 threads [no]
#  --enable-pthreads        use pthreads [no]
#  --enable-w32threads      use Win32 threads [no]
#  --enable-x11grab         enable X11 grabbing [no]
#  --disable-network        disable network support [no]
#  --disable-mpegaudio-hp   faster (but less accurate) MPEG audio decoding [no]
#  --enable-gray            enable full grayscale support (slower color)
#  --disable-swscale-alpha  disable alpha channel support in swscale
#  --disable-fastdiv        disable table-based division
#  --enable-small           optimize for size instead of speed
#  --disable-aandct         disable AAN DCT code
#  --disable-dct            disable DCT code
#  --disable-fft            disable FFT code
#  --disable-golomb         disable Golomb code
#  --disable-lpc            disable LPC code
#  --disable-mdct           disable MDCT code
#  --disable-rdft           disable RDFT code
#  --disable-vaapi          disable VAAPI code
#  --disable-vdpau          disable VDPAU code
#  --disable-dxva2          disable DXVA2 code
#  --enable-runtime-cpudetect detect cpu capabilities at runtime (bigger binary)
#  --enable-hardcoded-tables use hardcoded tables instead of runtime generation
#  --enable-memalign-hack   emulate memalign, interferes with memory debuggers
#  --enable-beos-netserver  enable BeOS netserver
#  --disable-encoder=NAME   disable encoder NAME
#  --enable-encoder=NAME    enable encoder NAME
#  --disable-encoders       disable all encoders
#  --disable-decoder=NAME   disable decoder NAME
#  --enable-decoder=NAME    enable decoder NAME
#  --disable-decoders       disable all decoders
#  --disable-hwaccel=NAME   disable hwaccel NAME
#  --enable-hwaccel=NAME    enable hwaccel NAME
#  --disable-hwaccels       disable all hwaccels
#  --disable-muxer=NAME     disable muxer NAME
#  --enable-muxer=NAME      enable muxer NAME
#  --disable-muxers         disable all muxers
#  --disable-demuxer=NAME   disable demuxer NAME
#  --enable-demuxer=NAME    enable demuxer NAME
#  --disable-demuxers       disable all demuxers
#  --enable-parser=NAME     enable parser NAME
#  --disable-parser=NAME    disable parser NAME
#  --disable-parsers        disable all parsers
#  --enable-bsf=NAME        enable bitstream filter NAME
#  --disable-bsf=NAME       disable bitstream filter NAME
#  --disable-bsfs           disable all bitstream filters
#  --enable-protocol=NAME   enable protocol NAME
#  --disable-protocol=NAME  disable protocol NAME
#  --disable-protocols      disable all protocols
#  --disable-indev=NAME     disable input device NAME
#  --disable-outdev=NAME    disable output device NAME
#  --disable-indevs         disable input devices
#  --disable-outdevs        disable output devices
#  --disable-devices        disable all devices
#  --enable-filter=NAME     enable filter NAME
#  --disable-filter=NAME    disable filter NAME
#  --disable-filters        disable all filters
#  --list-decoders          show all available decoders
#  --list-encoders          show all available encoders
#  --list-hwaccels          show all available hardware accelerators
#  --list-muxers            show all available muxers
#  --list-demuxers          show all available demuxers
#  --list-parsers           show all available parsers
#  --list-protocols         show all available protocols
#  --list-bsfs              show all available bitstream filters
#  --list-indevs            show all available input devices
#  --list-outdevs           show all available output devices
#  --list-filters           show all available filters
#
#External library support:
#  --enable-avisynth        enable reading of AVISynth script files [no]
#  --enable-bzlib           enable bzlib [autodetect]
#  --enable-libopencore-amrnb enable AMR-NB de/encoding via libopencore-amrnb [no]
#  --enable-libopencore-amrwb enable AMR-WB decoding via libopencore-amrwb [no]
#  --enable-libdc1394       enable IIDC-1394 grabbing using libdc1394
#                           and libraw1394 [no]
#  --enable-libdirac        enable Dirac support via libdirac [no]
#  --enable-libfaac         enable FAAC support via libfaac [no]
#  --enable-libfaad         enable FAAD support via libfaad [no]
#  --enable-libfaadbin      open libfaad.so.0 at runtime [no]
#  --enable-libgsm          enable GSM support via libgsm [no]
#  --enable-libmp3lame      enable MP3 encoding via libmp3lame [no]
#  --enable-libnut          enable NUT (de)muxing via libnut,
#                           native (de)muxer exists [no]
#  --enable-libopenjpeg     enable JPEG 2000 decoding via OpenJPEG [no]
#  --enable-libschroedinger enable Dirac support via libschroedinger [no]
#  --enable-libspeex        enable Speex decoding via libspeex [no]
#  --enable-libtheora       enable Theora encoding via libtheora [no]
#  --enable-libvorbis       enable Vorbis encoding via libvorbis,
#                           native implementation exists [no]
#  --enable-libx264         enable H.264 encoding via x264 [no]
#  --enable-libxvid         enable Xvid encoding via xvidcore,
#                           native MPEG-4/Xvid encoder exists [no]
#  --enable-mlib            enable Sun medialib [no]
#  --enable-zlib            enable zlib [autodetect]
#
#Advanced options (experts only):
#  --source-path=PATH       path to source code [/home/mhfan/devel/ffmpeg]
#  --cross-prefix=PREFIX    use PREFIX for compilation tools []
#  --enable-cross-compile   assume a cross-compiler is used
#  --sysroot=PATH           root of cross-build tree
#  --sysinclude=PATH        location of cross-build system headers
#  --target-os=OS           compiler targets OS [linux]
#  --target-exec=CMD        command to run executables on target
#  --target-path=DIR        path to view of build directory on target
#  --nm=NM                  use nm tool
#  --as=AS                  use assembler AS []
#  --cc=CC                  use C compiler CC [gcc]
#  --ld=LD                  use linker LD
#  --host-cc=HOSTCC         use host C compiler HOSTCC
#  --host-cflags=HCFLAGS    use HCFLAGS when compiling for host
#  --host-ldflags=HLDFLAGS  use HLDFLAGS when linking for host
#  --host-libs=HLIBS        use libs HLIBS when linking for host
#  --extra-cflags=ECFLAGS   add ECFLAGS to CFLAGS []
#  --extra-ldflags=ELDFLAGS add ELDFLAGS to LDFLAGS []
#  --extra-libs=ELIBS       add ELIBS []
#  --extra-version=STRING   version string suffix []
#  --build-suffix=SUFFIX    library name suffix []
#  --arch=ARCH              select architecture  [i686]
#  --cpu=CPU                select the minimum required CPU (affects
#                           instruction selection, may crash on older CPUs)
#  --enable-powerpc-perf    enable performance report on PPC
#                           (requires enabling PMC)
#  --disable-asm            disable all assembler optimizations
#  --disable-altivec        disable AltiVec optimizations
#  --disable-amd3dnow       disable 3DNow! optimizations
#  --disable-amd3dnowext    disable 3DNow! extended optimizations
#  --disable-mmx            disable MMX optimizations
#  --disable-mmx2           disable MMX2 optimizations
#  --disable-sse            disable SSE optimizations
#  --disable-ssse3          disable SSSE3 optimizations
#  --disable-armv5te        disable armv5te optimizations
#  --disable-armv6          disable armv6 optimizations
#  --disable-armv6t2        disable armv6t2 optimizations
#  --disable-armvfp         disable ARM VFP optimizations
#  --disable-iwmmxt         disable iwmmxt optimizations
#  --disable-mmi            disable MMI optimizations
#  --disable-neon           disable neon optimizations
#  --disable-vis            disable VIS optimizations
#  --disable-yasm           disable use of yasm assembler
#  --enable-pic             build position-independent code
#  --malloc-prefix=PFX      prefix malloc and related names with PFX
#  --enable-sram            allow use of on-chip SRAM
#
#Developer options (useful when working on FFmpeg itself):
#  --disable-debug          disable debugging symbols
#  --enable-debug=LEVEL     set the debug level []
#  --enable-gprof           enable profiling with gprof []
#  --disable-optimizations  disable compiler optimizations
#  --enable-extra-warnings  enable more compiler warnings
#  --disable-stripping      disable stripping of executables and shared libraries
#
#NOTE: Object files are built at the place where configure is launched.
#
#################### End Of File: build.sh ####################
# vim:sts=4:ts=8:

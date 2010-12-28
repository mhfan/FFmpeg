#!/bin/make -f
 ################################################################
 # $ID: Makefile       ¶þ, 28 12ÔÂ 2010 13:40:18 +0800  mhfan $ #
 #                                                              #
 # Description:                                                 #
 #                                                              #
 # Maintainer:  ·¶ÃÀ»Ô(MeiHui FAN)  <mhfan@ustc.edu>            #
 #                                                              #
 # CopyLeft (c)  2010  M.H.Fan                                  #
 #   All rights reserved.                                       #
 #                                                              #
 # This file is free software;                                  #
 #   you are free to modify and/or redistribute it  	        #
 #   under the terms of the GNU General Public Licence (GPL).   #
 ################################################################

ifneq (,$(findstring yes, \
	$(CONFIG_MPEG4_SSBMFC_DECODER) \
	$(CONFIG_MPEG2_SSBMFC_DECODER) \
	$(CONFIG_MPEG1_SSBMFC_DECODER) \
	$(CONFIG_H264_SSBMFC_DECODER)  \
	$(CONFIG_H263_SSBMFC_DECODER)  \
	$(CONFIG_VC1_SSBMFC_DECODER)   \
	))
OBJS-yes += mfc/ssbsip_mfc.o mfc/SsbSipMfcDecAPI.o
endif

ifneq (,$(findstring yes, \
	$(CONFIG_H263_SSBMFC_ENCODER)  \
	$(CONFIG_H264_SSBMFC_ENCODER)  \
	$(CONFIG_MPEG4_SSBMFC_ENCODER) \
	))
OBJS-yes += mfc/ssbsip_mfc.o mfc/SsbSipMfcEncAPI.o
endif

ifneq (,$(findstring yes, \
	$(CONFIG_MPEG4_MFC_DECODER) \
	$(CONFIG_MPEG2_MFC_DECODER) \
	$(CONFIG_MPEG1_MFC_DECODER) \
	$(CONFIG_H264_MFC_DECODER)  \
	$(CONFIG_H263_MFC_DECODER)  \
	$(CONFIG_VC1_MFC_DECODER)   \
	))
OBJS-yes += mfc/libmfc.o

$(SUBDIR)mfc/libmfc.o: CFLAGS += -DARCH_S3C64XX=1 -I/opt/mm-s3c/include

EXTRALIBS += -L/opt/mm-s3c/lib -lmfc
endif

ifneq (,$(findstring yes, \
	$(CONFIG_MPEG4_CDK_DECODER) \
	$(CONFIG_MPEG2_CDK_DECODER) \
	$(CONFIG_MPEG1_CDK_DECODER) \
	$(CONFIG_H264_CDK_DECODER)  \
	$(CONFIG_H263_CDK_DECODER)  \
	$(CONFIG_RV40_CDK_DECODER)  \
	$(CONFIG_RV30_CDK_DECODER)  \
	$(CONFIG_RV20_CDK_DECODER)  \
	$(CONFIG_RV10_CDK_DECODER)  \
	$(CONFIG_WMV3_CDK_DECODER)  \
	$(CONFIG_WMV2_CDK_DECODER)  \
	$(CONFIG_WMV1_CDK_DECODER)  \
	$(CONFIG_VC1_CDK_DECODER)   \
	$(CONFIG_FLV_CDK_DECODER)   \
	))
OBJS-yes += mfc/tcc_cdkvdec.o

$(SUBDIR)mfc/tcc_cdkvdec.o: \
	CFLAGS += -DTCC_89XX_INCLUDE=1 -I/opt/vpu-tcc/include

EXTRALIBS += -L/opt/vpu-tcc/lib -ltcccdk
endif

# vim:sts=4:ts=8:

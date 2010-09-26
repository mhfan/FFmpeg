#!/bin/make -f
 ################################################################
 # $ID: Android.mk     Wed, 01 Sep 2010 14:19:27 +0800  mhfan $ #
 #                                                              #
 # Description:                                                 #
 #                                                              #
 # Maintainer:  ∑∂√¿ª‘(MeiHui FAN)  <mhfan@ustc.edu>            #
 #                                                              #
 # CopyLeft (c)  2010  M.H.Fan                                  #
 #   All rights reserved.                                       #
 #                                                              #
 # This file is free software;                                  #
 #   you are free to modify and/or redistribute it  	        #
 #   under the terms of the GNU General Public Licence (GPL).   #
 ################################################################

ifeq ($(BUILD_WITH_FFDROID),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

SRC_PATH_BARE := $(LOCAL_PATH)

ANDROID_CFLAGS  := $(TARGET_GLOBAL_CFLAGS) \
	-isysroot $(CURDIR) $(addprefix -I =/,"" $(TARGET_C_INCLUDES))

ANDROID_LDFLAGS := $(TARGET_GLOBAL_LDFLAGS) \
	$(addsuffix $(CURDIR)/$(TARGET_OUT_INTERMEDIATE_LIBRARIES), \
		-Wl,-rpath-link= -L) -nostdlib -lc \
	$(CURDIR)/$(TARGET_CRTBEGIN_DYNAMIC_O) \
	#-Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections \
	#$(TARGET_FDO_LIB) $(TARGET_LIBGCC) $(TARGET_CRTEND_O) \
# XXX: build/core/combo/TARGET_linux-arm.mk

$(warning $(shell cd $(LOCAL_PATH) && test ! -r config.h && \
	EXTRA_CFLAGS="$(ANDROID_CFLAGS)" EXTRA_LDFLAGS="$(ANDROID_LDFLAGS)" \
	ARCH=$(TARGET_ARCH) ./build-droid.sh --cc=$(CURDIR)/$(TARGET_CC)))

define BUILD_FFLIB

OBJS :=
OBJS-yes :=
LOCAL_MODULE := $(1)
SUBDIR := $(LOCAL_PATH)/$$(LOCAL_MODULE)/

ifeq ($$(LOCAL_MODULE),libavcodec)
#LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
endif

include $$(SUBDIR)Makefile

OBJS := $$(subst $(LOCAL_PATH)/,,$$(OBJS))    # XXX: sort?
ALL_ASM_FILES := $$(shell find $$(SUBDIR) -name \*.S)

ifneq ($$(ALL_ASM_FILES),)
ALL_ASM_OBJS := $$(patsubst $(LOCAL_PATH)/%.S,%.o,$$(ALL_ASM_FILES))
OBJS_S := $$(filter $$(ALL_ASM_OBJS),$$(OBJS))
OBJS_C := $$(filter-out $$(OBJS_S),$$(OBJS))
else
OBJS_C := $$(OBJS)
OBJS_S :=
endif

LOCAL_SRC_FILES := $$(patsubst %.o,%.c,$$(OBJS_C)) \
		   $$(patsubst %.o,%.S,$$(OBJS_S))
#$$(warning $$(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES := $$(SUBDIR) external/zlib
LOCAL_CFLAGS += $$(CPPFLAGS) $$(CFLAGS) -DHAVE_AV_CONFIG_H=1 \
	-Wno-cast-qual -Wno-inline -Wno-format -Wno-bad-function-cast \
	-Wno-unused -Wno-unused-function -Wno-uninitialized -Wno-extra \
	-D__ISO_C_VISIBLE=1999 -D__XPG_VISIBLE=601 #-D__POSIX_VISIBLE=200112 \
	#-DFF_API_MAX_STREAMS=0	# XXX:

#LOCAL_LDLIBS := -llog #-L$(SYSROOT)/usr/lib
LOCAL_SHARED_LIBRARIES := $$(addprefix lib,$$(FFLIBS))
LOCAL_SHARED_LIBRARIES += libz liblog #libui

LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)
include $(CLEAR_VARS)

endef

ALLFFLIBS := avutil avcore avcodec avformat \
	    swscale avfilter avdevice postproc
$(foreach M,$(ALLFFLIBS),$(eval $(call BUILD_FFLIB,lib$(M))))

CFLAGS :=
LDFLAGS :=
CPPFLAGS :=

LOCAL_MODULE := ffdroid
LOCAL_SRC_FILES := android/ffdroidplayer.cpp
#LOCAL_C_INCLUDES := $(LOCAL_PATH)/android

LOCAL_CFLAGS += -DBUILD_WITH_FFDROID=1
LOCAL_CFLAGS += -Wno-empty-body #-Wno-missing-braces

LOCAL_CFLAGS += -D_XOPEN_SOURCE=600 -DNDEBUG=1 \
		-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 \
		-DGNU_SOURCE=1 -D_ISOC99_SOURCE=1 -D_POSIX_C_SOURCE=200112 \

LOCAL_CFLAGS += -D_UNDEF__STDC_CONSTANT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 \
		-D_UNDEF__STDC_FORMAT_MACROS=1   -D__STDC_FORMAT_MACROS=1 \
		-D_UNDEF__STDC_LIMIT_MACROS=1    -D__STDC_LIMIT_MACROS=1 \

LOCAL_CFLAGS += -D__ISO_C_VISIBLE=1999 -D__XPG_VISIBLE=601 \
		#-D__POSIX_VISIBLE=200112 \

LOCAL_SHARED_LIBRARIES := $(addprefix lib,$(ALLFFLIBS))
LOCAL_SHARED_LIBRARIES += libmedia libbinder libutils \
	libsurfaceflinger_client #libcutils #libui #libz liblog # XXX:

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
#include $(CLEAR_VARS)

endif

# vim:sts=4:ts=8:

#########################################################
#
#	Telechips Linuxtv Demux OpenMax Component Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# ROOT/os/
TOP := ../linux/

#########################################################
#	Read Telechips Linux Configuration File 
#########################################################
include $(TOP)config

#########################################################
#	Read Telechips Linux make rules File 
#########################################################
include $(TOP)rule.mk

#########################################################
#	Common Definitions
#########################################################
include $(TOP)common.mk

#########################################################
#	Common Path Include   
#########################################################
include $(BUILD_SYSTEM)basic_path.mk

#########################################################
#	Read  define file  
#########################################################
include	$(BUILD_SYSTEM)definition.mk

#########################################################
#	Reset Variable  
#########################################################
include $(RESET_VARS)

#########################################################
#	Version Set  
#########################################################
LOCAL_VERSION_MAJOR:=00
LOCAL_VERSION_MINOR:=01
LOCAL_VERSION := _$(LOCAL_VERSION_MAJOR).$(LOCAL_VERSION_MINOR)

#########################################################
#	Set Current Folder and Global Path 
#########################################################
LOCAL_PATH	:=./
LOCAL_SRC_PATH	:=./src/

#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./


#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := libdxbos


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)tcc_pthread_cond.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)tcc_ui_thread.cpp
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)tcc_audio_render.cpp
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)alsa_sound_system.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)tcc_video_render.cpp
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)Hwrenderer.cpp


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include 
LOCAL_INCLUDE_HEADERS += $(DXB_C_INCLUDES)
LOCAL_INCLUDE_HEADERS += ../middleware/common/dxb_utils/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_base/include
#ifdef OECORE_TARGET_SYSROOT
#LOCAL_INCLUDE_HEADERS += $(OE_SYS_INCLUDE)
#endif

#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:=
#LOCAL_CFLAGS += $(DXB_CFLAGS)
LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -D_TCC8930_
LOCAL_CFLAGS += -DTCC_VIDEO_DISPLAY_BY_VSYNC_INT
LOCAL_CFLAGS += -DUSE_LCD_VIDEO_VSYNC
LOCAL_CFLAGS += -DTCC_SKIP_UI_UPDATE_DURING_VIDEO

## this define for capture : viqe deinterlacing with scaling
LOCAL_CFLAGS += -DTCC_VIDEO_DEINTERLACE_SUPPORT


## ALSA PULSE AUDIO Setting
## This setting is only applied from ALS 2.5 SDK version.
#LOCAL_CFLAGS += -DALSA_PULSE_AUDIO

LOCAL_CXXFLAGS := $(LOCAL_CFLAGS)


#########################################################
#	Add  shared lib  
#########################################################
LOCAL_SHARED_LIB :=


#########################################################
#	Add  copy lib  
#########################################################
LOCAL_COPY_LIB :=


#########################################################
#	Add  static lib  
#########################################################
LOCAL_STATIC_LIB :=
ifdef OECORE_TARGET_SYSROOT
LOCAL_STATIC_LIB += -L $(OE_SYS_LIB) -l asound
else
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libasound.so
endif
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libdxbutils.so

include	$(BUILD_SYSTEM)build_object.mk

#########################################################
#	Make All Function 
#########################################################
all :
	$(call build-clean-obj)
	$(call build-lib-shared)

#########################################################
#	Make Install Function 
#########################################################
install : 
	$(MAKE) all 
ifdef 	LOCAL_COPY_LIB	
	$(call build-copy-src-to-dest,$(LOCAL_COPY_LIB),$(COPY_TO_LIB_PATH))
endif
	$(call build-copy-to-lib,$(LOCAL_TARGET)$(SHARED),$(COPY_TO_LIB_PATH))


#########################################################
#	Make Clean Function 
#########################################################	
clean : 
	$(call build-clean-obj)


#########################################################
#	Make Debug Function 
#########################################################	
debug : 
	$(call build-debug-print)

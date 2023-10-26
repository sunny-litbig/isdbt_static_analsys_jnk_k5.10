#########################################################
#
#	Telechips DxB Video Output Component  make define
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# framework/dxb_components/renderers/omx_fbdevsink_component
TOP := ../../../../linux/

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
LOCAL_PATH 	:=./
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
LOCAL_TARGET := libOMX.TCC.fbdevsink

#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES :=

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)jpu_jpeg.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)tcc_utils.cpp
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)tcc_dxb_surface.cpp
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)omx_fbdev_sink_component.cpp

	
#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS :=

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include/
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)/omx_engine/dxb_omx_include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)/omx_engine/dxb_omx_base/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)/decoders/dxb_cdk_library/cdk
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)/decoders/dxb_cdk_library/container
LOCAL_INCLUDE_HEADERS += $(TCCDXB_TOP)/framework/dxb_interface/include/interface
LOCAL_INCLUDE_HEADERS += $(DXB_C_INCLUDES)


#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:=

LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -D_TCC8930_
LOCAL_CFLAGS += -DTCC_VIDEO_DISPLAY_BY_VSYNC_INT
LOCAL_CFLAGS += -DUSE_LCD_VIDEO_VSYNC
LOCAL_CFLAGS += -DTCC_SKIP_UI_UPDATE_DURING_VIDEO

## this define for capture : viqe deinterlacing with scaling
LOCAL_CFLAGS += -DTCC_VIDEO_DEINTERLACE_SUPPORT
#LOCAL_CFLAGS += -DSUPPORT_SCALER_COPY

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

LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.DxB.base.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libpmap.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libdxbos.so
#LOCAL_STATIC_LIB += $(TCCDXB_OMX_TOP)/decoders/dxb_cdk_library/video_codec/vpu/libTCC89xx_JPUCODEC_ANDROID_V1.3.0.0.a

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
	#$(call build-copy-to-lib,$(LOCAL_TARGET)$(STATIC),$(COPY_TO_LIB_PATH))
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


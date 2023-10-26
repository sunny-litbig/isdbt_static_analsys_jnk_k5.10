#########################################################
#
#	DXB Interface Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# framework/dxb_interface
TOP := ../../linux/

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
LOCAL_TARGET := libTCCDxBInterface


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)internal/tcc_dxb_interface_omxcore.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)internal/tcc_dxb_interface_omx_events.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)internal/tcc_dxb_interface_omxil.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)internal/tcc_dxb_interface_system.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)interface/tcc_dxb_interface_tuner.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)interface/tcc_dxb_interface_demux.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)interface/tcc_dxb_interface_audio.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)interface/tcc_dxb_interface_video.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)interface/tcc_dxb_interface_sc.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)interface/tcc_dxb_interface_cipher.c


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include/interface
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include/internal
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_base/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)demuxers/omx_linuxtv_demux_component/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/cdk
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/container
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/video_codec
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/omx_dxb_audiodec_interface/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/omx_dxb_audiodec_component/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/omx_dxb_videodec_interface/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/omx_dxb_videodec_component/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)renderers/omx_alsasink_component/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)renderers/omx_fbdevsink_component/include
LOCAL_INCLUDE_HEADERS += $(DXB_C_INCLUDES)


#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= $(DXB_CFLAGS)

LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -DTCC_VIDEO_DISPLAY_BY_VSYNC_INT


LOCAL_CXXFLAGS:= $(LOCAL_CFLAGS)


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
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.Tuner.interface.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.LinuxTV.Demux.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.DxB.AudioDec.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.DxB.VideoDec.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.alsasink.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.fbdevsink.so


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

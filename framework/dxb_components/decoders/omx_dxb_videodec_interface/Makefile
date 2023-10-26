#########################################################
#
#	Telechips Video Interface Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# framework/dxb_components/demuxers/omx_dxb_videodec_interface
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
LOCAL_TARGET := libOMX.TCC.DxB.VPUDec


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)vdec_k.c


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include 
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/cdk 
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/container
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/video_codec
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)decoders/dxb_cdk_library/video_codec/vpu
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_base/include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_INTERFACE_TOP)include/interface
LOCAL_INCLUDE_HEADERS += $(DXB_C_INCLUDES)


#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= $(DXB_CFLAGS)

LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -fno-short-enums


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

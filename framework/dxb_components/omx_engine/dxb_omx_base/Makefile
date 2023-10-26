#########################################################
#
#	Telechips OpenMax Base Component Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# framework/dxb_components/omx_engine/dxb_omx_base
TOP := ../../../../linux/
PARENT := ../

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
LOCAL_TARGET := libOMX.TCC.DxB.base

#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += 	\
	$(LOCAL_SRC_PATH)omx_base_component.c \
	$(LOCAL_SRC_PATH)omx_base_port.c \
	$(LOCAL_SRC_PATH)omx_base_audio_port.c \
	$(LOCAL_SRC_PATH)omx_base_video_port.c \
	$(LOCAL_SRC_PATH)omx_base_image_port.c \
	$(LOCAL_SRC_PATH)omx_base_source.c \
	$(LOCAL_SRC_PATH)omx_base_filter.c \
	$(LOCAL_SRC_PATH)omx_base_sink.c \
	$(LOCAL_SRC_PATH)TCCFile.c \
	$(LOCAL_SRC_PATH)TCCMemory.c \
	$(LOCAL_SRC_PATH)queue.c \
	$(LOCAL_SRC_PATH)tsemaphore.c \
	$(LOCAL_SRC_PATH)omx_tcc_thread.c \
	$(LOCAL_SRC_PATH)TCC_Util.c


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include 
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_include


#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:=

LOCAL_CFLAGS += -fPIC

#########################################################
#	Add  shared lib  
#########################################################
LOCAL_SHARED_LIB := \


#########################################################
#	Add  copy lib  
#########################################################
LOCAL_COPY_LIB :=


#########################################################
#	Add  static lib  
#########################################################
LOCAL_STATIC_LIB := 

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

#########################################################
#
#	Telechips ISDBT Subtitle Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# middleware/lib_isdbt/lib/isdbt_subtitle
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
LOCAL_MW_COMMON_PATH := ../../../common/

#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./


#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := libisdbt_subtitle


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)TsParser_Subtitle.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)TsParser_Subtitle_Debug.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)TsParser_Subtitle_Pos.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_charset.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_display.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_draw.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_main.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_memory.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_parser.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_queue.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_queue_pos.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_scaler.c
LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)subtitle_system.c


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include 
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)../../include
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)../../include/middleware
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)../../include/manager
LOCAL_INCLUDE_HEADERS += $(LOCAL_MW_COMMON_PATH)dxb_tsdecoder/include
LOCAL_INCLUDE_HEADERS += $(LOCAL_MW_COMMON_PATH)dxb_png/include
LOCAL_INCLUDE_HEADERS += $(LOCAL_MW_COMMON_PATH)dxb_font/include
LOCAL_INCLUDE_HEADERS += $(LOCAL_MW_COMMON_PATH)dxb_utils/include
LOCAL_INCLUDE_HEADERS += $(DXB_C_INCLUDES)
LOCAL_INCLUDE_HEADERS += $(TCCDXB_INTERFACE_TOP)include/interface

#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= $(DXB_CFLAGS)

LOCAL_CFLAGS += -fPIC

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


include	$(BUILD_SYSTEM)build_object.mk

#########################################################
#	Make All Function 
#########################################################
all :
	$(call build-clean-obj)
	$(call build-lib-static)

#########################################################
#	Make Install Function 
#########################################################
install : 
	$(MAKE) all 
ifdef 	LOCAL_COPY_LIB	
	$(call build-copy-src-to-dest,$(LOCAL_COPY_LIB),$(COPY_TO_LIB_PATH))
endif
#	$(call build-copy-to-lib,$(LOCAL_TARGET)$(STATIC),$(COPY_TO_LIB_PATH))
#	$(call build-copy-to-lib,$(LOCAL_TARGET)$(SHARED),$(COPY_TO_LIB_PATH))


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

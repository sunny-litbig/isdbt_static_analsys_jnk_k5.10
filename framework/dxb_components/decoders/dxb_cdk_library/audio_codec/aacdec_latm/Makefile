#########################################################
#
#	Telechips AACDEC Shared Library
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# framework/dxb_components/decoders/dxb_cdk_library/audio_codec/aacdec_latm
TOP := ../../../../../../linux/

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
LOCAL_VERSOIN_MINOR:=01
LOCAL_VERSION := _$(LOCAL_VERSION_MAJOR).$(LOCAL_VERSOIN_MINOR)

#########################################################
#	Set Current Folder and Global Path 
#########################################################
LOCAL_PATH :=./
LOCAL_SRC_PATH :=

#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./lib/

#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := libTCC.DxB.latmdmx

#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := $(LOCAL_SRC_PATH)aacdec_latm.c

#LOCAL_SRC_FILES += 


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := ../

#LOCAL_INCLUDE_HEADERS +=

#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= -fPIC

LOCAL_CFLAGS += -DTCC_LATM_PARSER


#########################################################
#	Add  shared lib  
#########################################################
LOCAL_SHARED_LIB := \

LOCAL_LDFLAGS := \
	-L$(LOCAL_PATH)lib \
	-lTCC_ARMv7_LATMDMX_LINUX_V2.00.04HF


#########################################################
#	Add  copy lib  
#########################################################
LOCAL_COPY_LIB :=


#########################################################
#	Add  static lib  
#########################################################
LOCAL_STATIC_LIB := \

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

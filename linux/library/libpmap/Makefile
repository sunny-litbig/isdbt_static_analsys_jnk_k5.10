#########################################################
#
#	Telechips Linuxtv Demux OpenMax Component Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# library/libpmap
TOP := ../../

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
LOCAL_PATH	:=./
LOCAL_SRC_PATH := ./

KERNEL_TCCMISC_PATH := $(OECORE_TARGET_SYSROOT)/usr/src/kernel/include/uapi/misc/
#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./


#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := libpmap


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)pmap.c


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

#LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)
LOCAL_INCLUDE_HEADERS += $(SYSTEM_INCLUDE)
LOCAL_INCLUDE_HEADERS += $(KERNEL_TCCMISC_PATH)

#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= $(DXB_CFLAGS)

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
	@`[ -d $(SYSTEM_INCLUDE)libpmap ] || mkdir $(SYSTEM_INCLUDE)libpmap`
	@ cp -a  $(LOCAL_PATH)pmap.h $(SYSTEM_INCLUDE)libpmap -rf
	$(call build-copy-to-lib,$(LOCAL_TARGET)$(SHARED),$(SYSTEM_LIB))

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

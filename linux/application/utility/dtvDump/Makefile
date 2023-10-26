#########################################################
#
#	Telechips Broadcast dtvDump Make File
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# application\utility\dtvDump
TOP := ../../../

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


#########################################################
#	Set Current Folder and Global Path 
#########################################################
LOCAL_PATH :=./
COMMON_PATH :=../common/

#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./


#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := dtvDump

#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES :=

LOCAL_SRC_FILES += $(LOCAL_PATH)dtvDump.c
LOCAL_SRC_FILES += $(LOCAL_PATH)linuxtv_utils.c
LOCAL_SRC_FILES += $(LOCAL_PATH)isdbt_tuner.c

#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 								

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)
LOCAL_INCLUDE_HEADERS += $(TCCDXB_TOP)framework/dxb_components/omx_engine/dxb_omx_include
LOCAL_INCLUDE_HEADERS += $(KERNEL_INCLUDE)linux/dvb

#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS :=

LOCAL_CXXFLAGS := $(LOCAL_CFLAGS)

#########################################################
#	Add  Lib Path
#########################################################
LOCAL_LDFLAGS := 

LOCAL_LDFLAGS += $(COPY_TO_LIB_PATH)
#LOCAL_LDFLAGS += $(EXTENDDISPLAY_LIB)
#LOCAL_LDFLAGS += -Wl,-rpath=$(COPY_TO_LIB_PATH)

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

LOCAL_STATIC_LIB += -Wl,-rpath=$(COPY_TO_LIB_PATH)
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libpmap.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libdxbos.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libOMX.TCC.tcc353x.Tuner.so

#########################################################
#	Build Objects
#########################################################
include	$(BUILD_SYSTEM)build_object.mk

#########################################################
#	Make All Function 
#########################################################
all :
	$(call build-clean-obj)
ifdef LOCAL_COPY_LIB
	$(call build-copy-src-to-dest,$(LOCAL_COPY_LIB),$(COPY_TO_LIB_PATH))
endif
#	$(call build-exec)
	$(call build-cpp-exec)

install : 
	$(MAKE) all
	mkdir -p $(TCC_OUTPUT_PATH)/utility
	cp -arf $(LOCAL_TARGET)     $(TCC_OUTPUT_PATH)/utility
	
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

#########################################################
#
#	Telechips Broadcast ISDBT FULL-SEG Player Make File
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# application\isdbt
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
LOCAL_TARGET := tcc_dxb_isdbt_app

#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES :=

LOCAL_SRC_FILES += $(COMMON_PATH)memory/tcc_memory.c
LOCAL_SRC_FILES += $(COMMON_PATH)message/tcc_message.c
LOCAL_SRC_FILES += $(COMMON_PATH)semaphore/tcc_semaphore.c

LOCAL_SRC_FILES += $(LOCAL_PATH)tcc_dxb_isdbtfseg_main.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc_dxb_isdbt_cui_display_info.c
LOCAL_SRC_FILES += $(LOCAL_PATH)cui/tcc_dxb_isdbtfseg_cui.c
LOCAL_SRC_FILES += $(LOCAL_PATH)ui/tcc_dxb_isdbtfseg_ui_process.c
LOCAL_SRC_FILES += $(LOCAL_PATH)ui/tcc_dxb_isdbtfseg_ui.c
LOCAL_SRC_FILES += $(LOCAL_PATH)cmd/tcc_dxb_isdbtfseg_cmd.c
LOCAL_SRC_FILES += $(LOCAL_PATH)cmd/tcc_dxb_isdbtfseg_cmd_list.c
LOCAL_SRC_FILES += $(LOCAL_PATH)cmd/tcc_dxb_isdbtfseg_message.c
LOCAL_SRC_FILES += $(LOCAL_PATH)process/process_sub.c
LOCAL_SRC_FILES += $(LOCAL_PATH)process/tcc_dxb_isdbtfseg_process.cpp

#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 								

LOCAL_INCLUDE_HEADERS += $(COMMON_PATH)memory
LOCAL_INCLUDE_HEADERS += $(COMMON_PATH)message
LOCAL_INCLUDE_HEADERS += $(COMMON_PATH)semaphore
LOCAL_INCLUDE_HEADERS += $(COMMON_PATH)cui

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)process
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)cui
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)ui
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)cmd

ifdef OECORE_TARGET_SYSROOT
else
LOCAL_INCLUDE_HEADERS += $(SYSTEM_INCLUDE)sqlite
endif

LOCAL_INCLUDE_HEADERS += $(COPY_TO_HEADER_PATH)

#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS :=

LOCAL_CFLAGS += -DTCC_CUI_INCLUDE
LOCAL_CFLAGS += -DTCC_USE_PUT_MESSAGE2_INCLUDE

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
LOCAL_COPY_LIB :=	\

#########################################################
#	Add  static lib  
#########################################################
LOCAL_STATIC_LIB :=

LOCAL_STATIC_LIB += -Wl,-rpath=$(COPY_TO_LIB_PATH)
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libisdbt.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libdxbos.so
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libpmap.so
ifdef OECORE_TARGET_SYSROOT
LOCAL_STATIC_LIB += -L $(OE_SYS_LIB) -l sqlite3
else
LOCAL_STATIC_LIB += $(COPY_TO_LIB_PATH)libsqlite3.so
endif

ifeq ($(TRMP_ENABLE), Y)
LOCAL_CFLAGS += -DTRMP_ENABLE
endif

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
	mkdir -p $(TCC_OUTPUT_PATH)/$(TCC_DXB_ON_TARGET_PATH)bin
	mkdir -p $(TCC_OUTPUT_PATH)/$(TCC_DXB_ON_TARGET_PATH)config
	mkdir -p $(TCC_OUTPUT_PATH)/$(TCC_DXB_ON_TARGET_PATH)font
	cp -arf $(COPY_TO_LIB_PATH) $(TCC_OUTPUT_PATH)/$(TCC_DXB_ON_TARGET_PATH)
	cp -arf $(LOCAL_TARGET)     $(TCC_OUTPUT_PATH)/$(TCC_DXB_ON_TARGET_PATH)bin
	cp -arf $(SYSTEM_PATH)DroidSansFallback_DxB.ttf $(TCC_OUTPUT_PATH)/$(TCC_DXB_ON_TARGET_PATH)font
	cp -arf tcc_isdbt_api.sh $(TCC_OUTPUT_PATH)
	
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

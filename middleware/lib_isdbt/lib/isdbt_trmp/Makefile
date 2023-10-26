#########################################################
#
#	Telechips ISDBT Subtitle Library Make File 
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# middleware/lib_isdbt/lib/isdbt_trmp
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

#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./

#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := libisdbt_trmp


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_PATH)client/src/tcc_trmp_manager.c
LOCAL_SRC_FILES += $(LOCAL_PATH)client/src/tcc_trmp_client.c

ifeq ($(TRMP_ENABLE), Y)
	LOCAL_SRC_FILES +=  $(LOCAL_PATH)client/tee/src/tee_trmp_ca.c
endif


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include 
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)client/include 
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)../../include/middleware

ifeq ($(TRMP_ENABLE), Y)
	LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)client/tee/include
endif


#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= $(DXB_CFLAGS)

LOCAL_CFLAGS += -fPIC

ifeq ($(TRMP_ENABLE), Y)
	LOCAL_CFLAGS += -D__SECURITEE__
endif


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

#setting only for local build
ifeq ($(TRMP_ENABLE), Y)
ifdef OECORE_TARGET_SYSROOT
LOCAL_STATIC_LIB += -L $(OE_SYS_LIB) -l teec
LOCAL_STATIC_LIB += -L $(OE_SYS_LIB) -l ca_interface
endif
endif


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
#	$(call build-copy-to-lib,$(LOCAL_TARGET)$(STATIC),$(COPY_TO_LIB_PATH))
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

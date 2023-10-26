#########################################################
#
#	Telechips ISDBT FULL-SEG TCC353x Tuner OpenMax Component make define
#
#########################################################

#########################################################
#	Telechips Make File Initialize
#########################################################
# framework/dxb_components/tuners/omx_isdbttuner_tcc353x_component
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
LOCAL_TARGET := libOMX.TCC.tcc353x.Tuner

#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES :=
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/api/src/tcc353x_api.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/Linux_Adapt/tcc353x_linux_i2c.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/Linux_Adapt/tcc353x_linux_tccspi.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/PAL/tcc353x_pal_i2c.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/PAL/tcc353x_pal_tccspi.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/PAL/tcpal_android.c
#LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/PAL/tcpal_linux.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/sample/main/tcc353x_main.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/sample/monitoring/tcc353x_monitoring.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/sample/monitoring/tcc353x_monitoring_calculate.c
LOCAL_SRC_FILES += $(LOCAL_PATH)src/isdbt_tcc353x_tuner.c
LOCAL_SRC_FILES += $(LOCAL_PATH)src/isdbt_tuner_space.c
LOCAL_SRC_FILES += $(LOCAL_PATH)src/omx_isdbttuner_tcc353x_component.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_command_control.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_core.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_dpll_19200osc.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_dpll_38400osc.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_dpll_tcc3535.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_dpll_tcc3536.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_isdb.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_mailbox.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_register_control.c
LOCAL_SRC_FILES += $(LOCAL_PATH)tcc353xdriver/driver/src/tcc353x_rf.c

#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS :=
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/common/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/Linux_Adapt/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/PAL/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/sample/main/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/sample/monitoring/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/api/inc/
LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)tcc353xdriver/driver/inc/

LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_include
LOCAL_INCLUDE_HEADERS += $(TCCDXB_OMX_TOP)omx_engine/dxb_omx_base/include
LOCAL_INCLUDE_HEADERS += $(DXB_CTRL_INCLUDE)


	
#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:=

LOCAL_CFLAGS += -fPIC

#========= TCC353X Driver Build Guide =============
#=                                                =
#=                  TCC353X                       =
#=                                                =
#==================================================


#==================================================
# 1. Select TCC353X chip (TCC3530/TCC3531/TCC3532)
#==================================================

#LOCAL_CFLAGS    += -D_USE_TCC3530_
LOCAL_CFLAGS    += -D_USE_TCC3531_
#LOCAL_CFLAGS    += -D_USE_TCC3532_

#==================================================
# 2. Select Target Standard (1seg/fullseg(default))
#==================================================

#LOCAL_CFLAGS    += -D_USE_ONLY_1_SEGMENT_ 

#==================================================
# 3. Select Diversity Type (single/2~4Diversity)
#==================================================

#LOCAL_CFLAGS    += -D_USE_TCC353X_SINGLE_  
#LOCAL_CFLAGS    += -D_USE_TCC353X_2DIVERSITY_
#LOCAL_CFLAGS    += -D_USE_TCC353X_3DIVERSITY_
LOCAL_CFLAGS    += -D_USE_TCC353X_4DIVERSITY_

#==================================================
# 4. Select TUNER Type (DUAL BAND(default)/TRIPLE BAND)
#==================================================

#LOCAL_CFLAGS    += -D_USE_TCC353X_TRIPLE_BAND_


#==================================================
# 5. Select Tcc353x Power control type (Default _DXB_PWRCTRL_1_3_)
#
# [_DXB_PWRCTRL_1_3_]
#   DxbPower Control 0 : Master
#   DxbPower Control 1 : Slave1, Slave2, Slave3
#
# [_DXB_PWRCTRL_4_]
#   DxbPower Control 0 : Master, Slave1, Slave2, Slave3
#   DxbPower Control 1 : None
#
# [_DXB_PWRCTRL_2_2_]
#   DxbPower Control 0 : Master, Slave1
#   DxbPower Control 1 : Slave2, Slave3
#
#==================================================

#LOCAL_CFLAGS    += -D_DXB_PWRCTRL_1_3_
#LOCAL_CFLAGS    += -D_DXB_PWRCTRL_4_
LOCAL_CFLAGS    += -D_DXB_PWRCTRL_2_2_

LOCAL_CFLAGS += -D_HW_TCC8XXX_AK4601_SILAB_ISDBT_DAB_

#==================================================
# 6. Select Tcc353x Debug Log
#
#   DBG_MONITOR : enable monitoring log
#   USE_MONITORING_LOG : enable monitoring debug log
#==================================================

#LOCAL_CFLAGS    += -DDBG_MONITOR
#LOCAL_CFLAGS    += -DUSE_MONITORING_LOG

#====== End of TCC353X Driver Build Guide =========
#=                                                =
#=                  TCC353X                       =
#=                                                =
#==================================================

#LOCAL_CFLAGS    += -v


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
	@echo " LOCAL_CFLAGS= $(LOCAL_CFLAGS)"


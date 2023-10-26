#########################################################
#
#	Telechips Linux Application basic path setting
#
#########################################################

#########################################################
##
##	include/		top include path
##
#########################################################
#TOP_INCLUDE			:= $(TOP)include/
TOP_INCLUDE			:= 

#########################################################
##
##	build/			target build path
##
#########################################################
TCC_OUTPUT 			:= $(TARGET_BOARD)_Output
TCC_BUILD_PATH 			:= $(TOP)build
TCC_BUILD_TARGET		:= build/
TCC_BUILD_INCLUDE 		:= $(TCC_BUILD_TARGET)include/
TCC_BUILD_LIB 			:= $(TCC_BUILD_TARGET)lib/

COPY_TO_HEADER_PATH 		:= $(TOP)build/include/
COPY_TO_LIB_PATH 		:= $(TOP)build/lib/

TCC_OUTPUT_PATH 		:= $(TOP)$(TARGET_BOARD)_Output
TCC_OUTPUT_LIB			:= $(TOP)$(TARGET_BOARD)_Output/lib/
TCC_OUTPUT_EXEC_CONFIG_PATH	:= $(TOP)$(TARGET_BOARD)_Output/config
TCC_OUTPUT_EXEC_PATH		:= $(TOP)$(TARGET_BOARD)_Output/bin

BUILD_SYSTEM 			:= $(TOP)build/

SYSTEM_PATH			:= $(TOP)system/
SYSTEM_INCLUDE		:= $(SYSTEM_PATH)include/
SYSTEM_LIB			:= $(SYSTEM_PATH)

####With ADT Toolchain####
ifdef OECORE_TARGET_SYSROOT
KERNEL_PATH                    := $(OECORE_TARGET_SYSROOT)/usr/src/kernel/
endif

####With ALS SDK Build####
ifdef OECORE_TARGET_KERNEL
KERNEL_PATH                    := $(OECORE_TARGET_KERNEL)/
endif

KERNEL_INCLUDE			:= $(KERNEL_PATH)include/
DXB_C_INCLUDES			:= $(KERNEL_PATH)arch/arm/mach-$(TARGET_BOARD_PLATFORM)/include
DXB_C_INCLUDES			+= $(KERNEL_PATH)arch/arm/mach-$(TARGET_BOARD_PLATFORM)/include/mach
ifeq ($(TARGET_CHIPSET),TCC_803X)
JKTEST_INCLUDE			+= $(KERNEL_PATH)include/video/tcc/
DXB_C_INCLUDES			+= $(JKTEST_INCLUDE)
endif

ifdef OECORE_TARGET_SYSROOT
OE_SYS_INCLUDE += $(OECORE_TARGET_SYSROOT)/usr/include
OE_SYS_LIB     += $(OECORE_TARGET_SYSROOT)/usr/lib
endif
#########################################################
##
##	define top path of DXB folder
##
#########################################################
TCCDXB_TOP			:= $(TOP)../

#########################################################
##
##      middleware/common/dxb_utils/ dxb_common_utils
##
#########################################################
DXB_COMMON_UTILS_DEPENDENCY     := $(TCCDXB_TOP)middleware/common/dxb_utils/

#########################################################
##
##	os/			os dependency function
##
#########################################################
OS_DEPENDENCY			:= $(TCCDXB_TOP)os/

#########################################################
##
##	component/		openmax path setting
##
#########################################################
#OPENMAX_COMPONENT		:= $(TOP)component/
OPENMAX_COMPONENT		:= $(TOP)../framework/dxb_components/
#########################################################

#OMX_INCLUDE 			:= $(TOP)component/openmax_include/
#OMX_BASE_INCLUDE 		:= $(TOP)component/openmax_base/
#OMX_CORE_INCLUDE 		:= $(TOP)component/openmax_core/
#OMX_COMMON_INCLUDE 		:= $(TOP)component/openmax_common/

OMX_ENGINE_INCLUDE		:= $(OPENMAX_COMPONENT)omx_engine/dxb_omx_include/
TCCDXB_OMX_TOP			:= $(OPENMAX_COMPONENT)
TCCDXB_INTERFACE_TOP		:= $(TOP)../framework/dxb_interface/

#-------------------------------------------------------
#	dxb_components path
#-------------------------------------------------------
DXB_COMPONENTS_PATH		:= $(TOP)../framework/dxb_components/
DXB_COMPONENTS_DECODERS_PATH	:= $(DXB_COMPONENTS_PATH)decoders/
DXB_COMPONENTS_DEMUXERS_PATH	:= $(DXB_COMPONENTS_PATH)demuxers/
DXB_COMPONENTS_RENDERERS_PATH	:= $(DXB_COMPONENTS_PATH)renderers/
DXB_COMPONENTS_TUNERS_PATH	:= $(DXB_COMPONENTS_PATH)tuners/

DXB_COMPONENTS_INCLUDE		:=
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_DECODERS_PATH)omx_dxb_audiodec_component/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_DECODERS_PATH)omx_dxb_audiodec_interface/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_DECODERS_PATH)omx_dxb_videodec_component/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_DECODERS_PATH)omx_dxb_videodec_interface/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_DEMUXERS_PATH)common/dxb_linuxtv_tsdmx/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_DEMUXERS_PATH)omx_isdbtdemux_component/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_RENDERERS_PATH)omx_alsasink_component/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_RENDERERS_PATH)omx_fbdevsink_component/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_TUNERS_PATH)omx_isdbttuner_toshiba_component/include/
DXB_COMPONENTS_INCLUDE		+= $(DXB_COMPONENTS_TUNERS_PATH)omx_tuner_interface/include/

#-------------------------------------------------------
#	cdk_dxb_library path
#-------------------------------------------------------
CDK_DXB_PATH			:= $(DXB_COMPONENTS_DECODERS_PATH)/dxb_cdk_library/
CDK_DXB_INCLUDE			:=
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/audio_codec/
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/cdk/
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/container/
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/container/audio/
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/container/ts/
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/video_codec/
CDK_DXB_INCLUDE			+= $(CDK_DXB_PATH)/video_codec/vpu/

#########################################################
##
##	common_api/
##
#########################################################

#########################################################



#########################################################
##
##	drivers/		common drivers path setting
##
#########################################################
#########################################################



#########################################################
##
##	/lib			library path
##
#########################################################
#EXT_LIB_PATH			:= $(TOP)lib/
#########################################################

#-------------------------------------------------------
#	alsa header path
#-------------------------------------------------------
#ALSA_HEADER_INCLUDE		:= $(TOP)system


#########################################################
##
##	Platform DXB build output PATH
##
#########################################################
TCC_DXB_ON_TARGET_PATH := tccdxb/

LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR := $(LINUX_PLATFORM_BUILDDIR)/$(TCC_DXB_ON_TARGET_PATH)



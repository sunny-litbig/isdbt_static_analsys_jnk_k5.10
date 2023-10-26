#########################################################
#
#	Telechips Common Definition File 
#
#########################################################

#########################################################
#	Read Telechips Linux Configuration File 
#########################################################
#include $(TOP)config

#########################################################
#	Read Telechips Linux make rules File 
#########################################################
#include $(TOP)rule.mk

#########################################################
#	Set Common Definition
#########################################################

##########################################
# Board Definition
##########################################

#ifeq ($(TARGET_CHIPSET),LINUX_HOST)
#CFLAGS += -DTCC_892X_INCLUDE
#CFLAGS += -DLINUX_HOST_INCLUDE
#else
#CFLAGS += -DTCC_892X_INCLUDE
#endif

#ifeq ($(TARGET_BOARD),TCC_892X)
#CFLAGS += -DTCC_892X_INCLUDE
#endif

#ifeq ($(TARGET_BOARD),TCC_893X)
#CFLAGS += -DTCC_893X_INCLUDE
#endif

CFLAGS += -DHAVE_LINUX_PLATFORM
CXXFLAGS += -DHAVE_LINUX_PLATFORM
#CFLAGS += -DSUPPORT_PVR
#CXXFLAGS += -DSUPPORT_PVR
#########################################################
#	From Android - does it need to be moved to common.mk?
#########################################################
DXB_CFLAGS := 
ifeq ($(SUPPORT_RINGBUFFER),Y)
DXB_CFLAGS += -DRING_BUFFER_MODULE_ENABLE
endif
ifeq ($(TARGET_BOARD_PLATFORM),tcc893x)
DXB_CFLAGS += -DTCC_893X_INCLUDE
endif
ifeq ($(TARGET_BOARD_PLATFORM),tcc897x)
DXB_CFLAGS += -DTCC_897X_INCLUDE
endif
ifeq ($(TARGET_BOARD_PLATFORM),tcc898x)
DXB_CFLAGS += -DTCC_898X_INCLUDE
endif
ifeq ($(TARGET_BOARD_PLATFORM),tcc802x)
DXB_CFLAGS += -DTCC_802X_INCLUDE
endif

ifeq ($(TARGET_BOARD_PLATFORM),tcc803x)
DXB_CFLAGS += -DTCC_803X_INCLUDE
endif

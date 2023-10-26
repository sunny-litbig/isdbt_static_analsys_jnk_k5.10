#
#	Telechips Linux Application make rules 
#

ifndef OECORE_TARGET_SYSROOT
TOOCHAIN_DEFINE=Y

#########################################################
#	Crooss Compile Path Setting 
#########################################################

ifeq ($(TARGET_CHIPSET), TCC_897X)
	TARGET_CPU = cortex-a7
	TARGET_ARCH = armv7-a
	ifeq ($(TOOCHAIN_DEFINE), Y)
		CROSS_COMPILE_PATH :=/opt/arm-2013.11/bin/
	endif
else ifeq ($(TARGET_CHIPSET),TCC_898X)
	TARGET_CPU = cortex-a53
	TARGET_ARCH = armv8-a
	ifeq ($(TOOCHAIN_DEFINE), Y)
		CROSS_COMPILE_PATH :=/opt/arm-none-linux-gnueabi/bin/
	endif
else ifeq ($(TARGET_CHIPSET),TCC_802X)
        TARGET_CPU = cortex-a7
        TARGET_ARCH = armv7-a
        ifeq ($(TOOCHAIN_DEFINE), Y)
                CROSS_COMPILE_PATH :=/opt/arm-2013.11/bin/
        endif
else ifeq ($(TARGET_CHIPSET),TCC_803X)
	TARGET_CPU = cortex-a7
	TARGET_ARCH = armv7-a
	ifeq ($(TOOCHAIN_DEFINE), Y)
		CROSS_COMPILE_PATH :=/opt/arm-2013.11/bin/
	endif
endif

#########################################################
#	Setting Compile
#########################################################

	ifeq ($(TOOCHAIN_DEFINE), Y)
		CROSS_COMPILE	= $(CROSS_COMPILE_PATH)arm-none-linux-gnueabi-
	else
		CROSS_COMPILE	= arm-none-linux-gnueabi-
	endif

	LD		= $(CROSS_COMPILE)ld
	CC		= $(CROSS_COMPILE)gcc
	CPP		= $(CC) -E
	CXX		= $(CROSS_COMPILE)g++
	AR		= $(CROSS_COMPILE)ar
	NM		= $(CROSS_COMPILE)nm
	ASM		= $(CROSS_COMPILE)as
	ARMOBJCOPY	= $(CROSS_COMPILE)objcopy
	STRIP		= $(CROSS_COMPILE)strip

#########################################################
#	Setting Include
#########################################################

#INCLUDE +=  -I$(CROSS_COMPILE_PATH)codesourcery/arm-none-linux-gnueabic/libc/usr/include
#INCLUDE +=  -I$(CROSS_COMPILE_PATH)codesourcery/arm-none-linux-gnueabic/include
#INCLUDE +=  -Iarm-none-linux-gnueabi/libc/usr/include
#INCLUDE +=  -Iarm-none-linux-gnueabi/include

#########################################################
#	Setting Library 
#########################################################

LDFLAGS += -L$(CROSS_COMPILE_PATH)codesourcery/arm-none-linux-gnueabi/libc/usr/lib
LDFLAGS += -L$(CROSS_COMPILE_PATH)codesourcery/arm-none-linux-gnueabi/libc/lib
#LDFLAGS += -Larm-none-linux-gnueabi/libc/usr/lib
#LDFLAGS += -Larm-none-linux-gnueabi/libc/lib


#########################################################
#	Setting CFLAGS and CXXFLAGS
#########################################################

CFLAGS			+= -mcpu=$(TARGET_CPU)
#CXXFLAGS		+= -mcpu=$(TARGET_CPU)
AFLAGS			= -march=$(TARGET_ARCH) -EL 

ifeq ($(TARGET_BOARD),TCC_892X)
CFLAGS +=-msoft-float -mfpu=vfp
endif

else
CFLAGS += -mfpu=vfpv3-fp16

#END OF 'ifdef OECORE_TARGET_SYSROOT'
endif

CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
CFLAGS += -D_LINUX_OS_
ifeq ($(TARGET_CHIPSET),TCC_898X)
CFLAGS += -DCONFIG_ARCH_TCC898X
endif
ifeq ($(TARGET_CHIPSET),TCC_802X)
CFLAGS += -DCONFIG_ARCH_TCC802X
endif
ifeq ($(TARGET_CHIPSET),TCC_803X)
	CFLAGS += -DCONFIG_ARCH_TCC803X
endif

## include file from android system
INCLUDE +=  -I$(TOP)system/include
INCLUDE +=  -I$(TCCDXB_TOP)middleware/common/dxb_utils/include
## os dependency header
INCLUDE +=  -I$(TOP)../os/include
CXXFLAGS = $(CFLAGS)

#########################################################
#	Setting Debug Option 
#########################################################
DEBUG	=	N
ifeq ($(DEBUG),Y)
CFLAGS += -g -O0 -w -O
CXXFLAGS += -g -O0 -w -O
else
CFLAGS += -O2 -w
CXXFLAGS += -O2 -w
endif

## Platform
#DEBUG = N
#ifeq ($(DEBUG),Y)
#CFLAGS += -g -O0 -w -O
#CFLAGS += -D_DEBUG_
#else
#CFLAGS += -O2 -w -s
#endif

#########################################################
#	Set Value Setting 
#########################################################

BUILD_SYSTEM 	:= $(TOP)build/

LIB_STATIC	:=.a
LIB_SHARED	:=.so

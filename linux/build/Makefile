#
#	Telechips Library Setting Make File 
#



#########################################################
#	Telechips Make File Initialize
#########################################################
TOP := ../
CURRENT := ./


#########################################################
#	Read Telechips Linux Configuration File 
#########################################################
include $(TOP)config

#########################################################
#	Read Telechips Linux make rules File 
#########################################################
include $(TOP)rule.mk

ifeq ($(if $(filter TCC_89XX TCC_93XX TCC_88XX TCC_892X, $(TARGET_BOARD)),Y), Y)
ifeq ($(TCC_DXB_BROADCAST_TASK), N)
include $(CURRENT)tcc_cdk_lib.mk
endif
endif

include $(CURRENT)tcc_omx_lib.mk
include $(CURRENT)tcc_etc_lib.mk

#########################################################
#	Setting Sub Folder 
#########################################################


#########################################################
#	Setting  Current Foler 
#########################################################


#########################################################
#	ADD Option 
#########################################################

#########################################################
#	Make All Function 
#########################################################
all :
ifeq ($(if $(filter TCC_89XX TCC_93XX TCC_88XX TCC_892X, $(TARGET_BOARD)),Y), Y)
ifeq ($(TCC_DXB_BROADCAST_TASK), N)
	$(MAKE)	cdk_lib
endif
endif
	$(MAKE)	openmax_lib
	$(MAKE)	etc_lib	
	
#########################################################
#	Make Install Function 
#########################################################


#########################################################
#	Make Clean Function 
#########################################################	
clean :
	$(MAKE)	omx_lib_clean
ifeq ($(if $(filter TCC_89XX TCC_93XX TCC_88XX TCC_892X, $(TARGET_BOARD)),Y), Y)
ifeq ($(TCC_DXB_BROADCAST_TASK), N)
	$(MAKE)	cdk_lib_clean
endif
endif
	$(MAKE)	etc_lib_clean

		
#########################################################
#	Make Debug Function 
#########################################################	
debug : 
	$(MAKE) audio_lib_debug
	$(MAKE) video_lib_debug
	$(MAKE) com_lib_debug
	$(MAKE) photo_lib_debug
	$(MAKE) broad_lib_debug




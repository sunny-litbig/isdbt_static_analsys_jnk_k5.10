#
#	Telechips Linux APP Make File 
#

#########################################################
#	Telechips Make File Initialize
#########################################################
TOP := ./

#########################################################
#	Read Telechips Linux Configuration File 
#########################################################
include $(TOP)config

#########################################################
#	Read Telechips Linux make rules File 
#########################################################
include $(TOP)rule.mk

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
#	Setting Sub Directory 
#########################################################
LOCAL_SUB_DIR :=
ifeq ($(DXB_BROADCAST_ISDBT_FULLSEG),Y)
LOCAL_SUB_DIR += $(TOP)../middleware/lib_isdbt
LOCAL_SUB_DIR += $(TOP)application/isdbt
endif
ifeq ($(DXB_DUMP_UTIL),Y)
LOCAL_SUB_DIR += $(TOP)application/dtvdump
endif
#########################################################
#	Setting  Current Foler 
#########################################################

#########################################################
#	ADD Option 
#########################################################
TCC_APP_FOLDER_NAME := tcc_app_rom
TCC_APP_TAR_NAME := tcc_app_rom.tar


#########################################################
#	Make All Function 
#########################################################

bin: check_platfrom_env prepare commonutils osdep omx setting install

# DXB COMMON UTIL
commonutils:
	$(call build-clean-to-directory,$(DXB_COMMON_UTILS_DEPENDENCY))
	$(call build-install-to-directory,$(DXB_COMMON_UTILS_DEPENDENCY))

# OS dependency 
osdep:
	$(call build-clean-to-directory,$(OS_DEPENDENCY))
	$(call build-install-to-directory,$(OS_DEPENDENCY))

omx: 
	$(call build-clean-to-directory,$(OPENMAX_COMPONENT))
	$(call build-install-to-directory,$(OPENMAX_COMPONENT))

all : 
	$(call build-all-to-directory,$(LOCAL_SUB_DIR))

#########################################################
#	Make Install Function 
#########################################################
install : do_install tar 

do_install :
#	$(call build_output_dir,$(TCC_OUTPUT_PATH),$(TCC_APP_FOLDER_NAME))
#	$(call build_output_dir,$(TCC_OUTPUT_EXEC_PATH),$(TCC_OUTPUT_EXEC_CONFIG_PATH))
#	$(call build-all-to-directory,$(LOCAL_SUB_DIR))
	$(call build-install-to-directory,$(LOCAL_SUB_DIR))

	# copy libs and header to platform output folder
ifdef LINUX_PLATFORM_BUILDDIR
	cp -a $(SYSTEM_INCLUDE)sqlite/sqlite3.h $(TCC_BUILD_PATH)/include
	rm -rf $(LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR)
	mkdir -p $(LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR)
	cp -arf $(TCC_BUILD_PATH)/include  $(LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR)
	cp -arf $(TCC_BUILD_PATH)/lib      $(LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR)
ifeq ($(DXB_BROADCAST_ISDBT_FULLSEG),Y)
	mkdir -p $(LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR)font
	cp -arf $(SYSTEM_PATH)DroidSansFallback_DxB.ttf $(LINUX_PLATFORM_DXBLIB_BUILD_TARGET_DIR)font
endif
endif

#########################################################
#	Make Tar File After Install
#########################################################
tar :
#	rm $(TOP)$(TCC_APP_FOLDER_NAME) -rf
#	mkdir $(TOP)$(TCC_APP_FOLDER_NAME)
#	cd $(TOP)$(TCC_OUTPUT) ; rm -rf `find . -name ".svn"` #; tar -cf ../$(TCC_APP_FOLDER_NAME)/$(TCC_APP_TAR_NAME) ./*
	
#########################################################
#	Make Clean Function 
#########################################################
clean : 
	rm $(TOP)$(TCC_OUTPUT) -rf
#	rm $(TOP)$(TCC_APP_FOLDER_NAME) -rf
	rm $(COPY_TO_HEADER_PATH) -rf
	rm $(COPY_TO_LIB_PATH) -rf
	$(call build-clean-to-directory,$(OS_DEPENDENCY))
	$(call build-clean-to-directory,$(OPENMAX_COMPONENT))
	$(call build-clean-to-directory,$(LOCAL_SUB_DIR))

#########################################################
#	Make Library Setting 
#########################################################
setting :
#	$(call build-all-to-directory,$(TCC_BUILD_PATH))

check_platfrom_env:
ifndef LINUX_PLATFORM_BUILDDIR
ifeq ($(STOP_IF_PLATFORM_ENV_NOT_SET),Y)
	@echo =========================================================
	@echo Warning!
	@echo =========================================================
	@echo linux plarform environment is not set. please check it.
	@echo LINUX_PLATFORM_BUILDDIR = [$(LINUX_PLATFORM_BUILDDIR)]
	@echo If use dxb as standalone, modify as below at config
	@echo STOP_IF_PLATFORM_ENV_NOT_SET = Y
	@echo =========================================================
	pause
endif
endif


#########################################################
#	Prepare build
#########################################################
prepare:
	rm $(TOP)$(TCC_BUILD_INCLUDE) -rf
	rm $(TOP)$(TCC_BUILD_LIB) -rf
	mkdir $(TOP)$(TCC_BUILD_INCLUDE)
	mkdir $(TOP)$(TCC_BUILD_LIB)
ifdef OECORE_TARGET_SYSROOT
	@cp -a $(SYSTEM_LIB)libpmap.so $(SYSTEM_LIB)libproperties.so $(TOP)$(TCC_BUILD_LIB) 
else
	@cp -a $(SYSTEM_LIB)*.so* $(TOP)$(TCC_BUILD_LIB) 
endif
	@if [ -e proprietary/$(TARGET_BOARD_PLATFORM) ]; then \
		cp -a proprietary/$(TARGET_BOARD_PLATFORM)/*.so* $(TOP)$(TCC_BUILD_LIB); \
	fi

debug :
	$(call build-debug-print)
	@echo " APP_ROOT = $(APP_ROOT)"


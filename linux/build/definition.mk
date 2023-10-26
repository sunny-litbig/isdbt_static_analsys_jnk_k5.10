#########################################################
#
#	Telechips Linux Application make define
#
#########################################################
RESET_VARS:= $(BUILD_SYSTEM)reset_vars.mk

define build-clean-obj
@rm  $(LOCAL_OBJECTS) *.out $(LOCAL_TARGET)$(SHARED) $(LOCAL_TARGET)$(STATIC) $(LOCAL_TARGET) -rf;
endef

define build-lib-static
	$(call build-object)
	$(AR) rcv $(LOCAL_TARGET)$(STATIC) $(LOCAL_OBJECTS)
	ranlib $(LOCAL_TARGET)$(STATIC)
endef

define build-lib-shared
	$(call build-object)
	$(CC) -shared -o $(LOCAL_TARGET)$(SHARED) -Wl,-soname,$(LOCAL_TARGET)$(SHARED) -Wl,-Bsymbolic $(LOCAL_OBJECTS) $(LOCAL_LDFLAGS) $(LOCAL_STATIC_LIB)
endef

define build-exec
	$(call build-object)
	$(CC) -o $(LOCAL_TARGET) $(CFLAGS) \
		-Xlinker --start-group \
		$(LOCAL_OBJECTS) $(LDFLAGS) $(LOCAL_STATIC_LIB) \
		$(foreach incdir,$(LOCAL_LDFLAGS),-L$(incdir)) \
		-Xlinker --end-group \
		-lpthread -lrt $(INCLUDE) \
		$(foreach incdir,$(LOCAL_INCLUDE_HEADERS),-I $(incdir))
endef

define build-cpp-exec
	$(call build-object)
	$(CXX) -o $(LOCAL_TARGET) $(CXXFLAGS) \
		-Xlinker --start-group \
		$(LOCAL_OBJECTS) $(LDFLAGS) $(LOCAL_STATIC_LIB) \
		$(foreach incdir,$(LOCAL_LDFLAGS),-L$(incdir)) \
		-Xlinker --end-group \
		-lpthread -lstdc++ $(INCLUDE) \
		$(foreach incdir,$(LOCAL_INCLUDE_HEADERS),-I $(incdir))
endef

define build-exec-gtk
	$(call build-object)
	$(CC) -o $(LOCAL_TARGET) $(CFLAGS) \
		-Xlinker --start-group \
		$(LOCAL_OBJECTS) $(LDFLAGS) $(LOCAL_STATIC_LIB) \
		$(foreach incdir,$(LOCAL_LDFLAGS),-L$(incdir)) \
		-Xlinker --end-group \
		-lpthread -lm  -Wl,--rpath -Wl,/nand1/target/lib -ldl -lpthread  -lc $(INCLUDE) \
		$(foreach incdir,$(LOCAL_INCLUDE_HEADERS),-I $(incdir))
endef

define build-object
$(MAKE) $(LOCAL_OBJECTS) -j8
endef

define build-copy-to-header
@ cp -a $(LOCAL_PATH)*.h  $(COPY_TO_HEADER_PATH) -rf
endef

define build-copy-to-dir-header
@ cp -a $(1)*.h  $(COPY_TO_HEADER_PATH) -rf
endef

define build-copy-to-lib
@ cp -a $(1) $(2) -rf
@ rm $(1) -rf
endef

define build-copy-src-to-dest
@ cp -a $(1) $(2) -rf
endef

define build-copy-src-to-dest-lib
@ cp -a $(1)*.so $(2) -rf
endef

define build-copy-src-to-dest-sh
@ cp -a $(LOCAL_PATH)*.sh $(1) -rf
endef

define build-clean-to-directory
$(foreach dir,$(1),$(MAKE) -C $(dir) clean;)
endef

define build-install-to-directory
$(foreach dir,$(1),$(MAKE) -C $(dir) install;)
endef

define build-all-to-directory
$(foreach dir,$(1),make -C $(dir) all;)
endef

define build-all-test2-to-directory
	for incdir in $(1); \
		do (cd $$incdir && $(MAKE) all) \
	done;
endef

define build-all-test-to-directory
	for incdir in $(1); \
	do \
		(cd $$incdir); \
		if [ -ge make all ]; then \
			exit 1 \
		fi; \
	done;
endef

define build-copy-to-target
@ cp -a $(LOCAL_TARGET)*  $(1) -rf
endef

define build_output_dir
@	rm $(1) -rf
@	mkdir $(1)
@	rm $(2) -rf
@	mkdir $(2)
endef

define build_delete_suffix
@	rm $(1)$(2) -rf
endef

define build-debug-print
	@echo " ---------Default Debug ----------------"
	@echo " CFLAGS= $(CFLAGS)"
	@echo " INCLUDE= $(INCLUDE)"
	@echo " LDFLAGS= $(LDFLAGS)"
	@echo " LIBS= $(LIBS)"
	@echo " SOURCE_FILES= $(SOURCE_FILES)"
	@echo " OBJECTS= $(OBJECTS)"
	@echo " LOCAL_CFLAGS= $(LOCAL_CFLAGS)"
	@echo " LOCAL_INCLUDE_HEADERS= $(LOCAL_INCLUDE_HEADERS)"
	@echo " LOCAL_LDFLAGS= $(LOCAL_LDFLAGS)"
	@echo " LOCAL_STATIC_LIB= $(LOCAL_STATIC_LIB)"
	@echo " LOCAL_SRC_FILES= $(LOCAL_SRC_FILES)"
	@echo " LOCAL_OBJECTS= $(LOCAL_OBJECTS)"
	@echo " LOCAL_VERSION= $(LOCAL_VERSION)"
	@echo " LOCAL_TARGET= $(LOCAL_TARGET)"
	@echo " LOCAL_COPY_LIB= $(LOCAL_COPY_LIB)"
	@echo " LOCAL_PATH = $(LOCAL_PATH)"
endef


#!/bin/sh

#######################################################
## copy proprietary library
#######################################################
COPY_SHARED_LIB=" \
build/lib/libOMX.TCC.DxB.base.so \
build/lib/libOMX.TCC.LinuxTV.Demux.so \
build/lib/libTCC_CDK_DXB_LIB.so \
build/lib/libTCCDxBInterface.so \
build/lib/libdxbsc.so \
build/lib/libdxbcipher.so \
build/lib/libisdbt.so 
"

mkdir -p proprietary/tcc803x
cp -arf ${COPY_SHARED_LIB} proprietary/tcc803x/
arm-telechips-linux-gnueabi-strip -S proprietary/tcc803x/*.so
#######################################################
## clean
#######################################################
make clean

#######################################################
## move to root folder
#######################################################
cd ..

#######################################################
## strip static library
#######################################################
find . -name "*.a" | xargs arm-telechips-linux-gnueabi-strip -S

#######################################################
## changed basic_path.mk
#######################################################
rm -rf linux/build/basic_path.mk
mv linux/build/basic_path.mk_ linux/build/basic_path.mk

########################################################
## remove proprietary codes
########################################################
rm -rf framework/dxb_interface/include/internal
rm -rf framework/dxb_interface/lib
rm -rf framework/dxb_interface/src
rm -rf framework/dxb_interface/Makefile

#rm -rf framework/dxb_components/
#rm -rf framework/dxb_components/decoders/dxb_cdk_library
rm -f framework/dxb_components/decoders/dxb_cdk_library/Makefile
mv framework/dxb_components/decoders/dxb_cdk_library/Makefile_ framework/dxb_components/decoders/dxb_cdk_library/Makefile
find framework/dxb_components/decoders/dxb_cdk_library/cdk/ -name *.c | xargs rm -rf
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/aacenc



rm -f  framework/dxb_components/decoders/omx_dxb_audiodec_component/Makefile
mv framework/dxb_components/decoders/omx_dxb_audiodec_component/Makefile_ framework/dxb_components/decoders/omx_dxb_audiodec_component/Makefile


rm -rf framework/dxb_components/demuxers/omx_linuxtv_demux_component
rm -rf framework/dxb_components/demuxers/omx_section_component
rm -rf framework/dxb_components/demuxers/common
rm -rf framework/dxb_components/demuxers
rm -rf framework/dxb_components/omx_engine/dxb_omx_base/src
rm -f  framework/dxb_components/omx_engine/dxb_omx_base/Makefile
rm -f  framework/dxb_components/Makefile
mv     framework/dxb_components/Makefile_ framework/dxb_components/Makefile


rm -rf middleware/common/dxb_sc
rm -rf middleware/common/dxb_swmulti2
rm -rf middleware/common/dxb_png
rm -rf middleware/common/dxb_cipher/src
rm -f  middleware/common/dxb_cipher/Makefile
rm -f  middleware/common/dxb_tsdecoder/lib/libTSDecoder.a
rm -f  middleware/common/dxb_tsdecoder/lib/mk_ln.sh


######################## ISDBT #############################
rm -rf middleware/lib_isdbt/include/*.h
rm -rf middleware/lib_isdbt/include/manager
rm -rf middleware/lib_isdbt/include/middleware
rm -rf middleware/lib_isdbt/lib
rm -rf middleware/lib_isdbt/src/manager
rm -rf middleware/lib_isdbt/src/middleware
rm -rf middleware/lib_isdbt/src/player
rm -rf middleware/lib_isdbt/src/*.*
rm -rf middleware/lib_isdbt/Android.mk
rm -f  middleware/lib_isdbt/Makefile
mv     middleware/lib_isdbt/Makefile_ middleware/lib_isdbt/Makefile
############################################################

##############################
# remove open source lib 
############################## 
rm -rf linux/library/freetype
rm -rf linux/library/libpng
rm -rf linux/library/zlib
rm -rf linux/system/include/alsa
rm -rf linux/system/include/sqlite
rm -rf linux/system/libasound.so*
rm -rf linux/system/libft2.so
rm -rf linux/system/libpng.so
rm -rf linux/system/libsqlite3.so*
rm -rf linux/system/libz.so
rm -rf middleware/common/dxb_font/include/freetype
rm -rf middleware/common/dxb_font/include/ft2build.h

cd linux

rm -rf doc
#rm -f doc/*.doc
rm -f release*.sh


#!/bin/sh

#######################################################
## copy proprietary library
#######################################################
COPY_SHARED_LIB=" \
build/lib/libTCC_CDK_DXB_LIB.so \
build/lib/libisdbt_trmp.so \
"

mkdir -p proprietary/tcc897x
cp -arf ${COPY_SHARED_LIB} proprietary/tcc897x/
arm-none-linux-gnueabi-strip -S proprietary/tcc897x/*.so

#######################################################
## clean
#######################################################
make clean

#######################################################
## move to root folder
#######################################################
cd ..

#######################################################
## remove all Android.mk
#######################################################
find . -name Android.mk -type f -exec rm -rf {} \;

#######################################################
## remove folders and files related with only android
#######################################################
ANDROID_PATH="application/ proprietary/ service/ *.sh *.mk"
rm -rf ${ANDROID_PATH} 

#######################################################
## strip static library
#######################################################
find . -name "*.a" | xargs arm-none-linux-gnueabi-strip -S

########################################################
## remove audio codec for android
########################################################
CDK_AUDIO_LIB_PATH="framework/dxb_components/decoders/dxb_cdk_library/audio_codec"
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/aacenc
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/ac3dec
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/bsacdec
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/ddpdec
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/mp2dec
rm -rf framework/dxb_components/decoders/dxb_cdk_library/audio_codec/mp3enc
find ${CDK_AUDIO_LIB_PATH}/ -iname *_ANDROID_*.a -type f -exec rm -f {} \;


########################################################
## remove proprietary codes
########################################################
rm -rf middleware/lib_dvbt/lib/Teletext_v2
rm -rf middleware/lib_isdbt/lib/isdbt_trmp/client/src
rm -rf middleware/lib_isdbt/lib/isdbt_trmp/client/tee
rm -rf middleware/lib_isdbt/lib/isdbt_trmp/include/hciph
rm -rf middleware/lib_isdbt/lib/isdbt_trmp/include/tee
rm -rf middleware/lib_isdbt/lib/isdbt_trmp/Android.mk
rm -rf middleware/lib_isdbt/lib/isdbt_trmp/Makefile

#rm framework/dxb_components/decoders/dxb_cdk_library/Android.mk
find framework/dxb_components/decoders/dxb_cdk_library/cdk/ -name *.c | xargs rm

rm -rf middleware/lib_atsc
rm -rf middleware/lib_dvbt

######################## TDMB ###############################
#rm -rf middleware/lib_tdmb

##############################
# remove open source lib. 
# at linux, dxb use common library file at ADT system. (libasound, sqlite, libfreetype(libpng,libz) 
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
rm -f release_source.sh
#rm -f release.sh


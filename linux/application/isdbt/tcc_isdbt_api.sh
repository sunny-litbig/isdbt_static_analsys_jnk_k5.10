#!/bin/sh
ps -ef | grep Launcher | awk '{print $2}' | xargs kill

## make symblic link for device drivers 
ln -s /dev/dvb/adapter0/demux0 /dev/dvb0.demux0
ln -s /dev/dvb/adapter0/demux1 /dev/dvb0.demux1
ln -s /dev/dvb/adapter0/dvr0   /dev/dvb0.dvr0 
ln -s /dev/dvb/adapter0/dvr1   /dev/dvb0.dvr1 
ln -s /dev/dvb/adapter0/net0   /dev/dvb0.net0 
ln -s /dev/dvb/adapter0/net1   /dev/dvb0.net1
mkdir -p /dev/graphics
ln -s /dev/fb0 /dev/graphics/fb0
ln -s /dev/tcc_vsync /dev/tcc_vsync0

#EXEC_PATH="/nand1"
#EXEC_PATH="/nand2"
#EXEC_PATH="/mnt/SD1p1"
#EXEC_PATH="/run/media/ndda9"
#EXEC_PATH="/run/media/sda1"
EXEC_PATH="/opt/data"

echo ==================================
echo Exec Location = $EXEC_PATH
echo ==================================

DXB_APP_PATH=${EXEC_PATH}"/tccdxb/bin"
DXB_LIB_PATH=${EXEC_PATH}"/tccdxb/lib"

## set environment variable
export DXB_FONT_PATH=${EXEC_PATH}"/tccdxb/font"
export DXB_DATA_PATH=${EXEC_PATH}"/tccdxb_data"
mkdir -p ${DXB_DATA_PATH}
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${DXB_LIB_PATH}

## predefine properties
DXB_PROPERTY_PATH="/tmp/property/"
mkdir -p ${DXB_PROPERTY_PATH}
echo 1 > ${DXB_PROPERTY_PATH}/tcc.dxb.linuxtv.enable
echo 1 > ${DXB_PROPERTY_PATH}/tcc.video.deinterlace.support

## display PATH
echo DXB_APP_PATH=${DXB_APP_PATH}                      
echo DXB_LIB_PATH=${DXB_LIB_PATH}                      
echo DXB_FONT_PATH=${DXB_FONT_PATH}                    
echo DXB_PROPERTY_PATH=${DXB_PROPERTY_PATH}            
echo LD_LIBRARY_PATH=${LD_LIBRARY_PATH}                

## excute APP
${DXB_APP_PATH}/tcc_dxb_isdbt_app

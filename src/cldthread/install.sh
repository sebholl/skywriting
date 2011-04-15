#!/bin/bash

# cloud threading packages
KERNELVERSION=`uname -r`

KERNELHEADERSPACKAGE="linux-headers-$KERNELVERSION"
KERNELSOURCEPACKAGE="linux-ec2-source-${KERNELVERSION%%-*}"

PACKAGES="libcurl4-openssl-dev libpnglite-dev doxygen graphviz gcc g++ texlive-font-utils libcr-dev"

apt-get install gawk $KERNELSOURCEPACKAGE

cd /usr/src/

tar xjvf $KERNELSOURCEPACKAGE.tar.bz2

cd $KERNELSOURCEPACKAGE

cp /boot/config-$KERNELVERSION .config

make modules_prepare

apt-get install $KERNELHEADERSPACKAGE

cp -R /usr/src/linux-ec2-source-2.6.32 /lib/modules/2.6.32-305-ec2/source

apt-get install $KERNELHEADERSPACKAGE

apt-get install blcr-dkms
modprobe -i blcr

apt-get install $PACKAGES

cd ${0%/*}

make all



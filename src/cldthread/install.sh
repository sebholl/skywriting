#!/bin/bash

# cloud threading packages
KERNELVERSION=`uname -r`

KERNELHEADERSPACKAGE="linux-headers-$KERNELVERSION"
KERNELSOURCEPACKAGE="linux-ec2-source-${KERNELVERSION%%-*}"

PACKAGES="libcurl4-openssl-dev libpnglite-dev doxygen graphviz gcc g++ texlive-font-utils libcr-dev"

apt-get -qq -y update 1>&2 2>/dev/null
apt-get -qq -y install gawk $KERNELSOURCEPACKAGE

pushd /usr/src/

tar xjvf $KERNELSOURCEPACKAGE.tar.bz2 1>/dev/null

cd $KERNELSOURCEPACKAGE

cp /boot/config-$KERNELVERSION .config

make modules_prepare < echo "\r\n"

apt-get -qq -y  install $KERNELHEADERSPACKAGE

cp -R /usr/src/$KERNELHEADERSPACKAGE /lib/modules/$KERNELVERSION/source

apt-get -qq -y  install $KERNELHEADERSPACKAGE

apt-get -qq -y  install blcr-dkms
modprobe -i blcr

apt-get -qq -y  install $PACKAGES

popd
cd ${0%/*}

make all



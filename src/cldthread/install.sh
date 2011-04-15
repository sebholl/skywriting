#!/bin/bash

# cloud threading packages
KERNELVERSION=`uname -r`

KERNELHEADERSPACKAGE="linux-headers-$KERNELVERSION"
KERNELSOURCEPACKAGE="linux-ec2-source-${KERNELVERSION%%-*}"

PACKAGES="libcurl4-openssl-dev libpnglite-dev doxygen graphviz gcc g++ texlive-font-utils libcr-dev"

echo "Updating package information..."

apt-get -qq -y update 1>&2 2>/dev/null

echo "Retrieving kernel source..."

apt-get -qq -y install gawk $KERNELSOURCEPACKAGE 1>/dev/null

pushd /usr/src/

echo "Extracting source from tar..."

tar xjvf $KERNELSOURCEPACKAGE.tar.bz2 1>/dev/null

cd $KERNELSOURCEPACKAGE

echo "Copying config file from current instance..."

cp -f /boot/config-$KERNELVERSION .config

# add this option if it's not listed, otherwise module_prepare will
# request user input
if ! grep -q "CONFIG_IRQ_TIME_ACCOUNTING" .config
  then echo "CONFIG_IRQ_TIME_ACCOUNTING=n" >> .config
fi

echo "Configuring kernel source..."

make -s modules_prepare

echo "Installing header package..."

apt-get -qq -y  install $KERNELHEADERSPACKAGE 1>/dev/null

cp -R /usr/src/$KERNELHEADERSPACKAGE /lib/modules/$KERNELVERSION/source

apt-get -qq -y  install $KERNELHEADERSPACKAGE 1>/dev/null

echo "Installing BLCR-DKMS package..."

apt-get -qq -y  install blcr-dkms 1>/dev/null

echo "Installing BLCR kernel module..."

modprobe -i blcr

echo "Installing libCloudThread packages..."

apt-get -qq -y  install $PACKAGES 1>/dev/null

popd
cd ${0%/*}

echo "Running 'make all' on libCloudThreads..."

make -s all

echo "Finished"

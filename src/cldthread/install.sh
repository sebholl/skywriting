#!/bin/bash

# cloud threading packages
KERNELVERSION=`uname -r`

KERNELHEADERSPACKAGE="linux-headers-$KERNELVERSION"
KERNELSOURCEPACKAGE="linux-ec2-source-${KERNELVERSION%%-*}"

PACKAGES="libcurl4-openssl-dev libpnglite-dev doxygen graphviz gcc g++ texlive-font-utils libcr-dev"

echo "Updating package information..."

apt-get -qq -y update 1>&2 2>/dev/null

echo "Retrieving kernel source..."

apt-get -qq -y install gawk $KERNELSOURCEPACKAGE

echo "Extracting source from tar..."

pushd /usr/src/

tar xjvf $KERNELSOURCEPACKAGE.tar.bz2 1>/dev/null

cd $KERNELSOURCEPACKAGE

echo "Copying config file from current instance..."

cp -f /boot/config-$KERNELVERSION .config

# add this option if it's not listed, otherwise module_prepare will
# request user input
if ! grep -q "IRQ_TIME_ACCOUNTING" .config
  then echo "IRQ_TIME_ACCOUNTING=n" >> .config
fi

echo "Configuring kernel source..."

make modules_prepare

echo "Installing header package..."

apt-get -qq -y  install $KERNELHEADERSPACKAGE

cp -R /usr/src/$KERNELHEADERSPACKAGE /lib/modules/$KERNELVERSION/source

apt-get -qq -y  install $KERNELHEADERSPACKAGE

echo "Installing BLCR-DKMS package..."

apt-get -qq -y  install blcr-dkms

echo "Installing BLCR kernel module..."

modprobe -i blcr

echo "Installing libCloudThread packages..."

apt-get -qq -y  install $PACKAGES

echo "Running 'make all' on libCloudThreads..."

popd
cd ${0%/*}

make all

echo "Finished"

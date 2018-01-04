#!/bin/bash
#
# Build script to generate dlib Debian package.
#
# Copyright: 2017 Ditto Technologies. All Rights Reserved.
# Author: Frankie Li, Daran He

# We need a non-root path for make install and make package.
INSTALL_DIR=${PWD}/install
mkdir -p ${INSTALL_DIR}

BUILD_DIR=build
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ../dlib \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
	-DCPACK_GENERATOR="DEB" \
	-DCPACK_BINARY_DEB="ON" \
	-DCPACK_DEBIAN_PACKAGE_SHLIBDEPS="ON" \
	-DCPACK_PACKAGE_CONTACT="eng@ditto.com"

make -j$(nproc) install package

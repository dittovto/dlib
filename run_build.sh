#!/bin/bash
#
# Build script to generate dlib Debian package.
#
# Copyright: 2017 Ditto Technologies. All Rights Reserved.
# Author: Frankie Li, Daran He

BUILD_DIR=build
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ../dlib \
	-DCMAKE_BUILD_TYPE=Release \
	-DCPACK_PACKAGING_INSTALL_PREFIX="/usr/local" \
	-DCPACK_GENERATOR="DEB" \
	-DCPACK_BINARY_DEB="ON" \
	-DCPACK_DEBIAN_PACKAGE_SHLIBDEPS="ON" \
	-DCPACK_PACKAGE_CONTACT="eng@ditto.com"

make -j$(nproc)

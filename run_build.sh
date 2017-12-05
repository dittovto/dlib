#!/bin/bash
#
# Build script to generate dlib Debian package.


DLIB_BLD=build
mkdir -p ${DLIB_BLD}
cd ${DLIB_BLD}
cmake ../dlib -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR="DEB" -DCPACK_BINARY_DEB="ON" -DCPACK_DEBIAN_PACKAGE_SHLIBDEPS="ON" -DCPACK_PACKAGE_CONTACT="eng@ditto.com"

make -j 4
make package

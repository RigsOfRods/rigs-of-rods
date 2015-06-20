#!/bin/sh
source ./tools/travis/linux/config

cmake \
-DROR_USE_MYGUI="TRUE" \
-DROR_USE_OPENAL="TRUE" \
-DROR_USE_SOCKETW="TRUE" \
-DCMAKE_CXX_FLAGS="-O0" \
.

make $ROR_MAKEOPTS

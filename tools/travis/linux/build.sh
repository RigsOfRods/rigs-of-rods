#!/bin/sh
set -eu

mkdir -p ./build
cd ./build

cmake -DCMAKE_PREFIX_PATH=$DEPS_INSTALL_DIR \
-DROR_USE_MYGUI="TRUE" \
-DROR_USE_OPENAL="TRUE" \
-DROR_USE_SOCKETW="TRUE" \
-DROR_USE_PAGED="TRUE" \
-DROR_USE_CAELUM="TRUE" \
-DROR_USE_ANGELSCRIPT="TRUE" \
-DCMAKE_CXX_FLAGS="-O0 -pipe" \
-DBUILD_DOC_DOXYGEN="TRUE" \
..

make -j2

# Generate documentation (except for pull requests)
if [ "${TRAVIS_PULL_REQUEST}" = "false" ]
then
  # Supress output from doxygen in order not to exceed the log size limit of Travis
  make doc-doxygen > /dev/null 2>&1
fi

cd ..

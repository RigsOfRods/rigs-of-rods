#!/bin/sh
#
# Build script for travis-ci.org builds to handle compiles 
# and static analysis when ANALYZE=true.
#
if [ $ANALYZE = "true" ]; then
        cppcheck --version
        cppcheck \
          --template "{file}({line}): {severity} ({id}): {message}" \
          --enable=information --enable=performance \
          --force --std=c++11 -j2 . 2> cppcheck.txt
        if [ -s cppcheck.txt ]; then
            cat cppcheck.txt
            exit 1
        fi
else # no static analysis, do regular build
    ./tools/travis/linux/dependencies.sh
    ./tools/travis/linux/build.sh
fi
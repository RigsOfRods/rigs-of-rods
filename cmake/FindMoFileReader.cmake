# The MIT License (MIT)
#
# Copyright (c) 2015 Fabian Killus
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Find MoFileReader
# -----------------
#
# Find MoFileReader include directories and libraries.
# This module will set the following variables:
#
# * MoFileReader_FOUND         - True if MoFileReader is found
# * MoFileReader_INCLUDE_DIRS  - The include directory
# * MoFileReader_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * MoFileReader::MoFileReader
#

find_path(MoFileReader_INCLUDE_DIR moFileReader.h)
find_library(MoFileReader_LIBRARY
        NAMES MoFileReader.static moFileReader
        PATH_SUFFIXES Debug Release RelWithDebInfo
        )

set(MoFileReader_INCLUDE_DIRS ${MoFileReader_INCLUDE_DIR})
set(MoFileReader_LIBRARIES ${MoFileReader_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MoFileReader FOUND_VAR MoFileReader_FOUND
        REQUIRED_VARS MoFileReader_INCLUDE_DIRS MoFileReader_LIBRARIES
        )

if (MoFileReader_FOUND)
    add_library(MoFileReader::MoFileReader INTERFACE IMPORTED)
    set_target_properties(MoFileReader::MoFileReader PROPERTIES
            INTERFACE_LINK_LIBRARIES "${MoFileReader_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${MoFileReader_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(MoFileReader_INCLUDE_DIR MoFileReader_LIBRARY)

# The MIT License (MIT)
#
# Copyright (c) 2021 Edgar (Edgar@AnotherFoxGuy.com)
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

# Find OIS
# -----------
#
# Find OIS include directories and libraries.
# This module will set the following variables:
#
# * OIS_FOUND         - True if OIS is found
# * OIS_INCLUDE_DIRS  - The include directory
# * OIS_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * OIS::OIS
#

find_path(OIS_INCLUDE_DIR OIS.h PATH_SUFFIXES OIS)
find_library(OIS_LIBRARY OIS)

set(OIS_INCLUDE_DIRS ${OIS_INCLUDE_DIR})
set(OIS_LIBRARIES ${OIS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OIS FOUND_VAR OIS_FOUND
        REQUIRED_VARS OIS_INCLUDE_DIRS OIS_LIBRARIES
        )

if (OIS_FOUND)
    add_library(OIS::OIS INTERFACE IMPORTED)
    set_target_properties(OIS::OIS PROPERTIES
            INTERFACE_LINK_LIBRARIES "${OIS_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${OIS_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(OIS_INCLUDE_DIR OIS_LIBRARY)

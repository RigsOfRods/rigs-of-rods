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

# Find SocketW
# ------------
#
# Find SocketW include directories and libraries.
# This module will set the following variables:
#
# * SocketW_FOUND         - True if SocketW is found
# * SocketW_INCLUDE_DIRS  - The include directory
# * SocketW_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * SocketW::SocketW
#

find_path(SocketW_INCLUDE_DIR SocketW.h)
find_library(SocketW_LIBRARY SocketW)

set(SocketW_INCLUDE_DIRS ${SocketW_INCLUDE_DIR})
set(SocketW_LIBRARIES ${SocketW_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SocketW FOUND_VAR SocketW_FOUND
        REQUIRED_VARS SocketW_INCLUDE_DIRS SocketW_LIBRARIES
        )

if (SocketW_FOUND)
    add_library(SocketW::SocketW INTERFACE IMPORTED)
    set_target_properties(SocketW::SocketW PROPERTIES
            INTERFACE_LINK_LIBRARIES "${SocketW_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${SocketW_INCLUDE_DIRS}"
            )
    if (WIN32)
        set_property(TARGET SocketW::SocketW APPEND PROPERTY INTERFACE_LINK_LIBRARIES ws2_32)
    endif ()
endif ()

mark_as_advanced(SocketW_INCLUDE_DIR SocketW_LIBRARY)

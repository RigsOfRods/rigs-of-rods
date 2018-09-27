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

# Find Caelum
# -----------
#
# Find Caelum include directories and libraries.
# This module will set the following variables:
#
# * Caelum_FOUND         - True if Caelum is found
# * Caelum_INCLUDE_DIRS  - The include directory
# * Caelum_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * Caelum::Caelum
#

find_path(Caelum_INCLUDE_DIR Caelum.h PATH_SUFFIXES Caelum)
find_library(Caelum_LIBRARY Caelum)

set(Caelum_INCLUDE_DIRS ${Caelum_INCLUDE_DIR})
set(Caelum_LIBRARIES ${Caelum_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Caelum FOUND_VAR Caelum_FOUND
        REQUIRED_VARS Caelum_INCLUDE_DIRS Caelum_LIBRARIES
        )

if (Caelum_FOUND)
    add_library(Caelum::Caelum INTERFACE IMPORTED)
    set_target_properties(Caelum::Caelum PROPERTIES
            INTERFACE_LINK_LIBRARIES "${Caelum_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Caelum_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(Caelum_INCLUDE_DIR Caelum_LIBRARY)

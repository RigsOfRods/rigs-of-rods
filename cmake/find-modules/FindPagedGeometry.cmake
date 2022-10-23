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

# Find PagedGeometry
# -----------
#
# Find PagedGeometry include directories and libraries.
# This module will set the following variables:
#
# * PagedGeometry_FOUND         - True if PagedGeometry is found
# * PagedGeometry_INCLUDE_DIRS  - The include directory
# * PagedGeometry_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * PagedGeometry::PagedGeometry
#

find_path(PagedGeometry_INCLUDE_DIR PagedGeometry.h PATH_SUFFIXES PagedGeometry)
find_library(PagedGeometry_LIBRARY PagedGeometry)

set(PagedGeometry_INCLUDE_DIRS ${PagedGeometry_INCLUDE_DIR})
set(PagedGeometry_LIBRARIES ${PagedGeometry_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PagedGeometry FOUND_VAR PagedGeometry_FOUND
        REQUIRED_VARS PagedGeometry_INCLUDE_DIRS PagedGeometry_LIBRARIES
        )

if (PagedGeometry_FOUND)
    add_library(PagedGeometry::PagedGeometry INTERFACE IMPORTED)
    set_target_properties(PagedGeometry::PagedGeometry PROPERTIES
            INTERFACE_LINK_LIBRARIES "${PagedGeometry_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${PagedGeometry_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(PagedGeometry_INCLUDE_DIR PagedGeometry_LIBRARY)

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

# Find JsonCpp
# -----------------
#
# Find JsonCpp include directories and libraries.
# This module will set the following variables:
#
# * JsonCpp_FOUND         - True if JsonCpp is found
# * JsonCpp_INCLUDE_DIRS  - The include directory
# * JsonCpp_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * JsonCpp::JsonCpp
#

find_path( JsonCpp_INCLUDE_DIR json.h PATH_SUFFIXES json )
find_library( JsonCpp_LIBRARY jsoncpp )

set(JsonCpp_INCLUDE_DIRS ${JsonCpp_INCLUDE_DIR})
set(JsonCpp_LIBRARY ${JsonCpp_LIBRARY})

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( JsonCpp FOUND_VAR JsonCpp_FOUND
  REQUIRED_VARS JsonCpp_INCLUDE_DIRS JsonCpp_LIBRARY
)

if( JsonCpp_FOUND )
  add_library( JsonCpp::JsonCpp INTERFACE IMPORTED )
  set_target_properties( JsonCpp::JsonCpp PROPERTIES
    INTERFACE_LINK_LIBRARIES "${JsonCpp_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${JsonCpp_INCLUDE_DIRS}"
  )
endif()

mark_as_advanced( JsonCpp_INCLUDE_DIR JsonCpp_LIBRARY )

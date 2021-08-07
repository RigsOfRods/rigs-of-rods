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

# Find RapidJSON
# -----------
#
# Find RapidJSON include directories and libraries.
# This module will set the following variables:
#
# * RapidJSON_FOUND         - True if RapidJSON is found
# * RapidJSON_INCLUDE_DIRS  - The include directory
#
# In addition the following imported targets are defined:
#
# * RapidJSON::RapidJSON
#

find_path(RapidJSON_INCLUDE_DIR rapidjson.h PATH_SUFFIXES rapidjson)

set(RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON FOUND_VAR RapidJSON_FOUND
        REQUIRED_VARS RapidJSON_INCLUDE_DIRS
        )

if (RapidJSON_FOUND)
    add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    set_target_properties(RapidJSON::RapidJSON PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(RapidJSON_INCLUDE_DIR)

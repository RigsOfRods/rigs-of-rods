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

# Find discord_rpc
# -----------
#
# Find discord_rpc include directories and libraries.
# This module will set the following variables:
#
# * discord_rpc_FOUND         - True if discord_rpc is found
# * discord_rpc_INCLUDE_DIRS  - The include directory
# * discord_rpc_LIBRARIES     - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * discord-rpc::discord-rpc
#

find_package(discord-rpc QUIET CONFIG)

if (TARGET discord-rpc::discord-rpc)
    set(discord_rpc_FOUND TRUE)
    return()
endif ()

find_path(discord_rpc_INCLUDE_DIR discord_rpc.h)
find_library(discord_rpc_LIBRARY discord-rpc)

set(discord_rpc_INCLUDE_DIRS ${discord_rpc_INCLUDE_DIR})
set(discord_rpc_LIBRARIES ${discord_rpc_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(discord_rpc FOUND_VAR discord_rpc_FOUND
        REQUIRED_VARS discord_rpc_INCLUDE_DIRS discord_rpc_LIBRARIES
        )

if (discord_rpc_FOUND)
    add_library(discord-rpc::discord-rpc INTERFACE IMPORTED)
    set_target_properties(discord-rpc::discord-rpc PROPERTIES
            INTERFACE_LINK_LIBRARIES "${discord_rpc_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${discord_rpc_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(discord_rpc_INCLUDE_DIR discord_rpc_LIBRARY)

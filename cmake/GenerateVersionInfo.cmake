# The MIT License (MIT)
#
# Copyright (c) 2016 Fabian Killus
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
# associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
# NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# GenerateVersionInfo
# ===================
#
# This script generates an output file from a given input template by filling in the version
# information provided in the following variables
#
#  * BUILD_DEV_VERSION
#  * BUILD_CUSTOM_VERSION
#  * CUSTOM_VERSION
#
# The git executable is expected to be specified in
#
#  * GIT_EXECUTABLE
#
# The input template and the output file destination are defined via
#
#  * VERSION_FILE_INPUT
#  * VERSION_FILE_OUTPUT
#
# This implementation was inspired by
# https://github.com/minetest/minetest/blob/master/cmake/Modules/GenerateVersion.cmake

macro(version_str_to_ints version year month suffix)
    string(REGEX REPLACE "([0-9]+)\\.[0-9]+[^\n\r]*" "\\1" ${year} ${version})
    string(REGEX REPLACE "[0-9]+\\.([0-9]+)[^\n\r]*" "\\1" ${month} ${version})
    string(REGEX REPLACE "[0-9]+\\.[0-9]+([^\n\r]*)" "\\1" ${suffix} ${version})
endmacro(version_str_to_ints)

# Default variables
string(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)
string(TIMESTAMP BUILD_TIME "%H:%M" UTC)

# Define a suffix to append to the version string in case of a development build (as opposed to
# an official release). This suffix contains additional information gathered from the git VCS.
if (BUILD_CUSTOM_VERSION)
    version_str_to_ints(${CUSTOM_VERSION} VERSION_YEAR VERSION_MONTH VERSION_SUFFIX)
elseif (BUILD_DEV_VERSION)
    string(TIMESTAMP VERSION_YEAR "%Y" UTC)
    string(TIMESTAMP VERSION_MONTH "%m" UTC)

    # Check if we are inside an actual git repository
    if (GIT_EXECUTABLE)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE GIT_REPOSITORY_NOT_FOUND
                ERROR_QUIET
        )
    endif ()

    if (GIT_EXECUTABLE AND NOT GIT_REPOSITORY_NOT_FOUND)
        # Find hash of the latest git commit
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(VERSION_SUFFIX ${VERSION_SUFFIX}-${GIT_HASH})

        # Check if the code has been modified since the latest commit
        execute_process(
                COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                RESULT_VARIABLE IS_DIRTY
        )

        # Define the version string suffix containing
        #   * 'dev' to indicate development build
        #   * sha1sum of latest commit
        #   * 'dirty' flag in case of local modifications
        set(VERSION_SUFFIX "-dev-${GIT_HASH}")
        if (IS_DIRTY)
            set(VERSION_SUFFIX "${VERSION_SUFFIX}-dirty")
        endif ()

    else ()
        # Use the following version string suffix if git is not available or not inside a valid
        # git repository
        set(VERSION_SUFFIX "-dev-without-git")
    endif ()
endif ()

# Fill in the actual version information in the provided template
configure_file(${VERSION_FILE_INPUT} ${VERSION_FILE_OUTPUT} @ONLY)

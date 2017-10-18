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
#  * VERSION_MAJOR
#  * VERSION_MINOR
#  * VERSION_PATCH
#  * VERSION_TWEAK
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

MACRO(VERSION_STR_TO_INTS version major minor patch tweak suffix)
    STRING(REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+.[0-9]+.[^\n\r]+" "\\1" ${major} ${version})
    STRING(REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+.[0-9]+.[^\n\r]+" "\\1" ${minor} ${version})
    STRING(REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+).[0-9]+.[^\n\r]+" "\\1" ${patch} ${version})
    STRING(REGEX REPLACE "[0-9]+.[0-9]+.[0-9]+.([0-9]+).[^\n\r]+" "\\1" ${tweak} ${version})
    STRING(REGEX REPLACE "[0-9]+.[0-9]+.[0-9]+.[0-9]+.([^\n\r]+)" "\\1" ${suffix} ${version})
ENDMACRO(VERSION_STR_TO_INTS)

# Define a suffix to append to the version string in case of a development build (as opposed to
# an official release). This suffix contains additional information gathered from the git VCS.
if (BUILD_CUSTOM_VERSION)
    VERSION_STR_TO_INTS(${CUSTOM_VERSION} VERSION_MAJOR VERSION_MINOR VERSION_PATCH VERSION_TWEAK VERSION_SUFFIX)
    set(VERSION_SUFFIX "-${VERSION_SUFFIX}")
elseif (BUILD_DEV_VERSION)
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

# This whole if and else can replaced, when using cmake >= 3.8 (as it then uses environment variable SOURCE_DATE_EPOCH), with this:
# STRING(TIMESTAMP BUILD_DATE "%b %d %Y" UTC)
# STRING(TIMESTAMP BUILD_TIME "%H:%M" UTC)
if (DEFINED ENV{SOURCE_DATE_EPOCH})
    execute_process(
            COMMAND "date" "-u" "-d" "@$ENV{SOURCE_DATE_EPOCH}" "+%b %e %Y"
            OUTPUT_VARIABLE BUILD_DATE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
            COMMAND "date" "-u" "-d" "@$ENV{SOURCE_DATE_EPOCH}" "+%R"
            OUTPUT_VARIABLE BUILD_TIME
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    string(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC) # more correct would be "%b %d %Y" but this is only supported from cmake >= 3.7
    string(TIMESTAMP BUILD_TIME "%H:%M" UTC)
endif ()

# Fill in the actual version information in the provided template
configure_file(${VERSION_FILE_INPUT} ${VERSION_FILE_OUTPUT} @ONLY)

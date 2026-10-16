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
# Evaluate one version snapshot, then configure all requested outputs with copy-if-different.
# BUILD_CUSTOM_VERSION/CUSTOM_VERSION take precedence over BUILD_DEV_VERSION. The build target
# passes custom text in CUSTOM_VERSION_FILE so semicolons and generator expressions stay literal.
# PROJECT_VERSION and VERSION_SOURCE_DIR are explicit build-target inputs; the latter is the Git cwd.
# VERSION_INFO_INPUT_DIR/OUTPUT_DIR select the production cpp, header and JSON templates together.
# The original VERSION_FILE_INPUT/OUTPUT interface remains available for single-template callers.
# Originally inspired by https://github.com/minetest/minetest/blob/master/cmake/Modules/GenerateVersion.cmake

function(normalize_version_component value output)
    string(REGEX REPLACE "^0+" "" value "${value}")
    if(value STREQUAL "")
        set(value "0")
    endif()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

# Parse a numeric year.month prefix without interpreting or discarding the custom suffix.
function(version_str_to_ints version year month suffix)
    if(NOT "${version}" MATCHES "^([0-9]+)\\.([0-9]+)")
        message(FATAL_ERROR "Version must start with two decimal components (year.month)")
    endif()
    set(${year} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(${month} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    string(LENGTH "${CMAKE_MATCH_0}" prefix_length)
    string(SUBSTRING "${version}" ${prefix_length} -1 remainder)
    set(${suffix} "${remainder}" PARENT_SCOPE)
endfunction()

# CMake 3.16 has no string(JSON). Escape every representable JSON control character, and use
# fixed-width octal in C++ so an adjacent digit cannot extend an escape. CMake cannot hold NUL.
function(escape_version_string value format output)
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    set(hex_digits "0123456789abcdef")
    foreach(code RANGE 1 31)
        string(ASCII ${code} character)
        if(format STREQUAL "JSON")
            math(EXPR high "${code} / 16")
            math(EXPR low "${code} % 16")
            string(SUBSTRING "${hex_digits}" ${high} 1 high)
            string(SUBSTRING "${hex_digits}" ${low} 1 low)
            set(escaped "\\u00${high}${low}")
        else()
            math(EXPR high "${code} / 8")
            math(EXPR low "${code} % 8")
            set(escaped "\\0${high}${low}")
        endif()
        string(REPLACE "${character}" "${escaped}" value "${value}")
    endforeach()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

if(DEFINED CUSTOM_VERSION_FILE AND BUILD_CUSTOM_VERSION)
    file(READ "${CUSTOM_VERSION_FILE}" CUSTOM_VERSION)
endif()
if(NOT DEFINED VERSION_SOURCE_DIR)
    set(VERSION_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
endif()

# A single clock reading also respects SOURCE_DATE_EPOCH (supported by CMake TIMESTAMP).
string(TIMESTAMP build_timestamp "%Y-%m-%d;%H:%M" UTC)
list(GET build_timestamp 0 BUILD_DATE)
list(GET build_timestamp 1 BUILD_TIME)

set(FULL_REVISION_JSON "null")
set(DIRTY_JSON "null")
set(GIT_HASH "")
if(GIT_EXECUTABLE)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --verify HEAD
        WORKING_DIRECTORY "${VERSION_SOURCE_DIR}"
        RESULT_VARIABLE revision_result OUTPUT_VARIABLE full_revision
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )
    string(LENGTH "${full_revision}" revision_length)
    if(revision_result EQUAL 0 AND revision_length EQUAL 40 AND full_revision MATCHES "^[0-9a-fA-F]+$")
        set(FULL_REVISION_JSON "\"${full_revision}\"")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short "${full_revision}"
            WORKING_DIRECTORY "${VERSION_SOURCE_DIR}"
            RESULT_VARIABLE short_result OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
        )
        if(NOT short_result EQUAL 0)
            string(SUBSTRING "${full_revision}" 0 7 GIT_HASH)
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" diff-index --quiet "${full_revision}" --
            WORKING_DIRECTORY "${VERSION_SOURCE_DIR}"
            RESULT_VARIABLE dirty_result ERROR_QUIET
        )
        if(dirty_result EQUAL 0)
            set(DIRTY_JSON "false")
        elseif(dirty_result EQUAL 1)
            set(DIRTY_JSON "true")
        endif()
    endif()
endif()

set(CUSTOM_VERSION_JSON "null")
if(BUILD_CUSTOM_VERSION)
    version_str_to_ints("${CUSTOM_VERSION}" VERSION_YEAR VERSION_MONTH VERSION_SUFFIX)
    escape_version_string("${CUSTOM_VERSION}" JSON custom_json)
    set(CUSTOM_VERSION_JSON "\"${custom_json}\"")
elseif(BUILD_DEV_VERSION)
    string(SUBSTRING "${BUILD_DATE}" 0 4 VERSION_YEAR)
    string(SUBSTRING "${BUILD_DATE}" 5 2 VERSION_MONTH)
    if(GIT_HASH)
        set(VERSION_SUFFIX "-dev-${GIT_HASH}")
        if(DIRTY_JSON STREQUAL "true")
            string(APPEND VERSION_SUFFIX "-dirty")
        endif()
    else()
        set(VERSION_SUFFIX "-dev-without-git")
    endif()
else()
    # Ordinary releases previously left both components empty. Use the project release version.
    version_str_to_ints("${PROJECT_VERSION}" VERSION_YEAR VERSION_MONTH VERSION_SUFFIX)
endif()
set(SHORT_VERSION "${VERSION_YEAR}.${VERSION_MONTH}")
set(DISPLAY_VERSION "${SHORT_VERSION}${VERSION_SUFFIX}")
normalize_version_component("${VERSION_YEAR}" VERSION_RESOURCE_YEAR)
normalize_version_component("${VERSION_MONTH}" VERSION_RESOURCE_MONTH)

set(BUILD_DEV_VERSION_JSON "false")
if(BUILD_DEV_VERSION AND NOT BUILD_CUSTOM_VERSION)
    set(BUILD_DEV_VERSION_JSON "true")
endif()
foreach(field PROJECT_VERSION SHORT_VERSION DISPLAY_VERSION)
    escape_version_string("${${field}}" JSON ${field}_JSON)
endforeach()
foreach(field SHORT_VERSION DISPLAY_VERSION)
    escape_version_string("${${field}}" CXX ${field}_CXX)
endforeach()

if(DEFINED VERSION_INFO_INPUT_DIR)
    if(NOT "${PROJECT_VERSION}" MATCHES "^[0-9]+(\\.[0-9]+)*$")
        message(FATAL_ERROR "PROJECT_VERSION must contain decimal components for package metadata")
    endif()
    string(REPLACE "." ";" project_components "${PROJECT_VERSION}")
    list(LENGTH project_components component_count)
    set(APPLE_VERSION "")
    foreach(index RANGE 0 2)
        set(component "0")
        if(index LESS component_count)
            list(GET project_components ${index} component)
        endif()
        normalize_version_component("${component}" component)
        if(index GREATER 0)
            string(APPEND APPLE_VERSION ".")
        endif()
        string(APPEND APPLE_VERSION "${component}")
    endforeach()
    configure_file("${VERSION_INFO_INPUT_DIR}/RoRVersion.cpp.in"
        "${VERSION_INFO_OUTPUT_DIR}/version_info.cpp" @ONLY)
    configure_file("${VERSION_INFO_INPUT_DIR}/RoRVersionDef.h.in"
        "${VERSION_INFO_OUTPUT_DIR}/version_def.h" @ONLY)
    configure_file("${VERSION_INFO_INPUT_DIR}/package-version.json.in"
        "${VERSION_INFO_OUTPUT_DIR}/package-version.json" @ONLY)
endif()
if(DEFINED VERSION_FILE_INPUT)
    configure_file("${VERSION_FILE_INPUT}" "${VERSION_FILE_OUTPUT}" @ONLY)
endif()

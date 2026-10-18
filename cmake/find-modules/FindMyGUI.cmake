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

# Find MyGUI
# ----------
#
# Find MyGUI include directories and libraries.
# This module will set the following variables:
#
# * MyGUI_INCLUDE_DIRS  - True if MyGUI is found
# * MyGUI_LIBRARIES     - The include directory
# * MyGUI_FOUND         - The libraries to link against
#
# In addition the following imported targets are defined:
#
# * MyGUI::MyGUI
# * MyGUI::OgrePlatform
#

# Conan's CMakeDeps config is where the package prefix is recorded. The
# generated target metadata is incomplete for some versions of this legacy
# recipe, but its config file still exposes the actual package directory.
find_package(MyGUI CONFIG QUIET)

set(_MyGUI_PACKAGE_ROOTS
        "${MyGUI_PACKAGE_FOLDER_RELEASE}"
        "${MyGUI_PACKAGE_FOLDER_DEBUG}"
        "${mygui_PACKAGE_FOLDER_RELEASE}"
        "${mygui_PACKAGE_FOLDER_DEBUG}"
)
list(REMOVE_ITEM _MyGUI_PACKAGE_ROOTS "")

set(_MyGUI_INCLUDE_HINTS)
foreach(_root IN LISTS _MyGUI_PACKAGE_ROOTS)
    list(APPEND _MyGUI_INCLUDE_HINTS
            "${_root}/include"
            "${_root}/lib/Release/MyGUIEngine.framework/Headers"
            "${_root}/lib/Debug/MyGUIEngine.framework/Headers")
endforeach()
find_path(MyGUI_INCLUDE_DIR MyGUI.h
        HINTS ${_MyGUI_INCLUDE_HINTS}
        PATH_SUFFIXES MYGUI)

# Find release libraries
get_filename_component(MyGUI_INCLUDE_PARENT "${MyGUI_INCLUDE_DIR}" DIRECTORY)
get_filename_component(MyGUI_PACKAGE_ROOT "${MyGUI_INCLUDE_PARENT}" DIRECTORY)

set(_MyGUI_RELEASE_LIBRARY_PATHS)
set(_MyGUI_DEBUG_LIBRARY_PATHS)
foreach(_root IN LISTS _MyGUI_PACKAGE_ROOTS)
    list(APPEND _MyGUI_RELEASE_LIBRARY_PATHS
            "${_root}/lib" "${_root}/lib/Release" "${_root}/lib/release")
    list(APPEND _MyGUI_DEBUG_LIBRARY_PATHS
            "${_root}/lib/Debug" "${_root}/lib/debug" "${_root}/lib")
endforeach()
list(APPEND _MyGUI_RELEASE_LIBRARY_PATHS
        "${MyGUI_PACKAGE_ROOT}/lib" "${MyGUI_PACKAGE_ROOT}/lib/Release" "${MyGUI_PACKAGE_ROOT}/lib/release")
list(APPEND _MyGUI_DEBUG_LIBRARY_PATHS
        "${MyGUI_PACKAGE_ROOT}/lib/Debug" "${MyGUI_PACKAGE_ROOT}/lib/debug" "${MyGUI_PACKAGE_ROOT}/lib")

find_library(MyGUI_MyGUIEngine_LIBRARY_REL NAMES MyGUIEngine
        HINTS ${_MyGUI_RELEASE_LIBRARY_PATHS}
        PATH_SUFFIXES release relwithdebinfo minsizerel)
find_library(MyGUI_OgrePlatform_LIBRARY_REL NAMES MyGUI.OgrePlatform
        HINTS ${_MyGUI_RELEASE_LIBRARY_PATHS}
        PATH_SUFFIXES release relwithdebinfo minsizerel)

# Find debug libraries
find_library(MyGUI_MyGUIEngine_LIBRARY_DBG NAMES MyGUIEngine_d MyGUIEngine
        HINTS ${_MyGUI_DEBUG_LIBRARY_PATHS}
        PATH_SUFFIXES debug)
find_library(MyGUI_OgrePlatform_LIBRARY_DBG NAMES MyGUI.OgrePlatform_d MyGUI.OgrePlatform
        HINTS ${_MyGUI_DEBUG_LIBRARY_PATHS}
        PATH_SUFFIXES debug)

# set include directories and libraries
set(MyGUI_INCLUDE_DIRS ${MyGUI_INCLUDE_DIR})
set(MyGUI_LIBRARIES)
if (MyGUI_MyGUIEngine_LIBRARY_REL OR MyGUI_OgrePlatform_LIBRARY_REL)
    list(APPEND MyGUI_LIBRARIES optimized ${MyGUI_MyGUIEngine_LIBRARY_REL} ${MyGUI_OgrePlatform_LIBRARY_REL})
endif ()
if (MyGUI_MyGUIEngine_LIBRARY_DBG OR MyGUI_OgrePlatform_LIBRARY_DBG)
    list(APPEND MyGUI_LIBRARIES debug ${MyGUI_MyGUIEngine_LIBRARY_DBG} ${MyGUI_OgrePlatform_LIBRARY_DBG})
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MyGUI FOUND_VAR MyGUI_FOUND
        REQUIRED_VARS MyGUI_INCLUDE_DIR MyGUI_MyGUIEngine_LIBRARY_REL MyGUI_OgrePlatform_LIBRARY_REL
        )

if (MyGUI_FOUND)
    if (NOT TARGET MyGUI::MyGUI)
        add_library(MyGUI::MyGUI INTERFACE IMPORTED)
        set_target_properties(MyGUI::MyGUI PROPERTIES
                INTERFACE_LINK_LIBRARIES
                "$<$<CONFIG:Debug>:${MyGUI_MyGUIEngine_LIBRARY_DBG}>$<$<NOT:$<CONFIG:Debug>>:${MyGUI_MyGUIEngine_LIBRARY_REL}>"
                INTERFACE_INCLUDE_DIRECTORIES "${MyGUI_INCLUDE_DIRS}")
    endif()
    if (NOT TARGET MyGUI::OgrePlatform)
        add_library(MyGUI::OgrePlatform INTERFACE IMPORTED)
        set_target_properties(MyGUI::OgrePlatform PROPERTIES
                INTERFACE_LINK_LIBRARIES
                "$<$<CONFIG:Debug>:${MyGUI_OgrePlatform_LIBRARY_DBG}>$<$<NOT:$<CONFIG:Debug>>:${MyGUI_OgrePlatform_LIBRARY_REL}>"
                INTERFACE_INCLUDE_DIRECTORIES "${MyGUI_INCLUDE_DIRS}")
    endif()
    set_property(TARGET MyGUI::MyGUI APPEND PROPERTY INTERFACE_LINK_LIBRARIES
            "$<$<CONFIG:Debug>:${MyGUI_MyGUIEngine_LIBRARY_DBG}>$<$<NOT:$<CONFIG:Debug>>:${MyGUI_MyGUIEngine_LIBRARY_REL}>")
    set_property(TARGET MyGUI::OgrePlatform APPEND PROPERTY INTERFACE_LINK_LIBRARIES
            "$<$<CONFIG:Debug>:${MyGUI_OgrePlatform_LIBRARY_DBG}>$<$<NOT:$<CONFIG:Debug>>:${MyGUI_OgrePlatform_LIBRARY_REL}>;MyGUI::MyGUI")
endif ()

mark_as_advanced(
        MyGUI_MyGUIEngine_LIBRARY_REL
        MyGUI_OgrePlatform_LIBRARY_REL
        MyGUI_MyGUIEngine_LIBRARY_DBG
        MyGUI_OgrePlatform_LIBRARY_DBG
)

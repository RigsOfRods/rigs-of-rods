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

find_path(MyGUI_INCLUDE_DIR MyGUI.h PATH_SUFFIXES MYGUI)

# Find release libraries
find_library(MyGUI_MyGUIEngine_LIBRARY_REL MyGUIEngine PATH_SUFFIXES release relwithdebinfo minsizerel)
find_library(MyGUI_OgrePlatform_LIBRARY_REL MyGUI.OgrePlatform PATH_SUFFIXES release relwithdebinfo minsizerel)

# Find debug libraries
find_library(MyGUI_MyGUIEngine_LIBRARY_DBG NAMES MyGUIEngine_d MyGUIEngine PATH_SUFFIXES debug)
find_library(MyGUI_OgrePlatform_LIBRARY_DBG NAMES MyGUI.OgrePlatform_d MyGUI.OgrePlatform PATH_SUFFIXES debug)

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
        REQUIRED_VARS MyGUI_INCLUDE_DIRS MyGUI_LIBRARIES
        )

if (MyGUI_FOUND)
    add_library(MyGUI::MyGUI INTERFACE IMPORTED)
    set_target_properties(MyGUI::MyGUI PROPERTIES
            INTERFACE_LINK_LIBRARIES
            "$<$<CONFIG:Debug>:${MyGUI_MyGUIEngine_LIBRARY_DBG}>$<$<NOT:$<CONFIG:Debug>>:${MyGUI_MyGUIEngine_LIBRARY_REL}>"
            INTERFACE_INCLUDE_DIRECTORIES "${MyGUI_INCLUDE_DIRS}"
            )
    add_library(MyGUI::OgrePlatform INTERFACE IMPORTED)
    set_target_properties(MyGUI::OgrePlatform PROPERTIES
            INTERFACE_LINK_LIBRARIES
            "$<$<CONFIG:Debug>:${MyGUI_OgrePlatform_LIBRARY_DBG}>$<$<NOT:$<CONFIG:Debug>>:${MyGUI_OgrePlatform_LIBRARY_REL}>"
            INTERFACE_INCLUDE_DIRECTORIES "${MyGUI_INCLUDE_DIRS}"
            )
    set_property(TARGET MyGUI::OgrePlatform APPEND PROPERTY INTERFACE_LINK_LIBRARIES MyGUI::MyGUI)
endif ()

mark_as_advanced(
        MyGUI_MyGUIEngine_LIBRARY_REL
        MyGUI_OgrePlatform_LIBRARY_REL
        MyGUI_MyGUIEngine_LIBRARY_DBG
        MyGUI_OgrePlatform_LIBRARY_DBG
)

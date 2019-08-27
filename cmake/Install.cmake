# The MIT License (MIT)
#
# Copyright (c) 2019 Edgar (Edgar{at}AnotherFoxGuy{dot}com)
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

IF (NOT BUILD_REDIST_FOLDER)

    INSTALL(
            DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/
            DESTINATION ./
            USE_SOURCE_PERMISSIONS
            PATTERN "*.lib" EXCLUDE
    )
    # CPack
    set(CPACK_PACKAGE_DESCRIPTION "Rigs of Rods soft-body physics simulator")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rigs of Rods is an open source vehicle simulator based on soft-body physics.")
    set(CPACK_PACKAGE_CONTACT "Edgar@AnotherFoxGuy.com")
    set(CPACK_PACKAGE_VENDOR "AnotherFoxGuy")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/tools/windows/license.rtf")
    set(CPACK_PACKAGE_EXECUTABLES "RoR" "Rigs of Rods Simulator")
    #For Windows Desktop shortcuts
    set(CPACK_CREATE_DESKTOP_LINKS "RoR" "Rigs of Rods Simulator")

    # Windows Add or Remove Program properties
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Rigs of Rods Simulator")
    set(CPACK_WIX_PROPERTY_ARPCOMMENTS "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
    set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://discord.gg/rigsofrods")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://www.rigsofrods.org/")
    set(CPACK_WIX_PROPERTY_URLUPDATEINFO "https://github.com/RigsOfRods/rigs-of-rods/releases")

    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/source/main/ror.ico")
    set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/tools/windows/bannrbmp.bmp")
    set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/tools/windows/dlgbmp.bmp")

    set(CPACK_WIX_LICENSE_RTF "${CPACK_RESOURCE_FILE_LICENSE}")
    set(CPACK_WIX_UPGRADE_GUID "dc178e9c-840d-443f-b249-434433ae5fd1")

    IF (MSVC)
        set(CPACK_PACKAGE_NAME "Rigs of Rods")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "Rigs of Rods")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}")
        set(CPACK_GENERATOR ZIP;WIX)
        set(CPACK_MODULE_PATH "")
    ENDIF (MSVC)

    include(CPack)

ENDIF ()

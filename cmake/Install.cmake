# The MIT License (MIT)
#
# Copyright (c) 2017 Edgar (Edgar{at}AnotherFoxGuy{dot}com)
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

IF (UNIX)
    # install the files required for the runtime
    INSTALL(
            DIRECTORY ${RoR_SOURCE_DIR}/tools/linux/desktop-entry/
            DESTINATION share/applications/
            FILES_MATCHING PATTERN "*.desktop"
    )
    INSTALL(
            DIRECTORY ${RoR_SOURCE_DIR}/tools/linux/desktop-entry/
            DESTINATION share/pixmaps/
            FILES_MATCHING PATTERN "*.png"
    )
ENDIF (UNIX)

# CPack
set(CPACK_PACKAGE_DESCRIPTION "Rigs of Rods soft-body physics simulator")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rigs of Rods is an open source vehicle simulator based on soft-body physics.")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "angelscript, socketw, caelum, pagedgeometry, libogre-1.9.0v5, libmygui.ogreplatform0debian1v5, libopenal1, libgtk2.0-0, libwxgtk3.0-0v5, libois-1.3.0v5")
set(CPACK_PACKAGE_CONTACT "Edgar@AnotherFoxGuy.com")
set(CPACK_PACKAGE_VENDOR "AnotherFoxGuy")
SET(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_PACKAGE_EXECUTABLES "RoR;Rigs of Rods Simulator" "RoRConfig;Rigs of Rods Configurator")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_MODIFY_PATH "ON")
set(CPACK_NSIS_URL_INFO_ABOUT "https://rigsofrods.org")
SET(CPACK_NSIS_INSTALLED_ICON_NAME "RoR.exe")
SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/source/configurator/rorconf.ico")
SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/source/configurator/rorconf.ico")
SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "RoRConfig.exe")

SET(CPACK_GENERATOR ZIP)
IF (UNIX)
    set(CPACK_PACKAGE_NAME "rigsofrods")
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};STGZ;TGZ;DEB;RPM)
ENDIF (UNIX)
IF (MSVC)
    set(CPACK_PACKAGE_NAME "Rigs of Rods")
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME} ${PROJECT_VERSION}")
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};NSIS)
ENDIF (MSVC)

include(CPack)

IF (UNIX)
    # install the files required for the runtime
    INSTALL(
            DIRECTORY ${RoR_SOURCE_DIR}/bin/resources/
            DESTINATION games/RoR/resources
            FILES_MATCHING PATTERN "*"
    )
    INSTALL(
            DIRECTORY ${ROR_BINTOOL_DIR}/
            DESTINATION games/RoR
            FILES_MATCHING PATTERN "*"
    )
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
ELSEIF (WIN32)
  # install the files required for the runtime
  INSTALL(
          DIRECTORY ${RoR_SOURCE_DIR}/bin/resources/
          DESTINATION /resources
          FILES_MATCHING PATTERN "*"
  )
  INSTALL(
          DIRECTORY ${ROR_BINTOOL_DIR}/
          DESTINATION /
          FILES_MATCHING PATTERN "*"
  )
ENDIF (UNIX)

# CPack
set(CPACK_PACKAGE_DESCRIPTION "Rigs of Rods soft-body physics simulator")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rigs of Rods is an open source vehicle simulator based on soft-body physics.")
set(CPACK_PACKAGE_NAME "rigsofrods")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libogre-1.9.0v5, libmygui.ogreplatform0debian1v5, libopenal1, libgtk2.0-0, libwxgtk3.0-0v5, libois-1.3.0v5")
set(CPACK_PACKAGE_CONTACT "Edgar@AnotherFoxGuy.com")
set(CPACK_PACKAGE_VENDOR "Edgar@AnotherFoxGuy.com")
SET(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_PACKAGE_EXECUTABLES "RoR;Rigs of Rods Simulator" "RoRConfig;Rigs of Rods Configurator" )

SET(CPACK_GENERATOR ZIP)
IF(UNIX)
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};STGZ;TGZ;DEB;RPM)
ENDIF(UNIX)
IF(LINUX)
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};DEB;RPM)
ENDIF(LINUX)
IF(MSVC)
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};NSIS)
ENDIF(MSVC)
IF(APPLE)
    SET(CPACK_GENERATOR ${CPACK_GENERATOR};PackageMaker)
ENDIF(APPLE)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)

set(TMP_FILE_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/tmp)

# find packages

find_package(wxWidgets COMPONENTS base core html net adv)
find_package(OpenAL)
find_package(Threads REQUIRED)
find_package(OGRE REQUIRED)
find_package(OIS REQUIRED)
find_package(MyGUI REQUIRED)
find_package(SocketW)
find_package(AngelScript)
find_package(CURL)
find_package(PagedGeometry)
find_package(Caelum)

if (WIN32)
    find_package(MoFileReader REQUIRED)
endif ()

add_subdirectory(version_info)
add_subdirectory(main)

if(ROR_USE_ANGELSCRIPT)
	add_subdirectory(angelscript_addons)
endif()

set(ROR_BUILD_CONFIGURATOR "TRUE" CACHE BOOL "build the configurator")

IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
ENDIF()

IF (BUILD_APPIMAGE)

  # Download && install AppRun
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/AppRun")
    file(DOWNLOAD "https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64"
                  "${CMAKE_BINARY_DIR}/AppRun")
  endif()

  INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/AppRun
          DESTINATION ${CMAKE_BINARY_DIR}/AppDir/
  )

  message(WARNING "Run >make install< && >./appimagetool-x86_64.AppImage AppDir< for creating the AppImage")
ENDIF ()

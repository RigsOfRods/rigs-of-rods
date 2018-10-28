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

if (BUILD_REDIST_FOLDER)
  if (WIN32)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/tools/windows/.itch.toml
            DESTINATION ${CMAKE_BINARY_DIR}/redist
    )
  else()
    INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/linux/RunRoR
            DESTINATION ${CMAKE_BINARY_DIR}/redist
    )
    INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/linux/RunRoRConfig
            DESTINATION ${CMAKE_BINARY_DIR}/redist
    )
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/tools/linux/.itch.toml
            DESTINATION ${CMAKE_BINARY_DIR}/redist
    )
  endif()
endif()

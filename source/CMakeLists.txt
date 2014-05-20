add_subdirectory(main)

if(ROR_USE_SKIA)
add_subdirectory(OgreCanvas)
endif()

if(ROR_USE_ANGELSCRIPT)
	add_subdirectory(angelscript_addons)
endif()

set(ROR_BUILD_CONFIGURATOR "TRUE" CACHE BOOL "build the configurator")

IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
ENDIF()

set(ROR_BUILD_TWODREPLAY "FALSE" CACHE BOOL "build the tdr2js tool thatcan convert 2d replays")

IF(ROR_BUILD_TWODREPLAY)
  add_subdirectory(tdr2js)
ENDIF()

IF(ROR_BUILD_UPDATER)
  add_subdirectory(updater)
ENDIF()

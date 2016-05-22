add_subdirectory( version_info )

if(ROR_BUILD_SIM)
  add_subdirectory(main)
endif()

if(ROR_USE_ANGELSCRIPT)
	add_subdirectory(angelscript_addons)
endif()

set(ROR_BUILD_CONFIGURATOR "TRUE" CACHE BOOL "build the configurator")

IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
ENDIF()

IF(ROR_BUILD_UPDATER)
  add_subdirectory(updater)
ENDIF()

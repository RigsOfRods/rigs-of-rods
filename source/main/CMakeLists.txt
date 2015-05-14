project(RoR_Main)

#setup libraries


macro(add_ror_project BINNAME folder useWxWidgets)
  # config below
  IF(WIN32)
    include_directories(${PThread_INCLUDE_DIRS})
    link_directories   (${PThread_LIBRARY_DIRS})
    set(OS_LIBS "${PThread_LIBRARIES};dinput8.lib;dxguid.lib")

    # disable some annoying VS warnings:
    # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
    # warning C4305: 'initializing' : truncation from 'double' to 'const float'
    add_definitions("/wd4305 /wd4244 /wd4193 -DNOMINMAX")
	
    # enable mumble on the windows platform for now only, has no dependencies as its only using shared memory and URLs
    add_definitions("-DUSE_MUMBLE")	
  ELSEIF(UNIX)
    include_directories(${GTK_INCLUDE_DIRS})
    set(OS_LIBS "X11 -l${CMAKE_DL_LIBS}")
  ENDIF(WIN32)

  include_directories(${Ogre_INCLUDE_DIRS})
  link_directories   (${Ogre_LIBRARY_DIRS})
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories   (${Boost_LIBRARY_DIRS})
  
  include_directories(${Ogre_Terrain_INCLUDE_DIRS})
  link_directories   (${Ogre_Terrain_LIBRARY_DIRS})
  include_directories(${Ogre_Paging_INCLUDE_DIRS})
  link_directories   (${Ogre_Paging_LIBRARY_DIRS})
  include_directories(${Ogre_RTShader_INCLUDE_DIRS})
  link_directories   (${Ogre_RTShader_LIBRARY_DIRS})
  include_directories(${Ois_INCLUDE_DIRS})
  link_directories   (${Ois_LIBRARY_DIRS})

  # optional components
  setup_lib(MYGUI)
  setup_lib(OPENAL)
  setup_lib(CURL)
  setup_lib(SOCKETW)
  IF(WIN32)
    setup_lib(MOFILEREADER)
    setup_lib(CRASHRPT)
  ENDIF(WIN32)
  setup_lib(PAGED)
  setup_lib(CAELUM)
  setup_lib(ANGELSCRIPT)

  if(ROR_USE_ANGELSCRIPT)
    # CLEAR_FPU_STACK prevents angelscript from breaking our physics engine
    add_definitions("-DAS_USE_NAMESPACE")
    set(optional_libs ${optional_libs} angelscript_addons)
  endif()

  if(ROR_USE_OIS_G27)
    add_definitions("-DUSE_OIS_G27")
  endif(ROR_USE_OIS_G27)

  if(ROR_FEAT_TIMING)
    add_definitions("-DFEAT_TIMING")
  endif(ROR_FEAT_TIMING)
 

  #add_definitions("-DFEAT_DEBUG_MUTEX")


  add_definitions("-DBOOST_ALL_NO_LIB")
  add_definitions("-DUSE_RTSHADER_SYSTEM")
  
  include_directories(${RoR_Main_SOURCE_DIR})
  add_sub_dir(${BINNAME} audio)
  add_sub_dir(${BINNAME} ../common)
  add_sub_dir(${BINNAME} datatypes)
  add_sub_dir(${BINNAME} gameplay)
  add_sub_dir(${BINNAME} terrain)
  add_sub_dir(${BINNAME} terrain/map)
  add_sub_dir(${BINNAME} threadpool)
  add_sub_dir(${BINNAME} gfx)
  add_sub_dir(${BINNAME} gfx/camera)
  add_sub_dir(${BINNAME} gfx/dof)
  add_sub_dir(${BINNAME} gfx/particle)
  add_sub_dir(${BINNAME} gfx/hydrax)
  add_sub_dir(${BINNAME} gfx/procedural)
  add_sub_dir(${BINNAME} gui)
  add_sub_dir(${BINNAME} gui/panels)
  add_sub_dir(${BINNAME} gui/mygui)
  add_sub_dir(${BINNAME} physics)
  add_sub_dir(${BINNAME} physics/air)
  add_sub_dir(${BINNAME} physics/collision)
  add_sub_dir(${BINNAME} physics/flex)
  add_sub_dir(${BINNAME} physics/input_output)
  add_sub_dir(${BINNAME} physics/mplatform)
  add_sub_dir(${BINNAME} physics/threading)
  add_sub_dir(${BINNAME} physics/utils)
  add_sub_dir(${BINNAME} physics/water)
  add_sub_dir(${BINNAME} ../rig_file_input_output)
  add_sub_dir(${BINNAME} rig_editor)
  add_sub_dir(${BINNAME} rig_editor/rig_data)
  if(ROR_USE_ANGELSCRIPT)
    add_sub_dir(${BINNAME} scripting)
  endif()
  add_sub_dir(${BINNAME} utils)
  add_sub_dir(${BINNAME} utils/profiler)
  add_sub_dir(${BINNAME} network)
  add_sub_dir(${BINNAME} network/protocol)
  add_sub_dir(${BINNAME} network/IRC)
  add_sub_dir(${BINNAME} resources)
  add_sub_dir(${BINNAME} ${folder})
  add_main_dir(${BINNAME})
  
  if(${useWxWidgets})
    # find wxWidgets
    SET(wxWidgets_USE_UNICODE ON)
    # message("project ${BINNAME} using wxwidgets")
    FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS base core html net adv propgrid aui)
    include_directories(${wxWidgets_INCLUDE_DIRS})
    add_definitions(${wxWidgets_CXX_FLAGS})
    if(DEFINED wxWidgets_DEFINITIONS)
      add_definitions("-D${wxWidgets_DEFINITIONS}")
    endif()
    link_directories (${wxWidgets_LIB_DIR})
    set(optional_libs ${optional_libs};${wxWidgets_LIBRARIES})
	
	add_definitions("-DROR_EMBEDDED")
  endif()

  IF(WIN32)
	add_definitions("-DMYGUI_STATIC")
    add_executable(${BINNAME} WIN32 ${${BINNAME}_headers} ${${BINNAME}_sources} ${RoR_Main_SOURCE_DIR}/${folder}/icon.rc)
  ELSE(WIN32)
    add_executable(${BINNAME}       ${${BINNAME}_headers} ${${BINNAME}_sources})
  ENDIF(WIN32)

  target_link_libraries(${BINNAME}
		${Boost_LIBRARIES} ${Ogre_LIBRARIES} ${Ogre_Terrain_LIBRARIES} ${Ogre_Paging_LIBRARIES} ${Ogre_RTShader_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${optional_libs}
	)
	
if(WIN32)
  # always copy ogre files
  copy_lib_files_for_target(${BINNAME} Ogre)
  # and pthread
  copy_lib_files_for_target(${BINNAME} PThread)
  # platform binaries
  copy_files_for_target(${BINNAME} ${ROR_BINTOOL_DIR}/$(ConfigurationName)/*)  
  # then the optional ones
  if(ROR_USE_MYGUI)
	copy_lib_files_for_target(${BINNAME} MyGUI)
  endif()
  if(ROR_USE_OPENAL)
	copy_lib_files_for_target(${BINNAME} OpenALSoft)
  endif()
  if(ROR_USE_CURL)
	copy_lib_files_for_target(${BINNAME} Curl)
  endif()
  if(ROR_USE_CAELUM)
	copy_lib_files_for_target(${BINNAME} Caelum)
  endif()
  if(ROR_USE_MOFILEREADER)
	copy_lib_files_for_target(${BINNAME} MoFileReader)
  endif()
  if(ROR_USE_CRASHRPT)
	#copy_lib_files_for_target(${BINNAME} CrashRpt)
  endif()
else()
  copy_files_for_target("${BINNAME}" "${ROR_BINTOOL_DIR}/*")
endif()
	
endmacro(add_ror_project)


IF(ROR_BUILD_SIM)
	add_subdirectory(main_sim)
endif()

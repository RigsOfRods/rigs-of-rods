project(RoR_Main)

####################################################################################################
#  SOURCE FILES
####################################################################################################

set(SOURCE_FILES
        main.cpp
        Application.{h,cpp}
        ForwardDeclarations.h
        GlobalEnvironment.h
        MainMenu.{h,cpp}
        RoRPrerequisites.h
        audio/MumbleIntegration.{h,cpp}
        audio/Sound.{h,cpp}
        audio/SoundManager.{h,cpp}
        audio/SoundScriptManager.{h,cpp}
        datatypes/beam_t.h
        datatypes/node_t.h
        datatypes/rig_t.h
        datatypes/shock_t.h
        datatypes/wheel_t.h
        gameplay/AircraftSimulation.{h,cpp}
        gameplay/AutoPilot.{h,cpp}
        gameplay/BeamEngine.{h,cpp}
        gameplay/Character.{h,cpp}
        gameplay/CharacterFactory.{h,cpp}
        gameplay/ChatSystem.{h,cpp}
        gameplay/Landusemap.{h,cpp}
        gameplay/LandVehicleSimulation.{h,cpp}
        gameplay/OutProtocol.{h,cpp}
        gameplay/PositionStorage.{h,cpp}
        gameplay/ProceduralManager.{h,cpp}
        gameplay/Replay.{h,cpp}
        gameplay/Road.{h,cpp}
        gameplay/Road2.{h,cpp}
        gameplay/RoRFrameListener.{h,cpp}
        gameplay/SceneMouse.{h,cpp}
        gameplay/ScriptEvents.h
        gameplay/Scripting.h
        gameplay/SkinManager.{h,cpp}
        gameplay/TorqueCurve.{h,cpp}
        gameplay/VehicleAI.{h,cpp}
        gfx/AdvancedScreen.h
        gfx/ColoredTextAreaOverlayElement.{h,cpp}
        gfx/ColoredTextAreaOverlayElementFactory.h
        gfx/DecalManager.{h,cpp}
        gfx/DustManager.{h,cpp}
        gfx/DustPool.{h,cpp}
        gfx/EnvironmentMap.{h,cpp}
        gfx/GfxActor.{h,cpp}
        gfx/GlowMaterialListener.h
        gfx/HDRListener.{h,cpp}
        gfx/Heathaze.{h,cpp}
        gfx/HydraxWater.{h,cpp}
        gfx/IWater.h
        gfx/MovableText.{h,cpp}
        gfx/OgreSubsystem.{h,cpp}
        gfx/ShadowManager.{h,cpp}
        gfx/Skidmark.{h,cpp}
        gfx/SkyManager.{h,cpp}
        gfx/SkyXManager.{h,cpp}
        gfx/Water.{h,cpp}
        gfx/camera/CameraBehaviorCharacter.{h,cpp}
        gfx/camera/CameraBehaviorFixed.{h,cpp}
        gfx/camera/CameraBehaviorFree.{h,cpp}
        gfx/camera/CameraBehaviorIsometric.{h,cpp}
        gfx/camera/CameraBehaviorOrbit.{h,cpp}
        gfx/camera/CameraBehaviorStatic.{h,cpp}
        gfx/camera/CameraBehaviorVehicle.{h,cpp}
        gfx/camera/CameraBehaviorVehicleCineCam.{h,cpp}
        gfx/camera/CameraBehaviorVehicleSpline.{h,cpp}
        gfx/camera/CameraManager.{h,cpp}
        gfx/camera/PerVehicleCameraContext.h
        gfx/dof/DepthOfFieldEffect.{h,cpp}
        gfx/dof/Lens.{h,cpp}
        gfx/hydrax/CfgFileManager.{h,cpp}
        gfx/hydrax/DecalsManager.{h,cpp}
        gfx/hydrax/Enums.{h,cpp}
        gfx/hydrax/FFT.{h,cpp}
        gfx/hydrax/GodRaysManager.{h,cpp}
        gfx/hydrax/GPUNormalMapManager.{h,cpp}
        gfx/hydrax/Help.{h,cpp}
        gfx/hydrax/Hydrax.{h,cpp}
        gfx/hydrax/Image.{h,cpp}
        gfx/hydrax/MaterialManager.{h,cpp}
        gfx/hydrax/Mesh.{h,cpp}
        gfx/hydrax/Module.{h,cpp}
        gfx/hydrax/Noise.{h,cpp}
        gfx/hydrax/Perlin.{h,cpp}
        gfx/hydrax/Prerequisites.{h,cpp}
        gfx/hydrax/PressurePoint.{h,cpp}
        gfx/hydrax/ProjectedGrid.{h,cpp}
        gfx/hydrax/RadialGrid.{h,cpp}
        gfx/hydrax/Real.{h,cpp}
        gfx/hydrax/RttManager.{h,cpp}
        gfx/hydrax/SimpleGrid.{h,cpp}
        gfx/hydrax/TextureManager.{h,cpp}
        gfx/hydrax/Wave.{h,cpp}
        gfx/particle/ExtinguishableFireAffector.{h,cpp}
        gfx/particle/ExtinguishableFireAffectorFactory.h
        gfx/particle/FireExtinguisherAffector.{h,cpp}
        gfx/particle/FireExtinguisherAffectorFactory.h
        gfx/particle/OgreParticleCustomParam.h
        gfx/particle/OgreShaderParticleRenderer.{h,cpp}
        gfx/skyx/AtmosphereManager.{h,cpp}
        gfx/skyx/BasicController.{h,cpp}
        gfx/skyx/CloudsManager.{h,cpp}
        gfx/skyx/ColorGradient.{h,cpp}
        gfx/skyx/Controller.h
        gfx/skyx/GPUManager.{h,cpp}
        gfx/skyx/MeshManager.{h,cpp}
        gfx/skyx/MoonManager.{h,cpp}
        gfx/skyx/Prerequisites.{h,cpp}
        gfx/skyx/SCfgFileManager.{h,cpp}
        gfx/skyx/SkyX.{h,cpp}
        gfx/skyx/VCloudsManager.{h,cpp}
        gfx/skyx/VClouds/DataManager.{h,cpp}
        gfx/skyx/VClouds/Ellipsoid.{h,cpp}
        gfx/skyx/VClouds/FastFakeRandom.{h,cpp}
        gfx/skyx/VClouds/GeometryBlock.{h,cpp}
        gfx/skyx/VClouds/GeometryManager.{h,cpp}
        gfx/skyx/VClouds/Lightning.{h,cpp}
        gfx/skyx/VClouds/LightningManager.{h,cpp}
        gfx/skyx/VClouds/VClouds.{h,cpp}
        gui/Dashboard.{h,cpp}
        gui/DashBoardManager.{h,cpp}
        gui/GUIInputManager.{h,cpp}
        gui/GUIManager.{h,cpp}
        gui/GUIUtils.{h,cpp}
        gui/OverlayWrapper.{h,cpp}
        gui/RTTLayer.{h,cpp}
        gui/imgui/imgui.{h,cpp}
        gui/imgui/imgui_demo.cpp
        gui/imgui/imgui_draw.cpp
        gui/imgui/OgreImGui.{h,cpp}
        gui/imgui/imconfig.h
        gui/imgui/imgui_internal.h
        gui/imgui/stb_rect_pack.h
        gui/imgui/stb_textedit.h
        gui/imgui/stb_truetype.h
        gui/mygui/Attribute.h
        gui/mygui/BaseLayout.h
        gui/mygui/Dialog.{h,cpp}
        gui/mygui/GuiPanelBase.h
        gui/mygui/WrapsAttribute.h
        gui/panels/GUI_GameConsole.{h,cpp}
        gui/panels/GUI_LoadingWindow.{h,cpp}
        gui/panels/GUI_FrictionSettings.{h,cpp}
        gui/panels/GUI_TopMenubar.{h,cpp}
        gui/panels/GUI_TextureToolWindow.{h,cpp}
        gui/panels/GUI_DebugOptions.{h,cpp}
        gui/panels/GUI_DebugOptionsLayout.{h,cpp}
        gui/panels/GUI_GameAbout.{h,cpp}
        gui/panels/GUI_GameAboutLayout.{h,cpp}
        gui/panels/GUI_GameChatBox.{h,cpp}
        gui/panels/GUI_GameChatBoxLayout.{h,cpp}
        gui/panels/GUI_GameConsoleLayout.{h,cpp}
        gui/panels/GUI_GameMainMenu.{h,cpp}
        gui/panels/GUI_GameMainMenuLayout.{h,cpp}
        gui/panels/GUI_GamePauseMenu.{h,cpp}
        gui/panels/GUI_GamePauseMenuLayout.{h,cpp}
        gui/panels/GUI_GameSettings.{h,cpp}
        gui/panels/GUI_MainSelector.{h,cpp}
        gui/panels/GUI_MainSelectorLayout.{h,cpp}
        gui/panels/GUI_MessageBox.{h,cpp}
        gui/panels/GUI_MessageBoxLayout.{h,cpp}
        gui/panels/GUI_MultiplayerSelector.{h,cpp}
        gui/panels/GUI_MultiplayerClientList.{h,cpp}
        gui/panels/GUI_OpenSaveFileDialog.{h,cpp}
        gui/panels/GUI_RigSpawnerReportWindow.{h,cpp}
        gui/panels/GUI_RigSpawnerReportWindowLayout.{h,cpp}
        gui/panels/GUI_SimUtils.{h,cpp}
        gui/panels/GUI_SimUtilsLayout.{h,cpp}
        gui/panels/GUI_TeleportWindow.{h,cpp}
        gui/panels/GUI_TeleportWindowLayout.{h,cpp}
        gui/panels/GUI_VehicleDescription.{h,cpp}
        gui/panels/GUI_VehicleDescriptionLayout.{h,cpp}
        network/Network.{h,cpp}
        physics/ApproxMath.h
        physics/Beam.{h,cpp}
        physics/BeamData.h
        physics/BeamFactory.{h,cpp}
        physics/BeamForcesEuler.cpp
        physics/BeamSlideNode.cpp
        physics/CmdKeyInertia.{h,cpp}
        physics/Differentials.{h,cpp}
        physics/RigSpawner.{h,cpp}
        physics/RigSpawner_ProcessControl.cpp
        physics/SlideNode.{h,cpp}
        physics/air/AeroEngine.h
        physics/air/AirBrake.{h,cpp}
        physics/air/Airfoil.{h,cpp}
        physics/air/TurboJet.{h,cpp}
        physics/air/TurboProp.{h,cpp}
        physics/collision/CartesianToTriangleTransform.h
        physics/collision/Collisions.{h,cpp}
        physics/collision/DynamicCollisions.{h,cpp}
        physics/collision/PointColDetector.{h,cpp}
        physics/collision/Triangle.h
        physics/flex/Flexable.h
        physics/flex/FlexAirfoil.{h,cpp}
        physics/flex/FlexBody.{h,cpp}
        physics/flex/FlexFactory.{h,cpp}
        physics/flex/FlexMesh.{h,cpp}
        physics/flex/FlexMeshWheel.{h,cpp}
        physics/flex/FlexObj.{h,cpp}
        physics/flex/Locator_t.h
        physics/mplatform/MPlatformBase.{h,cpp}
        physics/mplatform/MPlatformFD.{h,cpp}
        physics/utils/BeamStats.{h,cpp}
        physics/utils/RigLoadingProfiler.h
        physics/water/Buoyance.{h,cpp}
        physics/water/ScrewProp.{h,cpp}
        resources/CacheSystem.{h,cpp}
        resources/ContentManager.{h,cpp}
        resources/otc_fileformat/OTCFileformat.{h,cpp}
        resources/rig_def_fileformat/RigDef_File.{h,cpp}
        resources/rig_def_fileformat/RigDef_Node.{h,cpp}
        resources/rig_def_fileformat/RigDef_Parser.{h,cpp}
        resources/rig_def_fileformat/RigDef_Prerequisites.h
        resources/rig_def_fileformat/RigDef_Regexes.h
        resources/rig_def_fileformat/RigDef_SequentialImporter.{h,cpp}
        resources/rig_def_fileformat/RigDef_Serializer.{h,cpp}
        resources/rig_def_fileformat/RigDef_Validator.{h,cpp}
        resources/terrn2_fileformat/Terrn2Fileformat.{h,cpp}
        terrain/IHeightFinder.h
        terrain/OgreTerrainPSSMMaterialGenerator.{h,cpp}
        terrain/TerrainGeometryManager.{h,cpp}
        terrain/TerrainManager.{h,cpp}
        terrain/TerrainObjectManager.{h,cpp}
        terrain/map/SurveyMapEntity.{h,cpp}
        terrain/map/SurveyMapManager.{h,cpp}
        terrain/map/SurveyMapTextureCreator.{h,cpp}
        threadpool/ThreadPool.h
        utils/CollisionTools.{h,cpp}
        utils/ConfigFile.{h,cpp}
        utils/ErrorUtils.{h,cpp}
        utils/FileSystemInfo.h
        utils/ForceFeedback.{h,cpp}
        utils/IBehavior.h
        utils/ImprovedConfigFile.h
        utils/InputEngine.{h,cpp}
        utils/InterThreadStoreVector.h
        utils/Language.{h,cpp}
        utils/MeshObject.{h,cpp}
        utils/PlatformUtils.{h,cpp}
        utils/RoRWindowEventUtilities.{h,cpp}
        utils/Settings.{h,cpp}
        utils/SHA1.{h,cpp}
        utils/SimpleGlob.h
        utils/SimpleOpt.h
        utils/Singleton.h
        utils/Timer.h
        utils/Utils.{h,cpp}
        utils/WriteTextToTexture.{h,cpp}
        utils/ZeroedMemoryAllocator.h
        utils/profiler/Profiler.{h,cpp}
        utils/profiler/RigLoadingProfilerControl.h
        )

if (ROR_USE_ANGELSCRIPT)
    list(APPEND SOURCE_FILES
            scripting/CBytecodeStream.{h,cpp}
            scripting/GameScript.{h,cpp}
            scripting/LocalStorage.{h,cpp}
            scripting/OgreAngelscript.{h,cpp}
            scripting/OgreScriptBuilder.{h,cpp}
            scripting/ScriptEngine.{h,cpp}
            )
endif ()


include(SourceFileUtils)

# Expand file extensions (i.e. path/to/file.{h,cpp} becomes path/to/file.h;path/to/file.cpp)
expand_file_extensions(SOURCE_FILES ${SOURCE_FILES})

# Generate source groups for use in IDEs
generate_source_groups(${SOURCE_FILES})


if (WIN32)
    # clang-cl doesn't support resource files
    if (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        list(APPEND SOURCE_FILES "icon.rc")
    endif()
endif ()


####################################################################################################
#  EXECUTABLE TARGET
####################################################################################################

set(BINNAME "RoR")
link_directories(${MYGUI_LIBRARY_DIRS})
add_executable(${BINNAME} ${SOURCE_FILES})

if (WIN32)
    set_target_properties(${BINNAME} PROPERTIES WIN32_EXECUTABLE YES)
		add_definitions(-DWIN32_LEAN_AND_MEAN)
endif ()

####################################################################################################
#  PREPROCESSOR DEFINITIONS
####################################################################################################

target_compile_definitions(${BINNAME} PRIVATE
        USE_MUMBLE  # build with support for Mumble positional audio, has no dependencies but requires linking against librt on UNIX
        #FEAT_DEBUG_MUTEX
        )

if (ROR_FEAT_TIMING)
    target_compile_definitions(${BINNAME} PRIVATE FEAT_TIMING)
endif ()

if (ROR_USE_OIS_G27)
    target_compile_definitions(${BINNAME} PRIVATE USE_OIS_G27)
endif ()


####################################################################################################
#  INCLUDE DIRECTORIES
####################################################################################################


include_directories(${MYGUI_INCLUDE_DIRS})
target_include_directories(${BINNAME} PRIVATE
        .
        audio
        datatypes
        gameplay
        gfx
        gfx/camera
        gfx/dof
        gfx/hydrax
        gfx/particle
        gfx/procedural
        gfx/skyx
        gui
        gui/imgui
        gui/mygui
        gui/panels
        network
        physics
        physics/air
        physics/collision
        physics/flex
        physics/mplatform
        physics/threading
        physics/utils
        physics/water
        resources
        resources/otc_fileformat/
        resources/rig_def_fileformat
        resources/terrn2_fileformat
        rig_editor
        rig_editor/rig_data
        scripting
        terrain
        terrain/map
        threadpool
        utils
        utils/profiler
        )


####################################################################################################
# EXTERNAL DEPENDENCIES
#
# In CMake searching for external dependencies is done with the help of FindModule scripts. These
# are run by calling the find_package() command.
# CMake already ships with many ready-made FindModules (e.g. for OpenAL). Additional FindModule
# scripts that can be used are placed in the CMakeModule directory `<ror_root>/cmake`. These
# scripts typically define a set of variables, such as
#
#   <dependency_name>_FOUND           # indicates if the dependecy was found
#   <dependency_name>_INCLUDE_DIRS    # directories with headers to include
#   <dependency_name>_LIBRARIES       # libraries to link against
#
# Instead of directly using these variables, in modern CMake it is common to use additionally
# provided 'imported targets' for the sought dependency. This greatly facilitates dealing with
# dependencies. Adding a dependency to  an executable target is now simply a matter of specifying
# the imported target of the dependency in the target_link_libraries() command.
#
####################################################################################################


#  REQUIRED DEPENDENCIES
# -----------------------

# TODO
IF (WIN32)
    set(OS_LIBS "dinput8.lib;dxguid.lib")

    # disable some annoying VS warnings:
    # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
    # warning C4305: 'initializing' : truncation from 'double' to 'const float'
    add_definitions("/wd4305 /wd4244 /wd4193 -DNOMINMAX")
ELSEIF (UNIX)
    #  include_directories(${GTK_INCLUDE_DIRS})
    set(OS_LIBS "X11 -l${CMAKE_DL_LIBS} -lrt")
ENDIF (WIN32)
target_link_libraries(${BINNAME} PRIVATE
        ${OS_LIBS}
        version_info
        )

# --- Threading support (still needed for GCC even with C++11)
set(CMAKE_THREAD_PREFER_PTHREAD YES)
target_link_libraries(${BINNAME} PRIVATE
        Threads::Threads
        )

# --- Ogre 3D graphics engine ---
target_link_libraries(${BINNAME} PRIVATE
        ${OGRE_LIBRARIES}
        ${OGRE_Paging_LIBRARIES}
        ${OGRE_RTShaderSystem_LIBRARIES}
        ${OGRE_Terrain_LIBRARIES}
        )
target_include_directories(${BINNAME} PRIVATE
        ${OGRE_INCLUDE_DIRS}
        ${OGRE_Paging_INCLUDE_DIRS}
        ${OGRE_RTShaderSystem_INCLUDE_DIRS}
        ${OGRE_Terrain_INCLUDE_DIRS}
        )
if (OGRE_VERSION VERSION_GREATER 1.8)
    target_link_libraries(${BINNAME} PRIVATE ${OGRE_Overlay_LIBRARIES})
    target_include_directories(${BINNAME} PRIVATE ${OGRE_Overlay_INCLUDE_DIRS})
endif ()


# --- Object Oriented Input System ---
target_link_libraries(${BINNAME} PRIVATE ${OIS_LIBRARIES})
target_include_directories(${BINNAME} PRIVATE ${OIS_INCLUDE_DIRS})


# --- MoFileReader ---
if (WIN32)
    target_link_libraries(${BINNAME} PRIVATE MoFileReader::MoFileReader)
    target_compile_definitions(${BINNAME} PRIVATE USE_MOFILEREADER)
endif ()


#  OPTIONAL DEPENDENCIES
# -----------------------

# --- MyGUI - graphical user inferface ---
if ((TARGET MyGUI::MyGUI) AND (TARGET MyGUI::OgrePlatform))
    target_link_libraries(${BINNAME} PRIVATE
            MyGUI::MyGUI
            MyGUI::OgrePlatform
            )
    target_compile_definitions(${BINNAME} PRIVATE
            USE_MYGUI
            $<$<PLATFORM_ID:WINDOWS>:MYGUI_STATIC>
            )
else ()
    message(WARNING "Building ${BINNAME} without MyGUI. Some functionality may be missing.")
endif ()

# --- OpenAL - audio library ---
if(CONAN_OPENAL_ROOT)
  target_link_libraries(${BINNAME} PRIVATE CONAN_PKG::openal)
  target_include_directories(${BINNAME} PRIVATE CONAN_PKG::openal)
  target_compile_definitions(${BINNAME} PRIVATE USE_OPENAL)
else ()
  if (OPENAL_FOUND)
      target_link_libraries(${BINNAME} PRIVATE ${OPENAL_LIBRARY})
      target_include_directories(${BINNAME} PRIVATE ${OPENAL_INCLUDE_DIR})
      target_compile_definitions(${BINNAME} PRIVATE USE_OPENAL)
  else ()
      message(WARNING "Building ${BINNAME} without OpenAL. Some functionality may be missing.")
  endif ()
endif ()


# --- SocketW - networking library ---
if (TARGET SocketW::SocketW)
    target_link_libraries(${BINNAME} PRIVATE SocketW::SocketW)
    target_compile_definitions(${BINNAME} PRIVATE USE_SOCKETW)
else ()
    message(WARNING "Building ${BINNAME} without SocketW. Some functionality may be missing.")
endif ()
# Add openssl
if(CONAN_OPENSSL_ROOT)
	target_include_directories(${BINNAME} PRIVATE ${CONAN_INCLUDE_DIRS_OPENSSL})
endif()


# --- AngelScript - scripting interface ---
if (TARGET AngelScript::AngelScript)
    target_link_libraries(${BINNAME} PRIVATE
            AngelScript::AngelScript
            AngelScript::use_namespace
            )
    target_compile_definitions(${BINNAME} PRIVATE USE_ANGELSCRIPT)

    # TODO angelscript_addons
    target_include_directories(${BINNAME} PRIVATE ../angelscript_addons)
    target_link_libraries(${BINNAME} PRIVATE angelscript_addons)
else ()
    message(WARNING "Building ${BINNAME} without AngelScript. Some functionality may be missing.")
endif ()


# --- cURL ---
if (CURL_FOUND)
    target_link_libraries(${BINNAME} PRIVATE ${CURL_LIBRARY})
    target_include_directories(${BINNAME} PRIVATE ${CURL_INCLUDE_DIR})
    target_compile_definitions(${BINNAME} PRIVATE USE_CURL)
else ()
    message(WARNING "Building ${BINNAME} without cURL. Some functionality may be missing.")
endif ()


# --- rapidjson ---
if (CONAN_RAPIDJSON_ROOT)
    target_include_directories(${BINNAME} PRIVATE ${CONAN_INCLUDE_DIRS_RAPIDJSON})
endif ()


# --- Caelum -- Ogre addon for realistic sky rendering ---
if (TARGET Caelum::Caelum)
    set(CFG_OGRE_PLUGIN_CAELUM "Plugin=../libCaelum.so")
    target_link_libraries(${BINNAME} PRIVATE Caelum::Caelum)
    target_compile_definitions(${BINNAME} PRIVATE USE_CAELUM)
endif ()


# --- PagedGeometry -- Ogre addon ---
if (TARGET PagedGeometry::PagedGeometry)
    target_link_libraries(${BINNAME} PRIVATE PagedGeometry::PagedGeometry)
    target_compile_definitions(${BINNAME} PRIVATE USE_PAGED)
else ()
    message(WARNING "Building ${BINNAME} without PagedGeometry. Some functionality may be missing.")
endif ()

set_target_properties(${BINNAME} PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
cotire(${BINNAME})


####################################################################################################
#  POST-BUILD STEPS
####################################################################################################

# configure plugins.cfg
if (WIN32)
  set(CFG_OGRE_RENDERSYSTEM_D3D9 "Plugin=RenderSystem_Direct3D9")
endif ()

if(BUILD_REDIST_FOLDER AND UNIX)
  set(PLUGINS_FOLDER "lib")
elseif(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  set(PLUGINS_FOLDER ".")
else()
  set(PLUGINS_FOLDER ${OGRE_PLUGIN_DIR_REL})
endif()

configure_file(plugins.cfg.in ${RUNTIME_OUTPUT_DIRECTORY}/plugins.cfg)


set(TMP_FILE_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/tmp)
get_sub_dirs(ROR_RESOURCES_DIRS "${CMAKE_SOURCE_DIR}/resources")
set(ROR_RESOURCES_ZIPS)
set(TMP_RES_DIR ${TMP_FILE_DIR}/resources)
file(MAKE_DIRECTORY ${TMP_RES_DIR})

foreach(RES_DIR ${ROR_RESOURCES_DIRS})

  file(GLOB ZIP_FILES RELATIVE "${CMAKE_SOURCE_DIR}/resources/${RES_DIR}" "${CMAKE_SOURCE_DIR}/resources/${RES_DIR}/*" )
  set(ZIP_FILES_TXT "")
  foreach(ZIP_FILE ${ZIP_FILES})
     set(ZIP_FILES_TXT "${ZIP_FILES_TXT}${ZIP_FILE}\n")
  endforeach()
  file(WRITE "${TMP_FILE_DIR}/${RES_DIR}-filelist.txt" ${ZIP_FILES_TXT})

  set(ZIP_NAME "${TMP_RES_DIR}/${RES_DIR}.zip")
  add_custom_command(
    OUTPUT ${ZIP_NAME}
    COMMAND ${CMAKE_COMMAND} -E tar "cf" ${ZIP_NAME} --format=zip --files-from="${TMP_FILE_DIR}/${RES_DIR}-filelist.txt"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/${RES_DIR}"
  )
  list(APPEND ROR_RESOURCES_ZIPS ${ZIP_NAME})
endforeach()

# Copy resources needed for execution to the build directory
add_custom_target(
    zip_and_copy_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TMP_RES_DIR} $<TARGET_FILE_DIR:${BINNAME}>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/managed_materials $<TARGET_FILE_DIR:${BINNAME}>/resources/managed_materials
    COMMENT "Zip and copy resources to build directory"
    DEPENDS ${ROR_RESOURCES_ZIPS}
    VERBATIM
)

IF (BUILD_REDIST_FOLDER)
    INSTALL(
            TARGETS ${BINNAME}
            RUNTIME DESTINATION ${REDIST_FOLDER}
            LIBRARY DESTINATION ${REDIST_FOLDER}
            ARCHIVE DESTINATION ${REDIST_FOLDER}
    )
    INSTALL(
            DIRECTORY ${TMP_RES_DIR}/
            DESTINATION ${REDIST_FOLDER}/resources/
    )
    INSTALL(
            DIRECTORY ${CMAKE_SOURCE_DIR}/resources/managed_materials/
            DESTINATION ${REDIST_FOLDER}/resources/managed_materials/
    )
    INSTALL(
            FILES ${RUNTIME_OUTPUT_DIRECTORY}/plugins.cfg
            DESTINATION ${REDIST_FOLDER}
    )
ENDIF ()

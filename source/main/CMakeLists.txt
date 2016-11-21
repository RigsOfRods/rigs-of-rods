project(RoR_Main)


set( BINNAME "RoR" )


####################################################################################################
#  FIND EXTERNAL DEPENDENCIES
####################################################################################################

#  REQUIRED DEPENDENCIES
# -----------------------

#  OPTIONAL DEPENDENCIES
# -----------------------



####################################################################################################
#  SOURCE FILES
####################################################################################################

set( SOURCE_FILES
  main.cpp
  Application.{h,cpp}
  ForwardDeclarations.h
  GlobalEnvironment.h
  MainThread.{h,cpp}
  RoRPrerequisites.h
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
  gameplay/MaterialReplacer.{h,cpp}
  gameplay/OutProtocol.{h,cpp}
  gameplay/PlayerColours.{h,cpp}
  gameplay/PositionStorage.{h,cpp}
  gameplay/ProceduralManager.{h,cpp}
  gameplay/Replay.{h,cpp}
  gameplay/Road.{h,cpp}
  gameplay/Road2.{h,cpp}
  gameplay/RoRFrameListener.{h,cpp}
  gameplay/SceneMouse.{h,cpp}
  gameplay/ScriptEvents.h
  gameplay/Scripting.h
  gameplay/Skin.{h,cpp}
  gameplay/SkinManager.{h,cpp}
  gameplay/TorqueCurve.{h,cpp}
  gameplay/VehicleAI.{h,cpp}
  gfx/AdvancedScreen.h
  gfx/ColoredTextAreaOverlayElement.{h,cpp}
  gfx/ColoredTextAreaOverlayElementFactory.h
  gfx/DecalManager.{h,cpp}
  gfx/DustManager.{h,cpp}
  gfx/DustPool.{h,cpp}
  gfx/DynamicLines.{h,cpp}
  gfx/DynamicRenderable.{h,cpp}
  gfx/EnvironmentMap.{h,cpp}
  gfx/GlowMaterialListener.h
  gfx/HDRListener.{h,cpp}
  gfx/Heathaze.{h,cpp}
  gfx/HydraxWater.{h,cpp}
  gfx/IWater.h
  gfx/MaterialFunctionMapper.{h,cpp}
  gfx/Mirrors.{h,cpp}
  gfx/MovableText.{h,cpp}
  gfx/OgreSubsystem.{h,cpp}
  gfx/ShadowManager.{h,cpp}
  gfx/Skidmark.{h,cpp}
  gfx/SkyManager.{h,cpp}
  gfx/VideoCamera.{h,cpp}
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
  gui/Dashboard.{h,cpp}
  gui/DashBoardManager.{h,cpp}
  gui/GUIInputManager.{h,cpp}
  gui/GUIManager.{h,cpp}
  gui/OverlayWrapper.{h,cpp}
  gui/RTTLayer.{h,cpp}
  gui/TruckHUD.{h,cpp}
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
  gui/panels/GUI_GameSettingsLayout.{h,cpp}
  gui/panels/GUI_MainSelector.{h,cpp}
  gui/panels/GUI_MainSelectorLayout.{h,cpp}
  gui/panels/GUI_MessageBox.{h,cpp}
  gui/panels/GUI_MessageBoxLayout.{h,cpp}
  gui/panels/GUI_MultiplayerSelector.{h,cpp}
  gui/panels/GUI_MultiplayerSelectorLayout.{h,cpp}
  gui/panels/GUI_MultiplayerClientList.{h,cpp} 
  gui/panels/GUI_OpenSaveFileDialog.{h,cpp}
  gui/panels/GUI_RigSpawnerReportWindow.{h,cpp}
  gui/panels/GUI_RigSpawnerReportWindowLayout.{h,cpp}
  gui/panels/GUI_SimUtils.{h,cpp}
  gui/panels/GUI_SimUtilsLayout.{h,cpp}
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
  resources/rig_def_fileformat/RigDef_File.{h,cpp}
  resources/rig_def_fileformat/RigDef_Node.{h,cpp}
  resources/rig_def_fileformat/RigDef_Parser.{h,cpp}
  resources/rig_def_fileformat/RigDef_Prerequisites.h
  resources/rig_def_fileformat/RigDef_Regexes.h
  resources/rig_def_fileformat/RigDef_SequentialImporter.{h,cpp}
  resources/rig_def_fileformat/RigDef_Serializer.{h,cpp}
  resources/rig_def_fileformat/RigDef_Validator.{h,cpp}
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
  utils/IBehaviorManager.h
  utils/ImprovedConfigFile.h
  utils/InputEngine.{h,cpp}
  utils/InterThreadStoreVector.h
  utils/Language.{h,cpp}
  utils/MeshObject.{h,cpp}
  utils/MumbleIntegration.{h,cpp}
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

if(ROR_USE_ANGELSCRIPT)
  list( APPEND SOURCE_FILES
    scripting/CBytecodeStream.{h,cpp}
    scripting/GameScript.{h,cpp}
    scripting/LocalStorage.{h,cpp}
    scripting/OgreAngelscript.{h,cpp}
    scripting/OgreScriptBuilder.{h,cpp}
    scripting/ScriptEngine.{h,cpp}
  )
endif()


include( SourceFileUtils )

# Expand file extensions (i.e. path/to/file.{h,cpp} becomes path/to/file.h;path/to/file.cpp)
expand_file_extensions( SOURCE_FILES ${SOURCE_FILES} )

# Generate source groups for use in IDEs
generate_source_groups( ${SOURCE_FILES} )


if(WIN32)
  list( APPEND SOURCE_FILES "icon.rc")
endif()



####################################################################################################
#  EXECUTABLE TARGET
####################################################################################################

link_directories   (${Ogre_LIBRARY_DIRS})
link_directories   (${Ogre_Terrain_LIBRARY_DIRS})
link_directories   (${Ogre_Overlay_LIBRARY_DIRS})
link_directories   (${Ogre_Paging_LIBRARY_DIRS})
link_directories   (${Ogre_RTShader_LIBRARY_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})


add_executable(${BINNAME} ${SOURCE_FILES} )

if(WIN32)
  set_target_properties( ${BINNAME} PROPERTIES WIN32_EXECUTABLE YES )
endif()


# Post build actions to be performed
if(WIN32)
  # always copy ogre files
  copy_lib_files_for_target(${BINNAME} Ogre)
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
    copy_lib_files_for_target(${BINNAME} CrashRpt)		
  endif()
else()
  copy_files_for_target("${BINNAME}" "${ROR_BINTOOL_DIR}/*")
endif()

# Copy resources needed for execution to the build directory
add_custom_command(
  TARGET ${BINNAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/resources $<TARGET_FILE_DIR:${BINNAME}>/resources
  COMMENT "Copy resources to build directory"
  VERBATIM
)


####################################################################################################
#  PREPROCESSOR DEFINITIONS
####################################################################################################

target_compile_definitions( ${BINNAME} PRIVATE
  USE_MUMBLE  # build with support for Mumble positional audio, has no dependencies but requires linking against librt on UNIX
  USE_RTSHADER_SYSTEM
  #FEAT_DEBUG_MUTEX
)

if(WIN32)
  target_compile_definitions( ${BINNAME} PRIVATE MYGUI_STATIC)
endif()

if(ROR_USE_ANGELSCRIPT)
  target_compile_definitions( ${BINNAME} PRIVATE AS_USE_NAMESPACE)
endif()

if(ROR_USE_OIS_G27)
  target_compile_definitions( ${BINNAME} PRIVATE USE_OIS_G27)
endif()

if(ROR_FEAT_TIMING)
  target_compile_definitions( ${BINNAME} PRIVATE FEAT_TIMING)
endif()



####################################################################################################
#  INCLUDE DIRECTORIES
####################################################################################################


include_directories(${Ogre_INCLUDE_DIRS})
include_directories(${Ogre_Terrain_INCLUDE_DIRS})
include_directories(${Ogre_Overlay_INCLUDE_DIRS})
include_directories(${Ogre_Paging_INCLUDE_DIRS})
include_directories(${Ogre_RTShader_INCLUDE_DIRS})
include_directories(${Ois_INCLUDE_DIRS})


target_include_directories( ${BINNAME} PRIVATE
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
 gui
 gui/mygui
 gui/panels
 network
 network/IRC
 network/protocol
 physics
 physics/air
 physics/collision
 physics/flex
 physics/mplatform
 physics/threading
 physics/utils
 physics/water
 resources
 resources/rig_def_fileformat
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
#  LINK LIBRARIES
####################################################################################################

IF(WIN32)
  set(OS_LIBS "dinput8.lib;dxguid.lib")

  # disable some annoying VS warnings:
  # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
  # warning C4305: 'initializing' : truncation from 'double' to 'const float'
  add_definitions("/wd4305 /wd4244 /wd4193 -DNOMINMAX")
ELSEIF(UNIX)
  include_directories(${GTK_INCLUDE_DIRS})
  set(OS_LIBS "X11 -l${CMAKE_DL_LIBS} -lrt -lboost_system")
ENDIF(WIN32)


# optional components
setup_lib(MYGUI)
setup_lib(OPENAL)
setup_lib(CURL)
setup_lib(SOCKETW)
IF(WIN32)
  setup_lib(MOFILEREADER)
ENDIF(WIN32)
setup_lib(PAGED)
setup_lib(CAELUM)
setup_lib(ANGELSCRIPT)
setup_lib(JSONCPP)
setup_lib(CRASHRPT)

if(ROR_USE_ANGELSCRIPT)
  # CLEAR_FPU_STACK prevents angelscript from breaking our physics engine
  set(optional_libs ${optional_libs} angelscript_addons)
endif()


target_link_libraries(${BINNAME}
  ${Ogre_LIBRARIES}
  ${Ogre_Terrain_LIBRARIES}
  ${Ogre_Overlay_LIBRARIES}
  ${Ogre_Paging_LIBRARIES}
  ${Ogre_RTShader_LIBRARIES}
  ${Ois_LIBRARIES}
  ${OS_LIBS}
  ${optional_libs}
  version_info
)


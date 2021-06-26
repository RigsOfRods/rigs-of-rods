####################################################################################################
#  SOURCE FILES
####################################################################################################

set(SOURCE_FILES
        main.cpp
        Application.{h,cpp}
        ForwardDeclarations.h
        AppContext.{h,cpp}
        GameContext.{h,cpp}
        audio/MumbleIntegration.{h,cpp}
        audio/Sound.{h,cpp}
        audio/SoundManager.{h,cpp}
        audio/SoundScriptManager.{h,cpp}
        gameplay/AutoPilot.{h,cpp}
        gameplay/Character.{h,cpp}
        gameplay/CharacterFactory.{h,cpp}
        gameplay/ChatSystem.{h,cpp}
        gameplay/CruiseControl.cpp
        gameplay/EngineSim.{h,cpp}
        gameplay/Landusemap.{h,cpp}
        gameplay/ProceduralManager.{h,cpp}
        gameplay/RaceSystem.{h,cpp}
        gameplay/RecoveryMode.{h,cpp}
        gameplay/Replay.{h,cpp}
        gameplay/Road2.{h,cpp}
        gameplay/SceneMouse.{h,cpp}
        gameplay/ScriptEvents.h
        gameplay/TorqueCurve.{h,cpp}
        gameplay/TyrePressure.{h,cpp}
        gameplay/VehicleAI.{h,cpp}
        gfx/AdvancedScreen.h
        gfx/ColoredTextAreaOverlayElement.{h,cpp}
        gfx/ColoredTextAreaOverlayElementFactory.h
        gfx/DecalManager.{h,cpp}
        gfx/DustPool.{h,cpp}
        gfx/EnvironmentMap.{h,cpp}
        gfx/GfxActor.{h,cpp}
        gfx/GfxData.h
        gfx/GfxScene.{h,cpp}
        gfx/HydraxWater.{h,cpp}
        gfx/IWater.h
        gfx/MovableText.{h,cpp}
        gfx/Renderdash.{h,cpp}
        gfx/ShadowManager.{h,cpp}
        gfx/Skidmark.{h,cpp}
        gfx/SkyManager.{h,cpp}
        gfx/SkyXManager.{h,cpp}
        gfx/SurveyMapTextureCreator.{h,cpp}
        gfx/Water.{h,cpp}
        gfx/camera/CameraManager.{h,cpp}
        gfx/camera/PerVehicleCameraContext.h
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
        gui/DashBoardManager.{h,cpp}
        gui/GUIManager.{h,cpp}
        gui/GUIUtils.{h,cpp}
        gui/OverlayWrapper.{h,cpp}
        gui/RTTLayer.{h,cpp}
        gui/imgui/imgui.{h,cpp}
        gui/imgui/imgui_demo.cpp
        gui/imgui/imgui_draw.cpp
        gui/imgui/imgui_widgets.cpp
        gui/imgui/OgreImGuiOverlay.{h,cpp}
        gui/imgui/OgreImGui.{h,cpp}
        gui/imgui/imconfig.h
        gui/imgui/imgui_internal.h
        gui/imgui/imstb_rectpack.h
        gui/imgui/imstb_textedit.h
        gui/imgui/imstb_truetype.h
        gui/mygui/Attribute.h
        gui/mygui/BaseLayout.h
        gui/mygui/Dialog.{h,cpp}
        gui/mygui/GuiPanelBase.h
        gui/mygui/WrapsAttribute.h
        gui/overlay/BaseDashOverlayElement.{h,cpp}
        gui/overlay/DashPanelOverlayElement.{h,cpp}
        gui/overlay/DashTextAreaOverlayElement.{h,cpp}
        gui/panels/GUI_ConsoleView.{h,cpp}
        gui/panels/GUI_ConsoleWindow.{h,cpp}
        gui/panels/GUI_DirectionArrow.{h,cpp}
        gui/panels/GUI_LoadingWindow.{h,cpp}
        gui/panels/GUI_FrictionSettings.{h,cpp}
        gui/panels/GUI_TopMenubar.{h,cpp}
        gui/panels/GUI_TextureToolWindow.{h,cpp}
        gui/panels/GUI_GameControls.{h,cpp}
        gui/panels/GUI_GameAbout.{h,cpp}
        gui/panels/GUI_GameChatBox.{h,cpp}
        gui/panels/GUI_GameMainMenu.{h,cpp}
        gui/panels/GUI_GameSettings.{h,cpp}
        gui/panels/GUI_MainSelector.{h,cpp}
        gui/panels/GUI_MessageBox.{h,cpp}
        gui/panels/GUI_MultiplayerSelector.{h,cpp}
        gui/panels/GUI_MultiplayerClientList.{h,cpp}
        gui/panels/GUI_NodeBeamUtils.{h,cpp}
        gui/panels/GUI_SimActorStats.{h,cpp}
        gui/panels/GUI_SimPerfStats.{h,cpp}
        gui/panels/GUI_SurveyMap.{h,cpp}
        gui/panels/GUI_VehicleDescription.{h,cpp}
        network/DiscordRpc.{h,cpp}
        network/Network.{h,cpp}
        network/OutGauge.{h,cpp}
        physics/Actor.{h,cpp}
        physics/ApproxMath.h
        physics/ActorForcesEuler.cpp
        physics/ActorManager.{h,cpp}
        physics/ActorSlideNode.cpp
        physics/ActorSpawner.{h,cpp}
        physics/ActorSpawnerFlow.cpp
        physics/CmdKeyInertia.{h,cpp}
        physics/Differentials.{h,cpp}
        physics/Savegame.cpp
        physics/SimConstants.h
        physics/SimData.h
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
        physics/water/Buoyance.{h,cpp}
        physics/water/ScrewProp.{h,cpp}
        resources/CacheSystem.{h,cpp}
        resources/ContentManager.{h,cpp}
        resources/otc_fileformat/OTCFileFormat.{h,cpp}
        resources/odef_fileformat/ODefFileFormat.{h,cpp}
        resources/rig_def_fileformat/RigDef_File.{h,cpp}
        resources/rig_def_fileformat/RigDef_Node.{h,cpp}
        resources/rig_def_fileformat/RigDef_Parser.{h,cpp}
        resources/rig_def_fileformat/RigDef_Prerequisites.h
        resources/rig_def_fileformat/RigDef_Regexes.h
        resources/rig_def_fileformat/RigDef_SequentialImporter.{h,cpp}
        resources/rig_def_fileformat/RigDef_Serializer.{h,cpp}
        resources/rig_def_fileformat/RigDef_Validator.{h,cpp}
        resources/skin_fileformat/SkinFileFormat.{h,cpp}
        resources/terrn2_fileformat/Terrn2FileFormat.{h,cpp}
        resources/tobj_fileformat/TObjFileFormat.{h,cpp}
        system/AppCommandLine.cpp
        system/AppConfig.cpp
        system/Console.{h,cpp}
        system/ConsoleCmd.{h,cpp}
        system/CVar.{h,cpp}
        terrain/OgreTerrainPSSMMaterialGenerator.{h,cpp}
        terrain/TerrainEditor.{h,cpp}
        terrain/TerrainGeometryManager.{h,cpp}
        terrain/TerrainManager.{h,cpp}
        terrain/TerrainObjectManager.{h,cpp}
        threadpool/ThreadPool.h
        utils/CollisionTools.{h,cpp}
        utils/ConfigFile.{h,cpp}
        utils/ErrorUtils.{h,cpp}
        utils/FileSystemInfo.h
        utils/ForceFeedback.{h,cpp}
        utils/ImprovedConfigFile.h
        utils/InputEngine.{h,cpp}
        utils/InterThreadStoreVector.h
        utils/Language.{h,cpp}
        utils/MeshObject.{h,cpp}
        utils/PlatformUtils.{h,cpp}
        utils/SHA1.{h,cpp}
        utils/Utils.{h,cpp}
        utils/WriteTextToTexture.{h,cpp}
        utils/ZeroedMemoryAllocator.h
        )

if (ROR_USE_ANGELSCRIPT)
    list(APPEND SOURCE_FILES
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
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SOURCE_FILES})

if (WIN32)
    # clang-cl doesn't support resource files
    if (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        list(APPEND SOURCE_FILES "icon.rc")
    endif ()
endif ()


####################################################################################################
#  EXECUTABLE TARGET
####################################################################################################

set(BINNAME "RoR")
add_executable(${BINNAME} ${SOURCE_FILES})

if (WIN32)
    set_target_properties(${BINNAME} PROPERTIES WIN32_EXECUTABLE YES)
    # disable some annoying VS warnings:
    # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
    # warning C4305: 'initializing' : truncation from 'double' to 'const float'
    target_compile_options(${BINNAME} PRIVATE /wd4305 /wd4244)
    # Disable non-standard behavior
    target_compile_options(${BINNAME} PRIVATE /permissive-)
endif ()

if (MSVC)
    # set startup project for Visual Studio Builds
    set_property(
            DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            PROPERTY
            VS_STARTUP_PROJECT ${BINNAME}
    )
    # set Visual Studio working directory to the Configuration directory
    set_target_properties(
            ${BINNAME}
            PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}"
    )
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

if (WIN32)
    target_compile_definitions(${BINNAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif ()

####################################################################################################
#  INCLUDE DIRECTORIES
####################################################################################################

target_include_directories(${BINNAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/external/header_only
        .
        audio
        datatypes
        gameplay
        gfx
        gfx/camera
        gfx/hydrax
        gfx/particle
        gfx/procedural
        gfx/skyx
        gui
        gui/imgui
        gui/mygui
        gui/overlay
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
        resources/odef_fileformat/
        resources/otc_fileformat/
        resources/rig_def_fileformat
        resources/skin_fileformat
        resources/terrn2_fileformat
        resources/tobj_fileformat
        system
        rig_editor
        rig_editor/rig_data
        scripting
        terrain
        terrain/map
        threadpool
        utils
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

include("${CMAKE_SOURCE_DIR}/external/pagedgeometry.cmake")

if (UNIX)
    #  include_directories(${GTK_INCLUDE_DIRS})
    set(OS_LIBS "X11 -l${CMAKE_DL_LIBS} -lrt")
endif ()
target_link_libraries(${BINNAME} PRIVATE ${OS_LIBS} PagedGeometry version_info)

# --- Threading support (still needed for GCC even with C++11)
set(CMAKE_THREAD_PREFER_PTHREAD YES)
target_link_libraries(${BINNAME} PRIVATE Threads::Threads)


if (USE_PACKAGE_MANAGER)
    target_include_directories(${BINNAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/angelscript_addons)

    target_link_libraries(${BINNAME} PRIVATE
            CONAN_PKG::AngelScript
            CONAN_PKG::MyGUI
            CONAN_PKG::OGRE
            CONAN_PKG::OGREdeps
            CONAN_PKG::SocketW
            CONAN_PKG::discord-rpc
            CONAN_PKG::fmt
            CONAN_PKG::libcurl
            CONAN_PKG::ogre-caelum
            CONAN_PKG::openal
            CONAN_PKG::openssl
            CONAN_PKG::rapidjson
            CONAN_PKG::zlib
            Threads::Threads
            version_info
            angelscript_addons
            )

    target_compile_definitions(${BINNAME} PRIVATE
            AS_USE_NAMESPACE
            WIN32_LEAN_AND_MEAN
            USE_OPENAL
            USE_MOFILEREADER
            USE_MYGUI
            $<$<PLATFORM_ID:WINDOWS>:MYGUI_STATIC>
            USE_SOCKETW
            USE_ANGELSCRIPT
            AS_USE_NAMESPACE
            USE_CURL
            USE_CAELUM
            USE_DISCORD_RPC
            )

    if (UNIX)
        set(CFG_OGRE_PLUGIN_CAELUM "Plugin=libCaelum.so")
        set(CFG_OGRE_PLUGIN_CAELUM_D "Plugin=libCaelum_d.so")
    else ()
        set(CFG_OGRE_PLUGIN_CAELUM "Plugin=Caelum")
        set(CFG_OGRE_PLUGIN_CAELUM_D "Plugin=Caelum_d")
    endif ()

else (USE_PACKAGE_MANAGER)
    target_link_libraries(${BINNAME} PRIVATE OgreMain OgreBites OgreRTShaderSystem OgreOverlay OgreTerrain OgreMeshLodGenerator)

    target_link_libraries(${BINNAME} PRIVATE fmt::fmt)

    target_link_libraries(${BINNAME} PRIVATE ${OIS_LIBRARIES})
    target_include_directories(${BINNAME} PRIVATE ${OIS_INCLUDE_DIRS})

    include_directories(${MYGUI_INCLUDE_DIRS})
    target_link_libraries(${BINNAME} PRIVATE
            MyGUI::MyGUI
            MyGUI::OgrePlatform
            )
    target_compile_definitions(${BINNAME} PRIVATE
            USE_MYGUI
            $<$<PLATFORM_ID:WINDOWS>:MYGUI_STATIC>
            )

    if (OPENAL_FOUND)
        target_link_libraries(${BINNAME} PRIVATE ${OPENAL_LIBRARY})
        target_include_directories(${BINNAME} PRIVATE ${OPENAL_INCLUDE_DIR})
        target_compile_definitions(${BINNAME} PRIVATE USE_OPENAL)
    endif ()

    if (TARGET SocketW::SocketW)
        target_link_libraries(${BINNAME} PRIVATE SocketW::SocketW)
        target_compile_definitions(${BINNAME} PRIVATE USE_SOCKETW)
    endif ()

    if (TARGET AngelScript::AngelScript)
        target_link_libraries(${BINNAME} PRIVATE
                AngelScript::AngelScript
                AngelScript::use_namespace
                )
        target_compile_definitions(${BINNAME} PRIVATE USE_ANGELSCRIPT)
        target_include_directories(${BINNAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/angelscript_addons)
        target_link_libraries(${BINNAME} PRIVATE angelscript_addons)
    endif ()

    if (CURL_FOUND)
        target_link_libraries(${BINNAME} PRIVATE ${CURL_LIBRARY})
        target_include_directories(${BINNAME} PRIVATE ${CURL_INCLUDE_DIR})
        target_compile_definitions(${BINNAME} PRIVATE USE_CURL)

        if (WIN32)
            add_compile_definitions(WIN32_LEAN_AND_MEAN)
        endif ()
    endif ()

    if (TARGET Caelum::Caelum)
        set(CFG_OGRE_PLUGIN_CAELUM "Plugin=../libCaelum.so")
        set(CFG_OGRE_PLUGIN_CAELUM_D "Plugin=../libCaelum_d.so")
        target_link_libraries(${BINNAME} PRIVATE Caelum::Caelum)
        target_compile_definitions(${BINNAME} PRIVATE USE_CAELUM)
    endif ()

endif (USE_PACKAGE_MANAGER)

if (USE_PHC)
    target_precompile_headers(${BINNAME} PRIVATE phc.h)
endif ()

extract_pot("${SOURCE_FILES}")

####################################################################################################
#  POST-BUILD STEPS
####################################################################################################


# Configure plugins.cfg
# -----------------------
if (NOT WIN32)
    set(CFG_COMMENT_RENDERSYSTEM_D3D9 "# ")
endif ()

set(CFG_COMMENT_RENDERSYSTEM_D3D11 "# ")
set(CFG_COMMENT_RENDERSYSTEM_GL3PLUS "# ")

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    set(PLUGINS_FOLDER ".")
else ()
    set(PLUGINS_FOLDER ${OGRE_PLUGIN_DIR})
endif ()

configure_file(plugins.cfg.in ${RUNTIME_OUTPUT_DIRECTORY}/plugins.cfg)
configure_file(plugins_d.cfg.in ${RUNTIME_OUTPUT_DIRECTORY}/plugins_d.cfg)


# Copy resources to bin directory
# -----------------------
recursive_zip_folder("${CMAKE_SOURCE_DIR}/resources" "${RUNTIME_OUTPUT_DIRECTORY}/resources")
recursive_zip_folder("${CMAKE_SOURCE_DIR}/content" "${RUNTIME_OUTPUT_DIRECTORY}/content")

fast_copy("${CMAKE_SOURCE_DIR}/resources/managed_materials" "${RUNTIME_OUTPUT_DIRECTORY}/resources/managed_materials")
fast_copy("${CMAKE_SOURCE_DIR}/resources/fonts" "${RUNTIME_OUTPUT_DIRECTORY}/languages")
fast_copy("${CMAKE_SOURCE_DIR}/languages" "${RUNTIME_OUTPUT_DIRECTORY}/languages")


#  Install targets
# -----------------------
install(TARGETS ${BINNAME} DESTINATION .)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/fonts/ DESTINATION languages)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/languages/ DESTINATION languages FILES_MATCHING PATTERN "*.mo")
install(DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/resources/ DESTINATION resources)
install(DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/content/ DESTINATION content)

if (WIN32)
    install(FILES ${RUNTIME_OUTPUT_DIRECTORY}/plugins.cfg DESTINATION .)
    install(FILES ${CMAKE_SOURCE_DIR}/tools/windows/.itch.toml DESTINATION .)
    install(CODE "file(GLOB files ${RUNTIME_OUTPUT_DIRECTORY}/*.dll)
            file(INSTALL \${files} DESTINATION \${CMAKE_INSTALL_PREFIX})")
else ()
    set(PLUGINS_FOLDER "lib")
    configure_file(plugins.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins-install.cfg)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugins-install.cfg DESTINATION . RENAME plugins.cfg)
    install(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/linux/RunRoR DESTINATION .)
    install(FILES ${CMAKE_SOURCE_DIR}/tools/linux/.itch.toml DESTINATION .)
    install(CODE "file(GLOB_RECURSE files ${RUNTIME_OUTPUT_DIRECTORY}/*.so)
            file(INSTALL \${files} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib FOLLOW_SYMLINK_CHAIN)")
endif ()


#  CPack
# -----------------------
set(CPACK_PACKAGE_DESCRIPTION "Rigs of Rods soft-body physics simulator")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rigs of Rods is an open source vehicle simulator based on soft-body physics.")
set(CPACK_PACKAGE_CONTACT "Edgar@AnotherFoxGuy.com")
set(CPACK_PACKAGE_VENDOR "AnotherFoxGuy")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/tools/windows/license.rtf")
set(CPACK_PACKAGE_EXECUTABLES "RoR" "Rigs of Rods Simulator")


if (WIN32)
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

    set(CPACK_PACKAGE_NAME "Rigs of Rods")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Rigs of Rods")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}")
    set(CPACK_GENERATOR ZIP;WIX)
    set(CPACK_MODULE_PATH "")
else ()
    set(CPACK_GENERATOR ZIP)
endif ()

include(CPack)
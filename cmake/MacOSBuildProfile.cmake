# Opt-in compile foundation. Dependency recipes and source pins live in ror-dependencies.
if(NOT APPLE OR NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "ROR_MACOS_GL3_PROFILE requires a native macOS Apple Silicon host.")
endif()
execute_process(COMMAND uname -m OUTPUT_VARIABLE _ror_host_arch OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _ror_host_arch STREQUAL "arm64" OR NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "ROR_MACOS_GL3_PROFILE requires native arm64 and -DCMAKE_OSX_ARCHITECTURES=arm64.")
endif()
if(CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "ROR_MACOS_GL3_PROFILE supports single-configuration Release only; use Ninja and -DCMAKE_BUILD_TYPE=Release with a Release prefix.")
endif()
if(NOT ROR_CREATE_CONTENT_FOLDER)
    message(FATAL_ERROR "ROR_MACOS_GL3_PROFILE requires -DROR_CREATE_CONTENT_FOLDER=ON for its pinned base content output.")
endif()
find_package(Python3 3.7 REQUIRED COMPONENTS Interpreter)
set(ROR_MACOS_HELPER "${CMAKE_SOURCE_DIR}/scripts/macos/profile.py")
get_filename_component(ROR_DEPENDENCY_DIR "${ROR_DEPENDENCY_DIR}" REALPATH)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${ROR_MACOS_HELPER}" "${CMAKE_SOURCE_DIR}/source/main/plugins-macos.cfg.in"
    "${ROR_DEPENDENCY_DIR}/share/ror-dependencies/manifest.json"
    "${ROR_DEPENDENCY_DIR}/share/ror-dependencies/build-receipt.json"
    "${ROR_DEPENDENCY_DIR}/share/ror-dependencies/build-receipt.txt")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${ROR_MACOS_HELPER}" preflight
    --prefix "${ROR_DEPENDENCY_DIR}" --sdk "${CMAKE_OSX_SYSROOT}"
    --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}"
    RESULT_VARIABLE _ror_result OUTPUT_VARIABLE _ror_output ERROR_VARIABLE _ror_error)
if(NOT _ror_result EQUAL 0)
    message(FATAL_ERROR "macOS dependency contract: ${_ror_error}")
endif()
set(CMAKE_PREFIX_PATH "${ROR_DEPENDENCY_DIR}")
set(CMAKE_FIND_USE_PACKAGE_REGISTRY OFF)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY OFF)
set(CMAKE_FIND_FRAMEWORK LAST)
list(APPEND CMAKE_IGNORE_PREFIX_PATH /opt/homebrew /usr/local)
set(_ror_profile_features ROR_USE_OPENAL ROR_USE_ANGELSCRIPT ROR_USE_CURL
    ROR_USE_SOCKETW ROR_USE_DISCORD_RPC ROR_USE_CAELUM ROR_USE_PAGED)
# Preserve any additional requirements supplied by the caller. The existing gate
# diagnoses disabled features after dependency discovery; this does not enable them.
list(APPEND ROR_REQUIRED_FEATURES ${_ror_profile_features})
list(REMOVE_DUPLICATES ROR_REQUIRED_FEATURES)

include(MacOSProfileArtifacts)

function(ror_macos_build_handoff target)
    foreach(variable OPENAL_LIBRARY OPENAL_INCLUDE_DIR Caelum_LIBRARY Caelum_INCLUDE_DIR
        OIS_LIBRARY OIS_INCLUDE_DIR MyGUI_MyGUIEngine_LIBRARY_REL MyGUI_OgrePlatform_LIBRARY_REL
        MyGUI_INCLUDE_DIR PagedGeometry_LIBRARY PagedGeometry_INCLUDE_DIR RapidJSON_INCLUDE_DIRS OGRE_INCLUDE_DIRS)
        foreach(path IN LISTS ${variable})
            ror_macos_prefix_path("${path}" "${variable}")
        endforeach()
        if(NOT ${variable})
            message(FATAL_ERROR "macOS profile requires selected ${variable} inside the prefix")
        endif()
    endforeach()
    foreach(dependency OgreMain OgreBites OgreOverlay OgrePaging OgreRTShaderSystem OgreMeshLodGenerator OgreTerrain
        fmt::fmt Angelscript::angelscript CURL::libcurl discord-rpc::discord-rpc SocketW::SocketW OpenSSL::SSL OpenSSL::Crypto
        MyGUI::MyGUI MyGUI::OgrePlatform ois::ois Caelum::Caelum PagedGeometry::PagedGeometry)
        ror_macos_target(${dependency})
    endforeach()
    get_target_property(ogre_type OgreMain TYPE)
    if(NOT ogre_type STREQUAL "SHARED_LIBRARY")
        message(FATAL_ERROR "macOS profile requires shared OgreMain")
    endif()
    ror_macos_prefix_path("${OGRE_PLUGIN_DIR}" "OGRE_PLUGIN_DIR")
    set(plugin_args)
    foreach(plugin RenderSystem_GL3Plus Codec_STBI Plugin_ParticleFX Plugin_OctreeSceneManager)
        if(NOT plugin IN_LIST OGRE_PLUGINS)
            message(FATAL_ERROR "macOS profile requires ${plugin} in installed OGRE_PLUGINS")
        endif()
        list(APPEND plugin_args --plugin "${plugin}" "${OGRE_PLUGIN_DIR}/${plugin}.dylib")
    endforeach()
    set(handoff "${CMAKE_BINARY_DIR}/macos/Release")
    file(MAKE_DIRECTORY "${handoff}")
    # The validated MyGUI conditional interface must be unmapped Release, so
    # its selected library and the framework source below are the same input.
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${ROR_MACOS_HELPER}" handoff
        --prefix "${ROR_DEPENDENCY_DIR}" --source "${CMAKE_SOURCE_DIR}" --build "${CMAKE_BINARY_DIR}"
        --sdk "${CMAKE_OSX_SYSROOT}" --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        --plugin-folder "${OGRE_PLUGIN_DIR}" --caelum "${Caelum_LIBRARY}"
        --mygui-engine "${MyGUI_MyGUIEngine_LIBRARY_REL}" ${plugin_args}
        RESULT_VARIABLE result ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "macOS handoff: ${error}")
    endif()
    # Target path is emitted by CMake, then JSON-escaped by the serializer.
    file(GENERATE OUTPUT "${handoff}/target-path.txt" CONTENT "$<TARGET_FILE:${target}>")
    add_custom_target(macos-build-handoff ALL
        COMMAND "${Python3_EXECUTABLE}" "${ROR_MACOS_HELPER}" finalize --build "${CMAKE_BINARY_DIR}"
        VERBATIM)
    add_dependencies(macos-build-handoff ${target})
    target_link_options(${target} PRIVATE -Wl,-headerpad_max_install_names)
    file(REMOVE "${RUNTIME_OUTPUT_DIRECTORY}/plugins_d.cfg")
endfunction()

# Installation for the native macOS dependency profile. CPack uses these same rules.
function(ror_configure_macos_bundle target)
    if (CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        message(FATAL_ERROR "macOS packaging currently requires a single-configuration Release build")
    endif ()
    if (NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        message(FATAL_ERROR "macOS packaging currently supports the arm64 dependency profile")
    endif ()

    set(bundle "${target}.app")
    set(resources "${bundle}/Contents/Resources")
    set(runtime "${resources}/runtime")
    set(frameworks "${bundle}/Contents/Frameworks")
    set(plugins "${bundle}/Contents/PlugIns/OGRE")
    set(notices "${resources}/Notices")

    foreach (component MAJOR MINOR PATCH)
        string(REGEX REPLACE "^0+" "" ROR_BUNDLE_VERSION_${component} "${PROJECT_VERSION_${component}}")
        if (ROR_BUNDLE_VERSION_${component} STREQUAL "")
            set(ROR_BUNDLE_VERSION_${component} 0)
        endif ()
    endforeach ()
    set(ROR_BUNDLE_VERSION "${ROR_BUNDLE_VERSION_MAJOR}.${ROR_BUNDLE_VERSION_MINOR}.${ROR_BUNDLE_VERSION_PATCH}")
    set(ROR_BUNDLE_MINIMUM_OS "")
    if (CMAKE_OSX_DEPLOYMENT_TARGET)
        set(ROR_BUNDLE_MINIMUM_OS "<key>LSMinimumSystemVersion</key>\n    <string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>")
    endif ()
    configure_file("${CMAKE_SOURCE_DIR}/tools/macos/Info.plist.in"
        "${CMAKE_CURRENT_BINARY_DIR}/RoR-Info.plist" @ONLY)
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/RoR-Info.plist")
    target_link_options(${target} PRIVATE "-Wl,-headerpad_max_install_names")
    install(TARGETS ${target} BUNDLE DESTINATION . COMPONENT Base_Game)
    install(FILES "${CMAKE_SOURCE_DIR}/tools/macos/ror.icns" DESTINATION "${resources}" COMPONENT Base_Game)
    install(DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/resources/" DESTINATION "${runtime}/resources" COMPONENT Base_Game)
    install(DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/languages/" DESTINATION "${runtime}/languages" COMPONENT Base_Game)
    install(DIRECTORY DESTINATION "${runtime}/content" COMPONENT Base_Game)
    if (ROR_CREATE_CONTENT_FOLDER)
        install(DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/content/" DESTINATION "${runtime}/content" COMPONENT Base_Content)
    endif ()

    # These plugins are loaded by name, so linked-library traversal cannot find them.
    set(plugin_files "")
    set(plugin_config "# Native macOS runtime plugins\nPluginFolder=../PlugIns/OGRE\n")
    foreach (name RenderSystem_GL3Plus Codec_STBI Plugin_ParticleFX Plugin_OctreeSceneManager)
        unset(plugin_file)
        unset(plugin_file CACHE)
        find_file(plugin_file NAMES "${name}.dylib" PATHS
            "${OGRE_PLUGIN_DIR}" "${ROR_DEPENDENCY_DIR}/lib/OGRE" NO_DEFAULT_PATH)
        if (NOT plugin_file)
            message(FATAL_ERROR "macOS packaging requires ${name}.dylib in the selected OGRE plugin directory")
        endif ()
        get_filename_component(plugin_real "${plugin_file}" REALPATH)
        install(FILES "${plugin_real}" DESTINATION "${plugins}" RENAME "${name}.dylib" COMPONENT Base_Game)
        list(APPEND plugin_files "${plugins}/${name}.dylib")
        string(APPEND plugin_config "Plugin=${name}\n")
    endforeach ()
    unset(plugin_file CACHE)

    if (ROR_USE_CAELUM)
        set(caelum "${Caelum_LIBRARY}")
        if (NOT EXISTS "${caelum}" OR NOT caelum MATCHES "\\.dylib$")
            message(FATAL_ERROR "macOS packaging requires the selected shared Caelum library")
        endif ()
        get_filename_component(caelum_name "${caelum}" NAME)
        install(FILES "${caelum}" DESTINATION "${frameworks}" COMPONENT Base_Game)
        list(APPEND plugin_files "${frameworks}/${caelum_name}")
        string(APPEND plugin_config "Plugin=../../Frameworks/${caelum_name}\n")
    endif ()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugins-macos.cfg" "${plugin_config}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plugins-macos.cfg" DESTINATION "${runtime}"
        RENAME plugins.cfg COMPONENT Base_Game)

    # MyGUI already uses its future @executable_path/../Frameworks install name.
    # Make the full versioned framework available before BundleUtilities resolves it.
    set(mygui "${MyGUI_MyGUIEngine_LIBRARY_REL}")
    if (NOT IS_DIRECTORY "${mygui}" OR NOT mygui MATCHES "\\.framework$")
        message(FATAL_ERROR "macOS packaging requires MyGUI_MyGUIEngine_LIBRARY_REL to be a complete framework")
    endif ()
    install(DIRECTORY "${mygui}" DESTINATION "${frameworks}" COMPONENT Base_Game)

    install(DIRECTORY "${ROR_DEPENDENCY_DIR}/share/ror-dependencies/licenses/"
        DESTINATION "${notices}/dependencies" COMPONENT Base_Game)
    foreach (notice COPYING source/main/COPYING source/main/gui/imgui/LICENSE-ImGui.txt
            source/main/gui/imgui/LICENSE-OGRE.txt external/header_only/half/LICENSE.txt
            external/header_only/SimpleGlob.h external/header_only/SimpleOpt.h)
        get_filename_component(notice_dir "${notice}" DIRECTORY)
        install(FILES "${CMAKE_SOURCE_DIR}/${notice}" DESTINATION "${notices}/rigs-of-rods/${notice_dir}" COMPONENT Base_Game)
    endforeach ()
    foreach (directory external/header_only/utf8 external/angelscript_addons)
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/${directory}/" DESTINATION "${notices}/rigs-of-rods/${directory}"
            COMPONENT Base_Game FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.cpp" PATTERN "*.txt" PATTERN "*.md")
    endforeach ()
    install(FILES "${CMAKE_SOURCE_DIR}/content/LICENSE" DESTINATION "${notices}/base-content" COMPONENT Base_Game)
    install(FILES "${CMAKE_SOURCE_DIR}/resources/fonts/FONT-NOTICES.md"
        "${CMAKE_SOURCE_DIR}/resources/fonts/LICENSE-APACHE2.txt" "${CMAKE_SOURCE_DIR}/resources/fonts/LICENSE-OFL.txt"
        DESTINATION "${notices}/fonts" COMPONENT Base_Game)

    set(ROR_MACOS_BUNDLE "${bundle}")
    set(ROR_MACOS_PLUGIN_FILES "${plugin_files}")
    set(ROR_MACOS_SEARCH_DIRS "${ROR_DEPENDENCY_DIR}/lib;${ROR_DEPENDENCY_DIR}/lib/OGRE;${OGRE_PLUGIN_DIR}")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/MacOSBundleInstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MacOSBundleInstall.cmake" @ONLY)
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/MacOSBundleInstall.cmake" COMPONENT Base_Game)
endfunction()

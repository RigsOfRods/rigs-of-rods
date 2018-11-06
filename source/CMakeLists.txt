include(FetchContent)

find_package(Threads REQUIRED)

if (NOT WIN32)
    find_package(wxWidgets COMPONENTS base core html net adv)
endif ()

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    # find packages

    find_package(OGRE 1.11 REQUIRED COMPONENTS Bites HLMS Overlay Paging RTShaderSystem MeshLodGenerator Terrain)
    find_package(OpenAL)
    find_package(OIS REQUIRED)
    find_package(MyGUI REQUIRED)
    find_package(SocketW)
    find_package(AngelScript)
    find_package(CURL)
    find_package(Caelum)

    if (WIN32)
        find_package(MoFileReader REQUIRED)
    endif ()
endif ()

add_subdirectory(version_info)
add_subdirectory(main)

if (ROR_USE_ANGELSCRIPT)
    add_subdirectory(angelscript_addons)
endif ()

set(ROR_BUILD_CONFIGURATOR "TRUE" CACHE BOOL "build the configurator")

IF (ROR_BUILD_CONFIGURATOR)
    add_subdirectory(configurator)
ENDIF ()

####################################################################################################
#  Setup content and resources for Rigs of Rods
####################################################################################################


FetchContent_Declare(
        RoR_Content
        GIT_REPOSITORY https://github.com/RigsOfRods/content.git
)

FetchContent_GetProperties(RoR_Content)
if (NOT ror_content_POPULATED)
    FetchContent_Populate(RoR_Content)
endif ()

recursive_zip_folder(resources "${CMAKE_SOURCE_DIR}/resources" "${RUNTIME_OUTPUT_DIRECTORY}/resources/" ROR_RESOURCES_ZIPS)
recursive_zip_folder(content "${ror_content_SOURCE_DIR}" "${RUNTIME_OUTPUT_DIRECTORY}/content" ROR_CONTENT_ZIPS)

# Copy resources needed for execution to the build directory
add_custom_target(
        zip_and_copy_resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/managed_materials ${RUNTIME_OUTPUT_DIRECTORY}/resources/managed_materials
        COMMENT "Zip and copy resources to build directory"
        DEPENDS ${ROR_RESOURCES_ZIPS} ${ROR_CONTENT_ZIPS}
        VERBATIM
)

if (BUILD_REDIST_FOLDER)
    INSTALL(
            DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/resources/
            DESTINATION ${REDIST_FOLDER}/resources/
    )
    INSTALL(
            DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/content/
            DESTINATION ${REDIST_FOLDER}/content/
    )
    INSTALL(
            DIRECTORY ${CMAKE_SOURCE_DIR}/resources/managed_materials/
            DESTINATION ${REDIST_FOLDER}/resources/managed_materials/
    )
    if (WIN32)
        INSTALL(FILES ${CMAKE_SOURCE_DIR}/tools/windows/.itch.toml
                DESTINATION ${CMAKE_BINARY_DIR}/redist
                )
    else ()
        INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/linux/RunRoR
                DESTINATION ${CMAKE_BINARY_DIR}/redist
                )
        INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/tools/linux/RunRoRConfig
                DESTINATION ${CMAKE_BINARY_DIR}/redist
                )
        INSTALL(FILES ${CMAKE_SOURCE_DIR}/tools/linux/.itch.toml
                DESTINATION ${CMAKE_BINARY_DIR}/redist
                )
    endif ()
endif ()

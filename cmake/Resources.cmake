####################################################################################################
#  Setup content and resources for Rigs of Rods
####################################################################################################

recursive_zip_folder(resources "${CMAKE_SOURCE_DIR}/resources" "${RUNTIME_OUTPUT_DIRECTORY}/resources/" ROR_RESOURCES_ZIPS)
recursive_zip_folder(content "${CMAKE_SOURCE_DIR}/content" "${RUNTIME_OUTPUT_DIRECTORY}/content" ROR_CONTENT_ZIPS)

file(GLOB_RECURSE mofiles "${CMAKE_SOURCE_DIR}/languages/*/*.mo")

foreach (mofile IN LISTS mofiles)
    get_filename_component(dir "${mofile}" DIRECTORY)
    get_filename_component(lang_name "${dir}" NAME)
    get_filename_component(filename ${mofile} NAME)
    set(dest_dir "${RUNTIME_OUTPUT_DIRECTORY}/languages/${lang_name}")
    set(dest_file "${dest_dir}/${filename}")
    add_custom_command(
            COMMENT "Copying ${lang_name}.mo"
            OUTPUT ${dest_file}
            DEPENDS ${mofile}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${dest_dir}
            COMMAND ${CMAKE_COMMAND} -E copy ${mofile} ${dest_file}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    list(APPEND ROR_MOFILES ${dest_file})
endforeach ()

# Copy resources needed for execution to the build directory
add_custom_target(
        zip_and_copy_resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/managed_materials ${RUNTIME_OUTPUT_DIRECTORY}/resources/managed_materials
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/fonts/Roboto-Medium.ttf ${RUNTIME_OUTPUT_DIRECTORY}/languages/Roboto-Medium.ttf
        COMMENT "Zip and copy resources to build directory"
        DEPENDS ${ROR_RESOURCES_ZIPS} ${ROR_CONTENT_ZIPS} ${ROR_MOFILES}
        VERBATIM
)

if (BUILD_REDIST_FOLDER)
    INSTALL(
            DIRECTORY ${CMAKE_SOURCE_DIR}/resources/fonts/
            DESTINATION ${REDIST_FOLDER}/languages/
    )
    INSTALL(
            DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}/languages/
            DESTINATION ${REDIST_FOLDER}/languages/
    )
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
        INSTALL(FILES ${CMAKE_SOURCE_DIR}/tools/linux/.itch.toml
                DESTINATION ${CMAKE_BINARY_DIR}/redist
                )
    endif ()
endif ()

###################################################################################################
# Build a library which provides the versioning info defined for the current CMake project
###################################################################################################

# All three generated outputs share one evaluation of the version, clock and Git state.
set(VERSION_INFO_CPP "${CMAKE_CURRENT_BINARY_DIR}/version_info.cpp")
set(VERSION_DEF_H "${CMAKE_CURRENT_BINARY_DIR}/version_def.h")
set(PACKAGE_VERSION_JSON "${CMAKE_CURRENT_BINARY_DIR}/package-version.json")

find_package(Git)

set(VERSION_BUILD_DEV "${ROR_BUILD_DEV_VERSION}")
set(VERSION_BUILD_CUSTOM FALSE)
set(VERSION_CUSTOM "")
if(ROR_BUILD_INSTALLER STREQUAL "Off")
    if(DEFINED ROR_CUSTOM_VERSION)
        set(VERSION_BUILD_CUSTOM TRUE)
        set(VERSION_CUSTOM "${ROR_CUSTOM_VERSION}")
    endif()
else()
    # Installer version selection takes precedence over both custom and dev versions.
    set(VERSION_BUILD_DEV OFF)
    set(VERSION_BUILD_CUSTOM TRUE)
    set(VERSION_CUSTOM "${PROJECT_VERSION}")
endif()

# Pass arbitrary custom text as data, avoiding CMake list/command and generator-expression
# interpretation. The file is consumed only when VERSION_BUILD_CUSTOM is true.
set(VERSION_CUSTOM_FILE "${CMAKE_CURRENT_BINARY_DIR}/custom-version.txt")
file(WRITE "${VERSION_CUSTOM_FILE}" "${VERSION_CUSTOM}")

# Custom targets always run. configure_file keeps unchanged output timestamps, avoiding
# unnecessary recompilation. Declaring every byproduct also supports clean/rebuild with Ninja.
add_custom_target(
    generate_version
    COMMAND "${CMAKE_COMMAND}"
        "-DBUILD_DEV_VERSION=${VERSION_BUILD_DEV}"
        "-DBUILD_CUSTOM_VERSION=${VERSION_BUILD_CUSTOM}"
        "-DCUSTOM_VERSION_FILE=${VERSION_CUSTOM_FILE}"
        "-DPROJECT_VERSION=${PROJECT_VERSION}"
        "-DVERSION_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DGIT_EXECUTABLE=${GIT_EXECUTABLE}"
        "-DVERSION_INFO_INPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DVERSION_INFO_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateVersionInfo.cmake"
    BYPRODUCTS "${VERSION_INFO_CPP}" "${VERSION_DEF_H}" "${PACKAGE_VERSION_JSON}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generate files containing version information"
    VERBATIM
)

# The version_info library target
# Other targets can make use of the version information with `target_link_libraries( <target> version_info )`
add_library(version_info STATIC ${VERSION_INFO_CPP})
add_dependencies(version_info generate_version)
target_include_directories(
        version_info PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

set_property(TARGET version_info PROPERTY FOLDER "Version Generation")
set_property(TARGET generate_version PROPERTY FOLDER "Version Generation")
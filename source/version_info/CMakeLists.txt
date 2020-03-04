###################################################################################################
# Build a library which provides the versioning info defined for the current CMake project
###################################################################################################

# Temporary generated files
set(VERSION_INFO_CPP ${CMAKE_CURRENT_BINARY_DIR}/version_info.cpp)
set(VERSION_DEF_H ${CMAKE_CURRENT_BINARY_DIR}/version_def.h)

# Search for Git executable
find_package(Git)

# CMake custom targets are always out of date. Therefore the following target will update the
# versioning information on each build. It executes the GenerateVersionInfo.cmake script which
# generates the version_info.cpp and version_def.h files.
set(VERSION_FLAGS -DBUILD_DEV_VERSION=${BUILD_DEV_VERSION})

add_custom_target(generate_version
        COMMAND ${CMAKE_COMMAND} ${VERSION_FLAGS}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -DVERSION_FILE_INPUT=${CMAKE_CURRENT_SOURCE_DIR}/RoRVersion.cpp.in
        -DVERSION_FILE_OUTPUT=${VERSION_INFO_CPP}
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateVersionInfo.cmake"
        COMMAND ${CMAKE_COMMAND} ${VERSION_FLAGS}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -DVERSION_FILE_INPUT=${CMAKE_CURRENT_SOURCE_DIR}/RoRVersionDef.h.in
        -DVERSION_FILE_OUTPUT=${VERSION_DEF_H}
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateVersionInfo.cmake"
        BYPRODUCTS ${VERSION_INFO_CPP} ${VERSION_DEF_H}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generate files containing version information"
        )

# The version_info library target
# Other targets can make use of the version information with `target_link_libraries( <target> version_info )`
add_library(version_info STATIC ${VERSION_INFO_CPP})
add_dependencies(version_info generate_version)
target_include_directories(version_info PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        )

cmake_minimum_required(VERSION 3.16)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/find-modules)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" CACHE STRING "Configuration types")

include(CMakeDependentOption)
include(Macros)
include(FeatureSummary)

################################################################################
# Project definition
################################################################################
set(PROJECT_NAME_SHORT "RoR")
set(PROJECT_NAME_UNDERSCORE "Rigs_of_Rods")
set(PROJECT_NAME_LONG "Rigs of Rods")

set(REQUIRED_DEPS_VERSION 31)

project(${PROJECT_NAME_UNDERSCORE})

################################################################################
# Available build options
################################################################################
option(BUILD_DEV_VERSION "Disable this for official releases" ON)
option(BUILD_DOC_DOXYGEN "Build documentation from sources with Doxygen" OFF)
option(USE_PCH "Use a Precompiled header for speeding up the build" ON)
option(CREATE_CONTENT_FOLDER "Create the base content folder" ON)
set(ROR_DEPENDENCY_DIR "${CMAKE_SOURCE_DIR}/dependencies" CACHE PATH "Path to the dependencies")
set(ROR_FEAT_TIMING OFF)

# global cmake options
SET(BUILD_SHARED_LIBS ON)
SET(CMAKE_USE_RELATIVE_PATHS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)

# setup paths
SET(CMAKE_PREFIX_PATH ${ROR_DEPENDENCY_DIR} ${CMAKE_PREFIX_PATH})
SET(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})

set(CMAKE_DEBUG_POSTFIX _d)

if (NOT USE_PCH)
    find_program(SCCACHE_PROGRAM sccache)
    if (SCCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        message("Using ${SCCACHE_PROGRAM} for speeding up build")
    endif ()
endif ()

# Check deps version
if (EXISTS "${ROR_DEPENDENCY_DIR}/version.cmake")
    include("${ROR_DEPENDENCY_DIR}/version.cmake")
    if (NOT ${REQUIRED_DEPS_VERSION} EQUAL ${DEPENDENCIES_VERSION})
        message(FATAL_ERROR "\n The ror-dependencies version is ${DEPENDENCIES_VERSION}, while RoR requires ${REQUIRED_DEPS_VERSION} \n Please update the dependencies")
    endif ()
endif ()

# hide some settings
mark_as_advanced(
        CMAKE_DEBUG_POSTFIX
        CMAKE_BACKWARDS_COMPATIBILITY
        EXECUTABLE_OUTPUT_PATH
        LIBRARY_OUTPUT_PATH
)

if (WIN32)
    add_definitions("/D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /wd4251 /wd4275 /wd4099 /nologo")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}        /MP /GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /MP /Zi")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}     /MP /Zi")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /Od /Zi /Gy /fp:except /GF- /GS /Ob0")
    #if(ROR_USING_VISUAL_STUDIO_PROFESSIONAL)
    #  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /analyze") # /analyze only available when using VS professional
    #endif(ROR_USING_VISUAL_STUDIO_PROFESSIONAL)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}          /MP /Od /Zi /RTC1 /Gy /fp:except /GF- /GS /Ob0")
    # some general flags
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    include_directories(${DirectX_INCLUDE_DIR})

    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /DEBUG /LARGEADDRESSAWARE ")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /DEBUG /LARGEADDRESSAWARE /OPT:REF /RELEASE /LTCG ")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /DEBUG /LARGEADDRESSAWARE /OPT:REF /LTCG ")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /LARGEADDRESSAWARE /OPT:NOREF /MAP /DYNAMICBASE:NO /FIXED /NXCOMPAT:NO ")

    # fix executable paths for windows
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_OUTPUT_DIRECTORY})

ELSEIF (UNIX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}          -Og -g -Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}        -O2 -ffast-math -DNDEBUG -Wno-deprecated-declarations -Wno-attributes")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -ffast-math -g -fno-omit-frame-pointer -Wno-deprecated-declarations -Wno-attributes")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}     -Os -ffast-math -DNDEBUG")

    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")

    # add additional gdb debugging stuff
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Og -ggdb")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} -Og -ggdb")
endif (WIN32)

include(DependenciesConfig)

################################################################################
# Recurse into subdirectories
################################################################################
add_subdirectory(external/angelscript_addons)
add_subdirectory(source/version_info)
add_subdirectory(source/main)
add_subdirectory(doc)

feature_summary(WHAT ALL)

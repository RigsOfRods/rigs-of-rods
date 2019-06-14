cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/3rdparty)

include(cotire)
include(CMakeDependentOption)
include(Macros)
include(FeatureSummary)

################################################################################
# Project definition
################################################################################
set(PROJECT_NAME_SHORT "RoR")
set(PROJECT_NAME_UNDERSCORE "Rigs_of_Rods")
set(PROJECT_NAME_LONG "Rigs of Rods")

project(${PROJECT_NAME_UNDERSCORE} VERSION 0.4.8.0)

################################################################################
# Available build options
################################################################################
option(BUILD_DEV_VERSION "Disable this for official releases" ON)
option(BUILD_DOC_DOXYGEN "Build documentation from sources with Doxygen" OFF)
option(BUILD_REDIST_FOLDER "Build a folder for redistributing the game" OFF)
option(USE_PACKAGE_MANAGER "Use conan for managing packages" ON)
cmake_dependent_option(USE_PMM "Use PMM for managing conan" OFF MSVC ON)

# global cmake options
SET(BUILD_SHARED_LIBS ON)
SET(CMAKE_USE_RELATIVE_PATHS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# setup paths
SET(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
IF (BUILD_REDIST_FOLDER)
    if (WIN32)
        set(REDIST_FOLDER ${CMAKE_BINARY_DIR}/redist)
    else ()
        set(REDIST_FOLDER ${CMAKE_BINARY_DIR}/redist/bin)
    endif ()
ENDIF ()

set(CMAKE_DEBUG_POSTFIX _d)

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
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /DEBUG /LARGEADDRESSAWARE /OPT:REF /LTCG:STATUS /RELEASE")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /DEBUG /LARGEADDRESSAWARE /OPT:REF /LTCG:STATUS ")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /LARGEADDRESSAWARE /OPT:NOREF /MAP /DYNAMICBASE:NO /FIXED /NXCOMPAT:NO")

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


################################################################################
# Check for dependencies
################################################################################
include(DependenciesConfig)


################################################################################
# Recurse into subdirectories
################################################################################
add_subdirectory(source/angelscript_addons)
add_subdirectory(source/version_info)
add_subdirectory(source/main)
add_subdirectory(test)

include(Resources)
include(Install)

add_subdirectory(doc)

feature_summary(WHAT ALL)

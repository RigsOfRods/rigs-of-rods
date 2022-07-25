
####################################################################################################
#  SOURCE FILES
####################################################################################################

set(SOURCE_FILES
        main.cpp
        MessageHandler.{h,cpp}
    )
    
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
        .
        ${CMAKE_SOURCE_DIR}/external/header_only
        ${CMAKE_SOURCE_DIR}/source/main
        ${CMAKE_SOURCE_DIR}/source/main/audio
        ${CMAKE_SOURCE_DIR}/source/main/datatypes
        ${CMAKE_SOURCE_DIR}/source/main/gameplay
        ${CMAKE_SOURCE_DIR}/source/main/gfx
        ${CMAKE_SOURCE_DIR}/source/main/gfx/camera
        ${CMAKE_SOURCE_DIR}/source/main/gfx/hydrax
        ${CMAKE_SOURCE_DIR}/source/main/gfx/particle
        ${CMAKE_SOURCE_DIR}/source/main/gfx/procedural
        ${CMAKE_SOURCE_DIR}/source/main/gfx/skyx
        ${CMAKE_SOURCE_DIR}/source/main/gui
        ${CMAKE_SOURCE_DIR}/source/main/gui/imgui
        ${CMAKE_SOURCE_DIR}/source/main/gui/mygui
        ${CMAKE_SOURCE_DIR}/source/main/gui/panels
        ${CMAKE_SOURCE_DIR}/source/main/network
        ${CMAKE_SOURCE_DIR}/source/main/physics
        ${CMAKE_SOURCE_DIR}/source/main/physics/air
        ${CMAKE_SOURCE_DIR}/source/main/physics/collision
        ${CMAKE_SOURCE_DIR}/source/main/physics/flex
        ${CMAKE_SOURCE_DIR}/source/main/physics/mplatform
        ${CMAKE_SOURCE_DIR}/source/main/physics/threading
        ${CMAKE_SOURCE_DIR}/source/main/physics/utils
        ${CMAKE_SOURCE_DIR}/source/main/physics/water
        ${CMAKE_SOURCE_DIR}/source/main/resources
        ${CMAKE_SOURCE_DIR}/source/main/resources/odef_fileformat/
        ${CMAKE_SOURCE_DIR}/source/main/resources/otc_fileformat/
        ${CMAKE_SOURCE_DIR}/source/main/resources/rig_def_fileformat
        ${CMAKE_SOURCE_DIR}/source/main/resources/skin_fileformat
        ${CMAKE_SOURCE_DIR}/source/main/resources/terrn2_fileformat
        ${CMAKE_SOURCE_DIR}/source/main/resources/tobj_fileformat
        ${CMAKE_SOURCE_DIR}/source/main/system
        ${CMAKE_SOURCE_DIR}/source/main/rig_editor
        ${CMAKE_SOURCE_DIR}/source/main/rig_editor/rig_data
        ${CMAKE_SOURCE_DIR}/source/main/scripting
        ${CMAKE_SOURCE_DIR}/source/main/terrain
        ${CMAKE_SOURCE_DIR}/source/main/terrain/map
        ${CMAKE_SOURCE_DIR}/source/main/threadpool
        ${CMAKE_SOURCE_DIR}/source/main/utils
        )

target_include_directories(${BINNAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/external/header_only
        ..
        .
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

target_link_libraries(${BINNAME} PRIVATE RoR_Main)

if (UNIX)
    #  include_directories(${GTK_INCLUDE_DIRS})
    set(OS_LIBS "X11 -l${CMAKE_DL_LIBS} -lrt")
endif ()
target_link_libraries(${BINNAME} PRIVATE ${OS_LIBS} version_info)

# --- Threading support (still needed for GCC even with C++11)
set(CMAKE_THREAD_PREFER_PTHREAD YES)
target_link_libraries(${BINNAME} PRIVATE
        Threads::Threads
        OGRE::OGRE
        OIS::OIS

        MyGUI::MyGUI
        RapidJSON::RapidJSON
        )

if (TARGET MyGUI::OgrePlatform)
    target_link_libraries(${BINNAME} PRIVATE MyGUI::OgrePlatform)
endif ()
target_compile_definitions(${BINNAME} PRIVATE $<$<PLATFORM_ID:WINDOWS>:MYGUI_STATIC>)



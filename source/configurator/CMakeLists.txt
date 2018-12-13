# ================================================================================================ #
#  CONFIGURATOR                                                                                    #
#
# This CMake listfile builds the Configurator for Rigs of Rods. This tool is allows to configure
# various settings of the Simulator.
#
project(RoR_Configurator)


if (WIN32)
    set(OS_LIBS "${PThread_LIBRARIES};dinput8.lib;dxguid.lib")
    # clang-cl doesn't support resource files
    if (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        set(OS_SOURCE "icon.rc")
    endif ()

    # disable some annoying VS warnings:
    # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
    # warning C4305: 'initializing' : truncation from 'double' to 'const float'
    add_definitions("/wd4305 /wd4244")
elseif (UNIX)
    find_package(PkgConfig)
    PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
    include_directories(${GTK_INCLUDE_DIRS})

    set(OS_LIBS)

endif ()

file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

include_directories(../main/)
include_directories(../main/network/)
include_directories(../main/utils/)

add_definitions("-DNOOGRE -D_UNICODE")

if (WIN32)
    add_executable(RoRConfig WIN32 ${sources} ${headers} ${OS_SOURCE} ../main/utils/SHA1.cpp ../main/utils/ErrorUtils.cpp)
    target_link_libraries(RoRConfig PRIVATE Version.lib)
else (WIN32)
    add_definitions("-fPIC")
    add_executable(RoRConfig ${sources} ${headers} ${OS_SOURCE}../main/utils/SHA1.cpp ../main/utils/ErrorUtils.cpp)
endif (WIN32)


if (DEFINED CONAN_WXWIDGETS_ROOT AND WIN32)
    # wxwidgets from Conan
    target_include_directories(RoRConfig PRIVATE CONAN_PKG::wxwidgets)
    target_link_libraries(RoRConfig PRIVATE CONAN_PKG::wxwidgets)
else ()
    set(wxWidgets_USE_UNICODE ON)
    include(${wxWidgets_USE_FILE})
    target_link_libraries(RoRConfig PRIVATE ${wxWidgets_LIBRARIES})
endif ()

if (DEFINED CONAN_OGRE_ROOT)
    include_directories(${CONAN_INCLUDE_DIRS_OGRE}/OGRE)
    target_link_libraries(RoRConfig PRIVATE CONAN_PKG::OGRE)
else ()
    target_link_libraries(RoRConfig PRIVATE
            ${OGRE_LIBRARIES}
            ${OGRE_Paging_LIBRARIES}
            ${OGRE_RTShaderSystem_LIBRARIES}
            ${OGRE_Terrain_LIBRARIES}
            ${OGRE_Overlay_LIBRARIES}
            )
    target_include_directories(RoRConfig PRIVATE
            ${OGRE_INCLUDE_DIRS}
            ${OGRE_Paging_INCLUDE_DIRS}
            ${OGRE_RTShaderSystem_INCLUDE_DIRS}
            ${OGRE_Terrain_INCLUDE_DIRS}
            ${OGRE_Overlay_INCLUDE_DIRS}
            )
endif ()

if (DEFINED CONAN_OGREDEPS_ROOT)
    include_directories(${CONAN_INCLUDE_DIRS_OGREDEPS}/OIS)
    target_link_libraries(RoRConfig PRIVATE CONAN_PKG::OGREdeps)
else ()
    target_link_libraries(RoRConfig PRIVATE ${OIS_LIBRARIES})
    target_include_directories(RoRConfig PRIVATE ${OIS_INCLUDE_DIRS})
endif ()

if (DEFINED CONAN_MYGUI_ROOT)
    include_directories(${CONAN_INCLUDE_DIRS_MYGUI}/MYGUI)
    target_link_libraries(RoRConfig PRIVATE CONAN_PKG::MyGUI)
else ()
    target_link_libraries(RoRConfig PRIVATE
            MyGUI::MyGUI
            MyGUI::OgrePlatform
            )
    target_compile_definitions(RoRConfig PRIVATE
            USE_MYGUI
            $<$<PLATFORM_ID:WINDOWS>:MYGUI_STATIC>
            )
endif ()

if (DEFINED CONAN_MOFILEREADER_ROOT)
    target_include_directories(RoRConfig PRIVATE CONAN_PKG::MofileReader)
    target_link_libraries(RoRConfig PRIVATE CONAN_PKG::MofileReader)
    target_compile_definitions(RoRConfig PRIVATE USE_MOFILEREADER)
elseif (MoFileReader_FOUND)
    target_link_libraries(RoRConfig PRIVATE MoFileReader::MoFileReader)
    target_compile_definitions(RoRConfig PRIVATE USE_MOFILEREADER)
else()
    target_compile_definitions(RoRConfig PRIVATE NOLANG)
endif ()

# OpenAL - audio library
if (DEFINED CONAN_OPENAL_ROOT)
    target_link_libraries(RoRConfig PRIVATE CONAN_PKG::openal)
    target_include_directories(RoRConfig PRIVATE CONAN_PKG::openal)
    target_compile_definitions(RoRConfig PRIVATE USE_OPENAL)
else ()
    if (OPENAL_FOUND)
        target_link_libraries(RoRConfig PRIVATE ${OPENAL_LIBRARY})
        target_include_directories(RoRConfig PRIVATE ${OPENAL_INCLUDE_DIR})
        target_compile_definitions(RoRConfig PRIVATE USE_OPENAL)
    else ()
        message(WARNING "Building RoRConfig without OpenAL. Some functionality may be missing.")
    endif ()
endif ()

target_link_libraries(RoRConfig PRIVATE
        ${OS_LIBS}
        ${optional_libs}
        version_info
        )

IF (BUILD_REDIST_FOLDER)
    INSTALL(TARGETS RoRConfig
            RUNTIME DESTINATION ${REDIST_FOLDER}
            LIBRARY DESTINATION ${REDIST_FOLDER}
            ARCHIVE DESTINATION ${REDIST_FOLDER}
            )
ENDIF ()

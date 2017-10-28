# ================================================================================================ #
#  CONFIGURATOR                                                                                    #
#
# This CMake listfile builds the Configurator for Rigs of Rods. This tool is allows to configure
# various settings of the Simulator.
#
project(RoR_Configurator)


if (WIN32)
    set(OS_LIBS "${PThread_LIBRARIES};dinput8.lib;dxguid.lib")
    set(OS_SOURCE "icon.rc")

    # disable some annoying VS warnings:
    # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
    # warning C4305: 'initializing' : truncation from 'double' to 'const float'
    add_definitions("/wd4305 /wd4244")
elseif (UNIX)
    find_package(PkgConfig)
    PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
    include_directories(${GTK_INCLUDE_DIRS})

    set(OS_LIBS "-lboost_system")

endif ()

file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

include_directories(../main/)
include_directories(../main/network/)
include_directories(../main/utils/)

add_definitions("-DNOOGRE -D_UNICODE")

# TODO OpenCL

# TODO GTK


if (WIN32)
    add_executable(RoRConfig WIN32 ${sources} ${headers} ${OS_SOURCE} ../main/utils/SHA1.cpp ../main/utils/ErrorUtils.cpp)
    target_link_libraries(RoRConfig PRIVATE Version.lib)
else (WIN32)
    add_definitions("-fPIC")
    add_executable(RoRConfig ${sources} ${headers} ${OS_SOURCE}../main/utils/SHA1.cpp ../main/utils/ErrorUtils.cpp)
ENDIF (WIN32)


# find wxwidgets
set(wxWidgets_USE_UNICODE ON)
find_package(wxWidgets REQUIRED COMPONENTS base core html net adv)
include(${wxWidgets_USE_FILE})
target_link_libraries(RoRConfig PRIVATE ${wxWidgets_LIBRARIES})


find_package(OGRE REQUIRED)
target_link_libraries(RoRConfig PRIVATE
        ${OGRE_LIBRARIES}
        ${OGRE_Paging_LIBRARIES}
        ${OGRE_RTShaderSystem_LIBRARIES}
        ${OGRE_Terrain_LIBRARIES}
        )
target_include_directories(RoRConfig PRIVATE
        ${OGRE_INCLUDE_DIRS}
        ${OGRE_Paging_INCLUDE_DIRS}
        ${OGRE_RTShaderSystem_INCLUDE_DIRS}
        ${OGRE_Terrain_INCLUDE_DIRS}
        )
if (OGRE_VERSION VERSION_GREATER 1.8)
    target_link_libraries(RoRConfig PRIVATE ${OGRE_Overlay_LIBRARIES})
    target_include_directories(RoRConfig PRIVATE ${OGRE_Overlay_INCLUDE_DIRS})
endif ()


find_package(OIS REQUIRED)
target_link_libraries(RoRConfig PRIVATE ${OIS_LIBRARIES})
target_include_directories(RoRConfig PRIVATE ${OIS_INCLUDE_DIRS})


find_package(MyGUI REQUIRED)
target_link_libraries(RoRConfig PRIVATE
        MyGUI::MyGUI
        MyGUI::OgrePlatform
        )
target_compile_definitions(RoRConfig PRIVATE
        USE_MYGUI
        $<$<PLATFORM_ID:WINDOWS>:MYGUI_STATIC>
        )

# OpenAL - audio library
find_package(OpenAL)
if (OPENAL_FOUND)
    target_link_libraries(RoRConfig PRIVATE ${OPENAL_LIBRARY})
    target_include_directories(RoRConfig PRIVATE ${OPENAL_INCLUDE_DIR})
    target_compile_definitions(RoRConfig PRIVATE USE_OPENAL)
else ()
    message(WARNING "Building RoRConfig without OpenAL. Some functionality may be missing.")
endif ()

target_link_libraries(RoRConfig PRIVATE
        ${OS_LIBS}
        ${optional_libs}
        version_info
)

IF (UNIX)
  INSTALL(TARGETS RoRConfig
          RUNTIME DESTINATION games/RoR
          LIBRARY DESTINATION games/RoR
          ARCHIVE DESTINATION games/RoR
  )
ENDIF (UNIX)

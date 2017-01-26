project(RoR_Configurator)

#setup libraries
macro(setup_lib name)
   if(ROR_USE_${name})
      include_directories(${${name}_INCLUDE_DIRS})
      link_directories   (${${name}_LIBRARY_DIRS})
      add_definitions("-DUSE_${name}")
      set(optional_libs ${optional_libs} ${${name}_LIBRARIES})
      #message(STATUS "${name} Enabled:      \tYES")
      #message(STATUS "${name}_INCLUDE_DIRS: \t${${name}_INCLUDE_DIRS}")
      #if(${${name}_LIBRARY_DIRS})
      #   message(STATUS "${name}_LIBRARY_DIRS: \t${${name}_LIBRARY_DIRS}")
      #endif(${${name}_LIBRARY_DIRS})
      #message(STATUS "${name}_LIBRARIES:    \t${${name}_LIBRARIES}")
   else()
	   #message(STATUS "${name} Enabled:      \tNO")
   endif(ROR_USE_${name})
endmacro(setup_lib)

if(WIN32)
	# use precompiled stuff
	set(wxWidgets_INCLUDE_DIRS "${ROR_DEPENDENCIES_DIR}/includes/${ARCH_DIR}/WxWidgets" CACHE PATH "The wxWidgets include path to use")
	set(wxWidgets_LIBRARY_DIRS "${ROR_DEPENDENCIES_DIR}/libs/${ARCH_DIR}/WxWidgets/Release" CACHE PATH "The wxWidgets lib path to use")
	set(wxWidgets_LIBRARIES    "wxbase31u.lib;wxmsw31u_core.lib;wxmsw31u_html.lib;wxbase31u_net.lib;wxmsw31u_adv.lib;wxpng.lib;wxtiff.lib;wxjpeg.lib;wxzlib.lib;wxregexu.lib;wxexpat.lib;winmm.lib;comctl32.lib;rpcrt4.lib;wsock32.lib;" CACHE STRING "The wxWidgets lib to link against")
	add_definitions("-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DUNICODE")
	include_directories(${wxWidgets_INCLUDE_DIRS})
	link_directories (${wxWidgets_LIBRARY_DIRS})
else(WIN32)
	# find wxwidgets
	SET(wxWidgets_USE_UNICODE ON)
	FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS base core html net adv)
	include( ${wxWidgets_USE_FILE} )
	#include_directories(${wxWidgets_INCLUDE_DIRS})
	#add_definitions("-D${wxWidgets_DEFINITIONS}" ${wxWidgets_CXX_FLAGS})
	#link_directories (${wxWidgets_LIB_DIR})
endif(WIN32)

IF(WIN32)
   set(OS_LIBS "dinput8.lib;dxguid.lib")
   set(OS_SOURCE "icon.rc")

   # disable some annoying VS warnings:
   # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
   # warning C4305: 'initializing' : truncation from 'double' to 'const float'
   add_definitions("/wd4305 /wd4244")


   #now: delay loading for opencl
   if(ROR_USE_OPENCL)
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /DELAYLOAD:OpenCL.dll")
      set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /DELAYLOAD:OpenCL.dll")
      set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /DELAYLOAD:OpenCL.dll")
      set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DELAYLOAD:OpenCL.dll")
      set(OS_LIBS ${OS_LIBS} Delayimp.lib)
   endif(ROR_USE_OPENCL)
ELSEIF(UNIX)
   find_package(PkgConfig)
   PKG_CHECK_MODULES  (GTK gtk+-2.0 REQUIRED)
   INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})

   PKG_CHECK_MODULES  (Ogre OGRE REQUIRED)
   PKG_CHECK_MODULES  (Ois OIS REQUIRED)

   set(OS_LIBS "-lboost_system")

ENDIF(WIN32)

setup_lib(OPENCL)
setup_lib(OPENAL)
include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})
include_directories(${MYGUI_INCLUDE_DIRS})

add_definitions("-DNOOGRE -D_UNICODE")
include_directories(../main/)
include_directories(../main/network/)
include_directories(../main/utils/)

FILE(GLOB sources "*.cpp")
FILE(GLOB headers "*.h")

IF(WIN32)
	add_executable(RoRConfig WIN32 ${sources} ${headers} ${OS_SOURCE} ../main/utils/SHA1.cpp ../main/utils/ErrorUtils.cpp)
	TARGET_LINK_LIBRARIES(RoRConfig Version.lib)
ELSE(WIN32)
        add_definitions ("-fPIC")
	add_executable(RoRConfig ${sources} ${headers} ${OS_SOURCE} ../main/utils/SHA1.cpp ../main/utils/ErrorUtils.cpp)
ENDIF(WIN32)

TARGET_LINK_LIBRARIES(RoRConfig ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${optional_libs} ${wxWidgets_LIBRARIES} version_info)




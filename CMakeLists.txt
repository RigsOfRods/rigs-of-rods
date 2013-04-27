# ensure cmake features we need
CMAKE_MINIMUM_REQUIRED( VERSION 2.6.2 )
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
# done with cmake check

# WIN32: remove MinSizeRel, Debug builds : unsupported
if (WIN32) 
	set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release" CACHE STRING "" FORCE)
endif()
	
project(RoR)
include(CMakeMacros.txt)


# check 64 bit
set(ARCH_DIR "")
if (WIN32) 
	message( STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
	message( STATUS "CMAKE_CL_64: ${CMAKE_CL_64}")
	if (CMAKE_CL_64) 
		set(ARCH_DIR "x64") 
		message(STATUS "using 64 bit libraries")
	else() 
		set(ARCH_DIR "x86") 
		message(STATUS "using 32 bit libraries")
	endif() 
endif() 

#setup the default build mode

# global cmake options
SET(BUILD_SHARED_LIBS ON)
SET(CMAKE_USE_RELATIVE_PATHS OFF)

# setup paths
SET(ROR_DEPENDENCY_DIR       "${RoR_SOURCE_DIR}/source/dependencies")
SET(RUNTIME_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH   ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH      ${RUNTIME_OUTPUT_DIRECTORY})
SET(RoR_Dependencies_CMAKE_DIR "${ROR_DEPENDENCY_DIR}/cmake")

# set the tools binary directory accordingly to the used platform
IF(WIN32)
  SET(ROR_BINTOOL_DIR             ${RoR_SOURCE_DIR}/tools/windows/binaries)
ELSEIF(UNIX)
  SET(ROR_BINTOOL_DIR             ${RoR_SOURCE_DIR}/tools/linux/binaries)
ELSEIF(APPLE)
  SET(ROR_BINTOOL_DIR             ${RoR_SOURCE_DIR}/tools/linux/binaries)
ENDIF(WIN32)

set(CMAKE_DEBUG_POSTFIX _d)

# hide some settings
mark_as_advanced(
  CMAKE_DEBUG_POSTFIX
  CMAKE_BACKWARDS_COMPATIBILITY
  Boost_LIB_DIAGNOSTIC_DEFINITIONS
  EXECUTABLE_OUTPUT_PATH
  LIBRARY_OUTPUT_PATH
)

if(WIN32)
  #set(ROR_USING_VISUAL_STUDIO_PROFESSIONAL "FALSE" CACHE BOOL "if you use VS professional, enable this. DO NOT enable this when using express versions")
  
  set(warnings /W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /wd4251 /wd4275 /wd4099 /nologo)
  add_definitions(${warnings})

  set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}        /MP /GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /MP /Zi")
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}     /MP /Zi")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /Od /Zi /Gy /fp:except /GF- /GS /Ob0")
  #if(ROR_USING_VISUAL_STUDIO_PROFESSIONAL)
  #  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /analyze") # /analyze only available when using VS professional
  #endif(ROR_USING_VISUAL_STUDIO_PROFESSIONAL)
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}          /MP /Od /Zi /RTC1 /Gy /fp:except /GF- /RTCc /GS /Ob0")
  # some general flags
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  include_directories(${DirectX_INCLUDE_DIR})

  set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /DEBUG /LARGEADDRESSAWARE ")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /DEBUG /LARGEADDRESSAWARE /OPT:REF /LTCG /RELEASE")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /DEBUG /LARGEADDRESSAWARE /OPT:REF /LTCG ")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /LARGEADDRESSAWARE /OPT:NOREF /MAP /DYNAMICBASE:NO /FIXED /NXCOMPAT:NO")

	# fix executable paths for windows
	#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
	#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_OUTPUT_DIRECTORY})
	#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUNTIME_OUTPUT_DIRECTORY})
	#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_OUTPUT_DIRECTORY})

ELSEIF(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          -O0 -ggdb")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        -mfpmath=sse -msse2 -mmmx -msse -msse3 -m3dnow -O2 -fomit-frame-pointer -fstrict-aliasing -ffast-math -ftracer")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} -O0")

  # add additional gdb debugging stuff
  set(CMAKE_SHARED_LINKER_FLAGS_DEBUG       "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -O0 -ggdb")
  set(CMAKE_MODULE_LINKER_FLAGS_DEBUG       "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} -O0 -ggdb")
endif(WIN32)

include(CMakeDependenciesConfig.txt)


add_subdirectory(source)

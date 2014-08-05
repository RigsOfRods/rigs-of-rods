project(RoR_Installer)

set(resource_files download.xpm action.xpm dest.xpm extrapack.xpm finished.xpm licence.xpm mainpack.xpm streams.xpm unknown.xpm welcome.xpm icon.rc)
set(source_files ConfigManager.cpp wizard.cpp wthread.cpp cevent.cpp symlink.cpp wxStrel.cpp utils.cpp threadpool.cpp wsyncdownload.cpp installerlog.cpp SHA1.cpp)
set(include_files ConfigManager.h wizard.h wxStrel.h wthread.h cevent.h symlink.h utils.h)


IF(WIN32)
   set(INSTALLER_PLATFORM "win32" CACHE PATH "Installer chosen platform")
ENDIF(WIN32)
IF(UNIX)
   # use the LSB stuff if possible :)
   EXECUTE_PROCESS(
      COMMAND cat /etc/lsb-release
      COMMAND grep DISTRIB_ID
      COMMAND awk -F= "{ print $2 }"
      COMMAND tr "\n" " "
      COMMAND sed "s/ //"
      OUTPUT_VARIABLE LSB_ID
      RESULT_VARIABLE LSB_ID_RESULT
   )
   EXECUTE_PROCESS(
      COMMAND cat /etc/lsb-release
      COMMAND grep DISTRIB_RELEASE
      COMMAND awk -F= "{ print $2 }"
      COMMAND tr "\n" " "
      COMMAND sed "s/ //"
      OUTPUT_VARIABLE LSB_VER
      RESULT_VARIABLE LSB_VER_RESULT
   )
   #message("LSB output: ${LSB_ID_RESULT}:${LSB_ID} ${LSB_VER_RESULT}:${LSB_VER}")
   if(NOT ${LSB_ID} STREQUAL "")
      # found some, use it :D
      set(INSTALLER_PLATFORM "${LSB_ID}-${LSB_VER}" CACHE PATH "Installer chosen platform")
   else(NOT ${LSB_ID} STREQUAL "")
      set(INSTALLER_PLATFORM "linux-generic" CACHE PATH "Installer chosen platform")
   endif(NOT ${LSB_ID} STREQUAL "")
ENDIF(UNIX)

add_definitions("-DINSTALLER_PLATFORM=\"${INSTALLER_PLATFORM}\"")

#message("the default installer platform is \"${INSTALLER_PLATFORM}\". To change this, edit INSTALLER_PLATFORM value in cmake.")

include_directories(${CURL_INCLUDE_DIRS})
link_directories   (${CURL_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories   (${Boost_LIBRARY_DIRS})

include_directories(${RoR_Installer_SOURCE_DIR}/../main)

# find wxwidgets
SET(wxWidgets_USE_UNICODE ON)
FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS base core html net adv)
include_directories(${wxWidgets_INCLUDE_DIRS})
add_definitions("-D${wxWidgets_DEFINITIONS}" ${wxWidgets_CXX_FLAGS})
link_directories (${wxWidgets_LIB_DIR})

IF(WIN32)
   add_definitions("-D__WIN32__ -D_CRT_SECURE_NO_WARNINGS -DPLATFORM=1 -DWIN32_LEAN_AND_MEAN")
ENDIF(WIN32)
IF(UNIX)
   add_definitions("-DPLATFORM=2")
   FIND_PACKAGE(PkgConfig REQUIRED)
   PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
   INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
   link_directories (${wxWidgets_LIB_DIR})

   # and boost
   FIND_PACKAGE(Boost REQUIRED system filesystem)
ENDIF(UNIX)

add_definitions("-DNOOGRE -D_UNICODE")

# always use no debug, otherwise we cannot link against the wxWidgets lib (that used no debug mode on compile time)
add_definitions("-DNDEBUG")

#add_definitions("${CMAKE_WXWINDOWS_CXX_FLAGS}")

IF(WIN32)
   add_executable(updater WIN32 ${source_files} ${include_files} ${resource_files})
   # no more static linking :/
   #SET_TARGET_PROPERTIES(installer PROPERTIES COMPILE_FLAGS "/bigobj /MT /MP")
   # TODO: fix UAC and VS2010
   SET_TARGET_PROPERTIES(updater PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
   
   TARGET_LINK_LIBRARIES(updater Version.lib)
ELSE(WIN32)
   add_executable(updater ${source_files} ${include_files} ${resource_files})
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(updater ${wxWidgets_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARIES})
   


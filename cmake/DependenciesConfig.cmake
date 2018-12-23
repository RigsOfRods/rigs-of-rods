# components
set(ROR_USE_OPENAL "TRUE" CACHE BOOL "use OPENAL")
set(ROR_USE_SOCKETW "TRUE" CACHE BOOL "use SOCKETW")
set(ROR_USE_PAGED "TRUE" CACHE BOOL "use paged-geometry")
set(ROR_USE_CAELUM "TRUE" CACHE BOOL "use caelum sky")
set(ROR_USE_ANGELSCRIPT "TRUE" CACHE BOOL "use angelscript")
set(ROR_USE_CURL "TRUE" CACHE BOOL "use curl, required for communication with online services")
set(ROR_USE_MOFILEREADER "TRUE" CACHE BOOL "use mofilereader")

# some obsolete options:
# disabled some options for now
#set(ROR_FEAT_TIMING     "FALSE" CACHE BOOL "enable beam statistics. For core development only")
set(ROR_BUILD_UPDATER OFF)
set(ROR_FEAT_TIMING OFF)
set(ROR_BUILD_SIM ON)

find_package(Threads REQUIRED)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    # find packages

    find_package(OGRE 1.11 REQUIRED COMPONENTS Bites Overlay Paging RTShaderSystem MeshLodGenerator Terrain)
    find_package(OpenAL)
    find_package(OIS REQUIRED)
    find_package(MyGUI REQUIRED)
    find_package(SocketW)
    find_package(AngelScript)
    find_package(CURL)
    find_package(Caelum)
    find_package(MoFileReader)

endif ()

IF (WIN32)

    # directX
    set(DirectX_INCLUDE_DIRS "$ENV{DXSDK_DIR}/Include" CACHE PATH "The DirectX include path to use")
    set(DirectX_LIBRARY_DIRS "$ENV{DXSDK_DIR}/lib/${ARCH_DIR}/" CACHE PATH "The DirectX lib path to use")
    include_directories(${DirectX_INCLUDE_DIRS})
    link_directories(${DirectX_LIBRARY_DIRS})
ENDIF (WIN32)

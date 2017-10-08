# components
set(ROR_USE_OPENAL "TRUE" CACHE BOOL "use OPENAL")
set(ROR_USE_SOCKETW "TRUE" CACHE BOOL "use SOCKETW")
set(ROR_USE_PAGED "TRUE" CACHE BOOL "use paged geometry")
set(ROR_USE_CAELUM "TRUE" CACHE BOOL "use caelum sky")
set(ROR_USE_ANGELSCRIPT "TRUE" CACHE BOOL "use angel script")
set(ROR_USE_CURL "TRUE" CACHE BOOL "use curl, required for communication with online services")
set(ROR_USE_CRASHRPT "FALSE" CACHE BOOL "use crash report tool")


# some obsolete options:
# disabled some options for now
#set(ROR_FEAT_TIMING     "FALSE" CACHE BOOL "enable beam statistics. For core development only")
set(ROR_BUILD_UPDATER OFF)
set(ROR_FEAT_TIMING OFF)
set(ROR_BUILD_SIM ON)

IF (WIN32)
    set(ROR_USE_MOFILEREADER "TRUE" CACHE BOOL "use mofilereader")

    # directX
    set(DirectX_INCLUDE_DIRS "$ENV{DXSDK_DIR}/Include" CACHE PATH "The DirectX include path to use")
    set(DirectX_LIBRARY_DIRS "$ENV{DXSDK_DIR}/lib/${ARCH_DIR}/" CACHE PATH "The DirectX lib path to use")
    include_directories(${DirectX_INCLUDE_DIRS})
    link_directories(${DirectX_LIBRARY_DIRS})

ELSEIF (UNIX)
    find_package(PkgConfig)
    PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
    PKG_CHECK_MODULES(GTK_PIXBUF gdk-pixbuf-2.0 REQUIRED)
    include_directories(${GTK_INCLUDE_DIRS})
    include_directories(${GTK_PIXBUF_INCLUDE_DIRS})
ENDIF (WIN32)

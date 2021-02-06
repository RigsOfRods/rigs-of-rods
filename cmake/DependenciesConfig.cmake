find_package(Threads REQUIRED)

# some obsolete options:
# disabled some options for now
#set(ROR_FEAT_TIMING     "FALSE" CACHE BOOL "enable beam statistics. For core development only")
set(ROR_BUILD_UPDATER OFF)
set(ROR_FEAT_TIMING OFF)
set(ROR_BUILD_SIM ON)

if (USE_PACKAGE_MANAGER)
    set(ROR_USE_OPENAL TRUE)
    set(ROR_USE_SOCKETW TRUE)
    set(ROR_USE_PAGED TRUE)
    set(ROR_USE_CAELUM TRUE)
    set(ROR_USE_ANGELSCRIPT TRUE)
    set(ROR_USE_CURL TRUE)

    include(pmm)
    pmm(CONAN REMOTES ror-dependencies https://api.bintray.com/conan/anotherfoxguy/ror-dependencies BINCRAFTERS)

else (USE_PACKAGE_MANAGER)
    # components
    set(ROR_USE_OPENAL "TRUE" CACHE BOOL "use OPENAL")
    set(ROR_USE_SOCKETW "TRUE" CACHE BOOL "use SOCKETW")
    set(ROR_USE_PAGED "TRUE" CACHE BOOL "use paged-geometry")
    set(ROR_USE_CAELUM "TRUE" CACHE BOOL "use caelum sky")
    set(ROR_USE_ANGELSCRIPT "TRUE" CACHE BOOL "use angelscript")
    set(ROR_USE_CURL "TRUE" CACHE BOOL "use curl, required for communication with online services")

    # find packages
    find_package(OpenAL)
    find_package(OIS REQUIRED)
    #find_package(MyGUI REQUIRED)
    #find_package(SocketW)
    find_package(AngelScript)
    find_package(CURL)
    #find_package(Caelum)
    find_package(fmt REQUIRED)
    
    # find OGRE, see https://ogrecave.github.io/ogre-next/api/2.2/_using_ogre_in_your_app.html and https://forums.ogre3d.org/viewtopic.php?p=547467#p547467
    include( cmake/3rdparty/OGRE.cmake )
    setupOgre( OGRE_SOURCE, OGRE_BINARIES, OGRE_LIBRARIES, FALSE )

endif (USE_PACKAGE_MANAGER)

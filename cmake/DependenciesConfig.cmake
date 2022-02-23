include(DependenciesFunctions)
find_package(Threads REQUIRED)

set(ROR_FEAT_TIMING OFF)

# Some pkg-config files are broken, that is why they are commented out

add_external_lib(
        OGRE
        ogre3d/13.3.1@anotherfoxguy/stable
        CONAN_PKG_NAME OGRE
        REQUIRED
        # PKG_CONFIG "OGRE = 1.11.6"
        FIND_PACKAGE_OPTIONS 13 COMPONENTS Bites Overlay Paging RTShaderSystem MeshLodGenerator Terrain
)

add_external_lib(
        OpenAL
        openal/1.21.1
        REQUIRED
        PKG_CONFIG "openal >= 1.18"
        FIND_PACKAGE_OPTIONS CONFIG
)

add_external_lib(
        OIS
        ois/1.5.1@anotherfoxguy/stable
        REQUIRED
        PKG_CONFIG "ois >= 1.4"
        FIND_PACKAGE
)

add_external_lib(
        MyGUI
        mygui/3.4.1@anotherfoxguy/stable
        REQUIRED
        # PKG_CONFIG "MYGUI = 3.4.0"
        FIND_PACKAGE
)
add_external_lib(
        SocketW
        socketw/3.10.27@anotherfoxguy/stable
        PKG_CONFIG "socketw >= 3.10"
        FIND_PACKAGE
)

add_external_lib(
        AngelScript
        angelscript/2.32.0@anotherfoxguy/stable
        # PKG_CONFIG "angelscript = 2.32.0"
        FIND_PACKAGE
)

add_external_lib(
        CURL
        libcurl/7.79.1
        PKG_CONFIG "libcurl >= 7.6"
        FIND_PACKAGE_OPTIONS CONFIG
        INTERFACE_NAME CURL::libcurl
)

add_external_lib(
        Caelum
        # Temporary switch back to the rigs of rods version, since the OGRE version is broken
        ogre3d-caelum/0.6.4.1@anotherfoxguy/stable
        # PKG_CONFIG "Caelum >= 0.6.3"
        CONAN_PKG_NAME Caelum
        FIND_PACKAGE
)
add_external_lib(
        PagedGeometry
        ogre3d-pagedgeometry/1.3.0.1@anotherfoxguy/stable
        # PKG_CONFIG "PagedGeometry >= 1.2"
        FIND_PACKAGE
        CONAN_PKG_NAME PagedGeometry
        SYMBOL PAGED
)

add_external_lib(
        fmt
        fmt/8.0.1
        REQUIRED
        PKG_CONFIG "fmt >= 6"
        FIND_PACKAGE_OPTIONS CONFIG
)

add_external_lib(
        discord_rpc
        discord-rpc/3.4.0@anotherfoxguy/stable
        FIND_PACKAGE
        INTERFACE_NAME discord-rpc::discord-rpc
)

add_external_lib(
        RapidJSON
        rapidjson/cci.20200410
        REQUIRED
        PKG_CONFIG "RapidJSON >= 1.1"
        FIND_PACKAGE_OPTIONS CONFIG
)

add_external_lib(
        OpenSSL
        openssl/1.1.1l
        PKG_CONFIG "openssl >= 1.1.1"
        FIND_PACKAGE
        INTERFACE_NAME OpenSSL::SSL
)

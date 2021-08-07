include(DependenciesFunctions)
find_package(Threads REQUIRED)

set(ROR_FEAT_TIMING OFF)

add_external_lib(
        OGRE
        ogre3d/1.11.6.1@anotherfoxguy/stable
        REQUIRED
        FIND_PACKAGE_OPTIONS COMPONENTS Bites Overlay Paging RTShaderSystem MeshLodGenerator Terrain
        INTERFACE_NAME OGRE::OGRE
)

add_external_lib(
        OpenAL
        openal/1.19.1
        FIND_PACKAGE
        INTERFACE_NAME OpenAL::OpenAL
)

add_external_lib(
        OIS
        ois/1.5.1@anotherfoxguy/stable
        REQUIRED
        FIND_PACKAGE
        INTERFACE_NAME OIS::OIS
)

add_external_lib(
        MyGUI
        mygui/3.4.0@anotherfoxguy/stable
        REQUIRED
        FIND_PACKAGE
        INTERFACE_NAME MyGUI::MyGUI
)
add_external_lib(
        SocketW
        socketw/3.10.27@anotherfoxguy/stable
        FIND_PACKAGE
        INTERFACE_NAME SocketW::SocketW
)

add_external_lib(
        AngelScript
        angelscript/2.32.0@anotherfoxguy/stable
        FIND_PACKAGE
        INTERFACE_NAME AngelScript::AngelScript
)

add_external_lib(
        CURL
        libcurl/7.69.1
        FIND_PACKAGE
        INTERFACE_NAME CURL::CURL
)

add_external_lib(
        Caelum
        ogre3d-caelum/0.6.4@anotherfoxguy/stable
        FIND_PACKAGE
        INTERFACE_NAME Caelum::Caelum
)
add_external_lib(
        PagedGeometry
        ogre3d-pagedgeometry/1.2.1@anotherfoxguy/stable
        FIND_PACKAGE
        SYMBOL PAGED
        INTERFACE_NAME PagedGeometry::PagedGeometry
)

add_external_lib(
        fmt
        fmt/7.1.3
        FIND_PACKAGE
        INTERFACE_NAME fmt::fmt
)

add_external_lib(
        discord-rpc
        discord-rpc/3.4.0@anotherfoxguy/stable
        FIND_PACKAGE
        OPTION_NAME DISCORD_RPC
        INTERFACE_NAME discord-rpc::discord-rpc
)

add_external_lib(
        RapidJSON
        rapidjson/cci.20200410
        FIND_PACKAGE
        INTERFACE_NAME RapidJSON::RapidJSON
)

add_external_lib(
        OpenSSL
        openssl/1.1.1g
        FIND_PACKAGE
        INTERFACE_NAME OpenSSL::OpenSSL
)

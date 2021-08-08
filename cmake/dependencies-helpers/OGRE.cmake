# Add the OGRE target

if (${_PREFIX}USE_OGRE STREQUAL "SYSTEM")
    if (NOT TARGET OGRE::OGRE)
        find_package(OGRE QUIET)
        message(STATUS "Adding OGRE::OGRE target")
        add_library(OGRE::OGRE INTERFACE IMPORTED)
        set_target_properties(OGRE::OGRE PROPERTIES
                INTERFACE_LINK_LIBRARIES "${OGRE_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${OGRE_INCLUDE_DIRS}"
                )
    endif ()
endif ()
# Refind OpenAL to add the OpenAL::OpenAL target

if (${_PREFIX}USE_OPENAL STREQUAL "SYSTEM")
    find_package(OpenAL QUIET)
    if (NOT TARGET OpenAL::OpenAL)
        add_library(OpenAL::OpenAL INTERFACE IMPORTED)
        set_target_properties(OpenAL::OpenAL PROPERTIES
                INTERFACE_LINK_LIBRARIES "${OPENAL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}"
                )
    endif ()
endif ()
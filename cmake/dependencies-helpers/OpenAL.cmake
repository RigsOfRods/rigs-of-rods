# Add the OpenAL target
if (${_PREFIX}USE_OPENAL STREQUAL "SYSTEM")
    if (OPENAL_FOUND AND NOT TARGET OpenAL::OpenAL)
        message(STATUS "Adding OpenAL::OpenAL target")
        add_library(OpenAL::OpenAL INTERFACE IMPORTED)
        set_target_properties(OpenAL::OpenAL PROPERTIES
                INTERFACE_LINK_LIBRARIES "${OPENAL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}"
                )
    endif ()
endif ()
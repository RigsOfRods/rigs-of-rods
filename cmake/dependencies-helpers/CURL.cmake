# Add the CURL target
if (${_PREFIX}USE_CURL STREQUAL "SYSTEM")
    if (CURL_FOUND AND NOT TARGET CURL::CURL)
        message(STATUS "Adding CURL::CURL target")
        add_library(CURL::CURL INTERFACE IMPORTED)
        set_target_properties(CURL::CURL PROPERTIES
                INTERFACE_LINK_LIBRARIES "${CURL_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
                )
    endif ()
endif ()
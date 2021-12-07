# Add the RapidJSON target

if (${_PREFIX}USE_RAPIDJSON STREQUAL "SYSTEM")
    if (NOT TARGET RapidJSON::RapidJSON)
        message(STATUS "Adding RapidJSON::RapidJSON target")
        add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    endif ()
endif ()
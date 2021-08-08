# Add the RapidJSON target

if (${_PREFIX}USE_RAPIDJSON STREQUAL "SYSTEM")
    if (NOT TARGET RapidJSON::RapidJSON AND TARGET rapidjson)
        message(STATUS "Adding RapidJSON::RapidJSON target")
        set_target_properties(rapidjson PROPERTIES IMPORTED_GLOBAL TRUE)
        add_library(RapidJSON::RapidJSON ALIAS rapidjson)
    endif ()
endif ()
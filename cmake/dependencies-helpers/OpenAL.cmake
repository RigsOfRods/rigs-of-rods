# Refind OpenAL to add the OpenAL::OpenAL target
if (${_PREFIX}USE_OPENAL STREQUAL "SYSTEM")
    if (NOT TARGET OpenAL::OpenAL)
        find_package(OpenAL QUIET CONFIG)
    endif ()
endif ()
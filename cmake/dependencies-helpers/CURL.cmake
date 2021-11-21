# Fixup broken CURL target

if (${_PREFIX}USE_CURL STREQUAL "CONAN")
    if (NOT TARGET CURL::libcurl)
        add_library(CURL::libcurl ALIAS CONAN_PKG::CURL)
    endif ()
endif ()
# Called in the dependency discovery scope, where normal variables contain the
# effective cmake_dependent_option values (the cache may still contain ON).
function(ror_check_required_features)
    # Snapshot the effective input before CACHE initialization: under CMP0126
    # OLD, creating the cache entry can remove an inherited normal binding.
    set(required_features "${ROR_REQUIRED_FEATURES}")
    set(ROR_REQUIRED_FEATURES "${required_features}" CACHE STRING
        "Semicolon-separated ROR_USE_* options that must be effectively enabled")
    set(known_features
        ROR_USE_OPENAL
        ROR_USE_DISCORD_RPC
        ROR_USE_SOCKETW
        ROR_USE_ANGELSCRIPT
        ROR_USE_CURL
        ROR_USE_CAELUM
        ROR_USE_PAGED
    )
    set(receipt "${CMAKE_BINARY_DIR}/ror-features.cmake")
    # A rejected requirement must not leave an earlier successful gate receipt.
    file(REMOVE "${receipt}")

    foreach(feature IN LISTS required_features)
        if(NOT feature IN_LIST known_features)
            message(FATAL_ERROR
                "Unknown required feature '${feature}' in ROR_REQUIRED_FEATURES. "
                "Supported names: ${known_features}")
        endif()
        if(NOT ${feature})
            message(FATAL_ERROR
                "Required feature ${feature} is OFF after dependency discovery. "
                "Set -D${feature}=ON and ensure its dependency is available; "
                "see the package discovery output above. "
                "ROR_REQUIRED_FEATURES does not enable features.")
        endif()
    endforeach()

    # Check the targets used by source/main, including packages whose dependent
    # option uses a *_FOUND variable rather than a target condition.
    foreach(feature_target IN ITEMS
        "ROR_USE_DISCORD_RPC|discord-rpc::discord-rpc"
        "ROR_USE_SOCKETW|SocketW::SocketW"
        "ROR_USE_ANGELSCRIPT|Angelscript::angelscript"
        "ROR_USE_CURL|CURL::libcurl"
        "ROR_USE_CAELUM|Caelum::Caelum"
        "ROR_USE_PAGED|PagedGeometry::PagedGeometry"
    )
        string(REPLACE "|" ";" feature_target "${feature_target}")
        list(GET feature_target 0 feature)
        list(GET feature_target 1 dependency_target)
        if(feature IN_LIST required_features AND NOT TARGET ${dependency_target})
            message(FATAL_ERROR
                "Required feature ${feature} is ON, but dependency target "
                "${dependency_target} is missing. Check the package discovery output.")
        endif()
    endforeach()

    # Canonical requirements and explicit booleans make this path-free receipt
    # suitable for build tooling. It describes discovery, not runtime capability.
    list(REMOVE_DUPLICATES required_features)
    list(SORT required_features)
    set(contents "# Effective optional features after dependency discovery.\n")
    string(APPEND contents "set(ROR_REQUIRED_FEATURES \"${required_features}\")\n")
    foreach(feature IN LISTS known_features)
        if(${feature})
            set(value ON)
        else()
            set(value OFF)
        endif()
        string(APPEND contents "set(${feature} ${value})\n")
    endforeach()
    file(WRITE "${receipt}" "${contents}")
endfunction()

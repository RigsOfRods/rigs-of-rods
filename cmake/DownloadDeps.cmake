include(FetchContent)
include(CheckCXXSourceRuns)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

if (WIN32)
    FetchContent_Declare(
        dependencies
        URL      http://prdownloads.sourceforge.net/rigs-of-rods/dependencies_win_31.tar.gz
        URL_HASH SHA1=83f843d21868d6a5731630cd1c3bf81859778ab0
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
elseif (UNIX)
    FetchContent_Declare(
        dependencies
        URL      http://prdownloads.sourceforge.net/rigs-of-rods/dependencies_linux_31.tar.gz
        URL_HASH SHA1=75f56cb4e181864bfe7dd464f368c5b5c9361b6d
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif ()

FetchContent_MakeAvailable(dependencies)

function(run_deps_test)
    # Run a simple test to see if the prebuild dependencies are compatible with the used compiler 
    include("${dependencies_SOURCE_DIR}/ZLIB-release-x86_64-data.cmake")

    set(CMAKE_REQUIRED_INCLUDES ${zlib_INCLUDE_DIRS_RELEASE})
    if (WIN32)
        set(CMAKE_REQUIRED_LIBRARIES "${zlib_LIB_DIRS_RELEASE}/zlib.lib")
    elseif (UNIX)
        set(CMAKE_REQUIRED_LIBRARIES "${zlib_LIB_DIRS_RELEASE}/libz.a")
    endif ()

    check_cxx_source_runs([==[
    #include <stdio.h>
    #include <zlib.h>

    int main(void) {
        printf("ZLIB VERSION: %s", zlibVersion());
        return 0;
    }
    ]==] DEPS_CHECK)

    if(NOT DEPS_CHECK)
        message(FATAL_ERROR "Failed to build test program with prebuild deps, please use conan")
    endif()    
endfunction(run_deps_test)

run_deps_test()
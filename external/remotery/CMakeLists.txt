include(FetchContent)

if (USE_REMOTERY)
    FetchContent_Declare(
            remotery
            GIT_REPOSITORY https://github.com/Celtoys/Remotery.git
            GIT_TAG v1.2.1
    )

    FetchContent_MakeAvailable(remotery)

    add_library(remotery STATIC "${remotery_SOURCE_DIR}/lib/Remotery.c")
    target_include_directories(remotery PUBLIC "${remotery_SOURCE_DIR}/lib/")
endif ()
project(angelscript_addons)

# TODO list explicitely
FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.h)

add_library(angelscript_addons STATIC ${headers} ${sources})

if (USE_PACKAGE_MANAGER)
    target_link_libraries(angelscript_addons PRIVATE CONAN_PKG::AngelScript)
    target_compile_definitions(angelscript_addons PRIVATE USE_ANGELSCRIPT)
    target_compile_definitions(angelscript_addons PRIVATE AS_USE_NAMESPACE)
elseif (TARGET AngelScript::AngelScript)
    target_link_libraries(angelscript_addons
            AngelScript::AngelScript
            AngelScript::use_namespace
            )
else ()
    message(WARNING "Building angelscript_addons without AngelScript. Some functionality may be missing.")
endif ()

if (USE_PHC)
    target_precompile_headers(angelscript_addons PRIVATE [["angelscript.h"]])
endif ()
project(angelscript_addons)

# TODO list explicitely
FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.h)

add_library(angelscript_addons STATIC ${headers} ${sources})

if (USE_ANGELSCRIPT)
    target_link_libraries(angelscript_addons AngelScript::AngelScript)
    target_compile_definitions(angelscript_addons PRIVATE AS_USE_NAMESPACE)
else ()
    message(WARNING "Building angelscript_addons without AngelScript. Some functionality may be missing.")
endif ()

if (USE_PHC)
    target_precompile_headers(angelscript_addons PRIVATE [["angelscript.h"]])
endif ()

set_property(TARGET angelscript_addons PROPERTY FOLDER "External dependencies")
project(angelscript_addons)

# TODO list explicitely
FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.h)

add_library(angelscript_addons STATIC ${headers} ${sources})

if (ROR_USE_ANGELSCRIPT)
    target_link_libraries(angelscript_addons Angelscript::angelscript)
    target_compile_definitions(angelscript_addons PRIVATE AS_USE_NAMESPACE)
endif ()

set_property(TARGET angelscript_addons PROPERTY FOLDER "External dependencies")
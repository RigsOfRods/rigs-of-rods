project(angelscript_addons)

# TODO list explicitely
FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.h)

add_library(angelscript_addons STATIC ${headers} ${sources})

target_link_libraries( angelscript_addons
   AngelScript::AngelScript
   AngelScript::use_namespace
   ${optional_libs} # TODO is this needed?
)

set_target_properties(angelscript_addons PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
cotire(angelscript_addons)
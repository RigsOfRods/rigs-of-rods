project(angelscript_addons)


# TODO list explicitely
FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.cpp)


find_package( AngelScript )

add_library(angelscript_addons STATIC ${headers} ${sources})

target_link_libraries( angelscript_addons
   AngelScript::AngelScript 
   AngelScript::use_namespace
  ${optional_libs} # TODO is this needed?
)


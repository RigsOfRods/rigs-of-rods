project(angelscript_addons)

# TODO list explicitely
FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.h)

add_library(angelscript_addons STATIC ${headers} ${sources})

if(DEFINED CONAN_ANGELSCRIPT_ROOT)
  target_link_libraries(angelscript_addons PRIVATE CONAN_PKG::AngelScript)
  target_include_directories(angelscript_addons PRIVATE CONAN_PKG::AngelScript)
  target_compile_definitions(angelscript_addons PRIVATE USE_ANGELSCRIPT)
  target_compile_definitions(angelscript_addons PRIVATE AS_USE_NAMESPACE)
elseif (TARGET AngelScript::AngelScript)
  target_link_libraries( angelscript_addons
     AngelScript::AngelScript
     AngelScript::use_namespace
  )
else ()
    message(WARNING "Building angelscript_addons without AngelScript. Some functionality may be missing.")
endif ()

set_target_properties(angelscript_addons PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
cotire(angelscript_addons)

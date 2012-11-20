project(angelscript_addons)

FILE(GLOB_RECURSE sources *.cpp)
FILE(GLOB_RECURSE headers *.cpp)


#setup libraries
macro(setup_lib name)
   if(ROR_USE_${name})
      include_directories(${${name}_INCLUDE_DIRS})
      link_directories   (${${name}_LIBRARY_DIRS})
      add_definitions("-DUSE_${name}")
      set(optional_libs ${optional_libs} ${${name}_LIBRARIES})
      #message(STATUS "${name} Enabled:      \tYES")
      #message(STATUS "${name}_INCLUDE_DIRS: \t${${name}_INCLUDE_DIRS}")
      #if(${${name}_LIBRARY_DIRS})
      #   message(STATUS "${name}_LIBRARY_DIRS: \t${${name}_LIBRARY_DIRS}")
      #endif(${${name}_LIBRARY_DIRS})
      #message(STATUS "${name}_LIBRARIES:    \t${${name}_LIBRARIES}")
   else()
	   #message(STATUS "${name} Enabled:      \tNO")
   endif(ROR_USE_${name})
endmacro(setup_lib)

# optional components
setup_lib(ANGELSCRIPT)

if(ROR_USE_ANGELSCRIPT)
	add_definitions("-DAS_USE_NAMESPACE")
endif()

add_library(angelscript_addons STATIC ${headers} ${sources})
target_link_libraries(angelscript_addons ${optional_libs})


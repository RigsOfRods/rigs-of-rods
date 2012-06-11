project(OgreCanvas)

FILE(GLOB_RECURSE sources *.cpp *.h)

include_directories(${Ogre_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SKIA_INCLUDE_DIRS})

add_definitions(-DOGRE_CANVAS_LIB)

setup_lib(SKIA)
set(optional_libs ${optional_libs};${SKIA_LIBRARIES})

add_library(OgreCanvas STATIC ${sources} )

target_link_libraries(OgreCanvas ${optional_libs})


# define the project
project(Caelum VERSION 0.6.3)

# some versioning things
SET(LIB_MAJOR_VERSION ${Caelum_VERSION_MAJOR})
SET(LIB_MINOR_VERSION ${Caelum_VERSION_MINOR})
SET(LIB_BUILD_VERSION ${Caelum_VERSION_PATCH})

set(CAELUM_SCRIPT_SUPPORT TRUE)

configure_file(
    "${CMAKE_SOURCE_DIR}/external/caelum/main/include/CaelumConfig.h.in"
    "${CMAKE_SOURCE_DIR}/external/caelum/main/include/CaelumConfig.h"
)

file(GLOB Caelum_sources ${CMAKE_SOURCE_DIR}/external/caelum/main/src/*.cpp ${CMAKE_SOURCE_DIR}/external/caelum/main/include/*.h)

add_library(${PROJECT_NAME} STATIC ${Caelum_sources})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/external/caelum/main/include/)
target_compile_definitions(${PROJECT_NAME} PUBLIC "CAELUM_LIB")

if (USE_PACKAGE_MANAGER)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            CONAN_PKG::OGRE
            Threads::Threads
            )

else (USE_PACKAGE_MANAGER)
    target_link_libraries(${PROJECT_NAME} PRIVATE OgreMain OgreBites OgreRTShaderSystem OgreOverlay OgreTerrain)
endif (USE_PACKAGE_MANAGER)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "External dependencies")
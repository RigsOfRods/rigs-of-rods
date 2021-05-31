set(PAGEDGEOMETRY_MAJOR_VERSION "1")
set(PAGEDGEOMETRY_MINOR_VERSION "2")
set(PAGEDGEOMETRY_BUILD_VERSION "1")
set(PAGEDGEOMETRY_VERSION "${PAGEDGEOMETRY_MAJOR_VERSION}.${PAGEDGEOMETRY_MINOR_VERSION}.${PAGEDGEOMETRY_BUILD_VERSION}")

project(PagedGeometry VERSION ${PAGEDGEOMETRY_VERSION})

set(PAGEDGEOMETRY_ALTERNATE_COORDSYSTEM FALSE)
set(PAGEDGEOMETRY_USER_DATA FALSE)

configure_file(
        "${CMAKE_SOURCE_DIR}/external/pagedgeometry/include/PagedGeometryConfig.h.in"
        "${CMAKE_SOURCE_DIR}/external/pagedgeometry/include/PagedGeometryConfig.h")

file(GLOB PagedGeometry_sources ${CMAKE_SOURCE_DIR}/external/pagedgeometry/source/*.cpp ${CMAKE_SOURCE_DIR}/external/pagedgeometry/include/*.h)

add_library(${PROJECT_NAME} STATIC ${PagedGeometry_sources})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/external/pagedgeometry/include/)

if (USE_PACKAGE_MANAGER)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            CONAN_PKG::OGRE
            Threads::Threads
            )

else (USE_PACKAGE_MANAGER)
    target_link_libraries(${PROJECT_NAME} PRIVATE OgreMain OgreBites OgreRTShaderSystem OgreOverlay OgreTerrain)
endif (USE_PACKAGE_MANAGER)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "External dependencies")
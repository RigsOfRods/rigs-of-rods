project(UtilsTest)

include_directories(
    "${CMAKE_SOURCE_DIR}/source/main"           # <RoRPrerequisites.h>
    "${CMAKE_SOURCE_DIR}/source/main/utils"     # <Utils.h>
    "${CMAKE_SOURCE_DIR}/source/main/network"   # <RoRnet.h>
    "${CMAKE_SOURCE_DIR}/source/version_info"   # <RoRVersion.h>
)

add_executable(UtilsTest
    "UtilsTest.cpp"
    "${CMAKE_SOURCE_DIR}/source/main/utils/Utils.cpp"
    "${CMAKE_SOURCE_DIR}/source/main/utils/SHA1.cpp"
)

if (USE_PACKAGE_MANAGER)
    target_link_libraries(UtilsTest PRIVATE
            CONAN_PKG::gtest
            CONAN_PKG::MyGUI   # Adds OGRE to linked libraries
            version_info
    )
else()
    target_link_libraries(UtilsTest PRIVATE
            gtest
            MyGUI
            version_info
    )    
endif()

install(TARGETS UtilsTest DESTINATION bin)  

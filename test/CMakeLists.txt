####################################################################################################
# Rigs of Rods test harness, see README.md
####################################################################################################

function(rigsofrods_test_suite)
    cmake_parse_arguments(
        RORTEST   # prefix
        ""        # options
        "NAME"    # single-value arg names
        "SOURCES" # multi-value arg names
        ${ARGN}
    )

    project(${RORTEST_NAME})

    include_directories(
        "${CMAKE_SOURCE_DIR}/test/suites/${RORTEST_NAME}"  # Must come first, see README
        "${CMAKE_SOURCE_DIR}/test/proxies"                 # Must come second, see README
        "${CMAKE_SOURCE_DIR}/source/main"                  # Must come last: Application.h, RoRPrerequisites.h, datatypes/{node_t.h, beam_t.h etc...}
    )
  
    add_definitions(
        -D_WINSOCKAPI_ # Prevents <windows.h> including <winsock.h> - without this, build fails with "redefinition" errors because RoR uses <winsock2.h>, see https://stackoverflow.com/questions/1372480/c-redefinition-header-files-winsock2-h
    )
  
    add_executable(${RORTEST_NAME}
        "${RORTEST_NAME}.cpp"
        "${CMAKE_SOURCE_DIR}/source/main/Application.cpp"
        ${RORTEST_SOURCES}
    )

    set_target_properties(${RORTEST_NAME} PROPERTIES FOLDER "Tests")

    if (USE_PACKAGE_MANAGER)
        target_link_libraries(${RORTEST_NAME} PRIVATE
                CONAN_PKG::gtest
                CONAN_PKG::OGRE
                CONAN_PKG::MyGUI
                CONAN_PKG::rapidjson
                version_info
        )
    else()
        target_link_libraries(${RORTEST_NAME} PRIVATE
                gtest
                OGRE
                MyGUI
                rapidjson
                version_info
        )    
    endif() 
endfunction()

if( ROR_USE_GOOGLETEST )

   add_subdirectory("suites")
   
endif()

####################################################################################################
# Doxygen Documentation
####################################################################################################
if( ROR_BUILD_DOC_DOXYGEN )
  # The doxygen package is required in order to build documentation
  find_package( Doxygen REQUIRED )

  # Setup source and destination paths
  set( DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doxygen" )
  set( DOXYGEN_BUILD_DIR  "${CMAKE_CURRENT_BINARY_DIR}/doxygen" )

  # Generate Doxygen configuration file from template by replacing specific CMake variables with
  # corresponding values (e.g. project name, project version). The resulting config file is copied
  # to the directory where the documentation is going to be built.
  configure_file( ${DOXYGEN_SOURCE_DIR}/Doxyfile.conf.in ${DOXYGEN_BUILD_DIR}/Doxyfile.conf @ONLY )

  # Provide custom target which allows to build the documentation using the generated configuration file.
  add_custom_target( doc-doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile.conf
    WORKING_DIRECTORY ${DOXYGEN_BUILD_DIR}
    COMMENT "Generating documentation with Doxygen."
    VERBATIM
  )
endif()

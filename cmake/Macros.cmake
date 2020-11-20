macro(copy_files srcDir destDir)
    message(STATUS "Copying ${srcDir} -> ${destDir} ...")
    #make_directory(${destDir})

    file(GLOB files RELATIVE ${srcDir} ${srcDir}/*)
    foreach (filename ${files})
        set(srcTemplatePath ${srcDir}/${filename})
        if (NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Copying file ${filename}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${filename}
                    COPYONLY)
        endif (NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach (filename)
endmacro(copy_files)

macro(copy_files_for_target TRGT SRC)
    set(mysrc ${SRC})
    set(mydst ${EXECUTABLE_OUTPUT_PATH}/$(ConfigurationName)/)
    if (WIN32)
        STRING(REGEX REPLACE "/" "\\\\" mysrc ${mysrc})
        STRING(REGEX REPLACE "/" "\\\\" mydst ${mydst})
        add_custom_command(
                TARGET ${TRGT}
                POST_BUILD
                COMMAND xcopy /S /Y ${mysrc} ${mydst}
                COMMENT "Copying ${mysrc} to ${mydst}"
        )
    else ()
        set(mydst ${EXECUTABLE_OUTPUT_PATH}/)
        add_custom_command(
                TARGET ${TRGT}
                POST_BUILD
                COMMAND cp -r ${mysrc} ${mydst}
                COMMENT "Copying ${mysrc} to ${mydst}"
        )
    endif ()
endmacro(copy_files_for_target)

macro(copy_lib_files_for_target TRGT SRC)
    copy_files_for_target("${TRGT}" "${ROR_DEPENDENCIES_DIR}/bin/${ARCH_DIR}/${SRC}/$(ConfigurationName)/*" ${SRC})
endmacro(copy_lib_files_for_target)

macro(setup_lib name)
    if (ROR_USE_${name})
        include_directories(${${name}_INCLUDE_DIRS})
        link_directories(${${name}_LIBRARY_DIRS})
        add_definitions("-DUSE_${name}")
        set(optional_libs ${optional_libs};${${name}_LIBRARIES})
        message(STATUS "${name} Enabled:      \tYES")
        message(STATUS "${name}_INCLUDE_DIRS: \t${${name}_INCLUDE_DIRS}")
        #if(${${name}_LIBRARY_DIRS})
        message(STATUS "${name}_LIBRARY_DIRS: \t${${name}_LIBRARY_DIRS}")
        #endif(${${name}_LIBRARY_DIRS})
        message(STATUS "${name}_LIBRARIES:    \t${${name}_LIBRARIES}")
    else ()
        message(STATUS "${name} Enabled:      \tNO")
    endif (ROR_USE_${name})
endmacro(setup_lib)

macro(add_sub_dir BINNAME name)
    FILE(GLOB_RECURSE ${BINNAME}_${name}_source ${RoR_Main_SOURCE_DIR}/${name}/*.cpp ${RoR_Main_SOURCE_DIR}/${name}/*.c)
    FILE(GLOB_RECURSE ${BINNAME}_${name}_header ${RoR_Main_SOURCE_DIR}/${name}/*.h)
    SOURCE_GROUP("${name}" FILES ${${BINNAME}_${name}_source} ${${BINNAME}_${name}_header})
    include_directories(${RoR_Main_SOURCE_DIR}/${name})
    set(${BINNAME}_sources ${${BINNAME}_sources} ${${BINNAME}_${name}_source})
    set(${BINNAME}_headers ${${BINNAME}_headers} ${${BINNAME}_${name}_header})
endmacro(add_sub_dir)

macro(add_main_dir BINNAME)
    FILE(GLOB ${BINNAME}_main_source ${RoR_Main_SOURCE_DIR}/*.cpp ${RoR_Main_SOURCE_DIR}/*.c)
    FILE(GLOB ${BINNAME}_main_header ${RoR_Main_SOURCE_DIR}/*.h)
    SOURCE_GROUP("globals" FILES ${${BINNAME}_main_source} ${${BINNAME}_main_header})
    set(${BINNAME}_sources ${${BINNAME}_sources} ${${BINNAME}_main_source})
    set(${BINNAME}_headers ${${BINNAME}_headers} ${${BINNAME}_main_header})
endmacro(add_main_dir)

macro(get_sub_dirs result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH (child ${children})
        IF (IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF ()
    ENDFOREACH ()
    SET(${result} ${dirlist})
endmacro(get_sub_dirs)

macro(recursive_zip_folder name in_dir out_dir result)
    set(TMP_FILE_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/tmp/${name})
    get_sub_dirs(SUB_DIRS "${in_dir}")
    file(MAKE_DIRECTORY ${out_dir})

    foreach (ZIP_DIR ${SUB_DIRS})
        if ("${ZIP_DIR}" STREQUAL ".git")
            continue()
        endif ()

        file(GLOB_RECURSE ZIP_FILES LIST_DIRECTORIES TRUE "${in_dir}/${ZIP_DIR}/*")

        set(ZIP_FILES_TXT "")
        set(ZIP_FILES_DEP "")
        foreach (ZIP_FILE ${ZIP_FILES})
            file(RELATIVE_PATH REL_PATH "${in_dir}/${ZIP_DIR}" "${ZIP_FILE}")
            set(ZIP_FILES_TXT "${ZIP_FILES_TXT}${REL_PATH}\n")
            if(NOT IS_DIRECTORY ${ZIP_FILE})
                list(APPEND ZIP_FILES_DEP ${ZIP_FILE})
            endif()
        endforeach ()
        file(WRITE "${TMP_FILE_DIR}/${ZIP_DIR}-filelist.txt" ${ZIP_FILES_TXT})

        set(ZIP_NAME "${out_dir}/${ZIP_DIR}.zip")
        add_custom_command(
                OUTPUT ${ZIP_NAME}
                DEPENDS ${ZIP_FILES_DEP}
                COMMAND ${CMAKE_COMMAND} -E tar c ${ZIP_NAME} --format=zip --files-from="${TMP_FILE_DIR}/${ZIP_DIR}-filelist.txt"
                WORKING_DIRECTORY "${in_dir}/${ZIP_DIR}"
        )
        list(APPEND ${result} ${ZIP_NAME})
    endforeach ()
endmacro(recursive_zip_folder)
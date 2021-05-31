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

function(recursive_zip_folder in_dir out_dir)
    get_filename_component(FROM_DIR_NAME ${in_dir} NAME)
    set(TMP_FILE_DIR ${CMAKE_BINARY_DIR}/tmp/${name})
    get_sub_dirs(SUB_DIRS "${in_dir}")
    file(MAKE_DIRECTORY ${out_dir})

    foreach (ZIP_DIR ${SUB_DIRS})
        if ("${ZIP_DIR}" STREQUAL ".git")
            continue()
        endif ()

        file(GLOB_RECURSE ZIP_FILES LIST_DIRECTORIES FALSE "${in_dir}/${ZIP_DIR}/*")

        set(ZIP_FILES_TXT "")
        set(ZIP_FILES_DEP "")
        foreach (ZIP_FILE ${ZIP_FILES})
            file(RELATIVE_PATH REL_PATH "${in_dir}/${ZIP_DIR}" "${ZIP_FILE}")
            set(ZIP_FILES_TXT "${ZIP_FILES_TXT}${REL_PATH}\n")
            if (NOT IS_DIRECTORY ${ZIP_FILE})
                list(APPEND ZIP_FILES_DEP ${ZIP_FILE})
            endif ()
        endforeach ()
        file(WRITE "${TMP_FILE_DIR}/${ZIP_DIR}-filelist.txt" ${ZIP_FILES_TXT})

        set(ZIP_NAME "${out_dir}/${ZIP_DIR}.zip")
        add_custom_command(
                OUTPUT ${ZIP_NAME}
                DEPENDS ${ZIP_FILES_DEP}
                COMMAND ${CMAKE_COMMAND} -E tar c ${ZIP_NAME} --format=zip --files-from="${TMP_FILE_DIR}/${ZIP_DIR}-filelist.txt"
                WORKING_DIRECTORY "${in_dir}/${ZIP_DIR}"
        )
        list(APPEND ALL_ZIPS ${ZIP_NAME})
    endforeach ()

    add_custom_target(
            zip_folder_${FROM_DIR_NAME} ALL
            DEPENDS ${ALL_ZIPS}
    )
    set_property(TARGET zip_folder_${FROM_DIR_NAME} PROPERTY FOLDER "Scripts")
endfunction(recursive_zip_folder)

function(fast_copy FROM_DIR TO_DIR)
    file(GLOB_RECURSE files "${FROM_DIR}/*")
    get_filename_component(FROM_DIR_NAME ${FROM_DIR} NAME)

    foreach (file IN LISTS files)
        file(RELATIVE_PATH output_file ${FROM_DIR} ${file})
        set(dest "${TO_DIR}/${output_file}")

        add_custom_command(
                COMMENT "Copying ${output_file}"
                OUTPUT ${dest}
                DEPENDS ${file}
                COMMAND ${CMAKE_COMMAND} -E copy ${file} ${dest}
        )
        list(APPEND ALL_FILES ${dest})
    endforeach ()

    add_custom_target(
            fast_copy_${FROM_DIR_NAME} ALL
            DEPENDS ${ALL_FILES}
    )
    set_property(TARGET fast_copy_${FROM_DIR_NAME} PROPERTY FOLDER "Scripts")
endfunction()

function(extract_pot source_files_list)
    set(FILELIST ${CMAKE_BINARY_DIR}/tmp/xgettext-filelist.txt)
    set(ROR_POT "${CMAKE_SOURCE_DIR}/languages/ror.pot")

    list(JOIN source_files_list "\n" source_files)
    file(WRITE "${FILELIST}" "${source_files}")

    add_custom_target(
            extract_pot
            COMMENT "Extract pot"
            OUTPUT ${ROR_POT}
            COMMAND xgettext -i -F -k_L -k_LC:1c,2 -c -o${ROR_POT} -f ${FILELIST}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_property(TARGET extract_pot PROPERTY FOLDER "Scripts")
endfunction()

function(compile_mo)
    file(GLOB_RECURSE potfiles ${CMAKE_SOURCE_DIR}/languages/*/*.po CONFIGURE_DEPENDS)

    foreach (file IN LISTS potfiles)
        get_filename_component(dir ${file} DIRECTORY)
        get_filename_component(filename ${file} NAME_WE)
        get_filename_component(name "${dir}" NAME)
        set(out "${dir}/${filename}.mo")
        add_custom_command(
                COMMENT "Compiling ${name}"
                OUTPUT ${out}
                DEPENDS ${file}
                COMMAND msgfmt -o${out} ${file}
                WORKING_DIRECTORY ${dir}
        )
        list(APPEND MO_FILES ${out})
    endforeach ()

    add_custom_target(
            compile_mo
            DEPENDS ${MO_FILES}
    )
    set_property(TARGET compile_mo PROPERTY FOLDER "Scripts")
endfunction()